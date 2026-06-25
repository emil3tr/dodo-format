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
        case CHAR_EOF: return true;
        case CHAR_TAB: return true;
        default: return false;
    }
}

constexpr bool IS_ALLOWED_IN_NAME(int c) 
{
    return !(IS_BRACKET(c) || IS_WHITESPACE(c) || (c == CHAR_COLON) || (c == CHAR_EOF) || (c == CHAR_SEMICOLON));
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
    size_t start;
    size_t end;
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
    Iterator end() { return Iterator(this + end); }

    
};

class Parser
{

public:
     // Error Codes
    enum class Error {CmdParsedWithoutColon};

    Parser(std::istream& s);
    ~Parser();

    std::string parse_arg();
    int name_to_int(const std::string&& name);
    
   
private:
    // Variables for stream and buffer management.
    const size_t BUFFER_SIZE = (4 * 1024);
    std::istream& stream;
    std::vector<char> buffer = std::vector<char>(BUFFER_SIZE);
    const char* buffer_cursor = buffer.data();
    const char* buffer_end = buffer.data();

    // Current parsing information. The indent value refers to the current indent, i.e. the indent of the char
    // that nextc() returned last. The indent after a newline is -1.
    size_t indent = -1;
    int current_char = CHAR_NEWLINE;
    int last_char = CHAR_NEWLINE;
    int next_char = CHAR_NEWLINE;
    bool line_is_empty = true;

    // Variables for name management.
    std::unordered_map<std::string, int> internal_names;
    const std::unordered_map<std::string, int>& external_names;
    bool new_names_allowed = true;
    int next_name_id = 2048;

    // Parsing functions
    std::string parse_name();
    Cmd parse_cmd_decl();

    inline bool nextc();

    // State management
    std::vector<Cmd> commands;
    std::vector<Error> error_list;
};

/*
    Parses a name. Leaves current_char on the first character that does not belong to the name anymore.
    Expects current_char to be on the first character before the name on call.
*/
inline std::string Parser::parse_name()
{
    std::string result{};

    nextc();
    if(IS_INLINE_SPECIAL(current_char)) {
        result.push_back(current_char);
        return result;
    }

    do {
        if(!IS_ALLOWED_IN_NAME(current_char)) break;
        result.push_back(current_char);
    } while(nextc());
    return result;
}

/*
    Should be called with the current char on the first colon of the command declaration.
*/
Cmd Parser::parse_cmd_decl()
{
    Cmd out;
    bool whitespace_before = IS_WHITESPACE(last_char);

    //TODO: check if char before and whitespace after and if yes go into text mode
    if(current_char != CHAR_COLON) {
        error_list.push_back(Error::CmdParsedWithoutColon);
        return out;
    }

    out.name = name_to_int(parse_name());

    return out;
}

/*
  Retrieves the next char from the stream. Sets current_char, last_char, indent, next_char and is_empty_newline
  accordingly. If the stream ended will return false and set current_char and next to EOF. On success returns true.
*/
bool Parser::nextc()
{
    last_char = current_char;
    current_char = next_char;
    if(current_char == CHAR_EOF) [[unlikely]] {
        return false;
    }
    if(buffer_cursor == buffer_end) [[unlikely]] {
        stream.read(buffer.data(), BUFFER_SIZE);
        std::streamsize read_count = stream.gcount();
        buffer_cursor = buffer.data();
        buffer_end = buffer_cursor + read_count;
        if(read_count == 0) {
            next_char = CHAR_EOF;
        } else {
            next_char = *buffer_cursor++;
        }
    } else {
        next_char = *buffer_cursor++;
    }

    if(current_char == CHAR_NEWLINE) {
        indent = -1;
        line_is_empty = true;
    } else {
        indent++;
        line_is_empty = (line_is_empty && !IS_WHITESPACE(current_char));
    }
    return true;
}

Parser::Parser(std::istream &s) : stream{s}, external_names{DEFAULT_NAMES}
{
    // Set next_name_id larger than largest external id
    for(const auto& [_, id] : external_names) {
        next_name_id = std::max(next_name_id, id+1);
    }   

    //TODO: init vector etc
}

Parser::~Parser()
{
}

std::string Parser::parse_arg()
{
    std::string out{};
    int c = stream.get();
    while(!(c == EOF || c == ']')) {
        if(c == CHAR_COLON) {
            if((c = stream.get()) == ']') out.push_back(']');
            else {out.push_back(CHAR_COLON); out.push_back(c);}
        } else {
            out.push_back(c);
        }
        c = stream.get();
    }
    return out;
}

/*
  Translates a string name to an integer. If no new names are allowed and the name is not
  found in any of the two maps, it will return -1.
*/
int Parser::name_to_int(const std::string&& name)
{ /*
    if(external_names.contains(name)) {
        return external_names.at(name);
    } else if(internal_names.contains(name)) {
        return internal_names.at(name);
    } else if(new_names_allowed) {
        internal_names.emplace(std::move(name), next_name_id);
        return next_name_id++;
    } else {
        return -1;
    }*/
   return -1;
}

}