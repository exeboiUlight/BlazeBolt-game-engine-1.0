#include <utils/net/server.hpp>
#include <iostream>
#include <cstring>

namespace BlazeBolt {
namespace Net {

    // ==================== TcpServer ====================

    TcpServer::TcpServer()
        : listenSock(INVALID_SOCK), running(false), port(0), nextClientId(1)
    {
    }

    TcpServer::~TcpServer() {
        stop();
    }

    bool TcpServer::start(int port) {
        if (running) return false;

        this->port = port;
        listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSock == INVALID_SOCK) {
            std::cerr << "[TCP Server] Failed to create socket: " << netGetLastError() << std::endl;
            return false;
        }

        int opt = 1;
    #ifdef _WIN32
        setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    #else
        setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(listenSock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "[TCP Server] Failed to bind: " << netGetLastError() << std::endl;
            netCloseSocket(listenSock);
            listenSock = INVALID_SOCK;
            return false;
        }

        if (listen(listenSock, SOMAXCONN) < 0) {
            std::cerr << "[TCP Server] Failed to listen: " << netGetLastError() << std::endl;
            netCloseSocket(listenSock);
            listenSock = INVALID_SOCK;
            return false;
        }

        netSetNonBlocking(listenSock, true);

        running = true;
        acceptThread = std::thread(&TcpServer::acceptLoop, this);
        receiveThread = std::thread(&TcpServer::receiveLoop, this);

        std::cout << "[TCP Server] Started on port " << port << std::endl;
        return true;
    }

    void TcpServer::stop() {
        if (!running) return;

        running = false;

        if (acceptThread.joinable()) acceptThread.join();
        if (receiveThread.joinable()) receiveThread.join();

        if (listenSock != INVALID_SOCK) {
            netCloseSocket(listenSock);
            listenSock = INVALID_SOCK;
        }

        for (auto& client : clients) {
            if (client->connected) {
                netCloseSocket(client->sock);
                client->connected = false;
            }
        }
        clients.clear();

        std::cout << "[TCP Server] Stopped" << std::endl;
    }

    bool TcpServer::isRunning() const {
        return running;
    }

    void TcpServer::acceptLoop() {
        while (running) {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);

            SocketType clientSock = accept(listenSock, (struct sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSock == INVALID_SOCK) {
            #ifdef _WIN32
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK) {
                    std::cerr << "[TCP Server] Accept error: " << netGetLastError() << std::endl;
                }
            #else
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    std::cerr << "[TCP Server] Accept error: " << netGetLastError() << std::endl;
                }
            #endif
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            netSetNonBlocking(clientSock, true);

            auto client = std::make_shared<TcpClientState>();
            client->sock = clientSock;
            client->connected = true;
            client->id = nextClientId++;

            struct sockaddr_in* sin = (struct sockaddr_in*)&clientAddr;
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));

            NetAddress addr;
            addr.addrLen = sizeof(clientAddr);
            memcpy(&addr.addr, &clientAddr, sizeof(clientAddr));
            client->address = addr;

            std::lock_guard<std::mutex> lock(queueMutex);
            clients.push_back(client);

            std::cout << "[TCP Server] Client connected: " << ip << ":" << ntohs(sin->sin_port)
                      << " (id=" << client->id << ")" << std::endl;
        }
    }

    void TcpServer::receiveLoop() {
        while (running) {
            std::vector<std::shared_ptr<TcpClientState>> snapshot;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                snapshot = clients;
            }

            for (auto& client : snapshot) {
                if (!client->connected) continue;

                char buf[4096];
                while (true) {
                    int bytesReceived = recv(client->sock, buf, sizeof(buf), 0);
                    if (bytesReceived > 0) {
                        std::lock_guard<std::mutex> lock(client->mutex);
                        client->receiveBuffer.append(buf, bytesReceived);

                        while (client->receiveBuffer.size() >= HEADER_SIZE) {
                            PacketHeader header;
                            memcpy(&header, client->receiveBuffer.data(), HEADER_SIZE);

                            if (header.size > MAX_PACKET_SIZE || client->receiveBuffer.size() < HEADER_SIZE + header.size) {
                                break;
                            }

                            std::string message = client->receiveBuffer.substr(HEADER_SIZE, header.size);
                            client->receiveBuffer.erase(0, HEADER_SIZE + header.size);
                            client->incomingMessages.push_back(message);
                        }
                    } else if (bytesReceived == 0) {
                        client->connected = false;
                        std::cout << "[TCP Server] Client " << client->id << " disconnected" << std::endl;
                        break;
                    } else {
                        break;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    bool TcpServer::sendRaw(SocketType sock, const void* data, size_t len) {
        const char* ptr = (const char*)data;
        size_t remaining = len;
        while (remaining > 0) {
            int sent = send(sock, ptr, remaining, 0);
            if (sent <= 0) return false;
            ptr += sent;
            remaining -= sent;
        }
        return true;
    }

    void TcpServer::poll() {
        // Connections and receives are handled by threads
        // This method can be used for additional processing if needed
    }

    bool TcpServer::acceptConnection(int& clientId, std::string& clientAddr, uint16_t& clientPort) {
        // Check if there are new clients that haven't been acknowledged
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto& client : clients) {
            if (client->id > 0 && client->connected) {
                // Return first connected client
                clientId = client->id;
                clientAddr = client->address.getIP();
                clientPort = client->address.getPort();
                return true;
            }
        }
        return false;
    }

    bool TcpServer::sendToClient(int clientId, const std::string& data) {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto& client : clients) {
            if (client->id == clientId && client->connected) {
                PacketHeader header;
                header.size = static_cast<uint32_t>(data.size());

                std::vector<char> packet(HEADER_SIZE + data.size());
                memcpy(packet.data(), &header, HEADER_SIZE);
                memcpy(packet.data() + HEADER_SIZE, data.data(), data.size());

                return sendRaw(client->sock, packet.data(), packet.size());
            }
        }
        return false;
    }

    bool TcpServer::broadcast(const std::string& data) {
        std::lock_guard<std::mutex> lock(queueMutex);
        bool allSent = true;

        PacketHeader header;
        header.size = static_cast<uint32_t>(data.size());

        std::vector<char> packet(HEADER_SIZE + data.size());
        memcpy(packet.data(), &header, HEADER_SIZE);
        memcpy(packet.data() + HEADER_SIZE, data.data(), data.size());

        for (auto& client : clients) {
            if (client->connected) {
                if (!sendRaw(client->sock, packet.data(), packet.size())) {
                    allSent = false;
                }
            }
        }
        return allSent;
    }

    bool TcpServer::receiveFromClient(int clientId, std::string& data) {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto& client : clients) {
            if (client->id == clientId) {
                std::lock_guard<std::mutex> clientLock(client->mutex);
                if (!client->incomingMessages.empty()) {
                    data = client->incomingMessages.front();
                    client->incomingMessages.erase(client->incomingMessages.begin());
                    return true;
                }
                return false;
            }
        }
        return false;
    }

    void TcpServer::disconnectClient(int clientId) {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto it = clients.begin(); it != clients.end(); ++it) {
            if ((*it)->id == clientId) {
                (*it)->connected = false;
                netCloseSocket((*it)->sock);
                (*it)->sock = INVALID_SOCK;
                clients.erase(it);
                std::cout << "[TCP Server] Disconnected client " << clientId << std::endl;
                return;
            }
        }
    }

    int TcpServer::getClientCount() const {
        int count = 0;
        for (auto& client : clients) {
            if (client->connected) count++;
        }
        return count;
    }

    bool TcpServer::isClientConnected(int clientId) const {
        for (auto& client : clients) {
            if (client->id == clientId) return client->connected;
        }
        return false;
    }

    // ==================== TcpClient ====================

    TcpClient::TcpClient()
        : sock(INVALID_SOCK), connected(false), id(-1), remotePort(0)
    {
    }

    TcpClient::~TcpClient() {
        disconnect();
    }

    bool TcpClient::connect(const std::string& host, uint16_t port) {
        if (connected) return false;

        this->remoteHost = host;
        this->remotePort = port;

        struct addrinfo hints, *result = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        std::string portStr = std::to_string(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            std::cerr << "[TCP Client] Failed to resolve host: " << host << std::endl;
            return false;
        }

        sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock == INVALID_SOCK) {
            std::cerr << "[TCP Client] Failed to create socket: " << netGetLastError() << std::endl;
            freeaddrinfo(result);
            return false;
        }

        if (::connect(sock, result->ai_addr, result->ai_addrlen) < 0) {
            std::cerr << "[TCP Client] Failed to connect: " << netGetLastError() << std::endl;
            netCloseSocket(sock);
            sock = INVALID_SOCK;
            freeaddrinfo(result);
            return false;
        }
        freeaddrinfo(result);

        netSetNonBlocking(sock, true);
        connected = true;

        receiveThread = std::thread(&TcpClient::receiveLoop, this);

        std::cout << "[TCP Client] Connected to " << host << ":" << port << std::endl;
        return true;
    }

    void TcpClient::disconnect() {
        if (!connected) return;

        connected = false;

        if (receiveThread.joinable()) receiveThread.join();

        if (sock != INVALID_SOCK) {
            netCloseSocket(sock);
            sock = INVALID_SOCK;
        }

        std::cout << "[TCP Client] Disconnected" << std::endl;
    }

    bool TcpClient::isConnected() const {
        return connected;
    }

    bool TcpClient::send(const std::string& data) {
        if (!connected) return false;

        PacketHeader header;
        header.size = static_cast<uint32_t>(data.size());

        std::vector<char> packet(HEADER_SIZE + data.size());
        memcpy(packet.data(), &header, HEADER_SIZE);
        memcpy(packet.data() + HEADER_SIZE, data.data(), data.size());

        return sendRaw(packet.data(), packet.size());
    }

    bool TcpClient::sendRaw(const void* data, size_t len) {
        const char* ptr = (const char*)data;
        size_t remaining = len;
        while (remaining > 0) {
            int sent = ::send(sock, ptr, remaining, 0);
            if (sent <= 0) return false;
            ptr += sent;
            remaining -= sent;
        }
        return true;
    }

    void TcpClient::receiveLoop() {
        while (connected) {
            char buf[4096];
            int bytesReceived = recv(sock, buf, sizeof(buf), 0);
            if (bytesReceived > 0) {
                std::lock_guard<std::mutex> lock(mutex);
                receiveBuffer.append(buf, bytesReceived);

                while (receiveBuffer.size() >= HEADER_SIZE) {
                    PacketHeader header;
                    memcpy(&header, receiveBuffer.data(), HEADER_SIZE);

                    if (header.size > MAX_PACKET_SIZE || receiveBuffer.size() < HEADER_SIZE + header.size) {
                        break;
                    }

                    std::string message = receiveBuffer.substr(HEADER_SIZE, header.size);
                    receiveBuffer.erase(0, HEADER_SIZE + header.size);
                    incomingMessages.push_back(message);
                }
            } else if (bytesReceived == 0) {
                connected = false;
                std::cout << "[TCP Client] Connection closed by server" << std::endl;
                break;
            } else {
            #ifdef _WIN32
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK) {
                    connected = false;
                    break;
                }
            #else
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    connected = false;
                    break;
                }
            #endif
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    bool TcpClient::receive(std::string& data) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!incomingMessages.empty()) {
            data = incomingMessages.front();
            incomingMessages.erase(incomingMessages.begin());
            return true;
        }
        return false;
    }

}
}
