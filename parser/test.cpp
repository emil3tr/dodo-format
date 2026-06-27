#include "DoDoParser.hpp"

int main() {
    dodo::Cmd vec[10];
    vec[0].next = 1;
    vec[1].next = 2;
    vec[3].next = 1;
    vec[0].child_end = 4;
    for(int i = 0; i < 10; i++) vec[i].name = i;

    for(auto& child : vec[0]) {
        std::cout << " " << child.name;
        
    }
}