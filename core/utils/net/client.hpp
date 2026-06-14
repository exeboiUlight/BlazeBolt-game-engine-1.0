#pragma once

#include <utils/net/netCommon.hpp>
#include <thread>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <memory>

namespace BlazeBolt {
namespace Net {

    struct UdpPeer {
        NetAddress address;
        int id;
        std::string receiveBuffer;
        std::vector<std::string> incomingMessages;
        std::mutex mutex;
        bool active;

        UdpPeer() : id(-1), active(false) {}
    };

    struct UdpPacket {
        int peerId;
        std::string data;
    };

    class UdpServer {
    private:
        SocketType sock;
        std::unordered_map<std::string, std::shared_ptr<UdpPeer>> peers;
        std::vector<UdpPacket> incomingPackets;
        std::vector<UdpPacket> outgoingPackets;
        std::mutex mutex;
        std::atomic<bool> running;
        int port;
        int nextPeerId;
        std::thread receiveThread;

        void receiveLoop();
        std::string addressToKey(const NetAddress& addr);

    public:
        UdpServer();
        ~UdpServer();

        bool start(int port);
        void stop();
        bool isRunning() const;

        void poll();
        bool sendToPeer(int peerId, const std::string& data);
        bool receiveFromPeer(int peerId, std::string& data);
        bool receiveAny(int& peerId, std::string& data);
        void removePeer(int peerId);
        int getPeerCount() const;
        bool isPeerKnown(int peerId) const;
    };

    class UdpClient {
    private:
        SocketType sock;
        std::atomic<bool> connected;
        std::vector<std::string> incomingMessages;
        std::mutex mutex;
        std::thread receiveThread;
        NetAddress remoteAddr;

        void receiveLoop();

    public:
        UdpClient();
        ~UdpClient();

        bool connect(const std::string& host, uint16_t port);
        void disconnect();
        bool isConnected() const;

        bool send(const std::string& data);
        bool receive(std::string& data);
    };

}
}
