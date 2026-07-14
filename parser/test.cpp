#include "dodo.hpp"
#include <iostream>

int indent = 0;
void cs(std::string name, dodo::cmd_type type, std::string_view args) {
    for(int i = 0; i < indent; i++) std::cout << "  ";
    
    std::cout << name << ":" << args << "\n";
    indent++;
}

void en() {
    indent--;
}

void te(std::string_view text) {
 for(int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << text << "\n";
}

int main() {
    std::ifstream s{"test.txt"};
    dodo::parser p(s, cs, en, te);
    p.parse();
}
