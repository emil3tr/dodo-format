/**
 * Simple test program. Takes two filenames as argument. Parses first file and compares
 * it to the second file using the 'diff' command. The 'diff' utility must be available.
 * Outputs nothing if correct, otherwise the output is as for diff.
 */

#include "../dodo.hpp"

#include <cstdio>
#include <cstdlib>

using namespace dodo;
std::ofstream ostr;
std::size_t indent = 0;
std::stack<cmd_type> stack;
bool newl = false;

void print_indent()
{
    for (std::size_t i = 0; i < indent; i++) {
        ostr << "  ";
    }
}

void print_na(std::string_view n, std::string_view a)
{
    ostr << ":" << n;
    if (!a.empty()) {
        ostr << "[" << a << "]";
    }
}

void s(std::string_view n, dodo::cmd_type t, std::string_view a)
{
    if(newl) {
        ostr << "\n";
    }
    switch (t) {
    case cmd_type::Block:
        print_indent();
        print_na(n, a);
        ostr << ":";
        newl = true;
        indent++;
        break;
    case cmd_type::Code:
    case cmd_type::Text:
        print_indent();
        print_na(n, a);
        ostr << ":";
        break;
    default:
        print_na(n, a);
        ostr << "{";
        break;
    }
    stack.push(t);
}

void e()
{
    switch (stack.top()) {
    case cmd_type::Block:
        indent--;
        break;
    case cmd_type::Text:
    case cmd_type::Code:
        ostr << "\n";
        break;
    default:
        ostr << "}";
    }
    stack.pop();
}

void t(std::string_view t) { ostr << t; }

int main(int argc, char** argv)
{
    std::string sysstr("diff TEMP_TO_COMPARE.out ");

    if (argc < 3) {
        std::cout << "Please give two files.";
        return 1;
    }
    std::ifstream istr(argv[1]);
    if (!istr) {
        std::cout << "Error.";
        return 1;
    }
    ostr = std::ofstream("TEMP_TO_COMPARE.out");
    if (!ostr) {
        std::cout << "Error.";
        return 1;
    }

    parser p(istr, s, e, t);
    p.parse();

    ostr.close();
    istr.close();

    sysstr.append(argv[2]);
    std::system(sysstr.c_str());

    std::remove("TEMP_TO_COMPARE.out");
    return 0;
}
