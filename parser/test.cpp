#include "dodopp.hpp"

#include <iostream>

std::ifstream istr{"test.txt"};
dodo::inbuff ibu{istr};

int c = ibu.get();

void prtxt() {
    while (c != '.') {
        std::cout << (char) c;
        c = ibu.next();
    }
    ibu.next();
}

int main() {
    prtxt();
    ibu.skip_space_tab();
    prtxt();
    ibu.skip_whitespace();
    prtxt();
    ibu.find('%');
    ibu.next();
    prtxt();
    return 0;
}
