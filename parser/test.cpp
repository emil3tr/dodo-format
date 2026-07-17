#include "dodo.hpp"

#include <iostream>

int indent = 0;

struct ccc {
    std::string_view n;
    dodo::cmd_type t;
};

std::stack<ccc> st;

void put_indent()
{
    for (int i = 0; i < indent; i++) {
        std::cout << "    ";
    }
}

void put_cdec(std::string_view name, std::string_view& args)
{
    std::cout << "<" << name;
    if (args.length() > 0) {
        std::cout << " " << args;
    }
    std::cout << ">";
}

void put_edec(std::string_view name) { std::cout << "</" << name << ">"; }

void cs(std::string_view name, dodo::cmd_type type, std::string_view args)
{
    st.push(ccc{name, type});
    if (type == dodo::cmd_type::Block) {
        put_indent();
        indent++;
        put_cdec(name, args);
        std::cout << "\n";
    } else if (type == dodo::cmd_type::Text) {
        put_indent();
        put_cdec(name, args);
    } else {
        put_cdec(name, args);
    }
}

void en()
{
    if (st.top().t == dodo::cmd_type::Block) {
        indent--;
        put_indent();
        put_edec(st.top().n);
        std::cout << "\n";
    } else if (st.top().t == dodo::cmd_type::Text) {
        put_edec(st.top().n);
        std::cout << "\n";
    } else {
        put_edec(st.top().n);
    }
    st.pop();
}

void te(std::string_view text) { std::cout << text; }

int main()
{
    std::ifstream s{"test.txt"};
    dodo::parser p(s, cs, en, te);
    if (!p.parse()) {
        std::cout << p.get_error_message();
    }
}
