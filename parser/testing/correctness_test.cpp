/**
 * Simple test program. Takes two filenames as argument. Parses first file and compares
 * it to the second file using the 'diff' command. The 'diff' utility must be available.
 * Outputs nothing if correct, otherwise the output is as for diff.
 */

#include "../dodo.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>

using namespace dodo;

#define TEMP_NAME "dodotest__temp.txt"

std::ofstream ostr;
std::size_t indent = 0;
std::stack<cmd_type> stack;
bool newl = false;

const std::filesystem::path in_dir_path("in");
const std::filesystem::path out_dir_path("out");

const std::string sys_string{"diff " TEMP_NAME " "}; 

void print_indent()
{
    for (std::size_t i = 0; i < indent; i++) {
        ostr << "  ";
    }
}

void print_na(std::string_view n, std::string_view a)
{
    ostr << ":" << n;
    if (!a.empty()) {
        ostr << "[" << a << "]";
    }
}

void s(std::string_view n, dodo::cmd_type t, std::string_view a)
{
    
    if(newl) {
        ostr << "\n";
        newl = false;
    }
    switch (t) {
    case cmd_type::Block:
        print_indent();
        print_na(n, a);
        ostr << ":";
        newl = true;
        indent++;
        break;
    case cmd_type::Code:
    case cmd_type::Text:
        print_indent();
        print_na(n, a);
        ostr << ":";
        break;
    default:
        print_na(n, a);
        ostr << "{";
        break;
    }
    stack.push(t);
}

void e()
{
    switch (stack.top()) {
    case cmd_type::Block:
        indent--;
        break;
    case cmd_type::Text:
    case cmd_type::Code:
        newl = true;
        break;
    default:
        ostr << "}";
    }
    stack.pop();
}

void t(std::string_view t) { ostr << t; }

bool try_compare(const std::filesystem::directory_entry& file) {
    std::filesystem::path in_path(file.path());
    std::filesystem::path out_path(out_dir_path / in_path.filename());
        if(!std::filesystem::exists(out_path)) {
            std::cout << "Error, file found in 'in' but not 'out'.";
            return 1;
        }
        std::ifstream istr(in_path);
        ostr = std::ofstream(TEMP_NAME);
        if(!istr || !ostr) {
            std::cout << "Error, could not open files.";
            return 1;
        }

        std::cout << "### Testing file " << in_path.filename() << " ###\n";

        dodo::parser p(istr, s, e, t);
        p.parse();
        istr.close();
        ostr.close();

        std::string cmd{sys_string};
        cmd.append(out_path.string());
        int out = std::system(cmd.c_str());
        std::remove(TEMP_NAME);
        if(out) {
            std::cout << "### Test failed on file " << in_path.filename() << ". See error above. ###\n";  
        }
        return out;
}

int main(int argc, char** argv)
{
    int retval = 0;
    if(!std::filesystem::exists(in_dir_path) || !std::filesystem::exists(out_dir_path)) {
        std::cout << "Error, 'in' our 'out' folder do not exist.\n";
        return 1;
    }

    for(const std::filesystem::directory_entry& file : std::filesystem::directory_iterator(in_dir_path)) {
        if(try_compare(file)) {
            retval = 1;
        }    
    }
    return retval;
}
