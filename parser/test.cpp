#include "DoDoParser.hpp"

int main() {
    std::ifstream aa{"test.txt"};
   dodo::Parser parser(aa);
    return 0; 
}