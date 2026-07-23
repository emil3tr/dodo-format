/**
 * A Library to parse text in the dodo format. Interesting to the user are
 * + class parser
 * + enum class cmd_type
 * + functions is_textual, is_inline, is_block
 */

#pragma once

#include <algorithm>
#include <concepts>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <span>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>

namespace dodo
{

constexpr char NAME_CODE[] = "code";
constexpr char NAME_TEXT[] = "text";
constexpr char NAME_LINK[] = "link";
constexpr char NAME_BLOCK[] = "block";

inline constexpr char CHAR_SPACE = 32;
inline constexpr char CHAR_TAB = 9;
inline constexpr char CHAR_NEWLINE = 10;
inline constexpr char CHAR_COLON = 58;
inline constexpr char CHAR_SEMICOLON = 59;

inline constexpr std::size_t STACK_BUFFER_SIZE = (4 * 1024);

constexpr u_int8_t FLAG_INLINE_SPECIAL = 1;
constexpr u_int8_t FLAG_WHITESPACE = 2;
constexpr u_int8_t FLAG_ALLOWED_IN_NAME = 4;

constexpr std::array<u_int8_t, 256> MAKE_CHARFLAG_ARRAY()
{
    std::array<u_int8_t, 256> out{};
    for (int i = 0; i < 256; i++) {
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

constexpr std::array<u_int8_t, 256> CHARFLAG_ARRAY = MAKE_CHARFLAG_ARRAY();

constexpr bool IS_INLINE_SPECIAL(char c)
{ return ((CHARFLAG_ARRAY[static_cast<u_int8_t>(c)] & FLAG_INLINE_SPECIAL)); }

constexpr bool IS_WHITESPACE(char c)
{ return ((CHARFLAG_ARRAY[static_cast<u_int8_t>(c)] & FLAG_WHITESPACE)); }

constexpr bool IS_ALLOWED_IN_NAME(char c)
{ return ((CHARFLAG_ARRAY[static_cast<u_int8_t>(c)] & FLAG_ALLOWED_IN_NAME)); }

constexpr bool IS_TAB_SPACE(char c) { return c == CHAR_TAB || c == CHAR_SPACE; }

constexpr bool IS_TEXTUAL(char c) { return c != ':' && !IS_WHITESPACE(c); }

/* All range functions assume that the current character is not CHAR_EOF. */
class inbuff
{
private:
    std::string buffer{};
    char* cursor;
    char* buffer_end;
    char* buffer_start;
    std::size_t current_indent{0};
    std::size_t current_line{0};
    char* range_start;

    /*
        Each function touching temp_array must maintan the following invariant:
        temp_array[i] == false for all i in {0, 255}
    */
    std::array<bool, 256> temp_array{};

public:
    inbuff(std::istream& stream, std::size_t estimated_size = (32 * 1024))
    {
        char local_buffer[STACK_BUFFER_SIZE];
        buffer.reserve(estimated_size);

        while (stream.read(local_buffer, STACK_BUFFER_SIZE)) {
            buffer.append(local_buffer, local_buffer + stream.gcount());
        }

        if (stream.gcount() > 0) {
            buffer.append(local_buffer, local_buffer + stream.gcount());
        }

        buffer_start = buffer.data();
        buffer_end = buffer.data() + buffer.size();
        cursor = buffer_start;
        range_start = buffer_start;
    }

    inline char get() { return *cursor; }

    inline bool has() { return (cursor >= buffer_start && cursor < buffer_end); }

    inline bool next()
    {
        cursor++;
        if (!has()) [[unlikely]] {
            cursor = buffer_end;
            return false;
        }
        if (get() == CHAR_NEWLINE) {
            current_indent = 0;
            current_line++;
        } else {
            current_indent++;
        }
        return true;
    }

    inline bool next(std::size_t count)
    {
        for (std::size_t i = 0; i < count; i++) {
            next();
        }
        return has();
    }

    inline char prev()
    {
        if (cursor == buffer_start) {
            return '\n';
        }
        return *(cursor - 1);
    }

    inline bool can_peek() { return (cursor + 1 < buffer_end); }

    inline int peek() { return *(cursor + 1); }

    inline std::size_t indent() { return current_indent; }

    inline std::size_t line() { return current_line; }

    /* Starts a new range on and including the current_char. */
    inline void start_range() { range_start = cursor; }

    inline std::string_view get_range_here() { return std::string_view(range_start, cursor + 1); }

    inline std::string_view get_range_before() { return std::string_view(range_start, cursor); }

    inline std::string_view get_range_before(std::size_t remcount)
    {
        std::string_view out(range_start, cursor);
        out.remove_suffix(remcount);
        return out;
    }

    /*
        Overwrites the current char in the range with another char.
        Assumes the current range is not paused.
    */
    inline void overwrite(char c) { *cursor = c; }

    /* Moves the current char forward until EOF is reached or it sits on one of the passed
     * characters. Returns true if a char was found, false if EOF was reached. */
    inline bool find(std::convertible_to<u_int8_t> auto... cs)
    {
        ((temp_array[static_cast<u_int8_t>(cs)] = true), ...);
        if (!has) {
            return false;
        }
        while (!temp_array[static_cast<u_int8_t>(get())] && next()) {}
        ((temp_array[static_cast<u_int8_t>(cs)] = false), ...);
        return has();
    }

    /* Moves the current char forward until EOF is reached or predicate(current_char) !=
     * predicate_is. Returns false if EOF is reached and true else. */
    inline bool skip_while(std::invocable<char> auto&& predicate, bool predicate_is)
    {
        while (has() && (predicate(get()) == predicate_is)) {
            next();
        }
        return has();
    }

    /**
     * Checks if the given string exists in the buffer starting at the current cursor. Will not move
     * the cursor. Will not modify indent, line.
     */
    inline bool substring_here(std::string_view s)
    {
        char* old_cursor = cursor;
        for (char c : s) {
            if (!has() || get() != c) {
                cursor = old_cursor;
                return false;
            }
            cursor++;
        }
        cursor = old_cursor;
    }

    /**
     * @brief Calls next while the current character is c, at most limit-1 times. returns the number
     * of c's found. The number will be smaller or equal to limit. Leaves current char on the next
     * character after the last c found. (Unchanged if no c found.)
     * @param c character to find
     * @param limit max amount of characters to find, shall be > 0, If <= 0 will void.
     * @return Amount of c's found
     */
    inline std::size_t count_here(char c, std::size_t limit)
    {
        std::size_t count = 0;
        while (count < limit && has() && get() == c) {
            count++;
            next();
        }
        return count;
    }
};

/**
 * Specifies the type of a command.
 * + Text commands directly enclose text and Inline or InlineEmpty commands
 * + Code commands only directly enclose text
 * + Block commands directly enclose Text, Block and Code commands
 * + Inline commands directly enclose text or other Inline commands
 * + InlineEmpty commands enclose nothing
 * + Unknown is only used internally and will never appear as an output
 *
 * The callback function to output text is only called while a text or code command are
 * active. Note that Text / Code commands are functionally equivalent, but Code
 * commands will only contain text. Also Inline / InlineEmpty commands are functionally
 * equivalent, but InlineEmpty commands are closed immediatelly.
 */
enum class cmd_type { Block, Inline, Text, Unknown, InlineEmpty, Code };

/**
 * True exactly for Text and Code.
 */
inline constexpr bool is_textual(cmd_type t)
{ return (t == cmd_type::Text || t == cmd_type::Code); }

/**
 * True exactly for Inline and InlineEmpty.
 */
inline constexpr bool is_inline(cmd_type t)
{ return (t == cmd_type::Inline || t == cmd_type::InlineEmpty); }

/**
 * True exactly for Block.
 */
inline constexpr bool is_block(cmd_type t) { return (t == cmd_type::Block); }

/*
    Holds information about a command. For an inline command, end_symbol is the symbol it ends on.
    For a code command, end_symbol is the number of colons the command was started with.
*/
class cmd
{
public:
    enum class type { Inline, InlineEmpty, Text, Code, Block, Unknown };
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

/**
 * A class to hold memory and methods related to parsing.
 */
class parser
{
public:
    parser(std::istream& stream, std::size_t estimated_size);
    ~parser();

    bool parse(std::invocable<std::string_view, cmd_type, std::string_view> auto&& start_callback,
               std::invocable<> auto&& end_callback,
               std::invocable<std::string_view> auto&& text_callback);

    /**
     * Returns an error message. In case parsing failed, it will contain useful information.
     */
    std::string get_error_message();

private:
    /* Buffer for input and output. */
    inbuff buffer;
    std::vector<std::string_view> output_buffer{};

    inline void output_add(std::string_view v) { output_buffer.push_back(v); }

    inline void output_flush(std::invocable<std::span<std::string_view>> auto&& text_callback)
    {
        if (output_buffer.empty()) {
            return;
        }
        text_callback(std::span<std::string_view>(output_buffer));
        output_buffer.clear();
    }

    /**
     * Default = nothing about state known
     * CommandHere = cursor on colon which starts a command
     * TextHere = cursor on character that is part of text
     * EndCommand = cursor after :; which ends current command
     * StreamEnds = stream has no characters anymore
     * Error = error
     * InlineEnds = cursor on char after inline ender and current inline command ends
     * TextEnds = current text command ends and cursor on char after it
     * ColonHere = cursor on colon, but we have not checked what it means yet
     */
    enum class status {
        Default,
        CommandHere,
        TextHere,
        EndCommand,
        StreamEnds,
        Error,
        TextEnds,
        InlineEnds,
        ColonHere
    };
    status current_status{Default};

    /* Stack. */
    std::stack<cmd> cmd_stack;
    void start_command(cmd command);
    void end_command();
    void end_text();
    void end_all_commands();

    /* Error handling*/
    enum class error_t {
        None,
        CodeDoesNotEnd,

    };
    std::string_view error_info{};
    error_t error_type{None};

    /* Parsing. */
    cmd parse_name();
    std::string_view parse_arg();
    cmd parse_cmd_decl();
    status parse_code(std::size_t colons_to_end);
    status parse_colon();
    status parse_text(char inline_end);
    bool parse_text_whitespace();
    bool parse_text_edgecases(char inline_end);
};

/**
 * @brief Creates a new parser object. The object hosts the memory for all std::string_views
 * returned by its methods until it is destroyed.
 * @param stream Text to parse. The entire stream will be read once into memory when the object
 * is constructed.
 * @param estimated_size (optional) estimated size of stream (should be larger than actual size by
 * at least +4 since parser may add some chars).
 */
parser::parser(std::istream& stream, std::size_t estimated_size = (32 * 1024))
    : buffer(stream, estimated_size)
{ output_buffer.reserve(256); }

parser::~parser() {}

/**
 * @brief Parses. Returns true if parsing was successful. Returns false otherwise.
 * If false was returned, the error string is in get_error_message(). Note that parsing
 * only aborts once an error is found, which means callbacks are issued normall until that point.
 * @param start_callback Callback when a command starts. Of type void(std::string_view name,
 * dodo::cmd_type type, std::string_view args)
 * @param end_callback Callback when a command ends. Of type void()
 * @param text_callback Callback to output text. Of type void(std::string_view text)
 */
inline bool
parser::parse(std::invocable<std::string_view, cmd_type, std::string_view> auto&& start_callback,
              std::invocable<> auto&& end_callback,
              std::invocable<std::string_view> auto&& text_callback)
{
    while (true) {
        switch (current_status) {
        case status::Default:
            if(buffer.skip_while(IS_WHITESPACE, true)) {
                current_status = (IS_TEXTUAL(buffer.get()) ? status::TextHere : status::ColonHere);
            } else {
                current_status = status::StreamEnds;
            }
            break;
        case status::CommandHere:
        case status::EndCommand:
        case status::TextHere:
        case status::StreamEnds:
        case status::TextEnds:
        case status::InlineEnds:
            end_command(); //we call cend ?!
            current_status = status::TextHere;
            break;
        case status::ColonHere:
            current_status = parse_colon();
            break;
        case status::Error:
            return false;
        }
    }

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
}

/*
    Parses an argument and sets arg to it. Expects current_char to be on first char after '['.
    Leaves current_char on next char after ']'. Does not expect that there needs to be a character
    here.
*/
std::string_view parser::parse_arg()
{
    std::string_view out;
    buffer.start_range();
    if (!buffer.find(']')) {
        throw std::runtime_error("Argument does not end.");
    }
    buffer.next();
    return buffer.get_range_before();
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
    cmd_stack.pop();
}

/* Ends a current text command. Also clears all variables for text parsing. Does nothing if there is
 * no text command active. */
void parser::end_text()
{
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
    command.indent = buffer.indent() - 1;
    buffer.start_range();

    if (IS_INLINE_SPECIAL(buffer.get())) {
        command.name = buffer.get_range_here();
        command.type = cmd_type::Inline;
        command.end_symbol = buffer.get();
        buffer.next();
        return command;
    }

    if (buffer.get() == ':') {
        if (buffer.next() && buffer.get() == ':') {
            command.name = std::string_view(NAME_CODE);
            command.type = cmd_type::Code;
            command.end_symbol = 3;
            while (buffer.next() && buffer.get() == ':') {
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
    command.name = buffer.get_range_before();
    return command;
}

/*
    Parses a command declaration. Assumes current char is on the first character after the colon of
   the declaration. Assumes the cases of :; and :. and <char>:<whitespace> have been handled already
   and handles all other cases. The returned command has all fields filled correctly, including the
   indent. Assumes that there is a current character and a named command indeed starts here.
*/
cmd parser::parse_cmd_decl()
{
    cmd out = parse_name();
    if (out.type == cmd_type::Inline) {
        return out;
    }

    if (buffer.has() && buffer.get() == '[') {
        buffer.next();
        out.arg = parse_arg();
    } else {
        out.arg = std::string_view();
    }

    if (out.name.empty()) {
        if (buffer.has() && buffer.get() == '{') {
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

    if (!buffer.has()) {
        out.type = cmd_type::Block;
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

/**
 * @brief Parses code. Expects cursor to be on first char of code content. Leaves cursor on first
 * char after code end declaration. Adds all text segments to output_buffer.
 * @param colons_to_end amount of colons that the code end declaration needs
 */
parser::status parser::parse_code(std::size_t colons_to_end)
{
    std::size_t indent = 0;
    bool set_indent = false;
    bool did_output = false;
    bool did_end = false;

    buffer.start_range();

    if (!buffer.skip_while(IS_TAB_SPACE, true)) {
        error_type = error_t::CodeDoesNotEnd;
        return status::Error;
    }

    while (buffer.find(':', '\n')) {
        if (buffer.get() == ':') {
            if (buffer.count_here(':', colons_to_end) == colons_to_end) {
                output_add(buffer.get_range_before(colons_to_end));
                did_end = true;
                break;
            }
        } else { // found a newline
            output_add(buffer.get_range_here());
            did_output = true;
            buffer.next();
            if (set_indent) {
                while (buffer.indent() < indent && buffer.has() && IS_TAB_SPACE(buffer.get())) {
                    buffer.next();
                }
            } else {
                while (buffer.has() && IS_TAB_SPACE(buffer.get())) {
                    indent++;
                    buffer.next();
                }
                set_indent = true;
            }
            buffer.start_range();
        }
    }

    if (did_end) {
        if (did_output && output_buffer.back().ends_with('\n')) {
            output_buffer.back().remove_suffix(1);
        }
    } else {
        error_type = error_t::CodeDoesNotEnd;
        return status::Error;
    }
}

/**
 * @brief Parses text. Handles the edgecases <char>:<whitespace> and :. and :, and :<inline-end> and
 * :<end of stream> itself. Will parse until
 * + A colon (non-edgecase) is found -> leaves cursor on it
 * + End of stream is reached
 * + If the text ends because of an empty newline -> leaves cursor on start of next line
 * + An inline end character is found -> leaver cursor after that character
 * Puts all text in the output buffer
 * @param inline_end the current inline end character '\0' if no inline command active
 */
parser::status parser::parse_text(char inline_end)
{
    buffer.skip_while(IS_TAB_SPACE, true);
    buffer.start_range();
    while (buffer.has()) {
        if (buffer.get() == inline_end) {
            output_add(buffer.get_range_before());
            buffer.next();
            return status::InlineEnds;
        } else if (buffer.get() == ':') {
            if (!parse_text_edgecases(inline_end)) {
                output_add(buffer.get_range_before());
                return status::CommandHere;
            }
        } else if (IS_WHITESPACE(buffer.get())) {
            if (!parse_text_whitespace()) {
                return status::TextEnds;
            }
        } else [[likely]] {
            buffer.next();
        }
    }
    return status::StreamEnds;
}

/**
 * @brief Parses if a colon appears in text on that colon. Returns true if edgecase was handled
 * and text can continue and false otherwise.
 */
bool parser::parse_text_edgecases(char inline_end)
{
    if (!buffer.can_peek()) {
        buffer.next();
        return true;
    }
    if (IS_WHITESPACE(buffer.peek()) && !IS_WHITESPACE(buffer.prev())) {
        buffer.next();
        return true;
    }
    switch (buffer.peek()) {
    case '.':
        output_add(buffer.get_range_before());
        buffer.next();
        buffer.start_range();
        return true;
    case ',':
        output_add(buffer.get_range_before());
        buffer.next(2);
        buffer.start_range();
        buffer.next();
        return true;
    default:
        break;
    }
    if (inline_end != '\0' && buffer.peek() == inline_end) {
        output_add(buffer.get_range_before());
        buffer.next();
        buffer.start_range();
        return true;
    }
    return false;
}

/**
 * @brief Parses if a whitespace appears in text on that whitespace. Returns true if the text
 * continues and false if it ends. If true is returned, the buffer range is open. If false is
 * returned it was closes and handled.
 */
bool parser::parse_text_whitespace()
{
    if (buffer.can_peek() && IS_TEXTUAL(buffer.peek())) {
        buffer.overwrite(' ');
        buffer.next();
        return true;
    }

    output_add(buffer.get_range_before());
    if (!buffer.skip_while(IS_TAB_SPACE, true)) {
        return false;
    }

    if (!(buffer.get() == '\n')) {
        buffer.start_range();
        return true;
    }

    buffer.next();
    if (!buffer.skip_while(IS_TAB_SPACE, true) || buffer.get() == '\n') {
        buffer.next();
        return false;
    }

    buffer.start_range();
    return true;
}

/**
 * Parses on a colon. Handles edgecases <char>:<whitespace> and :. and :, and :<end of stream>.
 * Will return a status != ColonHere.
 */
parser::status parser::parse_colon()
{
    if (!buffer.can_peek()) {
        return status::StreamEnds;
    }
    switch (buffer.peek()) {
    case CHAR_SEMICOLON:
        buffer.next(2);
        return status::EndCommand;
    case '.':
    case ',':
        return status::TextHere;
    default:
        if (IS_WHITESPACE(buffer.peek()) && !IS_WHITESPACE(buffer.prev())) {
            return status::TextHere;
        }
        break;
    }

    return status::CommandHere;
    ? ? // handle command how
        buffer.next();
    command = parse_cmd_decl();
    if (text_wants_space_next
        && (command.type == cmd_type::Inline || command.type == cmd_type::InlineEmpty)) {
        ctext(std::string_view(TEXT_SINGLE_SPACE));
        text_wants_space_next = false;
    }
    start_command(command);
    if (command.type == cmd_type::Code) {
        ctext(parse_code(command.end_symbol));
        end_command();
    }
}

std::string parser::get_error_message()
{
    std::string out;
    switch (error_type) {
    case error_t::CodeDoesNotEnd:
        out = std::string("Code command does not end.");
        break;
    default:
        out = std::string("No error found.") break;
    }
    out.append("\n | at line / indent: ")
        .append(std::to_string(buffer.line()))
        .append(" / ")
        .append(std::to_string(buffer.indent()));
    .append("\n | Info: ").append(error_info);
    return out;
}
} // namespace dodo
