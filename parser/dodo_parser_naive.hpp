#include <istream>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stack>

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
// TODO: remove magic number for text_id and set it in class instead!
//TODO: make names low and be able to return array with mapppings if not too large

inline constexpr int NAMES_ID_START = 1024;
const std::unordered_map<std::string, int> DEFAULT_NAMES {
    {"text", 0},
    {"block", 1},
    {"code", 2},
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
    + (&cmd + next) = position of next child (of the parent command), is 1 if this is the last child
    + child_start = 0 if no children, 1 if has children, 2 if guaranteed that children are continuous in memory
    + (&cmd +child_ end - 1) = position of last child, 1 if no children
    + Note that it is guaranteed that (if the command has children) position of first child = (&cmd + 1) 
    + name = name id
    ## Text Commands
    + same as block command, but the argument is the text and the children are inline commands
*/
class cmd {
public:
    std::vector<char> arg{};
    size_t next{1};
    size_t child_start{0};
    size_t child_end{1};
    int name;

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

class parser
{

public:
    parser(std::istream& s, std::unordered_map<std::string, int> names, bool new_names);
    ~parser();
    int name_to_int(const std::string& name);
    
    /* Starts parsing. */
    void parse();
    /* Reference to the root command. */
    cmd& get_root();
   
private:
    // Variables for stream and buffer management.
    const size_t BUFFER_SIZE = (4 * 1024); // Shall be >= 4
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
    inline int prevc();

    // Variables for name management.
    std::unordered_map<std::string, int> internal_names;
    const std::unordered_map<std::string, int>& external_names;
    bool new_names_allowed;
    int next_name_id = 2048;
    int name_to_int(const std::string&& name);

    // Parsing functions
    std::string parse_name();
    std::vector<char> parse_arg();
    cmd parse_cmd_decl();
    void skip_whitespace();
    bool handle_cmd_decl_edgecases();
    void end_scope();
    void parse_code();
    /* Vector to store all commands. It is guaranteed that cmds[0] is the root command. */
    std::vector<cmd> cmds{};
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
inline parser::parser(std::istream &s, std::unordered_map<std::string, int> names = DEFAULT_NAMES, bool new_names = true)
    : stream{s}, external_names{names}, new_names_allowed{new_names}
{
    // Set next_name_id larger than largest external id
    for(const auto& [_, id] : external_names) {
        next_name_id = std::max(next_name_id, id+1);
    }   
    // Init commands with root command
    cmds.push_back(cmd(name_to_int("block")));
    cmd_stack.push(0);

    // Init buffer
    buffer[0] = CHAR_NEWLINE;

    // Call nextc() to set current char on first char
    nextc();

    //TODO: testing 
    parse_code(); 
}


parser::~parser()
{
}

/* Gives the integer id for a name. Will return -1 if the name does not exist. */
inline int parser::name_to_int(const std::string& name)
{
    if(external_names.contains(name)) return external_names.at(name);
    else if(internal_names.contains(name)) return internal_names.at(name);
    else return -1;
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

inline cmd &parser::get_root()
{
    return cmds[0];
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
    if(getc() == '[') { nextc(); out.arg = parse_arg(); }
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
    Ends the scope of the first non-text command on the stack. If the first non-text command
    on the stack is the root command, it will do nothing.
*/
inline void parser::end_scope()
{
    //TODO: implement
}

/*
    Parses code. Expects to be called on the first char after ':::'. Will leave on the first char
    after the code command ends. After ':::' and any argument, the first whitespace (if there) is 
    ignored and the rest is parsed verbatim until ':::' ending the command.
*/
inline void parser::parse_code()
{
    size_t activation_colons = 3;
    size_t colon_count = 0;
    std::vector<char> text{}; 
    std::vector<char> arg{};
    int c;
    while(getc() == CHAR_COLON)
        { activation_colons++; nextc(); }
    if(getc() == CHAR_BRACK_SQ_OPEN)
        arg = parse_arg();
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

/* Returns the current character that was also last returned by nextc(). Does not advance the cursor. */
inline int parser::getc()
{
    if(buffer_cursor >= buffer_end) [[unlikely]] {
        return CHAR_EOF;
    }
    return *buffer_cursor;
}

/*
    Retrieves the next character from the stream and advances the cursor by one. Also sets indent and
    line_is_empty accordingly. Will return CHAR_EOF if the stream ended.
    Maintains the follwing invariant: buffer_cursor-- always points to the previous char and buffer_cursor
    points to the current char as long as we did not reach the end of file. (after the function was called once.)
*/
int parser::nextc()
{
    buffer_cursor++;
    if(buffer_cursor >= buffer_end) [[unlikely]] {
        buffer[0] = *(buffer_cursor - 1);
        stream.read(buffer.data() + 1, BUFFER_SIZE - 1);
        std::streamsize read_count = stream.gcount();
        buffer_cursor = buffer.data() + 1;
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

inline int parser::prevc()
{
    return *(buffer_cursor - 1);
}

/*
    Parses a command argument. Expects to be called on the first char after '[' (the first char of the argument).
    Parses verbatim, except parses ':]' as ']' until ']' or CHAR_EOF is reached. Leaves cursor on first character after ']'.
*/
//TODO: test 
std::vector<char> parser::parse_arg()
{
    std::vector<char> out{};
    int c = getc(); 
    
    while(c != CHAR_EOF && c != ']') {
        if(c == CHAR_COLON) {
            while(nextc() == CHAR_COLON)
                out.push_back(CHAR_COLON);
            if(getc() == ']') { out.push_back(']'); nextc(); }
            else { out.push_back(CHAR_COLON); c = getc(); }
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

}