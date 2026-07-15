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

// TODO: BUG: edge cases with : and :: do not work (text with : needs to be added)
// TODO: BUG: inline special short form does not work
// TODO: BUG: inline command start / end does not work

// TODO: parsing text is slow and naive
// TODO: code should respect first indent
// TODO: change parsing from write to start of buffer to in-buffer modification / no modification to
// be faster
// TODO: improve callback interface
// TODO: remove strings, use string_view to buffer for names ...
// TODO: parse_code uses bad old interface and has bug with last newline
// TODO: change buffer interface to start_output_session ...

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

constexpr bool IS_INLINE_SPECIAL(int c)
{
    switch (c) {
    case '!':
        return true;
    case '"':
        return true;
    case '$':
        return true;
    case '&':
        return true;
    case '/':
        return true;
    case '=':
        return true;
    case '?':
        return true;
    case '\\':
        return true;
    case '*':
        return true;
    case '+':
        return true;
    case '~':
        return true;
    case '\'':
        return true;
    case '|':
        return true;
    case '^':
        return true;
    default:
        return false;
    }
}

constexpr bool IS_WHITESPACE(int c)
{
    switch (c) {
    case CHAR_NEWLINE:
        return true;
    case CHAR_SPACE:
        return true;
    case CHAR_TAB:
        return true;
    default:
        return false;
    }
}

constexpr bool IS_BRACKET(int c)
{
    switch (c) {
    case ')':
        return true;
    case '(':
        return true;
    case '[':
        return true;
    case ']':
        return true;
    case '{':
        return true;
    case '}':
        return true;
    default:
        return false;
    }
}

constexpr bool IS_ALLOWED_IN_NAME(int c)
{
    return !(IS_BRACKET(c) || IS_WHITESPACE(c) || (c == CHAR_COLON) || (c == CHAR_EOF)
             || (c == CHAR_SEMICOLON));
}

bool IS_INLINE_SPECIAL(std::string& name)
{ return (name.length() == 1 && IS_INLINE_SPECIAL(name[0])); }

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

enum class cmd_type { Block, Inline, Text, Root, Unknown, InlineEmpty };

class cmd
{
public:
    std::string name{};
    std::size_t indent{0};
    char inline_ender{'\0'};
    cmd_type type{cmd_type::Unknown};

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
    void start_command(cmd command, std::string_view arg);
    void end_command();
    void end_text();

    /* Error handling*/
    std::string error_string{"ERROR: "};

    /* Parsing. */
    cmd parse_name();
    std::string_view parse_arg();
    cmd parse_cmd_decl(std::string_view& arg);
    bool handle_cmd_decl_edgecases();
    void parse_code(std::string_view& text, std::string_view& arg);
    void parse_text(std::string_view& text);

    /* True iff there should be a space before the next word in text parsing. */
    bool space_before = false;
    /* True iff text / inline commands are currently parsed and there were already chars outputted.
     */
    bool text_before = false;
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
        int c;
        std::string_view text;
        cmd command;

        while (true) {
            buffer.skip_whitespace();
            c = buffer.get();
            if (c == CHAR_COLON) {
                if (!handle_cmd_decl_edgecases()) {
                    buffer.next();
                    command = parse_cmd_decl(text);
                    if (command.type == cmd_type::Block) {
                        end_text();
                    }
                    start_command(command, text);
                }

            } else if (c == CHAR_EOF) {
                while (cmd_stack.size() > 1) {
                    end_command();
                }
                return true;
            } else {
                if (!text_before) {
                    start_command(cmd::make_text(buffer.indent()), std::string_view{});
                }
                parse_text(text);
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

/* Starts a command. Puts it on the stack and calls cstart / cend accordingly. */
void parser::start_command(cmd command, std::string_view arg)
{
    if (command.type == cmd_type::InlineEmpty) {
        cstart(command.name, command.type, arg);
        cend();
        return;
    }
    if (command.type == cmd_type::Inline) {
        if (cmd_stack.size() < 1
            || (cmd_stack.top().type != cmd_type::Inline
                && cmd_stack.top().type != cmd_type::Text)) {
            throw std::runtime_error("Inline command outside text pushed.");
        }
        cmd_stack.push(command);
        cstart(command.name, command.type, arg);
        return;
    }
    end_text();
    while (cmd_stack.size() > 0 && cmd_stack.top().indent >= command.indent) {
        end_command();
    }
    cmd_stack.push(command);
    cstart(command.name, command.type, arg);
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
    space_before = text_before = false;
    while (!cmd_stack.empty() && cmd_stack.top().type == cmd_type::Inline) {
        end_command();
    }
    if (!cmd_stack.empty() && cmd_stack.top().type == cmd_type::Text) {
        end_command();
    }
    return;
}

/*
    Parses a name and fills the command. If name is inline special, fills all fields, otherwise
   fills name and indent field. Expects current_char to be on first char of name. Leaves
   current_char on first char after name.
*/
cmd parser::parse_name()
{
    cmd command;
    std::string result{};
    int c = buffer.get();
    command.indent = buffer.indent() - 1;

    if (IS_INLINE_SPECIAL(c)) {
        command.name.push_back('c');
        command.type = cmd_type::Inline;
        command.inline_ender = c;
    }

    while (IS_ALLOWED_IN_NAME(c)) {
        command.name.push_back(c);
        c = buffer.next();
    }

    return command;
}

/*
    Parses a command declaration and fills the command accordingly. Assumes that a command starts
   here and the ':' is not part of text. Assumes current_char on the first char after ':'. Leaves
   current_char on the first char after the declaration ends. Also assumes that the command is a
   normal one and not an edge case.
*/
cmd parser::parse_cmd_decl(std::string_view& arg)
{
    cmd out = parse_name();
    if (out.type == cmd_type::Inline) {
        return out;
    }

    if (buffer.get() == '[') {
        buffer.next();
        arg = parse_arg();
    } else {
        arg = std::string_view{};
    }
    switch (buffer.get()) {
    case '{':
        out.inline_ender = '}';
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
    Expects to be called on a colon. Returns true if handled and false if no edge case occured.
*/
bool parser::handle_cmd_decl_edgecases()
{
    switch (buffer.peek()) {
    case CHAR_COLON:
        end_text();
        buffer.next();
        if (buffer.next() == CHAR_COLON) {
            std::string_view text, arg;
            buffer.next();
            parse_code(text, arg);
            cstart(std::string("code"), cmd_type::Block, arg);
            cstart(std::string("text"), cmd_type::Text, std::string_view{});
            ctext(text);
            cend();
            cend();
            return true;
        } else {
            cmd c{std::string("block"), buffer.indent() - 2, '\0', cmd_type::Block};
            std::string_view arg;
            if (buffer.get() == '[') {
                arg = parse_arg();
            }
            start_command(c, arg);
            return true;
        }
    case '.':
        buffer.next();
        return true;
    case CHAR_SEMICOLON:
        end_text();
        end_command();
        buffer.next();
        buffer.next();
        return true;
    default:
        return false;
    }
}

/*
    Parses code. Expects current_char on the first char after ':::' and ends on the first char after
   the code (and its ending symbol) ends. Puts the code verbatim into text.
*/
void parser::parse_code(std::string_view& text, std::string_view& arg)
{
    size_t activation_colons = 3;
    size_t colon_count = 0;
    int c = buffer.get();
    std::size_t start;

    while (c == CHAR_COLON) {
        activation_colons++;
        c = buffer.next();
    }

    if (buffer.get() == '[') {
        buffer.next();
        arg = parse_arg();
    }
    if (IS_WHITESPACE(buffer.get())) {
        buffer.next();
    }
    start = buffer.get_cursor();

    while (buffer.find(':')) {
        colon_count = 1;
        while (buffer.next() == ':' && colon_count < activation_colons) {
            colon_count++;
        }
        if (colon_count == activation_colons) {
            text = buffer.view(buffer.output_range(start, buffer.get_cursor() - activation_colons));
            return;
        }
    }
    throw std::runtime_error("Code does not end.");
}

/*
    Parses text. Will end inline commands automatically and end_text() if there is an empty newline.
   Otherwise returns when another command is starting. Will return on a newline if it ended there
   because of an empty newline and return on a colon or CHAR_EOF otherwise.
*/
void parser::parse_text(std::string_view& text)
{
    int c = buffer.get();
    int inline_end = CHAR_EOF;
    std::size_t start = buffer.get_next_output_cursor();
    bool toend = false;

    if (!cmd_stack.empty() && cmd_stack.top().type == cmd_type::Inline) {
        inline_end = cmd_stack.top().inline_ender;
    }

    if (text_before) {
        if (IS_WHITESPACE(c)) {
            space_before = true;
        }
    } else {
        text_before = true;
    }

    buffer.skip_space_tab();
    c = buffer.get();
    while (c != CHAR_EOF) {
        if (c == inline_end) {
            end_command();
            if (!cmd_stack.empty() && cmd_stack.top().type == cmd_type::Inline) {
                inline_end = cmd_stack.top().inline_ender;
            } else {
                inline_end = CHAR_EOF;
            }
        } else if (c == CHAR_COLON) {
            if (IS_WHITESPACE(buffer.peek()) && !IS_WHITESPACE(buffer.prev())) {
                buffer.output_char(c);
            } else if (buffer.peek() == '.') {
                buffer.output_char(c);
                buffer.next();
            } else {
                break;
            }
        } else if (IS_WHITESPACE(c)) {
            space_before = true;
            if (c == '\n') {
                c = buffer.next();
                buffer.skip_space_tab();
                c = buffer.get();
                if (c == '\n') {
                    toend = true;
                    break;
                }
                continue;
            }
            buffer.skip_space_tab();
            c = buffer.get();
            continue;
        } else {
            if (space_before) {
                buffer.output_char(CHAR_SPACE);
                space_before = false;
            }
            buffer.output_char(c);
        }
        c = buffer.next();
    }
    text = buffer.view(start);
    ctext(text);
    if (toend) {
        end_text();
    }
    return;
}

} // namespace dodo
