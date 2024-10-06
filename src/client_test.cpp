#include "../include/RDT_Client.hpp"

int main(int argc, char const *argv[])
{
    ::my::SR_Client<5, 10> client;
    client.run();
    return 0;
}