/**
 * Sample implementation of a program using the dodo parsing library.
 * Parses text from a filename given and prints it to the standard output.
 */

#include "dodo.hpp"

#include <iostream>

int indent = 0;

struct cmd {
    std::string_view name;
    dodo::cmd_type type;
};

std::stack<cmd> stack;

void put_indent()
{
    for (int i = 0; i < indent; i++) {
        std::cout << "    ";
    }
}

void put_cdec(std::string_view name, std::string_view& args)
{
    std::cout << "<" << name;
    if (!args.empty()) {
        std::cout << " " << args;
    }
    std::cout << ">";
}

void put_edec(std::string_view name) { std::cout << "</" << name << ">"; }

void command_starts(std::string_view name, dodo::cmd_type type, std::string_view args)
{
    stack.push(cmd{name, type});
    switch (type) {
    case dodo::cmd_type::Block:
        put_indent();
        indent++;
        put_cdec(name, args);
        std::cout << "\n";
        break;
    case dodo::cmd_type::Text:
    case dodo::cmd_type::Code:
        put_indent();
        put_cdec(name, args);
        break;
    case dodo::cmd_type::Inline:
    case dodo::cmd_type::InlineEmpty:
    default:
        put_cdec(name, args);
        break;
    }
}

void command_ends()
{
    switch (stack.top().type) {
    case dodo::cmd_type::Block:
        indent--;
        put_indent();
        put_edec(stack.top().name);
        std::cout << "\n";
        break;
    case dodo::cmd_type::Text:
    case dodo::cmd_type::Code:
        put_edec(stack.top().name);
        std::cout << "\n";
        break;
    default:
        put_edec(stack.top().name);
        break;
    }
    stack.pop();
}

void text_given(std::string_view text) { std::cout << text; }

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "No filename.";
        return 1;
    }
    std::ifstream stream{*(argv + 1)};
    dodo::parser p(stream, command_starts, command_ends, text_given);
    if (!p.parse()) {
        std::cout << "\n\n" << p.get_error_message();
    }
    return 0;
}
