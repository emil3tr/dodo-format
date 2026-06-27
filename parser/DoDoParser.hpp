#include <istream>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stack>

namespace dodo {

inline constexpr int CHAR_SPACE = 32;
inline constexpr int CHAR_TAB = 9;
inline constexpr int CHAR_NEWLINE = 10;
inline constexpr int CHAR_COLON = 58;
inline constexpr int CHAR_SEMICOLON = 59;

inline constexpr int NAMES_ID_START = 1024;
const std::unordered_map<std::string, int> DEFAULT_NAMES {
    {"text", 0},
    {"block", 1},
    {"root", 2},
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
    }
}

constexpr bool IS_ALLOWED_IN_NAME(int c) 
{
    return !(IS_BRACKET(c) || IS_WHITESPACE(c) || (c == CHAR_COLON) || (c == CHAR_EOF) || (c == CHAR_SEMICOLON));
}

/*
    Holds information about a dodo command.
    # After Parsing
    ## Inline Commands
    + child_start = position of first character (0-indexed)
    + child_end = position of last character + 1
    + name = name id
    + arg = argument
    + next = unspecified
    ## Block Commands
    + arg = argument
    + (&Cmd + next) = position of next child (of the parent command), is 1 if this is the last child
    + child_start = 0 if no children, 1 if has children, 2 if guaranteed that children are continuous in memory
    + (&Cmd +child_ end - 1) = position of last child, 1 if no children
    + Note that it is guaranteed that (if the command has children) position of first child = (&Cmd + 1) 
    + name = name id
    ## Text Commands
    + same as block command, but the argument is the text and the children are inline commands
*/
class Cmd {
public:
    std::vector<char> arg{};
    size_t next{1};
    size_t child_start{0};
    size_t child_end{1};
    int name;

    Cmd(int n) : name{n} {}
    Cmd() : name{0} {}

    struct Iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Cmd;
        using pointer = Cmd*;
        using reference = Cmd&;

        Iterator(pointer ptr) : cmd_ptr{ptr} {}

        reference operator*() const { return *cmd_ptr; }
        pointer operator->() { return cmd_ptr; }
        Iterator& operator++() { cmd_ptr += (cmd_ptr->next); return *this; }
        Iterator& operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
        friend bool operator==(const Iterator& a, const Iterator& b) { return a.cmd_ptr == b.cmd_ptr; }
        friend bool operator!=(const Iterator& a, const Iterator& b) { return a.cmd_ptr != b.cmd_ptr; }

        private:
            pointer cmd_ptr;
    };

    Iterator begin() { return Iterator(this + 1); }
    Iterator end() { return Iterator(this + child_end); }

    
};

class Parser
{

public:
    Parser(std::istream& s, std::unordered_map<std::string, int> names, bool new_names);
    ~Parser();
    int name_to_int(const std::string& name);
    
    /* Starts parsing. */
    void parse();
    /* Reference to the root command. */
    Cmd& get_root();
   
private:
    // Variables for stream and buffer management.
    const size_t BUFFER_SIZE = (4 * 1024);
    std::istream& stream;
    std::vector<char> buffer = std::vector<char>(BUFFER_SIZE);
    /* Points to the current character, last returned by nextc(). */
    const char* buffer_cursor = buffer.data();
    /* Points to the position of last available char + 1. */
    const char* buffer_end = buffer.data();

    // Variables and functions to get characters from stream.
    /* Indent of the character that was last returned by getc() or next(). */
    size_t indent = -1;
    /* 
        True exactly if all ints returned by getc() and nextc() after the last newline returned
        satisfied IS_WHITESPACE().
    */
    bool line_is_empty = true;
    inline int getc();
    inline int nextc();

    // Variables for name management.
    std::unordered_map<std::string, int> internal_names;
    const std::unordered_map<std::string, int>& external_names;
    bool new_names_allowed;
    int next_name_id = 2048;
    int name_to_int(const std::string&& name);

    // Parsing functions
    std::string parse_name();
    std::vector<char> parse_arg();
    Cmd parse_cmd_decl();
    void skip_whitespace();

    /* Vector to store all commands. It is guaranteed that cmds[0] is the root command. */
    std::vector<Cmd> cmds{};
    /* Stack of commands while parsing. */
    std::stack<size_t> cmd_stack{};
    /* Stack of inline commands while parsing text. */
    std::stack<size_t> inline_cmd_stack{};
};

/* 
    Constructs a dodo parser for a stream s. The names argument supplies name-to-in bindings, it MUST map 'text' to 0 and shall only include positive numbers otherwise.
    If new_names is true, the parser will parse all names and select new ints for them. If it is false, it will parse all
    unknown names as -1.
*/
inline Parser::Parser(std::istream &s, std::unordered_map<std::string, int> names = DEFAULT_NAMES, bool new_names = true)
    : stream{s}, external_names{names}, new_names_allowed{new_names}
{
    // Set next_name_id larger than largest external id
    for(const auto& [_, id] : external_names) {
        next_name_id = std::max(next_name_id, id+1);
    }   
    // Init commands with root command
    cmds.push_back(Cmd(name_to_int("root")));
    cmd_stack.push(0);
}


Parser::~Parser()
{
}

/* Gives the integer id for a name. Will return -1 if the name does not exist. */
inline int Parser::name_to_int(const std::string& name)
{
    if(external_names.contains(name)) return external_names.at(name);
    else if(internal_names.contains(name)) return internal_names.at(name);
    else return -1;
}

inline void Parser::parse()
{

}

inline Cmd &Parser::get_root()
{
    return cmds[0];
}

/*
    Parses a name. Leaves cursor on the first character that does not belong to the name anymore.
    Expects cursor to be on the first character of the name on call.
*/
inline std::string Parser::parse_name()
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
    return result;
}

/*
    Parses a command. Expects the cursor to be on the first character after the colon of the command declaration.
    Will NOT check if there was <char>:<whitespace>. Assumes that this has already been checked by the caller and
    that there is indeed a command starting. Returns the Cmd object in the following state:
    + name = the name id of the command
    + arg = argument if the command is not text, if the command is text is empty
    + next = 0 if the command is inline, 1 if the command is block or text
    + child_start = if the command is inline, this is 0 if it has no body (ends immediatelly) and 1 if it has a body
    + child_end = if the command is inline and child_start == 1, this is the character value that the command ends on
*/
Cmd Parser::parse_cmd_decl() //TODO: implement
{
    Cmd out{name_to_int(parse_name())};
    if(out.name == 0) { // what to do on text?
        out.next = 1;
        return out;
    } //TODO: Do not parse arguments when the command is an inline special command!
    if(getc() == '[') { nextc(); out.arg = parse_arg(); } //TODO: test if it covers all cases
    //TODO: if it is an inline special command handle that!
    //TODO: cover edge cases for : and :: and :::
    //TODO: cover edge case where empty command is a link
    //TODO: make setting of Cmd fields more elegant
    switch(getc())
    {
        case '{':
            out.next = 0;
            out.child_start = 1;
            out.child_end = static_cast<size_t>('{');
            break;
        case ';':
            out.next = 0;
            out.child_start = 0;
            break;
        case ':':
            out.next = 1;
            break;
        default:
            out.next = 1;
    }
    return out;
}

/*
    Moves the cursor forward until it is on a non-whitespace character or EOF.
*/
//TODO: test
inline void Parser::skip_whitespace()
{
    int c = getc();
    while(IS_WHITESPACE(c) && !(c == CHAR_EOF)) { c = nextc(); }
}

/* Returns the current character that was also last returned by nextc(). Does not advance the cursor. */
inline int Parser::getc()
{
    if(buffer_cursor >= buffer_end) [[unlikely]] {
        return CHAR_EOF;
    }
    return *buffer_cursor;
}

/*
    Retrieves the next character from the stream and advances the cursor by one. Also sets indent and
    line_is_empty accordingly. Will return CHAR_EOF if the stream ended.
*/
int Parser::nextc()
{
    buffer_cursor++;
    if(buffer_cursor >= buffer_end) [[unlikely]] {
        stream.read(buffer.data(), BUFFER_SIZE);
        std::streamsize read_count = stream.gcount();
        buffer_cursor = buffer.data();
        buffer_end = buffer_cursor + read_count;
        if(read_count == 0) {
            return CHAR_EOF;
        } 
    }

    int c = *buffer_cursor;
    
    if(c == CHAR_NEWLINE) {
        indent = -1;
        line_is_empty = true;
    } else {
        indent++;
        line_is_empty = (line_is_empty && IS_WHITESPACE(c));
    }
    return c;
}

/*
    Parses a command argument. Expects to be called on the first char after '[' (the first char of the argument).
    Parses verbatim, except parses ':]' as ']' until ']' or CHAR_EOF is reached. Leaves cursor on first character after ']'.
*/
//TODO: test 
std::vector<char> Parser::parse_arg()
{
    std::vector<char> out{};
    int c = getc(); 

    while(c != CHAR_EOF && c != ']') {
        if(c == CHAR_COLON) {
            c = nextc();
            if(c == ']') out.push_back(']');
            else out.push_back(CHAR_COLON);
        } else {
            out.push_back(c);
            c = nextc();
        }
    }
    nextc();
    return out;
}

/*
    Gives the interger id for a string name. If the name does not exist and new names are allowed, it will
    create a new id. Otherwise it returns -1.
*/
int Parser::name_to_int(const std::string&& name)
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

}