#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <istream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dodo {

inline constexpr int CHAR_SPACE = 32;
inline constexpr int CHAR_TAB = 9;
inline constexpr int CHAR_NEWLINE = 10;
inline constexpr int CHAR_COLON = 58;
inline constexpr int CHAR_SEMICOLON = 59;
inline constexpr int CHAR_DOT = '.';

inline constexpr int CHAR_BRACK_CU_OPEN = '{';
inline constexpr int CHAR_BRACK_CU_CLOSE = '}';
inline constexpr int CHAR_BRACK_SQ_OPEN = '[';
inline constexpr int CHAR_BRACK_SQ_CLOSE = ']';

inline constexpr int TEXT_ID = 0;
inline constexpr int ROOT_ID = 1;
// TODO: remove magic number for text_id and set it in class instead!
// TODO: make names low and be able to return array with mapppings if not too large

inline constexpr int NAMES_ID_START = 1024;
const std::unordered_map<std::string, int> DEFAULT_NAMES{{"text", 0},
                                                         {"root", 1},
                                                         {"block", 2},
                                                         {"code", 3},
                                                         {"i", 10},
                                                         {"b", 11},
                                                         {"s", 12},
                                                         {"u", 13},
                                                         {"c", 14},
                                                         {"link", 40},
                                                         {"*", 10},
                                                         {"=", 11},
                                                         {"-", 12},
                                                         {"_", 13},
                                                         {"'", 14},
                                                         {"inline_italic", 10},
                                                         {"inline_bold", 11},
                                                         {"inline_strikethrough", 12},
                                                         {"inline_underline", 13},
                                                         {"inline_code", 14}};

inline constexpr int CHAR_EOF = std::char_traits<char>::eof();
inline constexpr std::size_t STACK_BUFFER_SIZE = (4 * 1024);

constexpr bool IS_INLINE_SPECIAL(int c) {
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

constexpr bool IS_WHITESPACE(int c) {
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

constexpr bool IS_BRACKET(int c) {
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

constexpr bool IS_ALLOWED_IN_NAME(int c) {
    return !(IS_BRACKET(c) || IS_WHITESPACE(c) || (c == CHAR_COLON) || (c == CHAR_EOF) || (c == CHAR_SEMICOLON));
}

bool IS_INLINE_SPECIAL(std::string& name) {
    return (name.length() == 1 && IS_INLINE_SPECIAL(name[0]));
}

class inbuff {
private:
    std::string buffer{};
    std::size_t cursor;
    std::size_t current_indent = 0;
    int current_char;
    std::size_t buffer_size;

public:
    inbuff(std::istream& stream, std::size_t estimated_size = (64 * 1024)) {
        char local_buffer[STACK_BUFFER_SIZE];
        buffer.reserve(estimated_size);
        buffer.push_back(CHAR_NEWLINE);

        while (stream.read(local_buffer, STACK_BUFFER_SIZE)) {
            buffer.insert(buffer.end(), local_buffer, local_buffer + STACK_BUFFER_SIZE);
        }

        if (stream.gcount() > 0) {
            buffer.insert(buffer.end(), local_buffer, local_buffer + stream.gcount());
        }
        buffer_size = buffer.size();
        cursor = 1;
        current_char = (cursor >= buffer_size) ? CHAR_EOF : buffer[cursor];
        current_indent = 0;
    }

    inline int get() {
        return current_char;
    }

    inline int next() {
        cursor++;
        if (cursor >= buffer_size) [[unlikely]] {
            cursor = buffer_size;
            return current_char = CHAR_EOF;
        }

        if (current_char == CHAR_NEWLINE) {
            current_indent = 0;
        } else {
            current_indent++;
        }
        return current_char = buffer[cursor];
    }

    inline int prev() {
        return buffer[cursor - 1];
    }

    inline int peek() {
        if (cursor + 1 >= buffer_size) [[unlikely]] {
            return CHAR_EOF;
        }
        return buffer[cursor + 1];
    }

    inline std::size_t indent() {
        return current_indent;
    }

    inline std::size_t get_cursor() {
        return cursor;
    }

    inline std::size_t get_size() {
        return buffer.size();
    }

inline void skip_whitespace() {
    int c = get();
    while (IS_WHITESPACE(c) && !(c == CHAR_EOF)) {
        c = next();
    }
}

inline void skip_space_tab() {
    int c = get();
    while (c == CHAR_SPACE || c == CHAR_TAB) {
        c = next();
    }
}

/* 
    Tries to find the first occurence of c on/after the current character. If found returns true and moves the 
    current character there. If not found returns false and does nothing else.
*/
inline bool find(char c) {
    std::size_t last_cursor = cursor, last_indent = current_indent;
    int last_char = current_char;
    while(get() != c && get() != CHAR_EOF) {
        next();
    }
    if(get() == c) {
        return true;
    } else {
        cursor = last_cursor;
        current_indent = last_indent;
        current_char = last_char;
        return false;   
    }
}

};

class outbuff {
private:
    std::string buffer;

public:
    outbuff(std::size_t estimated_size = (4 * 1024)) : buffer{} {
        buffer.reserve(estimated_size);
    }

    inline void output_char(char c) {
        buffer.push_back(c);
    }

    inline void output_range(char* start, std::size_t count) {
        buffer.append(start, count);
    }

    /* Returns the position where the next given char will be written to in the buffer. */
    inline std::size_t get_position() {
        return buffer.size();
    }

    inline std::string_view get_view(std::size_t position, std::size_t count) {
        return std::string_view(buffer.data() + position, count);
    }

};

/*
    Command Layout in memory:
    If the command has an argument, for block commands that argument is defined by arg_start and arg_end.
    For inline / text commands the has_argument_child flag is set and the next command in memory is an
    argument command that contains the argument in its arg_start and arg_end fields.
*/
class cmd {
public:
    /*
        Block Command: Points to first character of argument, nullptr if no argument.
        Inline Command / Text Command: Points to first character of text
    */
    char* arg_start{nullptr};
    /*
        Block Command: Points to last character of argument + 1
        Inline Command / Text Command: Points to last character of text + 1
    */
    char* arg_end{nullptr};
    /*this + next points to the next child of parent command. if 0 this is the last child. */
    std::ptrdiff_t next{0};
    /* Name ID of the command. */
    int name;
    /* Inline Command: Character that the command ends on. */
    char inline_ender{0};
    /*
        0-1: 0=block, 1=text, 2=inline, 3=arg
        2: has children
        3: has argument
    */
    char flags{0};

    static constexpr char FLAG_BLOCK = 0;
    static constexpr char FLAG_TEXT = 1;
    static constexpr char FLAG_INLINE = 2;
    static constexpr char FLAG_ARG = 3;
    static constexpr char FLAG_HAS_CHILD = 4;
    static constexpr char FLAG_HAS_ARG = 8;
    static constexpr char TYPE_MASK = 3;

    cmd(int name) : name{name} {}

    cmd(int name, char flags) : name{name}, flags{flags} {}

    void set_typeflag(char typeflag) {
        flags = (flags & TYPE_MASK) | typeflag;
    }

    void set_flag(char flag) {
        flags = (flags | flag);
    }

    char get_typeflag() {
        return flags & TYPE_MASK;
    }

    bool check_flags(char flags_to_check) {
        return (flags & flags_to_check) == flags_to_check;
    }

    std::string_view get_arg() {
        if (!check_flags(FLAG_HAS_ARG)) {
            return std::string_view();
        }
        if (get_typeflag() == FLAG_BLOCK) {
            return std::string_view(arg_start, arg_end);
        }
        return std::string_view((this + 1)->arg_start, (this + 1)->arg_end);
    }

    void set_arg(char* start, char* end) {
        switch (get_typeflag()) {
            case FLAG_BLOCK:
                arg_start = start;
                arg_end = end;
                break;
            case FLAG_ARG:
                return;
            default:
                (this + 1)->arg_start = start;
                (this + 1)->arg_end = end;
                break;
        }
    }

    bool arg_is_set() {
        switch (get_typeflag()) {
            case FLAG_BLOCK:
            case FLAG_ARG:
                return arg_end != nullptr;
            default:
                return (this + 1)->arg_is_set();
        }
    }

    cmd& get_arg_holder() {
        switch (get_typeflag()) {
            case FLAG_BLOCK:
            case FLAG_ARG:
                return *this;
            default:
                return *(this + 1);
        }
    }

    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = cmd;
        using pointer = cmd*;
        using reference = cmd&;

        Iterator(pointer ptr) : cmd_ptr{ptr} {}

        reference operator*() const {
            return *cmd_ptr;
        }

        pointer operator->() {
            return cmd_ptr;
        }

        Iterator& operator++() {
            if (cmd_ptr->next == 0) {
                cmd_ptr = nullptr;
            } else {
                cmd_ptr += (cmd_ptr->next);
            }
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) {
            return a.cmd_ptr == b.cmd_ptr;
        }

        friend bool operator!=(const Iterator& a, const Iterator& b) {
            return a.cmd_ptr != b.cmd_ptr;
        }

    private:
        pointer cmd_ptr;
    };

    Iterator begin() {
        if (check_flags(FLAG_HAS_CHILD)) {
            return (!check_flags(FLAG_BLOCK) && check_flags(FLAG_HAS_ARG)) ? (this + 2) : this + 1;
        }
        return nullptr;
    }

    Iterator end() {
        return Iterator(nullptr);
    }
};

class parser {
public:
    parser(std::istream& stream,
           std::size_t estimated_stream_size,
           std::unordered_map<std::string, int> names,
           bool new_names);
    ~parser();
    int name_to_int(const std::string& name);

    /* Starts parsing. */
    void parse();

private:
    inbuff input_buffer;
    outbuff text_output, arg_output;

    // Variables for name management.
    std::unordered_map<std::string, int> internal_names;
    const std::unordered_map<std::string, int>& external_names;
    bool new_names_allowed;
    int next_name_id = 2048;

    int name_to_int(const std::string&& name);

    // Parsing functions
    std::string parse_name();
    void fill_arg(cmd& command);
    char* skip_arg();
    cmd parse_cmd_decl();
    bool handle_cmd_decl_edgecases();
    void parse_code();
    bool parse_text();

    // State management
    /* Vector to store all commands. It is guaranteed that cmds[0] is the root command. */
    std::vector<cmd> cmds{};
    /* Stack of commands while parsing. Indexes into cmds.  first is the command index, second is the indent.*/
    std::stack<std::pair<std::size_t, std::size_t>> cmd_stack{};
    // TODO: make sure this gets cleared on end if no other function cleared them.
    // TODO: it is responsibility of whoever pops from stack to eiter parse the arg (if not already) or put it here
    /* Stack of commands that need arg parsed. Indexes into cmds. */
    std::stack<std::size_t> needs_arg_parsed;
    /* True if text_parse just skipped whitespace and wants to ouput a space before the next work. */
    bool whitespace_before = false;

    cmd& get_top_command();
    std::size_t get_top_indent();
    void end_text();
    cmd& start_cmd(cmd command, size_t indent);
    void end_inline(char* end);
    void end_scope();
};

/*
    Constructs a dodo parser for a stream s. The names argument supplies name-to-in bindings, it MUST map 'text' to 0
   and shall only include positive numbers otherwise. If new_names is true, the parser will parse all names and select
   new ints for them. If it is false, it will parse all unknown names as -1.
*/
inline parser::parser(std::istream& stream,
                      std::size_t estimated_stream_size = (8 * 1024),
                      std::unordered_map<std::string, int> names = DEFAULT_NAMES,
                      bool new_names = true)
    : input_buffer(stream, estimated_stream_size), external_names{names}, new_names_allowed{new_names} {
    // Set next_name_id larger than largest external id
    for (const auto& [_, id] : external_names) {
        next_name_id = std::max(next_name_id, id + 1);
    }

    text_output = outbuff(input_buffer.get_size());
    arg_output = outbuff(input_buffer.get_size() / 2);

    // Init commands with root command
    cmds.push_back(cmd(ROOT_ID));
    cmd_stack.push(&cmds[0]);

    // TODO: testing
    parse_code();
}

parser::~parser() {}

/* Gives the integer id for a name. Will return -1 if the name does not exist. */
inline int parser::name_to_int(const std::string& name) {
    if (external_names.contains(name)) {
        return external_names.at(name);
    } else if (internal_names.contains(name)) {
        return internal_names.at(name);
    } else {
        return -1;
    }
}
/*
    Gives the interger id for a string name. If the name does not exist and new names are allowed, it will
    create a new id. Otherwise it returns -1.
*/
int parser::name_to_int(const std::string&& name) {
    if (external_names.contains(name)) {
        return external_names.at(name);
    } else if (new_names_allowed) {
        if (internal_names.contains(name)) {
            return internal_names.at(name);
        } else {
            internal_names.emplace(std::move(name), next_name_id);
            return next_name_id++;
        }
    }
    return -1;
}

/**/ // TODO: implement
inline void parser::parse() {
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

// TODO: Test, inefficient, remake
/*
    Parses a command argument starting from the arg_start pointer and sets the appropriate values.
    Will do nothing if the command does not have an argument. Assumes that the command is in a correct state.
    Will do nothing if the command does not have an argument to be filled.
    will ntot move the cursor!
*/
void parser::fill_arg(cmd& command) {
    int c;
    size_t len = 0;
    char* last_cursor = cursor;

    if (!(command.check_flags(cmd::FLAG_HAS_ARG) && !command.arg_is_set())) {
        return;
    }

    cmd& target = command.get_arg_holder();

    move_cursor(target.arg_start);
    target.arg_start = get_output_cursor();
    c = getc();

    while (c != CHAR_EOF) {
        if (c == CHAR_BRACK_SQ_CLOSE) {
            nextc();
            break;
        } else if (c == CHAR_COLON && peekc() == CHAR_BRACK_SQ_CLOSE) {
            nextc();
            output_char(CHAR_BRACK_SQ_CLOSE);
            len++;
        } else {
            output_char(c);
            len++;
        }
        c = nextc();
    }

    target.arg_end = target.arg_start + len;
    move_cursor(last_cursor);
}

// TODO: test
/*
    Parses the current argument as fill_arg would, but does not store it. Returns start of argument.
    Leaves the cursor on first char after ']'.
*/
inline char* parser::skip_arg() {
    char* out = cursor;
    int c = nextc();
    while (!(c == CHAR_EOF || (c == CHAR_BRACK_SQ_CLOSE && prevc() != CHAR_COLON))) {
        c = nextc();
    }
    nextc();
    return out;
}

/*
    Parses a name. Leaves cursor on the first character that does not belong to the name anymore.
    Expects cursor to be on the first character of the name on call. Parses an empty name as 'link'.
*/
inline std::string parser::parse_name() {
    std::string result{};
    int c = getc();
    if (IS_INLINE_SPECIAL(c)) {
        result.push_back(c);
        nextc();
        return result;
    }

    while (IS_ALLOWED_IN_NAME(c)) {
        result.push_back(c);
        c = nextc();
    }

    if (result.empty()) {
        result = std::string("link");
    }
    return result;
}

/*
    Parses a command. Expects that a normal command starts here, edge cases have been checked already and the cursor
    is on the first character after the colon.
*/
cmd parser::parse_cmd_decl() {
    cmd out{0};
    std::string name = parse_name();
    out.name = name_to_int(name);
    if (IS_INLINE_SPECIAL(name)) {
        out.flags = cmd::FLAG_INLINE;
    }
    if (getc() == CHAR_BRACK_SQ_OPEN) {
        nextc();
        out.setskip_arg()
    }
    switch (getc()) {
        case CHAR_BRACK_CU_OPEN: // Inline command with {}
            return CHAR_BRACK_CU_CLOSE;
        case CHAR_SEMICOLON: // Inline command with ;
            return 1;
        default: // Block command with : or ending on anything else
            return 0;
    }
}


/*
    Expected to be called on the first character after a colon. Returns true iff and edge case was found. If it returns
   false, the cursor is not moved. If it returns true, the cursor is on the first char after the edge case was handled.
    Checks and handles the following cases:
    + ':.' starts text command (or resumes text command) on '.'
    + ':;' ends the scope of the last non-text command, is ignored if that command is the root command
    + '::' starts a block command
    + ':::' code command
    + '<whitespace>:<whitespace> text command
    + '<char>:<whitespace>' no command, parse as text
*/
inline bool parser::handle_cmd_decl_edgecases() {
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
    Parses code. Expects to be called on the first char after ':::'. Will leave on the first char
    after the code command ends. After ':::' and any argument, the first whitespace (if there) is
    ignored and the rest is parsed verbatim until ':::' ending the command.
*/
inline void parser::parse_code() {
    size_t activation_colons = 3; // TODO: implement
    size_t colon_count = 0;       // TODO: should assume that cmd on stack is text and it simply parses into it.
    // everything else is handled by parse
    std::vector<char> text{};
    std::vector<char> arg{};
    int c;
    while (getc() == CHAR_COLON) {
        activation_colons++;
        nextc();
    }
    if (getc() == CHAR_BRACK_SQ_OPEN) {
        // arg = fill_arg();
    }
    if (IS_WHITESPACE(getc())) {
        nextc();
    }

    c = getc();
    do {
        if (c != CHAR_COLON) {
            if (c == CHAR_EOF) {
                break;
            }
            text.push_back(c);
        } else {
            colon_count = 1;
            while (colon_count < activation_colons && nextc() == CHAR_COLON) {
                colon_count++;
            }
            if (colon_count == activation_colons) {
                break;
            } else {
                for (int i = 0; i < colon_count; i++) {
                    text.push_back(CHAR_COLON);
                }
                text.push_back(getc());
            }
        }

    } while ((c = nextc()) != CHAR_EOF);
    if (text.size() > 0 && text.back() == CHAR_NEWLINE) {
        text.pop_back();
    }
}

// TODO: implement, make sure that text command has something in arg start and end field whei ninitiated
/*
    Parses text into a text command that is on top of the stack. Assumes that there is such a command.
    Assumes that all fields of that text command have been set correctly and it only needs to modify the
    arg_len.
    Parses until it
    + hits an empty newline and returns false (ends on first char of new line)
    + hits a colon and returns true (ends on that colon)
    + hits CHAR_EOF and returns false
    Returns true exactly if it hit a command and wants to continue parsing in case it is inline. Returns
    false if text ends.
*/
inline bool parser::parse_text() { // TODO: if text ends remove last space if it is there.
    int c;
    cmd& text_cmd = *cmd_stack.top();

    skip_space_tab();
    c = getc();
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
    return false;
}

inline cmd& parser::get_top_command() {
    return cmds[cmd_stack.top().first];
}

inline std::size_t parser::get_top_indent() {
    return cmd_stack.top().second;
}

// TODO: test
/*
    If there is a text command (and possible inline commands) on the stack, removes them.
    Also resets all variables used for text parsing.
*/
inline void parser::end_text() {
    char* text_end;

    while (get_top_command().get_typeflag() == cmd::FLAG_INLINE) {
        needs_arg_parsed.push(cmd_stack.top().first);
        cmd_stack.pop();
    }
    if (get_top_command().get_typeflag() != cmd::FLAG_TEXT) {
        return;
    }

    text_end = get_top_command().arg_end;
    cmd_stack.pop();
    whitespace_before = false;

    while (!needs_arg_parsed.empty()) {
        fill_arg(cmds[needs_arg_parsed.top()]);
        cmds[needs_arg_parsed.top()].arg_end = text_end;
        needs_arg_parsed.pop();
    }
}

// TODO: test
/*
    Pushes a new command to the stack / places it in the cmds vector. Will close and handle all commands whose scope
   ends because of this. Returns a reference to the command on the stack. Pushes only a copy of the given command.
    Assumes that if an inline command is given, there is a text / inline command on the stack.
    Will also place an argument command behind it if needed. will not modify anything about the command.
*/
inline cmd& parser::start_cmd(cmd command, size_t indent) {
    if (command.get_typeflag() == cmd::FLAG_INLINE) {
        cmds.push_back(command);
        if (get_top_command().get_typeflag() == cmd::FLAG_TEXT) {
            get_top_command().set_flag(cmd::FLAG_HAS_CHILD);
        } else {
            get_top_command().next = 1;
        }
        cmd_stack.push(std::make_pair(cmds.size() - 1, indent));
        if (command.check_flags(cmd::FLAG_HAS_ARG)) {
            cmds.push_back(cmd(0, cmd::FLAG_ARG));
        }
        return;
    }

    end_text();
    std::size_t last_pop = 0;
    while (cmd_stack.size() > 1 && get_current_indent() >= indent) {
        last_pop = cmd_stack.top().first;
        cmd_stack.pop();
    }

    cmds.push_back(command);

    cmd& parent = get_top_command();
    if (parent.check_flags(cmd::FLAG_HAS_CHILD)) {
        cmds[last_pop].next = (cmds.size() - 1 - last_pop);
    } else {
        parent.set_flag(cmd::FLAG_HAS_CHILD);
    }
    if (command.check_flags(cmd::FLAG_HAS_ARG) && command.get_typeflag() == cmd::FLAG_TEXT) {
        cmds.push_back(cmd(0, cmd::FLAG_ARG));
    }
    cmd_stack.push(std::make_pair(cmds.size() - 1, indent));
}

/* Pops the current inline command from the stack. */
inline void parser::end_inline(char* end) {
    if (get_top_command().get_typeflag() != cmd::FLAG_INLINE) {
        return;
    }
    needs_arg_parsed.push(cmd_stack.top().first);
    get_top_command().arg_end = end;
    cmd_stack.pop();
}

/* Ends scope of any text commands and the first non-root command after it. */
inline void parser::end_scope() {
    end_text();
    if (cmd_stack.size() > 1) {
        cmd_stack.pop();
    }
}
} // namespace dodo