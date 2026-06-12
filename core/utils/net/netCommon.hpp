#pragma once

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #undef CreateWindow
    #undef CreateDialog
    #undef LoadImage
    #undef GetMessage
    #undef SendMessage
    #undef GetObject
    typedef SOCKET SocketType;
    #define INVALID_SOCK INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <cstring>
    typedef int SocketType;
    #define INVALID_SOCK (-1)
#endif

#include <string>
#include <vector>
#include <mutex>

namespace BlazeBolt {
namespace Net {

    inline bool netInit() {
    #ifdef _WIN32
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    #else
        return true;
    #endif
    }

    inline void netShutdown() {
    #ifdef _WIN32
        WSACleanup();
    #endif
    }

    inline std::string netGetLastError() {
    #ifdef _WIN32
        int err = WSAGetLastError();
        char buf[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buf, sizeof(buf), nullptr);
        return std::string(buf);
    #else
        return std::string(strerror(errno));
    #endif
    }

    inline bool netSetNonBlocking(SocketType sock, bool nonBlocking) {
    #ifdef _WIN32
        u_long mode = nonBlocking ? 1 : 0;
        return ioctlsocket(sock, FIONBIO, &mode) == 0;
    #else
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags == -1) return false;
        if (nonBlocking)
            flags |= O_NONBLOCK;
        else
            flags &= ~O_NONBLOCK;
        return fcntl(sock, F_SETFL, flags) != -1;
    #endif
    }

    inline void netCloseSocket(SocketType sock) {
    #ifdef _WIN32
        closesocket(sock);
    #else
        close(sock);
    #endif
    }

    struct NetAddress {
        struct sockaddr_storage addr;
        socklen_t addrLen;

        NetAddress() : addrLen(sizeof(addr)) {}

        std::string getIP() const {
            char ip[INET6_ADDRSTRLEN];
            if (addr.ss_family == AF_INET) {
                auto* sa = reinterpret_cast<const struct sockaddr_in*>(&addr);
                inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));
            } else {
                auto* sa = reinterpret_cast<const struct sockaddr_in6*>(&addr);
                inet_ntop(AF_INET6, &sa->sin6_addr, ip, sizeof(ip));
            }
            return std::string(ip);
        }

        uint16_t getPort() const {
            if (addr.ss_family == AF_INET) {
                return ntohs(reinterpret_cast<const struct sockaddr_in*>(&addr)->sin_port);
            }
            return ntohs(reinterpret_cast<const struct sockaddr_in6*>(&addr)->sin6_port);
        }

        bool operator==(const NetAddress& other) const {
            if (addrLen != other.addrLen) return false;
            return memcmp(&addr, &other.addr, addrLen) == 0;
        }

        bool operator!=(const NetAddress& other) const {
            return !(*this == other);
        }
    };

    struct PacketHeader {
        uint32_t size;
    };

    static const size_t MAX_PACKET_SIZE = 65507;
    static const size_t HEADER_SIZE = sizeof(PacketHeader);

}
}
