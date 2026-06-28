#include "dodo_parser_naive.hpp"

int main() {
    std::ifstream aa{"test.txt"};
   dodo::parser parser(aa);
    return 0; 
}