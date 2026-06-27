#include <istream>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>

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
    For inline commands:
    + start = first char
    + end = last char + 1
    + name = name id
    + so if the text is "abcdef" and start = 1, end = 4 it contains "bcd"
    For block commands:
    + arg = arg
    + *(Cmd& + next) = next child, is 1 if this is the last child
    + start = 0 if no children, 1 if has continuous children, 2 if has non-continuous children
    + *(Cmd& + end - 1) = last child, 1 if no children
    For text commands:
    + arg = text
    + other same as block command
*/
//TODO: layout is in DFS-first structure so it is cahce optimal in dfs search, guarantee thsi to user for faster stack-based iteration
class Cmd {
public:
    std::vector<char> arg;
    size_t next;
    size_t child_start;
    size_t child_end;
    int name;

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
     // Error Codes
    enum class Error {CmdParsedWithoutColon};

    Parser(std::istream& s);
    ~Parser();

    int name_to_int(const std::string&& name);
    
   
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
    bool new_names_allowed = true;
    int next_name_id = 2048;

    // Parsing functions
    std::string parse_name();
    std::vector<char> parse_arg();
    Cmd parse_cmd_decl();

    // State management
    std::vector<Cmd> commands;
    std::vector<Error> error_list;
};

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
    that there is indeed a command starting.
*/
Cmd Parser::parse_cmd_decl() //TODO: implement
{
    Cmd out;
    out.name = name_to_int(parse_name());
    if(out.name == 0) { // what to do on text?

    }
    if(getc() == '[') out.arg = parse_arg();
    // if char is '[' parse arg
    // if char is '{' its inline command ...
    if(getc() == '{') {} // ...
    // else if getc is :, ;, whitespace, ....
    return out;
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

Parser::Parser(std::istream &s) : stream{s}, external_names{DEFAULT_NAMES}
{
    // Set next_name_id larger than largest external id
    for(const auto& [_, id] : external_names) {
        next_name_id = std::max(next_name_id, id+1);
    }   

    int cc;
    while((cc = nextc()) != CHAR_EOF) {
        if(cc != getc()) std::cout << "NONO";
        else std::cout << (char) cc << " : " << indent << " " << line_is_empty << "\n";
    }
    //TODO: init vector etc
}

Parser::~Parser()
{
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
  Translates a string name to an integer. If no new names are allowed and the name is not
  found in any of the two maps, it will return -1.
*/
int Parser::name_to_int(const std::string&& name)
{ 
    if(external_names.contains(name)) {
        return external_names.at(name);
    } else if(internal_names.contains(name)) {
        return internal_names.at(name);
    } else if(new_names_allowed) {
        internal_names.emplace(std::move(name), next_name_id);
        return next_name_id++;
    } else {
        return -1;
    }
   return -1;
}

}