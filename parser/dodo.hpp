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

// TODO: change error handling to error flag in class and returning true / false
// TODO: add error handling
// TODO: parsing text is slow and naive
// TODO: edge case where text starts with :?

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

    inline std::size_t get_next_output_cursor() {
        return next_output_cursor;
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

    static cmd make_text(std::size_t indent) {
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
    enum class status { Okay, ArgNotEnded, CodeNotEnded, InlineNotEnded, CmdStackError, NoAction };

    inline bool failed(status s) { return s != status::Okay; }

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
    status end_text();

    /* Parsing. */
    status parse_name(cmd& command);
    status parse_arg(std::string_view& arg);
    status parse_cmd_decl(cmd& command, std::string_view& arg);
    status handle_cmd_decl_edgecases();
    status parse_code(std::string_view& text, std::string_view& arg);
    status parse_text(std::string_view& text);

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
    int c;
    std::string_view text;
    cmd command;

    cmd_stack.push(cmd{std::string("root"), 0, '\0', cmd_type::Root});
    while(true) {

    buffer.skip_whitespace();
    c = buffer.get();
    if(c == CHAR_COLON) {
        if(handle_cmd_decl_edgecases() == status::NoAction) {
            buffer.next();
            command = cmd{};
            parse_cmd_decl(command, text);
            if(command.type == cmd_type::Block) end_text();
            start_command(command, text);
        }
       
    } else if(c == CHAR_EOF) {
        while(cmd_stack.size() > 1) {
            end_command();
        }
        return;
    } else {
        if(!text_before) {
        start_command(cmd::make_text(buffer.indent()), std::string_view{});
        }
        if(failed(parse_text(text))) {
            error(status::Okay, "text error");
            return;
        } 
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
    cmd_type topt;
    if (cmd_stack.size() < 1) {
        return status::CmdStackError;
    }
    topt = cmd_stack.top().type;
    if(command.type == cmd_type::InlineEmpty) {
        cstart(command.name, command.type, arg);
        cend();
        return status::Okay;
    }
    if(command.type == cmd_type::Inline || command.type == cmd_type::Text) {
        cmd_stack.push(command);
        cstart(command.name, command.type, arg);
        return status::Okay;
    }

    while(topt == cmd_type::Inline || topt == cmd_type::Text) {
        end_command();
        topt = cmd_stack.top().type;
    }
    while (topt != cmd_type::Root && cmd_stack.top().indent >= command.indent) {
        end_command();
        topt == cmd_stack.top().type;
    }
    cmd_stack.push(command);
    cstart(command.name, command.type, arg);
    return status::Okay;
}

/* Ends the top command on the stack and calls cend. Errors when the only command on stack is root. */
inline parser::status parser::end_command()
{
    if (cmd_stack.size() < 2) {
        return status::CmdStackError;
    }
    cend();
    cmd_stack.pop();
    return status::Okay;
}

inline parser::status parser::end_text() { 
    space_before = text_before = false;
    if(cmd_stack.size() < 1) return status::CmdStackError;
    while(cmd_stack.top().type == cmd_type::Inline) {
        end_command();
    }
    if(cmd_stack.top().type != cmd_type::Text) return status::Okay;
    end_command();
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
    } else {
        arg = std::string_view{};
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
    buffer.next();

    return status::Okay;
}

/*
    Expects to be called on a colon.
*/
inline parser::status parser::handle_cmd_decl_edgecases()
{
    switch(buffer.peek()) {
        case CHAR_COLON:
            end_text();
            buffer.next();
            if(buffer.next() == CHAR_COLON) {
                std::string_view text, arg;
                parse_code(text, arg);
                cstart(std::string("code"), cmd_type::Block, arg);
                cstart(std::string("text"), cmd_type::Text, std::string_view{});
                ctext(text);
                cend(); cend();
            } else {
                cmd c{std::string("block"), buffer.indent() - 2, '\0', cmd_type::Block};
                std::string_view arg;
                if(buffer.get() == '[') {
                    parse_arg(arg);
                }
                start_command(c, arg);
                return status::Okay;
            } 
        case '.':
        buffer.next();
            return status::Okay;
        case CHAR_SEMICOLON:
            end_text();
            end_command();
            buffer.next();
            buffer.next();
            return status::Okay;
        default:
            return status::NoAction;
    }
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
    Parses text. Will end inline commands automatically and end_text() if there is an empty newline. Otherwise
    returns when another command is starting. Will return on a newline if it ended there because of an empty newline
    and return on a colon or CHAR_EOF otherwise.
*/
parser::status parser::parse_text(std::string_view& text)
{
    int c = buffer.get();
    int inline_end = CHAR_EOF;
    std::size_t start = buffer.get_next_output_cursor();
    bool toend = false;

    if(cmd_stack.top().type == cmd_type::Inline) {
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
            if(cmd_stack.top().type == cmd_type::Inline) {
                inline_end = cmd_stack.top().inline_ender;
            } else {
                inline_end = CHAR_EOF;
            }
        } else if (c == CHAR_COLON) {
            if(IS_WHITESPACE(buffer.peek()) && !IS_WHITESPACE(buffer.prev())) {
                buffer.output_char(c);
            } else if(buffer.peek() == '.') {
                buffer.output_char(c);
                buffer.next();
            } else {
                break;
            }
        } else if (IS_WHITESPACE(c)) {
                space_before = true;
            if(c == '\n') {
                c = buffer.next();
                buffer.skip_space_tab();
                c = buffer.get();
                if(c == '\n') {
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
    if(toend) end_text();

    return status::Okay;
}

} // namespace dodo
