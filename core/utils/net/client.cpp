#include <utils/net/client.hpp>
#include <iostream>
#include <cstring>

namespace BlazeBolt {
namespace Net {

    // ==================== UdpServer ====================

    UdpServer::UdpServer()
        : sock(INVALID_SOCK), running(false), port(0), nextPeerId(1)
    {
    }

    UdpServer::~UdpServer() {
        stop();
    }

    std::string UdpServer::addressToKey(const NetAddress& addr) {
        return addr.getIP() + ":" + std::to_string(addr.getPort());
    }

    bool UdpServer::start(int port) {
        if (running) return false;

        this->port = port;
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCK) {
            std::cerr << "[UDP Server] Failed to create socket: " << netGetLastError() << std::endl;
            return false;
        }

        int opt = 1;
    #ifdef _WIN32
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    #else
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "[UDP Server] Failed to bind: " << netGetLastError() << std::endl;
            netCloseSocket(sock);
            sock = INVALID_SOCK;
            return false;
        }

        netSetNonBlocking(sock, true);

        running = true;
        receiveThread = std::thread(&UdpServer::receiveLoop, this);

        std::cout << "[UDP Server] Started on port " << port << std::endl;
        return true;
    }

    void UdpServer::stop() {
        if (!running) return;

        running = false;

        if (receiveThread.joinable()) receiveThread.join();

        if (sock != INVALID_SOCK) {
            netCloseSocket(sock);
            sock = INVALID_SOCK;
        }

        peers.clear();
        std::cout << "[UDP Server] Stopped" << std::endl;
    }

    bool UdpServer::isRunning() const {
        return running;
    }

    void UdpServer::receiveLoop() {
        while (running) {
            char buf[65535];
            struct sockaddr_in fromAddr;
            socklen_t fromLen = sizeof(fromAddr);

            int bytesReceived = recvfrom(sock, buf, sizeof(buf), 0,
                                        (struct sockaddr*)&fromAddr, &fromLen);
            if (bytesReceived > 0) {
                NetAddress netAddr;
                netAddr.addrLen = fromLen;
                memcpy(&netAddr.addr, &fromAddr, fromLen);

                std::string key = addressToKey(netAddr);

                std::lock_guard<std::mutex> lock(mutex);
                auto it = peers.find(key);
                std::shared_ptr<UdpPeer> peer;
                if (it == peers.end()) {
                    peer = std::make_shared<UdpPeer>();
                    peer->address = netAddr;
                    peer->id = nextPeerId++;
                    peer->active = true;
                    peers[key] = peer;

                    std::cout << "[UDP Server] New peer " << peer->id
                              << " from " << netAddr.getIP() << ":" << netAddr.getPort() << std::endl;
                } else {
                    peer = it->second;
                }

                std::string data(buf, bytesReceived);
                std::lock_guard<std::mutex> peerLock(peer->mutex);
                peer->receiveBuffer.append(data);

                while (peer->receiveBuffer.size() >= HEADER_SIZE) {
                    PacketHeader header;
                    memcpy(&header, peer->receiveBuffer.data(), HEADER_SIZE);

                    if (header.size > MAX_PACKET_SIZE || peer->receiveBuffer.size() < HEADER_SIZE + header.size) {
                        break;
                    }

                    std::string message = peer->receiveBuffer.substr(HEADER_SIZE, header.size);
                    peer->receiveBuffer.erase(0, HEADER_SIZE + header.size);
                    peer->incomingMessages.push_back(message);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void UdpServer::poll() {
        // Receive is handled by thread
    }

    bool UdpServer::sendToPeer(int peerId, const std::string& data) {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& [key, peer] : peers) {
            if (peer->id == peerId && peer->active) {
                PacketHeader header;
                header.size = static_cast<uint32_t>(data.size());

                std::vector<char> packet(HEADER_SIZE + data.size());
                memcpy(packet.data(), &header, HEADER_SIZE);
                memcpy(packet.data() + HEADER_SIZE, data.data(), data.size());

                struct sockaddr_in* sin = (struct sockaddr_in*)&peer->address.addr;
                int sent = sendto(sock, packet.data(), packet.size(), 0,
                                 (struct sockaddr*)sin, peer->address.addrLen);
                return sent > 0;
            }
        }
        return false;
    }

    bool UdpServer::receiveFromPeer(int peerId, std::string& data) {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& [key, peer] : peers) {
            if (peer->id == peerId) {
                std::lock_guard<std::mutex> peerLock(peer->mutex);
                if (!peer->incomingMessages.empty()) {
                    data = peer->incomingMessages.front();
                    peer->incomingMessages.erase(peer->incomingMessages.begin());
                    return true;
                }
                return false;
            }
        }
        return false;
    }

    bool UdpServer::receiveAny(int& peerId, std::string& data) {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& [key, peer] : peers) {
            std::lock_guard<std::mutex> peerLock(peer->mutex);
            if (!peer->incomingMessages.empty()) {
                peerId = peer->id;
                data = peer->incomingMessages.front();
                peer->incomingMessages.erase(peer->incomingMessages.begin());
                return true;
            }
        }
        return false;
    }

    void UdpServer::removePeer(int peerId) {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto it = peers.begin(); it != peers.end(); ++it) {
            if (it->second->id == peerId) {
                peers.erase(it);
                return;
            }
        }
    }

    int UdpServer::getPeerCount() const {
        return static_cast<int>(peers.size());
    }

    bool UdpServer::isPeerKnown(int peerId) const {
        for (auto& [key, peer] : peers) {
            if (peer->id == peerId) return true;
        }
        return false;
    }

    // ==================== UdpClient ====================

    UdpClient::UdpClient()
        : sock(INVALID_SOCK), connected(false)
    {
    }

    UdpClient::~UdpClient() {
        disconnect();
    }

    bool UdpClient::connect(const std::string& host, uint16_t port) {
        if (connected) return false;

        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCK) {
            std::cerr << "[UDP Client] Failed to create socket: " << netGetLastError() << std::endl;
            return false;
        }

        struct addrinfo hints, *result = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        std::string portStr = std::to_string(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
            std::cerr << "[UDP Client] Failed to resolve host: " << host << std::endl;
            netCloseSocket(sock);
            sock = INVALID_SOCK;
            return false;
        }

        memcpy(&remoteAddr.addr, result->ai_addr, result->ai_addrlen);
        remoteAddr.addrLen = result->ai_addrlen;
        freeaddrinfo(result);

        netSetNonBlocking(sock, true);
        connected = true;

        receiveThread = std::thread(&UdpClient::receiveLoop, this);

        std::cout << "[UDP Client] Ready (connected to " << host << ":" << port << ")" << std::endl;
        return true;
    }

    void UdpClient::disconnect() {
        if (!connected) return;

        connected = false;

        if (receiveThread.joinable()) receiveThread.join();

        if (sock != INVALID_SOCK) {
            netCloseSocket(sock);
            sock = INVALID_SOCK;
        }

        std::cout << "[UDP Client] Disconnected" << std::endl;
    }

    bool UdpClient::isConnected() const {
        return connected;
    }

    bool UdpClient::send(const std::string& data) {
        if (!connected) return false;

        PacketHeader header;
        header.size = static_cast<uint32_t>(data.size());

        std::vector<char> packet(HEADER_SIZE + data.size());
        memcpy(packet.data(), &header, HEADER_SIZE);
        memcpy(packet.data() + HEADER_SIZE, data.data(), data.size());

        struct sockaddr_in* sin = (struct sockaddr_in*)&remoteAddr.addr;
        int sent = ::sendto(sock, packet.data(), packet.size(), 0,
                           (struct sockaddr*)sin, remoteAddr.addrLen);
        return sent > 0;
    }

    void UdpClient::receiveLoop() {
        while (connected) {
            char buf[65535];
            struct sockaddr_in fromAddr;
            socklen_t fromLen = sizeof(fromAddr);

            int bytesReceived = recvfrom(sock, buf, sizeof(buf), 0,
                                        (struct sockaddr*)&fromAddr, &fromLen);
            if (bytesReceived > 0) {
                std::string data(buf, bytesReceived);
                std::lock_guard<std::mutex> lock(mutex);

                while (data.size() >= HEADER_SIZE) {
                    PacketHeader header;
                    memcpy(&header, data.data(), HEADER_SIZE);

                    if (header.size > MAX_PACKET_SIZE || data.size() < HEADER_SIZE + header.size) {
                        break;
                    }

                    std::string message = data.substr(HEADER_SIZE, header.size);
                    data.erase(0, HEADER_SIZE + header.size);
                    incomingMessages.push_back(message);
                }
            } else {
            #ifdef _WIN32
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            #else
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            #endif
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    bool UdpClient::receive(std::string& data) {
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
