#include "dodo.hpp"
#include <iostream>

int indent = 0;
void cs(std::string name, dodo::cmd_type type, std::string_view args) {
    if(type == dodo::cmd_type::Inline) {
        std::cout << "<" << name << ":" << args << ">";
        return;
    }
    std::cout << "\n";
    for(int i = 0; i < indent; i++) std::cout << "   ";
    
    std::cout << name << ":" << args;
    indent++;
}

void en() {
    indent--;
    if(indent < 0) indent = 0;
}

void te(std::string_view text) {
    std::cout << " " << text; 
}

int main() {
    std::ifstream s{"test2.txt"};
    dodo::parser p(s, cs, en, te);
    if(!p.parse()) std::cout << p.get_error_message();
}
