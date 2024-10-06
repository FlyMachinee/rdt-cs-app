#include "../include/wsa_wapper.h"
#include "../include/pretty_log.hpp"
#include <format>

bool ::my::wsa_initialized = false;

/**
 * @brief Initialize Winsock.
 * @return true if Winsock is successfully initialized, false otherwise.
 */
bool ::my::init_wsa()
{
    if (wsa_initialized) {
        pretty_log << "Winsock already initialized";
        return true;
    }

    WSADATA wsaData;
    pretty_log
        << "Initializing Winsock..."
        << ::std::format("Expected Winsock version: {}.{}", HIBYTE(WINSOCK_VERSION), LOBYTE(WINSOCK_VERSION));

    if (WSAStartup(WINSOCK_VERSION, &wsaData)) {
        pretty_err
            << ::std::format("WSAStartup failed. Error code: {}", WSAGetLastError())
            << "Winsock initialization failed";
        return false;
    }

    pretty_log << "WSAStartup succeeded";

    if (LOBYTE(wsaData.wVersion) != LOBYTE(WINSOCK_VERSION) || HIBYTE(wsaData.wVersion) != HIBYTE(WINSOCK_VERSION)) {
        pretty_err << "Winsock version not supported"
                   << ::std::format("Get: {}.{}", HIBYTE(wsaData.wVersion), LOBYTE(wsaData.wVersion))
                   << "Winsock initialization failed";
        WSACleanup();
        return false;
    }

    pretty_log << "Winsock initialized";
    wsa_initialized = true;
    return true;
}

/**
 * @brief Clean up Winsock.
 * @return true if Winsock is successfully cleaned up, false otherwise.
 */
bool ::my::cleanup_wsa()
{
    if (!wsa_initialized) {
        pretty_log << "Winsock not initialized";
        return false;
    }

    pretty_log << "Cleaning up Winsock...";
    if (WSACleanup()) {
        pretty_err
            << ::std::format("WSACleanup failed. Error code: {}", WSAGetLastError())
            << "Winsock cleanup failed";
        return false;
    }

    pretty_log
        << "WSACleanup succeeded"
        << "Winsock cleaned up";
    wsa_initialized = false;
    return true;
}
