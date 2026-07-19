/**
 * A very simple runtime test. Creates 1 000 000 blocks of text, each containing three nested commands, one inline
 * command, a longer text command with bad spacing and some edge cases. Amounts to about 100 million characters.
 */

#include "../dodo.hpp"
#include <chrono>
#include <cstdio>

int acc = 0;

void s(std::string_view n, dodo::cmd_type t, std::string_view a) {
    acc += n.length();
}

void e() {
    acc++;
}

void t(std::string_view t) {
    acc += t.length();
}

int main() {
    std::ofstream test_text("test_text.dodo");
    if (!test_text)
    {
        std::cout << "Error";
        return 1;
    }

    for(int i = 0; i < 1000000; i++) {
        test_text << ":cmd[arg]: text\n  :nested: Hel :i{ll}lo!\n  :nested: Hi!\n";
        test_text << "This is   a   longer   paragraph     ?    :,: :.:. ?\n";
    }
    test_text.close();

    std::ifstream istr("test_text.dodo");

    std::chrono::time_point start = std::chrono::high_resolution_clock::now();
    dodo::parser p(istr, s, e, t);
    p.parse(); 
    std::chrono::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur = (end - start);
    std::cout << "\nTime: " << dur << "\n";
    std::remove("test_text.dodo");
}