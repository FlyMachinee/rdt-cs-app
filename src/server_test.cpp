#include "../include/RDT_Server.hpp"

int main(int argc, char const *argv[])
{
    ::my::SR_Server<5, 10> server;
    server.run();
    return 0;
}