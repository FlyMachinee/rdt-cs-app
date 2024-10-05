#include "../include/pretty_log.hpp"
using namespace my;

int main(int argc, char const *argv[])
{
    pretty_log
        << "Hello, World!"
        << "This is a test program."
        << "Good luck!";
    pretty_err
        << "Hello, World!"
        << "This is a test program."
        << "Good luck!";
    return 0;
}
