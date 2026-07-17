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

void put_cdec(std::string_view name, std::string_view& args, dodo::cmd_type type)
{
    if(type == dodo::cmd_type::Inline || type == dodo::cmd_type::InlineEmpty) {
        std::cout << "<span class=\"" << name <<"\">";
    } else {
        std::cout << "<div class=\"" << name << "\">";
    }
}

void put_edec(std::string_view name, dodo::cmd_type type) { 
   if(type == dodo::cmd_type::Inline || type == dodo::cmd_type::InlineEmpty) {
        std::cout << "</span>";
    } else {
        std::cout << "</div>";
    }
}

void cs(std::string_view name, dodo::cmd_type type, std::string_view args)
{
    st.push(ccc{name, type});
    if (type == dodo::cmd_type::Block) {
        put_indent();
        indent++;
        put_cdec(name, args, type);
        std::cout << "\n";
    } else if (type == dodo::cmd_type::Text) {
        put_indent();
        put_cdec(name, args, type);
    } else {
        put_cdec(name, args, type);
    }
}

void en()
{
    if (st.top().t == dodo::cmd_type::Block) {
        indent--;
        put_indent();
        put_edec(st.top().n, st.top().t);
        std::cout << "\n";
    } else if (st.top().t == dodo::cmd_type::Text) {
        put_edec(st.top().n, st.top().t);
        std::cout << "\n";
    } else {
        put_edec(st.top().n, st.top().t);
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
