#pragma once

#include <utils/net/netCommon.hpp>
#include <thread>
#include <atomic>
#include <queue>
#include <functional>

namespace BlazeBolt {
namespace Net {

    struct TcpClientState {
        SocketType sock;
        NetAddress address;
        std::string receiveBuffer;
        std::vector<std::string> incomingMessages;
        std::mutex mutex;
        bool connected;
        int id;

        TcpClientState() : sock(INVALID_SOCK), connected(false), id(-1) {}
    };

    class TcpServer {
    private:
        SocketType listenSock;
        std::vector<std::shared_ptr<TcpClientState>> clients;
        std::queue<std::pair<int, std::string>> messageQueue;
        std::mutex queueMutex;
        std::atomic<bool> running;
        int port;
        int nextClientId;
        std::thread acceptThread;
        std::thread receiveThread;

        void acceptLoop();
        void receiveLoop();
        void handleClient(std::shared_ptr<TcpClientState> client);
        bool sendRaw(SocketType sock, const void* data, size_t len);

    public:
        TcpServer();
        ~TcpServer();

        bool start(int port);
        void stop();
        bool isRunning() const;

        void poll();
        bool acceptConnection(int& clientId, std::string& clientAddr, uint16_t& clientPort);
        bool sendToClient(int clientId, const std::string& data);
        bool broadcast(const std::string& data);
        bool receiveFromClient(int clientId, std::string& data);
        void disconnectClient(int clientId);
        int getClientCount() const;
        bool isClientConnected(int clientId) const;
    };

    class TcpClient {
    private:
        SocketType sock;
        std::atomic<bool> connected;
        std::string receiveBuffer;
        std::vector<std::string> incomingMessages;
        std::mutex mutex;
        std::thread receiveThread;
        int id;
        std::string remoteHost;
        uint16_t remotePort;

        void receiveLoop();
        bool sendRaw(const void* data, size_t len);

    public:
        TcpClient();
        ~TcpClient();

        bool connect(const std::string& host, uint16_t port);
        void disconnect();
        bool isConnected() const;

        bool send(const std::string& data);
        bool receive(std::string& data);
    };

}
}
