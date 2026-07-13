#pragma once

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
};

enum class cmd_type { Block, Inline, Text, Root, Unknown, InlineEmpty };

class cmd
{
public:
    std::string name{};
    std::size_t indent{0};
    char inline_ender{'\0'};
    cmd_type type{cmd_type::Unknown};
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

    void parse();
    void set_error_callback(callback_error cerror);

private:
    /* Buffer for input and output. */
    iobuff buffer;

    /* Callback funtions. */
    callback_cmd_start cstart;
    callback_cmd_end cend;
    callback_text ctext;
    callback_error cerror;

    /* Error handling. */
    enum class status { Okay, ArgNotEnded, CodeNotEnded, InlineNotEnded, CmdStackError };
    bool error_callback_enabled = false;
    void error(status type, std::string info);

    std::string error_msg(status type)
    {
        switch (type) {
        case status::ArgNotEnded:
            return std::string("Argument did not end with ']'");
        case status::CodeNotEnded:
            return std::string("Code does not end");
        case status::InlineNotEnded:
            return std::string("Inline command does not end");
        case status::CmdStackError:
            return std::string("Error with command stack");
        default:
            return std::string("Error");
        }
    }

    // TODO: All parse functions returning a status do not error handle, the first that does not
    // does.

    /* Stack. */
    std::stack<cmd> cmd_stack;
    status start_command(cmd command, std::string_view arg);
    status end_command();

    /* Parsing. */
    status parse_name(cmd& command);
    status parse_arg(std::string_view& arg);
    status parse_cmd_decl(cmd& command, std::string_view& arg);
    bool handle_cmd_decl_edgecases();
    status parse_code(std::string_view& text, std::string_view& arg);
    bool parse_text();

    /* True iff the next word should place a space before it.*/
    bool space_before = false;
};

parser::parser(std::istream& stream, callback_cmd_start cstart, callback_cmd_end cend,
               callback_text ctext)
    : buffer(stream), cstart{cstart}, cend{cend}, ctext{ctext}
{
}

inline void parser::set_error_callback(callback_error cerror)
{
    this->cerror = cerror;
    error_callback_enabled = true;
}

inline void parser::error(parser::status type, std::string info)
{
    if (!error_callback_enabled) {
        return;
    }
    std::string out;
    out.reserve(256);
    out.append("ERROR: ")
        .append(error_msg(type))
        .append("\n | Line / Pos: ")
        .append(std::to_string(buffer.line()))
        .append(" / ")
        .append(std::to_string(buffer.line()))
        .append("\n | Info: ")
        .append(info);
    cerror(out);
}

parser::~parser() {}

/**/ // TODO: implement
inline void parser::parse()
{
    while (getc() != CHAR_EOF) {
        skip_whitespace();
        if (getc() == CHAR_COLON) {
            // handle command start
        } else if (getc() == CHAR_EOF) {
            // end parsing
        } else {
            // parse text
            push_text_cmd(get_current_indent());
            cmd_stack.top()->arg_start = get_output_cursor();
            while (parse_text()) {
                // read command and check what to do
                //  check edgecase
                //  parse_cmd_decl
                // if block then break
                //  else push inline etc and continue
            }
            end_text();
        }
    }
}

/*
    Parses an argument and sets arg to it. Expects current_char to be on first char after '['.
    Leaves current_char on next char after ']'.
*/
parser::status parser::parse_arg(std::string_view& arg)
{
    std::size_t start = buffer.get_cursor();

    if (buffer.find(']')) {
        start = buffer.output_range(start, buffer.get_cursor());
        arg = buffer.view(start);
        buffer.next();
    } else {
        return status::ArgNotEnded;
    }

    return status::Okay;
}

/* Starts a command. Puts it on the stack and calls cstart / cend accordingly. */
inline parser::status parser::start_command(cmd command, std::string_view arg)
{
    if (cmd_stack.size() < 1) {
        return status::CmdStackError;
    }

    while (cmd_stack.top().type != cmd_type::Root && cmd_stack.top().indent >= command.indent) {
        end_command();
    }
    cmd_stack.push(command);
    cstart(command.name, command.type, arg);
    return status::Okay;
}

/* Ends the top command on the stack and calls cend. */
inline parser::status parser::end_command()
{
    if (cmd_stack.size() < 1) {
        return status::CmdStackError;
    }
    cend();
    cmd_stack.pop();
    return status::Okay;
}

/*
    Parses a name and fills the command. If name is inline special, fills all fields, otherwise
   fills name and indent field. Expects current_char to be on first char of name. Leaves
   current_char on first char after name.
*/
parser::status parser::parse_name(cmd& command)
{
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

    return status::Okay;
}

/*
    Parses a command declaration and fills the command accordingly. Assumes that a command starts
   here and the ':' is not part of text. Assumes current_char on the first char after ':'. Leaves
   current_char on the first char after the declaration ends. Also assumes that the command is a
   normal one and not an edge case.
*/
parser::status parser::parse_cmd_decl(cmd& command, std::string_view& arg)
{
    status s;

    parse_name(command);
    if (command.type == cmd_type::Inline) {
        return status::Okay; // Command is inline special and finished
    }

    if (buffer.get() == '[') {
        buffer.next();
        s = parse_arg(arg);
        if (s != status::Okay) {
            return s;
        }
    }
    switch (buffer.get()) {
    case '{':
        command.inline_ender = '}';
        command.type = cmd_type::Inline;
        break;
    case ';':
        command.type = cmd_type::InlineEmpty;
        break;
    default:
        command.type = cmd_type::Block;
        break;
    }

    return status::Okay;
}

/*
    Expected to be called on the first character after a colon. Returns true iff and edge case was
   found. If it returns false, the cursor is not moved. If it returns true, the cursor is on the
   first char after the edge case was handled. Checks and handles the following cases:
    + ':.' starts text command (or resumes text command) on '.'
    + ':;' ends the scope of the last non-text command, is ignored if that command is the root
   command
    + '::' starts a block command
    + ':::' code command
    + '<whitespace>:<whitespace> text command
    + '<char>:<whitespace>' no command, parse as text
*/
inline bool parser::handle_cmd_decl_edgecases()
{
    switch (getc()) {
    case CHAR_DOT: // parse should parse this as text
        return true;
    case CHAR_SEMICOLON:
        break; // TODO: end the scope with function end_scope()
    case CHAR_COLON:
        break; // TODO: check if code or not and handle with parse_code()

    default:
        break;
    }
    if (IS_WHITESPACE(getc())) {
        if (IS_WHITESPACE(prevc())) {
            // make text command
        } else {
            // parse as text
        }
    }
    return false;
}

/*
    Parses code. Expects current_char on the first char after ':::' and ends on the first char after
   the code (and its ending symbol) ends. Puts the code verbatim into text.
*/
parser::status parser::parse_code(std::string_view& text, std::string_view& arg)
{
    size_t activation_colons = 3;
    size_t colon_count = 0;
    int c = buffer.get();
    status s;
    std::size_t start;

    while (c == CHAR_COLON) {
        activation_colons++;
        c = buffer.next();
    }

    if (buffer.get() == '[') {
        s = parse_arg(arg);
        if (s != status::Okay) {
            return s;
        }
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
            text = buffer.view(buffer.output_range(start, buffer.get_cursor()));
            return status::Okay;
        }
    }
    return status::CodeNotEnded;
}

/*
    Parses text until:
    + A colon starting a command is found (returns on the colon and outputs with ctext)
    + EOF is reached
    Ends inline commands itself. Assumes that space_before is set exactly if this text is part of a
   continuing text command and a space should be set before the next word.
*/
// TODO: Implement
parser::status parser::parse_text(std::string_view& text)
{
    int c;
    int inline_end;

    buffer.skip_space_tab();
    c = buffer.get();
    if (space_before && c !=) {
        while (c != CHAR_EOF) {
            if (is_inline_cmd_ender(c)) {
                // end_inline_cmd()
            } else if (c == CHAR_COLON) {
                // TOD0: first check next char to see if it is an inline ender and colon is escape!
                return true;
            } else if (IS_WHITESPACE(c)) {
                // check for empty newline
                // skip whitespace and set whitespace_before = true
            } else {
                // we have a character
                if (whitespace_before) {
                    whitespace_before = false;
                    output_char(CHAR_SPACE);
                    text_cmd.arg_len++;
                }
                output_char(c);
                text_cmd.arg_len++;
            }
            c = nextc();
        }
    }
    return false;
}

} // namespace dodo
