#pragma once

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>

// TODO: MISSING: code should respect indent and last newline

// TODO: parsing text is slow and naive
// TODO: change parsing from write to start of buffer to in-buffer modification / no modification to
// be faster
// TODO: improve callback interface
// TODO: remove strings, use string_view to buffer for names ...
// TODO: remake inlince special characters etc.

namespace dodo
{

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

class iobuff
{
private:
    std::string buffer;
    std::size_t cursor;
    std::size_t next_output_cursor;
    std::size_t current_indent = 0;
    std::size_t current_line = 0;
    int current_char;
    std::size_t buffer_size;

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
        next_output_cursor = 0;
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
        return current_char = buffer[cursor];
    }

    inline int prev() { return buffer[cursor - 1]; }

    inline int peek()
    {
        if (cursor + 1 >= buffer_size) [[unlikely]] {
            return CHAR_EOF;
        }
        return buffer[cursor + 1];
    }

    inline std::size_t indent() { return current_indent; }

    inline std::size_t get_cursor() { return cursor; }

    inline std::size_t get_size() { return buffer.size(); }

    /* View of [start, end). */
    inline std::string_view view(std::size_t start, std::size_t end)
    { return std::string_view(buffer.data() + start, (end - start)); }

    /* View from start to last character pushed. */
    inline std::string_view view(std::size_t start) { return view(start, next_output_cursor); }

    inline void skip_whitespace()
    {
        int c = get();
        while (IS_WHITESPACE(c) && !(c == CHAR_EOF)) {
            c = next();
        }
    }

    inline void skip_space_tab()
    {
        int c = get();
        while (c == CHAR_SPACE || c == CHAR_TAB) {
            c = next();
        }
    }

    /*
        Tries to find the first occurence of c on/after the current character. If found returns true
       and moves the current character there. If not found returns false and does nothing else.
    */
    inline bool find(char c)
    {
        std::size_t last_cursor = cursor, last_indent = current_indent;
        int last_char = current_char;
        while (get() != c && get() != CHAR_EOF) {
            next();
        }
        if (get() == c) {
            return true;
        } else {
            cursor = last_cursor;
            current_indent = last_indent;
            current_char = last_char;
            return false;
        }
    }

    inline std::size_t line() { return current_line; }

    inline std::size_t output_char(char c)
    {
        buffer[next_output_cursor] = c;
        return next_output_cursor++;
    }

    /* Outputs range [start, last). */
    inline std::size_t output_range(std::size_t start, std::size_t last)
    {
        std::copy(buffer.data() + start, buffer.data() + last, buffer.data() + next_output_cursor);
        next_output_cursor += last - start;
        return next_output_cursor - (last - start);
    }

    inline std::size_t get_next_output_cursor() { return next_output_cursor; }
};

enum class cmd_type { Block, Inline, Text, Unknown, InlineEmpty, Code };

/*
    Holds information about a command. For an inline command, end_symbol is the symbol it ends on.
    For a code command, end_symbol is the number of colons the command was started with.
*/
class cmd
{
public:
    std::string name;
    std::size_t indent{0};
    char end_symbol{'\0'};
    cmd_type type{cmd_type::Unknown};
    std::string_view arg;

    static cmd make_text(std::size_t indent)
    {
        cmd out;
        out.type = cmd_type::Text;
        out.indent = indent;
        out.name = std::string("text");
        return out;
    }
};

class parser
{
    using callback_cmd_start
        = std::function<void(std::string name, cmd_type type, std::string_view args)>;
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

inline bool parser::parse()
{
    try {
        while (true) {
            buffer.skip_whitespace();
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
    std::size_t start = buffer.get_cursor();
    std::string_view out;

    if (buffer.find(']')) {
        start = buffer.output_range(start, buffer.get_cursor());
        out = buffer.view(start);
        buffer.next();
    } else {
        throw std::runtime_error("Argument does not end.");
    }
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

    if (IS_INLINE_SPECIAL(c)) {
        command.name.push_back(c);
        command.type = cmd_type::Inline;
        command.end_symbol = c;
        buffer.next();
        return command;
    }

    if (c == ':') {
        c = buffer.next();
        if (c == ':') {
            command.name = std::string("code");
            command.type = cmd_type::Code;
            command.end_symbol = 3;
            while (buffer.next() == ':') {
                command.end_symbol++;
            }
            return command;
        } else {
            command.name = std::string("block");
            command.type = cmd_type::Block;
            return command;
        }
    }

    while (IS_ALLOWED_IN_NAME(c)) {
        command.name.push_back(c);
        c = buffer.next();
    }

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
            out.name = std::string("text");
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
    std::size_t start;

    if (IS_WHITESPACE(buffer.get())) {
        buffer.next();
    }
    start = buffer.get_cursor();

    while (buffer.find(':')) {
        colon_count = 1;
        while (buffer.next() == ':' && colon_count < colons_to_end) {
            colon_count++;
        }
        if (colon_count == colons_to_end) {
            return buffer.view(buffer.output_range(start, buffer.get_cursor() - colons_to_end));
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
    std::size_t start = buffer.get_next_output_cursor();
    bool text_ends = false, inline_ends = false;
    std::string_view text;

    if (active_text_ended_on_whitespace || IS_WHITESPACE(buffer.prev())) {
        buffer.output_char(CHAR_SPACE);
    }
    active_text_ended_on_whitespace = false;

    // start text command if not inside text currently
    if (cmd_stack.empty()
        || !(cmd_stack.top().type == cmd_type::Text || cmd_stack.top().type == cmd_type::Inline)) {
        start_command(cmd::make_text(buffer.indent()));
    } else if (cmd_stack.top().type == cmd_type::Inline) {
        inline_end = cmd_stack.top().end_symbol;
    }

    c = buffer.get();
    while (c != CHAR_EOF) {
        if (c == inline_end) {
            inline_ends = true;
            break;
        } else if (c == CHAR_COLON) {
            if (IS_WHITESPACE(buffer.peek()) && !IS_WHITESPACE(buffer.prev())) {
                buffer.output_char(':');
            } else if (buffer.peek() == '.') {
                buffer.output_char(':');
                buffer.next();
            } else if (buffer.peek() == '}' && inline_end == '}') {
                buffer.output_char('}');
                buffer.next();
            } else if (buffer.peek() == ',') {
                buffer.next();
                buffer.next();
                if (!IS_WHITESPACE(buffer.get()) && buffer.get() != CHAR_EOF) {
                    buffer.output_char(buffer.get());
                }
            } else {
                break;
            }
        } else if (IS_WHITESPACE(c)) {
            buffer.output_char(CHAR_SPACE);
            if (c == '\n') {
                c = buffer.next();
                buffer.skip_space_tab();
                c = buffer.get();
                if (c == '\n') {
                    text_ends = true;
                    break;
                }
                continue;
            }
            buffer.skip_space_tab();
            c = buffer.get();
            continue;
        } else [[likely]] {
            buffer.output_char(c);
        }
        c = buffer.next();
    }

    text = buffer.view(start);
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
