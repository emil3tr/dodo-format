#pragma once

#include <algorithm>
#include <concepts>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>

// TODO: MISSING: code should respect indent and last newline

// TODO: improve callback interface

namespace dodo
{

constexpr char NAME_CODE[5] = "code";
constexpr char NAME_TEXT[5] = "text";
constexpr char NAME_LINK[5] = "link";
constexpr char NAME_BLOCK[6] = "block";
constexpr char TEXT_SINGLE_SPACE[2] = " ";

inline constexpr int CHAR_SPACE = 32;
inline constexpr int CHAR_TAB = 9;
inline constexpr int CHAR_NEWLINE = 10;
inline constexpr int CHAR_COLON = 58;
inline constexpr int CHAR_SEMICOLON = 59;
inline constexpr int CHAR_EOF = std::char_traits<char>::eof();

inline constexpr std::size_t STACK_BUFFER_SIZE = (4 * 1024);
inline constexpr std::size_t OUTPUT_BUFFER_OFFSET = 1024;

constexpr u_int8_t FLAG_INLINE_SPECIAL = 1;
constexpr u_int8_t FLAG_WHITESPACE = 2;
constexpr u_int8_t FLAG_ALLOWED_IN_NAME = 4;

constexpr std::array<u_int8_t, 128> MAKE_CHARFLAG_ARRAY()
{
    std::array<u_int8_t, 128> out{};
    for (int i = 0; i < 128; i++) {
        out[i] = FLAG_ALLOWED_IN_NAME;
    }

    out['\n'] |= FLAG_WHITESPACE;
    out['\t'] |= FLAG_WHITESPACE;
    out[' '] |= FLAG_WHITESPACE;

    out['!'] |= FLAG_INLINE_SPECIAL;
    out['$'] |= FLAG_INLINE_SPECIAL;
    out['&'] |= FLAG_INLINE_SPECIAL;
    out['/'] |= FLAG_INLINE_SPECIAL;
    out['='] |= FLAG_INLINE_SPECIAL;
    out['?'] |= FLAG_INLINE_SPECIAL;
    out['*'] |= FLAG_INLINE_SPECIAL;
    out['~'] |= FLAG_INLINE_SPECIAL;
    out['^'] |= FLAG_INLINE_SPECIAL;
    out['_'] |= FLAG_INLINE_SPECIAL;
    out['-'] |= FLAG_INLINE_SPECIAL;

    out['('] &= ~FLAG_ALLOWED_IN_NAME;
    out[')'] &= ~FLAG_ALLOWED_IN_NAME;
    out['['] &= ~FLAG_ALLOWED_IN_NAME;
    out[']'] &= ~FLAG_ALLOWED_IN_NAME;
    out['{'] &= ~FLAG_ALLOWED_IN_NAME;
    out['}'] &= ~FLAG_ALLOWED_IN_NAME;
    out['<'] &= ~FLAG_ALLOWED_IN_NAME;
    out['>'] &= ~FLAG_ALLOWED_IN_NAME;
    out['\t'] &= ~FLAG_ALLOWED_IN_NAME;
    out['\n'] &= ~FLAG_ALLOWED_IN_NAME;
    out[' '] &= ~FLAG_ALLOWED_IN_NAME;
    out[':'] &= ~FLAG_ALLOWED_IN_NAME;
    out[';'] &= ~FLAG_ALLOWED_IN_NAME;
    return out;
}

constexpr std::array<u_int8_t, 128> CHARFLAG_ARRAY = MAKE_CHARFLAG_ARRAY();

constexpr bool IS_INLINE_SPECIAL(int c)
{ return (c >= 0) && (c < 128) && (CHARFLAG_ARRAY[c] & FLAG_INLINE_SPECIAL); }

constexpr bool IS_WHITESPACE(int c)
{ return (c >= 0) && (c < 128) && (CHARFLAG_ARRAY[c] & FLAG_WHITESPACE); }

constexpr bool IS_ALLOWED_IN_NAME(int c)
{ return (c != CHAR_EOF) && ((c >= 128) || (c < 0) || (CHARFLAG_ARRAY[c] & FLAG_ALLOWED_IN_NAME)); }

/* All range functions assume that the current character is not CHAR_EOF. */
class iobuff
{
private:
    std::string buffer;
    std::size_t cursor;
    std::size_t next_output_cursor = 0;
    std::size_t current_indent = 0;
    std::size_t current_line = 0;
    int current_char;
    int last_char = '\n';
    std::size_t buffer_size;

    /* First char of current range. */
    std::size_t range_start_cursor = 0;
    /* Next char of range is written here. */
    std::size_t range_write_cursor = 0;
    /* First char of current segment of the range. */
    std::size_t range_segment_start = 0;
    bool range_paused = false;

    /*
        Each function touching temp_array must maintan the following invariant:
        temp_array[i] == false for all i in {0, 255}
    */
    std::array<bool, 256> temp_array{};

public:
    iobuff(std::istream& stream, std::size_t estimated_size = (63 * 1024))
    {
        char local_buffer[STACK_BUFFER_SIZE];
        buffer.reserve(estimated_size + OUTPUT_BUFFER_OFFSET);
        buffer.append(OUTPUT_BUFFER_OFFSET, '\0');
        buffer.push_back(CHAR_NEWLINE);

        while (stream.read(local_buffer, STACK_BUFFER_SIZE)) {
            buffer.append(local_buffer, local_buffer + stream.gcount());
        }

        if (stream.gcount() > 0) {
            buffer.append(local_buffer, local_buffer + stream.gcount());
        }

        buffer_size = buffer.size();
        cursor = OUTPUT_BUFFER_OFFSET + 1;
        current_char = (cursor >= buffer_size) ? CHAR_EOF : buffer[cursor];
        current_indent = 0;
    }

    inline int get() { return current_char; }

    inline int next()
    {
        cursor++;
        if (cursor >= buffer_size) [[unlikely]] {
            cursor = buffer_size;
            return current_char = CHAR_EOF;
        }

        if (current_char == CHAR_NEWLINE) {
            current_indent = 0;
            current_line++;
        } else {
            current_indent++;
        }
        last_char = current_char;
        return current_char = buffer[cursor];
    }

    inline int prev() { return last_char; }

    inline int peek()
    {
        if (cursor + 1 >= buffer_size) [[unlikely]] {
            return CHAR_EOF;
        }
        return buffer[cursor + 1];
    }

    inline std::size_t indent() { return current_indent; }

    inline std::size_t line() { return current_line; }

    /* Starts a new range on and including the current_char. */
    void range_start()
    {
        range_start_cursor = cursor;
        range_write_cursor = cursor;
        range_segment_start = cursor;
        range_paused = false;
    }

    /*
        Overwrites the current char in the range with another char.
        Assumes the current range is not paused.
    */
    inline void range_overwrite(char c) { buffer[cursor] = c; }

    /*
        Puts one char at the end of the range.
        Assumes that the current range is paused and will not be
        activated on the current character.
        Use range_overwrite whenever possible.
    */
    inline void range_put(char c) { buffer[range_write_cursor++] = c; }

    /*
        Pauses the current range. This means that all characters from the point where is was last
        continued or started until (but not including) the current character are in the range.
        Retuns a view of the range.
        If the range is already paused, does nothing except returning the view.
    */
    std::string_view range_pause_before()
    {
        if (!range_paused) {
            range_paused = true;
            if (range_segment_start != range_write_cursor) {
                std::copy(buffer.data() + range_segment_start, buffer.data() + cursor,
                          buffer.data() + range_write_cursor);
            }
            range_write_cursor += (cursor - range_segment_start);
        }
        return std::string_view(buffer.data() + range_start_cursor,
                                buffer.data() + range_write_cursor);
    }

    /*
        Pauses the current range. This means that all characters from the point where is was last
        continued or started until (and including) the current character are in the range.
        Assumes that no new range will be started on the current character.
        Retuns a view of the range.
        If the range is already paused, does nothing except returning the view.
    */
    std::string_view range_pause_here()
    {
        if (!range_paused) {
            range_paused = true;
            if (range_segment_start != range_write_cursor) {
                std::copy(buffer.data() + range_segment_start, buffer.data() + cursor + 1,
                          buffer.data() + range_write_cursor);
            }
            range_write_cursor += (cursor + 1 - range_segment_start);
        }

        return std::string_view(buffer.data() + range_start_cursor,
                                buffer.data() + range_write_cursor);
    }

    /* Continues a range on (and including) the current character. */
    void range_continue()
    {
        range_paused = false;
        range_segment_start = cursor;
    }

    /* Moves the current char forward until EOF is reached or it sits on one of the passed
     * characters. Returns current char. */
    int find(std::convertible_to<u_int8_t> auto... cs)
    {
        ((temp_array[static_cast<u_int8_t>(cs)] = true), ...);
        int c = get();
        while (c != CHAR_EOF && !temp_array[static_cast<u_int8_t>(c)]) {
            c = next();
        }
        ((temp_array[static_cast<u_int8_t>(cs)] = false), ...);
        return c;
    }

    /* Moves the current char forward until EOF is reached or predicate(current_char) !=
     * predicate_is. Returns current char. */
    int skip_while(std::function<bool(int)> predicate, bool predicate_is)
    {
        int c = get();
        while (c != CHAR_EOF && (predicate(c) == predicate_is)) {
            c = next();
        }
        return c;
    }
};

enum class cmd_type { Block, Inline, Text, Unknown, InlineEmpty, Code };

/*
    Holds information about a command. For an inline command, end_symbol is the symbol it ends on.
    For a code command, end_symbol is the number of colons the command was started with.
*/
class cmd
{
public:
    std::string_view name;
    std::size_t indent{0};
    char end_symbol{'\0'};
    cmd_type type{cmd_type::Unknown};
    std::string_view arg;

    static cmd make_text(std::size_t indent)
    {
        cmd out;
        out.type = cmd_type::Text;
        out.indent = indent;
        out.name = std::string_view(NAME_TEXT);
        return out;
    }
};

class parser
{
    using callback_cmd_start
        = std::function<void(std::string_view name, cmd_type type, std::string_view args)>;
    using callback_cmd_end = std::function<void()>;
    using callback_text = std::function<void(std::string_view text)>;
    using callback_error = std::function<void(std::string msg)>;

public:
    parser(std::istream& stream, callback_cmd_start cstart, callback_cmd_end cend,
           callback_text ctext);
    ~parser();

    bool parse();

    std::string get_error_message() { return error_string; };

private:
    /* Buffer for input and output. */
    iobuff buffer;

    /* Callback funtions. */
    callback_cmd_start cstart;
    callback_cmd_end cend;
    callback_text ctext;

    /* Stack. */
    std::stack<cmd> cmd_stack;
    void start_command(cmd command);
    void end_command();
    void end_text();
    void end_all_commands();

    /* Error handling*/
    std::string error_string{"ERROR: "};

    /* Parsing. */
    cmd parse_name();
    std::string_view parse_arg();
    cmd parse_cmd_decl();
    std::string_view parse_code(std::size_t colons_to_end);
    void parse_on_text();
    void parse_on_colon();

    bool active_text_ended_on_whitespace = false;
};

parser::parser(std::istream& stream, callback_cmd_start cstart, callback_cmd_end cend,
               callback_text ctext)
    : buffer(stream), cstart{cstart}, cend{cend}, ctext{ctext}
{
}

parser::~parser() {}

/* Parses. Returns true if parsing was successful. Returns false*/
inline bool parser::parse()
{
    try {
        while (true) {
            buffer.skip_while(IS_WHITESPACE, true);
            switch (buffer.get()) {
            case CHAR_COLON:
                parse_on_colon();
                break;
            case CHAR_EOF:
                end_all_commands();
                return true;
            default:
                parse_on_text();
            }
        }
    } catch (const std::exception& e) {
        error_string.append(e.what())
            .append(" | at line / pos: ")
            .append(std::to_string(buffer.line()))
            .append(" / ")
            .append(std::to_string(buffer.indent()));
        return false;
    }
    return true;
}

/*
    Parses an argument and sets arg to it. Expects current_char to be on first char after '['.
    Leaves current_char on next char after ']'.
*/
std::string_view parser::parse_arg()
{
    std::string_view out;
    buffer.range_start();
    if (buffer.find(']') == CHAR_EOF) {
        throw std::runtime_error("Argument does not end.");
    }
    out = buffer.range_pause_before();
    buffer.next();
    return out;
}

/*
    Starts a command. Puts it on the stack and calls cstart / cend accordingly.
    If an inline or inline empty command is started, also starts a text command before it with the
   same indent.
*/
void parser::start_command(cmd command)
{
    if (command.type == cmd_type::Inline || command.type == cmd_type::InlineEmpty) {
        if (cmd_stack.empty()
            || (cmd_stack.top().type != cmd_type::Inline
                && cmd_stack.top().type != cmd_type::Text)) {
            start_command(cmd::make_text(command.indent));
        }
        cmd_stack.push(command);
        cstart(command.name, command.type, command.arg);
        if (command.type == cmd_type::InlineEmpty) {
            end_command();
        }
        return;
    }
    end_text();
    while (cmd_stack.size() > 0 && cmd_stack.top().indent >= command.indent) {
        end_command();
    }
    cmd_stack.push(command);
    cstart(command.name, command.type, command.arg);
    return;
}

/* Ends current command on stack. Will only end exactly one command. */
inline void parser::end_command()
{
    if (cmd_stack.size() < 1) {
        throw std::logic_error("end_command called on empty stack");
    }
    cend();
    cmd_stack.pop();
    return;
}

/* Ends a current text command. Also clears all variables for text parsing. Does nothing if there is
 * no text command active. */
void parser::end_text()
{
    active_text_ended_on_whitespace = false;
    while (!cmd_stack.empty() && cmd_stack.top().type == cmd_type::Inline) {
        end_command();
    }
    if (!cmd_stack.empty() && cmd_stack.top().type == cmd_type::Text) {
        end_command();
    }
    return;
}

/* Ends all commands on stack. */
inline void dodo::parser::end_all_commands()
{
    while (!cmd_stack.empty()) {
        end_command();
    }
}

/*
    Parses a command name. Expects current char on the first character after ':' of the command
   declaration. Assumes that a named command starts here, including an explict text command with :,
   block command with :: or code command with :::. Returns a filled command object with the indent
   and name of the command. For
    + inline special commands it sets end_symbol to the end symbol and type is inline
    + code commands it sets end_symbol to amount of :s and type is code
    + if the command name is empty (explicit text command or link inline command) the name is an
   empty string
    + if the command is a block command with :: sets type to block
    + otherwise fills name, indent and sets type to unknown
*/
cmd parser::parse_name()
{
    cmd command;
    int c = buffer.get();
    command.indent = buffer.indent() - 1;
    buffer.range_start();

    if (IS_INLINE_SPECIAL(c)) {
        command.name = buffer.range_pause_here();
        command.type = cmd_type::Inline;
        command.end_symbol = c;
        buffer.next();
        return command;
    }

    if (c == ':') {
        c = buffer.next();
        if (c == ':') {
            command.name = std::string_view(NAME_CODE);
            command.type = cmd_type::Code;
            command.end_symbol = 3;
            while (buffer.next() == ':') {
                command.end_symbol++;
            }
            return command;
        } else {
            command.name = std::string_view(NAME_BLOCK);
            command.type = cmd_type::Block;
            return command;
        }
    }

    buffer.skip_while(IS_ALLOWED_IN_NAME, true);
    command.name = buffer.range_pause_before();
    return command;
}

/*
    Parses a command declaration. Assumes current char is on the first character after the colon of
   the declaration. Assumes the cases of :; and :. and <char>:<whitespace> have been handled already
   and handles all other cases. The returned command has all fields filled correctly, including the
   indent.
*/
cmd parser::parse_cmd_decl()
{
    cmd out = parse_name();
    if (out.type == cmd_type::Inline) {
        return out;
    }

    if (buffer.get() == '[') {
        buffer.next();
        out.arg = parse_arg();
    } else {
        out.arg = std::string_view();
    }

    if (out.name.empty()) {
        if (buffer.get() == '{') {
            out.name = std::string("link");
            out.type = cmd_type::Inline;
            out.end_symbol = '}';
            buffer.next();
        } else {
            out.name = std::string_view(NAME_TEXT);
            out.type = cmd_type::Text;
        }
        return out;
    } else if (out.type == cmd_type::Code || out.type == cmd_type::Block) {
        return out;
    }

    switch (buffer.get()) {
    case '{':
        out.end_symbol = '}';
        out.type = cmd_type::Inline;
        break;
    case ';':
        out.type = cmd_type::InlineEmpty;
        break;
    default:
        out.type = cmd_type::Block;
        break;
    }
    buffer.next();
    return out;
}

/*
    Parses code. Expects the current_char to be on the first character after the code command
   declaration. Leaves the current_char on first char after the code end declaration. Has no other
   side effects other than the effects on the buffer. Returns the parsed code in a string_view.
*/
std::string_view parser::parse_code(std::size_t colons_to_end)
{
    size_t colon_count = 0;
    std::string_view out;

    if (IS_WHITESPACE(buffer.get())) {
        buffer.next();
    }
    buffer.range_start();

    while (buffer.find(':') != CHAR_EOF) {
        colon_count = 1;
        while (buffer.next() == ':' && colon_count < colons_to_end) {
            colon_count++;
        }
        if (colon_count == colons_to_end) {
            out = buffer.range_pause_before();
            out.remove_suffix(colon_count);
            return out;
        }
    }
    throw std::runtime_error("Code does not end.");
}

/*
    Parses on text. Starts and ends text commands and handles edge-cases in text parsing. Returns
    + if a colon (that is not parsed as text) is found returns on that colon
    + if EOF is found returns
    + if the text ends because of an empty newline returns there on the next line
    Handles the following edgecases:
    + <char>:<whitespace>
    + :.
    + :,<char>
    Assumes that the current character is non-whitespace and if a new text command starts here that
   the current character is the indent of that text command.
*/
void parser::parse_on_text()
{
    int c = buffer.get();
    int inline_end = CHAR_EOF;
    bool text_ends = false, inline_ends = false;
    std::string_view text;

    // start text command if not inside text currently
    if (cmd_stack.empty()
        || !(cmd_stack.top().type == cmd_type::Text || cmd_stack.top().type == cmd_type::Inline)) {
        start_command(cmd::make_text(buffer.indent()));
    } else if (cmd_stack.top().type == cmd_type::Inline) {
        inline_end = cmd_stack.top().end_symbol;
    }

    if (active_text_ended_on_whitespace || IS_WHITESPACE(buffer.prev())) {
        ctext(std::string_view(TEXT_SINGLE_SPACE));
    }
    active_text_ended_on_whitespace = false;

    buffer.range_start();
    c = buffer.get();
    while (c != CHAR_EOF) {
        if (c == inline_end) {
            inline_ends = true;
            break;
        } else if (c == CHAR_COLON) {
            if (IS_WHITESPACE(buffer.peek()) && !IS_WHITESPACE(buffer.prev())) {
                // do nothing
            } else if (buffer.peek() == '.') {
                buffer.range_pause_here();
                buffer.next();
                c = buffer.next();
                buffer.range_continue();
                continue;
            } else if (buffer.peek() == '}' && inline_end == '}') {
                buffer.range_pause_before();
                buffer.next();
                buffer.range_continue();
            } else if (buffer.peek() == ',') {
                buffer.range_pause_before();
                buffer.next();
                buffer.next();
                buffer.range_continue();
            } else {
                break;
            }
            c = buffer.next();
        } else if (IS_WHITESPACE(c)) {
            buffer.range_overwrite(CHAR_SPACE);
            buffer.range_pause_here();
            if (c == '\n') {
                buffer.next();
                c = buffer.skip_while([](int c) { return (c == CHAR_SPACE || c == CHAR_TAB); },
                                      true);
                if (c == '\n') {
                    text_ends = true;
                    buffer.range_continue();
                    break;
                }
            } else {
                c = buffer.skip_while([](int c) { return (c == CHAR_SPACE || c == CHAR_TAB); },
                                      true);
            }
            buffer.range_continue();
        } else [[likely]] {
            c = buffer.next();
        }
    }

    text = buffer.range_pause_before();
    if (text.ends_with(' ')) {
        text.remove_suffix(1);
        active_text_ended_on_whitespace = true;
    }
    if (!text.empty()) {
        ctext(text);
    }
    if (inline_ends) {
        end_command();
        buffer.next();
    } else if (text_ends) {
        end_text();
    }
    return;
}

/* Is called on a colon to parse from here. */
inline void parser::parse_on_colon()
{
    cmd command;
    switch (buffer.peek()) {
    case CHAR_SEMICOLON:
        end_text();
        end_command();
        buffer.next();
        buffer.next();
        return;
    case '.':
    case ',':
        parse_on_text();
        return;
    default:
        if (IS_WHITESPACE(buffer.peek()) && !IS_WHITESPACE(buffer.prev())) {
            parse_on_text();
            return;
        }
        break;
    }

    buffer.next();
    command = parse_cmd_decl();
    start_command(command);
    if (command.type == cmd_type::Code) {
        ctext(parse_code(command.end_symbol));
        end_command();
    }
}

} // namespace dodo
