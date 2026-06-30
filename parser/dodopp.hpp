#pragma once

#include <istream>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stack>
#include <utility>
#include <string>
//TODO: commands should also not start with . and , add :. and :,
//TODO: make :; end the scope of last non-text command


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
//TODO: make names low and be able to return array with mapppings if not too large

inline constexpr int NAMES_ID_START = 1024;
const std::unordered_map<std::string, int> DEFAULT_NAMES {
    {"text", 0},
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
    {"inline_code", 14}

};

inline constexpr int CHAR_EOF = std::char_traits<char>::eof();

constexpr bool IS_INLINE_SPECIAL(int c)
{
    switch (c)
    {
        case '!': return true;
        case '"': return true;
        case '$': return true;
        case '&': return true;
        case '/': return true;
        case '=': return true;
        case '?': return true;
        case '\\': return true;
        case '*': return true;
        case '+': return true;
        case '~': return true;
        case '\'': return true;
        case '|': return true;
        case '^': return true;
        default: return false;
    }
}

constexpr bool IS_WHITESPACE(int c)
{
    switch (c)
    {
        case CHAR_NEWLINE: return true;
        case CHAR_SPACE: return true;
        case CHAR_TAB: return true;
        default: return false;
    }
}

constexpr bool IS_BRACKET(int c)
{
    switch(c)
    {
        case ')': return true;
        case '(': return true;
        case '[': return true;
        case ']': return true;
        case '{': return true;
        case '}': return true;
        default: return false;
    }
}

constexpr bool IS_ALLOWED_IN_NAME(int c) 
{
    return !(IS_BRACKET(c) || IS_WHITESPACE(c) || (c == CHAR_COLON) || (c == CHAR_EOF) || (c == CHAR_SEMICOLON));
}

bool IS_INLINE_SPECIAL(std::string& name)
{
    return (name.length() == 1 && IS_INLINE_SPECIAL(name[0]));
}

/*
    During parsing holds:
    + arg_start = start of arg in stream buffer
    + arg_len = indent
    + next = real
    + child_start = real
    + child_end = real
    + name = real
*/
class cmd {
public:
    /* Points to first char of arg. */
    char* arg_start;
    /* Length of argument. */
    size_t arg_len{0};
    /* Points to next child of parent command. If this is the last child, is a nullptr. */
    cmd* next{nullptr};
    /*
        Block commands: 1 if has children, 0 else
        Inline commands: adress of first char of text + child_start = first char in command
    */
    std::ptrdiff_t child_start{0};
    /*
        Block command: this address + child_end = address of last child + 1
        Inline command: child_end + address of first char of text -1 = last char in command
    */
    std::ptrdiff_t child_end{0};
    /* Name ID */
    int name;

    cmd(int n, char* arg, size_t indent) : name{n}, arg_start{arg}, arg_len{indent} {}
    cmd(int n, char* arg) : name{n}, arg_start{arg} {}
    cmd(int n) : name{n} {}
    cmd() : name{0} {}

    struct Iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = cmd;
        using pointer = cmd*;
        using reference = cmd&;

        Iterator(pointer ptr) : cmd_ptr{ptr} {}

        reference operator*() const { return *cmd_ptr; }
        pointer operator->() { return cmd_ptr; }
        Iterator& operator++() { cmd_ptr = (cmd_ptr->next == nullptr) ? cmd_ptr + 1 : cmd_ptr->next; return *this; }
        Iterator& operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
        friend bool operator==(const Iterator& a, const Iterator& b) { return a.cmd_ptr == b.cmd_ptr; }
        friend bool operator!=(const Iterator& a, const Iterator& b) { return a.cmd_ptr != b.cmd_ptr; }

        private:
            pointer cmd_ptr;
    };

    Iterator begin() { return Iterator(this + 1); }
    Iterator end() { return Iterator(this + child_end); }

    
};

class parser
{

public:
    parser(std::istream& stream, std::size_t estimated_stream_size, std::unordered_map<std::string, int> names, bool new_names);
    ~parser();
    int name_to_int(const std::string& name);
    
    /* Starts parsing. */
    void parse();
   
private:
    // Stream buffer management
    std::istream& stream;
    std::size_t local_buffer_size;
    std::vector<char> stream_buffer;
    char* stream_buffer_start;
    char* stream_buffer_end;
    char* cursor;
    /* Distance from current char to the last newline - 1. */
    std::size_t indent = 0;
    int current_char;

    void read_stream_in_buffer();
    inline int getc();
    inline int nextc();
    inline int prevc();
    inline int peekc();
    inline int move_cursor(char* to);

    // Output buffer
    std::vector<char> output_buffer;
    /* Points to next location that can be written to. */
    char* output_cursor;

    inline char* get_output_cursor();
    inline void output_char(char c);
    inline void output_range(char* start, char* end);

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
    void skip_whitespace();
    inline void skip_space_tab();
    bool handle_cmd_decl_edgecases();
    void parse_code();
    void parse_text();


    // State management
    /* Vector to store all commands. It is guaranteed that cmds[0] is the root command. */
    std::vector<cmd> cmds{};
    /* Stack of commands while parsing. Indexes into cmds. */
    std::stack<cmd*> cmd_stack{};
    /* Stack of inline commands while parsing text. */
    std::stack<cmd*> inline_cmd_stack{};
    /* Stores set of characters that end inline specials in current text. */
    std::unordered_map<char, int> active_inline_enders{};
    void end_text();
    void push_cmd(int name, char* arg, size_t indent);
    void end_scope();
};

/* 
    Constructs a dodo parser for a stream s. The names argument supplies name-to-in bindings, it MUST map 'text' to 0 and shall only include positive numbers otherwise.
    If new_names is true, the parser will parse all names and select new ints for them. If it is false, it will parse all
    unknown names as -1.
*/
inline parser::parser(std::istream &stream, std::size_t estimated_stream_size = (8 * 1024), std::unordered_map<std::string, int> names = DEFAULT_NAMES, bool new_names = true)
    : stream{stream}, external_names{names}, new_names_allowed{new_names}
{
    // Set next_name_id larger than largest external id
    for(const auto& [_, id] : external_names) {
        next_name_id = std::max(next_name_id, id+1);
    }   

    // Init stream_buffer
    stream_buffer.reserve(estimated_stream_size + 4);
    read_stream_in_buffer();


    // Init output_buffer
    output_buffer.reserve(estimated_stream_size + 4);
    output_cursor = output_buffer.data();

    // Init commands with root command
    cmds.push_back(cmd(ROOT_ID));
    cmd_stack.push(&cmds[0]);

    //TODO: testing 
    parse_code(); 
}

parser::~parser()
{
}

inline void parser::read_stream_in_buffer()
{
    std::vector<char> local_buffer(local_buffer_size);
    stream_buffer.push_back(CHAR_NEWLINE);

    while(stream.read(local_buffer.data(), local_buffer.size())) {
        stream_buffer.insert(stream_buffer.end(), local_buffer.data(), local_buffer.data() + local_buffer.size());
    }

    if(stream.gcount() > 0) {
        stream_buffer.insert(stream_buffer.end(), local_buffer.data(), local_buffer.data() + stream.gcount());
    }
    
    stream_buffer_start = stream_buffer.data();
    stream_buffer_end = stream_buffer.data() + stream_buffer.size();
    cursor = stream_buffer_start;
    nextc();
}

inline int parser::getc()
{
    return current_char;
}

inline int parser::nextc()
{
    cursor++;
    if(cursor >= stream_buffer_end) [[unlikely]]
    {
        cursor = stream_buffer_end;
        current_char = CHAR_EOF;
    }

    if(prevc() == CHAR_NEWLINE) indent = 0;
    else indent++;
    return current_char;
}

inline int parser::prevc()
{
    return *(cursor - 1);
}

inline int parser::peekc()
{
    if(cursor + 1 >= stream_buffer_end) [[unlikely]] {
        return CHAR_EOF;
    }
    return *(cursor + 1);
}

inline int parser::move_cursor(char *to)
{
    cursor = to;
    indent = 0;
}

inline char* parser::get_output_cursor()
{
    return output_cursor;
}

inline void parser::output_char(char c)
{
    *output_cursor++ = c;
}

inline void parser::output_range(char* start, char* end)
{
    std::copy(cursor, start, end);
    cursor = end;
}

/* Gives the integer id for a name. Will return -1 if the name does not exist. */
inline int parser::name_to_int(const std::string& name)
{
    if(external_names.contains(name)) return external_names.at(name);
    else if(internal_names.contains(name)) return internal_names.at(name);
    else return -1;
}

/*
    Gives the interger id for a string name. If the name does not exist and new names are allowed, it will
    create a new id. Otherwise it returns -1.
*/
int parser::name_to_int(const std::string&& name)
{ 
    if(external_names.contains(name)) {
        return external_names.at(name);
    } else if(new_names_allowed) {
        if(internal_names.contains(name)) {
            return internal_names.at(name);
        } else {
            internal_names.emplace(std::move(name), next_name_id);
            return next_name_id++;
        }
    }
    return -1;
}


inline void parser::parse()
{
    // LOOP
    //if insied text command parse_text
    // skip whitespace
    // if char then text command and parse_text
    // else check edge case
    // if edge case then continue to next iteration
    // else parse command name and handle accordingly
}

//TODO: Test, inefficient
/*
    Parses a command argument starting from the arg_start pointer and sets the appropriate values.
*/
void parser::fill_arg(cmd& command)
{
    move_cursor(command.arg_start);
    int c = getc();
    size_t len = 0;
    command.arg_start = get_output_cursor();

    while(c != CHAR_EOF) {
        if(c == CHAR_BRACK_SQ_CLOSE) {
            nextc(); break;
        } else if(c == CHAR_COLON && peekc() == CHAR_BRACK_SQ_CLOSE) {
            nextc(); 
            output_char(CHAR_BRACK_SQ_CLOSE);
            len++;
        } else {
            output_char(c);
            len++;
        }
        c = nextc();
    }

    command.arg_len = len;
}

//TODO: test
/* Parses the current argument as parse_arg would, but does not store it. */
inline char *parser::skip_arg()
{
    char* out = cursor;
    int c = nextc();
    while(!(c == CHAR_EOF || (c == CHAR_BRACK_SQ_CLOSE && prevc() != CHAR_COLON))) c = nextc();
    nextc();
    return out;
}

/*
    Parses a name. Leaves cursor on the first character that does not belong to the name anymore.
    Expects cursor to be on the first character of the name on call. Parses an empty name as 'link'.
*/
inline std::string parser::parse_name()
{
    std::string result{};
    int c = getc();
    if(IS_INLINE_SPECIAL(c)) {
        result.push_back(c);
        nextc();
        return result;
    }

    while(IS_ALLOWED_IN_NAME(c)) {
        result.push_back(c);
        c = nextc();
    }

    if(result.empty()) result = std::string("link");
    return result;
}

/*
    Parses a command. Expects the cursor to be on the first character after the colon of the command declaration.
    Will NOT check if there was <char>:<whitespace> or ':.' or ':;' or '::' or ' : ' or ':::'. 
    Assumes that this has already been checked by the caller and that there is indeed a command starting.
    If the command name is empty, will parse the name as 'link'.
    Returns the Cmd object in the following state:
    + name = the name id of the command
    + arg = argument if the command is not text, if the command is text is empty
    + next = 0 if the command is inline, 1 if the command is block or text
    + child_start = if the command is inline, this is 0 if it has no body (ends immediatelly) and 1 if it has a body
    + child_end = if the command is inline and child_start == 1, this is the character value that the command ends on
*/
cmd parser::parse_cmd_decl()
{
    std::string name = parse_name();
    cmd out{name_to_int(name)};
    if(IS_INLINE_SPECIAL(name)) {
        out.next = 0;
        out.child_start = 1;
        out.child_end = name[0];
        return out;
    }
    if(getc() == '[') { nextc(); out.arg = fill_arg(); }
    switch(getc())
    {
        case CHAR_BRACK_CU_OPEN: // Inline command with {}
            out.next = 0;
            out.child_start = 1;
            out.child_end = static_cast<size_t>(CHAR_BRACK_CU_CLOSE);
            break;
        case CHAR_SEMICOLON: // Inline command with ;
            out.next = 0;
            out.child_start = 0;
            break;
        default: // Block command with : or ending on anything else
            out.next = 1;
            break;
    }
    return out;
}

/*
    Moves the cursor forward until it is on a non-whitespace character or EOF.
*/
//TODO: test
inline void parser::skip_whitespace()
{
    int c = getc();
    while(IS_WHITESPACE(c) && !(c == CHAR_EOF)) { c = nextc(); }
}

/* Skips until the current character is not space or tab. Ends on that character. */
inline void parser::skip_space_tab()
{
    int c = getc();
    while(c == CHAR_SPACE || c == CHAR_TAB) {c = nextc();}
}

/*
    Expected to be called on the first character after a colon. Returns true iff and edge case was found. If it returns false, the cursor
    is not moved. If it returns true, the cursor is on the first char after the edge case was handled.
    Checks and handles the following cases:
    + ':.' starts text command (or resumes text command) on '.'
    + ':;' ends the scope of the last non-text command, is ignored if that command is the root command
    + '::' starts a block command
    + ':::' code command
    + '<whitespace>:<whitespace> text command
    + '<char>:<whitespace>' no command, parse as text
*/
inline bool parser::handle_cmd_decl_edgecases()
{
    switch(getc())
    {
        case CHAR_DOT: // parse should parse this as text
            return true;
        case CHAR_SEMICOLON:
            break; // TODO: end the scope with function end_scope()
        case CHAR_COLON:
            break; // TODO: check if code or not and handle with parse_code()
        default:
            break; 
    }
    if(IS_WHITESPACE(getc())) {
        if(IS_WHITESPACE(prevc())) {
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
inline void parser::parse_code()
{
    size_t activation_colons = 3; //TODO: implement
    size_t colon_count = 0;
    std::vector<char> text{}; 
    std::vector<char> arg{};
    int c;
    while(getc() == CHAR_COLON)
        { activation_colons++; nextc(); }
    if(getc() == CHAR_BRACK_SQ_OPEN)
        arg = fill_arg();
    if(IS_WHITESPACE(getc()))
        nextc();

    c = getc();
    do {
        if(c != CHAR_COLON) {
            if(c == CHAR_EOF)
                break;
            text.push_back(c);
        } else {
            colon_count = 1;
            while(colon_count < activation_colons && nextc() == CHAR_COLON)
                colon_count++;
            if(colon_count == activation_colons)
                break;
            else {
                for(int i = 0; i < colon_count; i++)
                    text.push_back(CHAR_COLON);
                text.push_back(getc());
            }
        }

    } while((c = nextc()) != CHAR_EOF);
    if(text.size() > 0 && text.back() == CHAR_NEWLINE) text.pop_back();
}

// TODO: handle inline special commands ending
inline void parser::parse_text()
{ // TODO: if text ends remove last space if it is there.
    std::vector<char>& arg = cmds[cmd_stack.top()].arg;
    int c = getc();
    while(c != CHAR_COLON && c != CHAR_EOF) {
        if(IS_WHITESPACE(c)) {
            if(c == CHAR_NEWLINE && line_is_empty) return;
            if(arg.size() > 0 && arg.back() != CHAR_SPACE) arg.push_back(CHAR_SPACE);
        } else {
            arg.push_back(c);
        }
        c = nextc();
    }
}

/* Gracefully ends and removes from stack a text command (if it is there). Clears inline stack ...*/
inline void parser::end_text()
{
    //TODO: implement
}

//TODO: test
/* Pushes a new command to the stack. */
inline void parser::push_cmd(int name, char* arg, size_t indent) 
{
    end_text();
    while(cmd_stack.size() > 1 && cmd_stack.top()->arg_len >= indent) { cmd_stack.pop(); }
    cmds.emplace_back(name, arg, indent);

    cmd* parent = cmd_stack.top();
    if(parent->child_start == 1) {
        (parent + parent->child_end - 1)->next = &cmds.back();
    }
    parent->child_start = 1;
    parent->child_end = &cmds.back() - cmd_stack.top() + 1;
    cmd_stack.push(&cmds.back());
}

/* Ends scope of any text commands and the first non-root command after it. */
inline void parser::end_scope()
{
    end_text();
    if(cmd_stack.size() > 1) cmd_stack.pop();
}
}