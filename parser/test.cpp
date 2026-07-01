#include "dodopp.hpp"

int main() {
    std::ifstream aa{"test.txt"};
   dodo::parser parser(aa);
    return 0; 
}