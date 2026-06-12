#pragma once

#include <lua.hpp>
#include <lauxlib.h>
#include <utils/net/server.hpp>
#include <utils/net/client.hpp>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace LuaEngine {

    // Global networking state
    struct NetState {
        std::unordered_map<int, std::shared_ptr<BlazeBolt::Net::TcpServer>> tcpServers;
        std::unordered_map<int, std::shared_ptr<BlazeBolt::Net::TcpClient>> tcpClients;
        std::unordered_map<int, std::shared_ptr<BlazeBolt::Net::UdpServer>> udpServers;
        std::unordered_map<int, std::shared_ptr<BlazeBolt::Net::UdpClient>> udpClients;
        int nextId;
        bool initialized;

        NetState() : nextId(1), initialized(false) {}
    };

    inline NetState& getNetState() {
        static NetState state;
        return state;
    }

    // ==================== NETWORK LUA FUNCTIONS ====================
    class _netFunctions {
    public:

        // Init / Shutdown
        static int NetInit(lua_State* state) {
            NetState& ns = getNetState();
            if (ns.initialized) {
                lua_pushboolean(state, true);
                return 1;
            }
            ns.initialized = BlazeBolt::Net::netInit();
            lua_pushboolean(state, ns.initialized);
            return 1;
        }

        static int NetShutdown(lua_State* state) {
            NetState& ns = getNetState();
            ns.tcpServers.clear();
            ns.tcpClients.clear();
            ns.udpServers.clear();
            ns.udpClients.clear();
            BlazeBolt::Net::netShutdown();
            ns.initialized = false;
            return 0;
        }

        // ==================== TCP SERVER ====================

        static int CreateTCPServer(lua_State* state) {
            int port = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();

            auto server = std::make_shared<BlazeBolt::Net::TcpServer>();
            if (!server->start(port)) {
                lua_pushnil(state);
                return 1;
            }

            int id = ns.nextId++;
            ns.tcpServers[id] = server;
            lua_pushinteger(state, id);
            return 1;
        }

        static int TCPServerStop(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.tcpServers.find(id);
            if (it != ns.tcpServers.end()) {
                it->second->stop();
                ns.tcpServers.erase(it);
            }
            return 0;
        }

        static int TCPServerIsRunning(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.tcpServers.find(id);
            if (it != ns.tcpServers.end()) {
                lua_pushboolean(state, it->second->isRunning());
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        static int TCPServerPoll(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.tcpServers.find(id);
            if (it != ns.tcpServers.end()) {
                it->second->poll();
            }
            return 0;
        }

        static int TCPServerAccept(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.tcpServers.find(id);
            if (it != ns.tcpServers.end()) {
                int clientId;
                std::string clientAddr;
                uint16_t clientPort;
                if (it->second->acceptConnection(clientId, clientAddr, clientPort)) {
                    lua_pushinteger(state, clientId);
                    lua_pushstring(state, clientAddr.c_str());
                    lua_pushinteger(state, clientPort);
                    return 3;
                }
            }
            lua_pushnil(state);
            return 1;
        }

        static int TCPServerSend(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            int clientId = luaL_checkinteger(state, 2);
            const char* data = luaL_checkstring(state, 3);
            NetState& ns = getNetState();
            auto it = ns.tcpServers.find(id);
            if (it != ns.tcpServers.end()) {
                lua_pushboolean(state, it->second->sendToClient(clientId, std::string(data)));
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        static int TCPServerBroadcast(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            const char* data = luaL_checkstring(state, 2);
            NetState& ns = getNetState();
            auto it = ns.tcpServers.find(id);
            if (it != ns.tcpServers.end()) {
                lua_pushboolean(state, it->second->broadcast(std::string(data)));
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        static int TCPServerReceive(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            int clientId = luaL_checkinteger(state, 2);
            NetState& ns = getNetState();
            auto it = ns.tcpServers.find(id);
            if (it != ns.tcpServers.end()) {
                std::string data;
                if (it->second->receiveFromClient(clientId, data)) {
                    lua_pushstring(state, data.c_str());
                    return 1;
                }
            }
            lua_pushnil(state);
            return 1;
        }

        static int TCPServerDisconnect(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            int clientId = luaL_checkinteger(state, 2);
            NetState& ns = getNetState();
            auto it = ns.tcpServers.find(id);
            if (it != ns.tcpServers.end()) {
                it->second->disconnectClient(clientId);
            }
            return 0;
        }

        static int TCPServerGetClientCount(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.tcpServers.find(id);
            if (it != ns.tcpServers.end()) {
                lua_pushinteger(state, it->second->getClientCount());
            } else {
                lua_pushinteger(state, 0);
            }
            return 1;
        }

        static int TCPServerIsClientConnected(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            int clientId = luaL_checkinteger(state, 2);
            NetState& ns = getNetState();
            auto it = ns.tcpServers.find(id);
            if (it != ns.tcpServers.end()) {
                lua_pushboolean(state, it->second->isClientConnected(clientId));
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        // ==================== TCP CLIENT ====================

        static int CreateTCPClient(lua_State* state) {
            NetState& ns = getNetState();
            auto client = std::make_shared<BlazeBolt::Net::TcpClient>();
            int id = ns.nextId++;
            ns.tcpClients[id] = client;
            lua_pushinteger(state, id);
            return 1;
        }

        static int TCPClientConnect(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            const char* host = luaL_checkstring(state, 2);
            int port = luaL_checkinteger(state, 3);
            NetState& ns = getNetState();
            auto it = ns.tcpClients.find(id);
            if (it != ns.tcpClients.end()) {
                lua_pushboolean(state, it->second->connect(host, static_cast<uint16_t>(port)));
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        static int TCPClientSend(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            const char* data = luaL_checkstring(state, 2);
            NetState& ns = getNetState();
            auto it = ns.tcpClients.find(id);
            if (it != ns.tcpClients.end()) {
                lua_pushboolean(state, it->second->send(std::string(data)));
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        static int TCPClientReceive(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.tcpClients.find(id);
            if (it != ns.tcpClients.end()) {
                std::string data;
                if (it->second->receive(data)) {
                    lua_pushstring(state, data.c_str());
                    return 1;
                }
            }
            lua_pushnil(state);
            return 1;
        }

        static int TCPClientDisconnect(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.tcpClients.find(id);
            if (it != ns.tcpClients.end()) {
                it->second->disconnect();
                ns.tcpClients.erase(it);
            }
            return 0;
        }

        static int TCPClientIsConnected(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.tcpClients.find(id);
            if (it != ns.tcpClients.end()) {
                lua_pushboolean(state, it->second->isConnected());
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        // ==================== UDP SERVER ====================

        static int CreateUDPServer(lua_State* state) {
            int port = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();

            auto server = std::make_shared<BlazeBolt::Net::UdpServer>();
            if (!server->start(port)) {
                lua_pushnil(state);
                return 1;
            }

            int id = ns.nextId++;
            ns.udpServers[id] = server;
            lua_pushinteger(state, id);
            return 1;
        }

        static int UDPServerStop(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.udpServers.find(id);
            if (it != ns.udpServers.end()) {
                it->second->stop();
                ns.udpServers.erase(it);
            }
            return 0;
        }

        static int UDPServerIsRunning(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.udpServers.find(id);
            if (it != ns.udpServers.end()) {
                lua_pushboolean(state, it->second->isRunning());
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        static int UDPServerPoll(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.udpServers.find(id);
            if (it != ns.udpServers.end()) {
                it->second->poll();
            }
            return 0;
        }

        static int UDPServerSend(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            int peerId = luaL_checkinteger(state, 2);
            const char* data = luaL_checkstring(state, 3);
            NetState& ns = getNetState();
            auto it = ns.udpServers.find(id);
            if (it != ns.udpServers.end()) {
                lua_pushboolean(state, it->second->sendToPeer(peerId, std::string(data)));
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        static int UDPServerReceive(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            int peerId = luaL_checkinteger(state, 2);
            NetState& ns = getNetState();
            auto it = ns.udpServers.find(id);
            if (it != ns.udpServers.end()) {
                std::string data;
                if (it->second->receiveFromPeer(peerId, data)) {
                    lua_pushstring(state, data.c_str());
                    return 1;
                }
            }
            lua_pushnil(state);
            return 1;
        }

        static int UDPServerReceiveAny(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.udpServers.find(id);
            if (it != ns.udpServers.end()) {
                int peerId;
                std::string data;
                if (it->second->receiveAny(peerId, data)) {
                    lua_pushinteger(state, peerId);
                    lua_pushstring(state, data.c_str());
                    return 2;
                }
            }
            lua_pushnil(state);
            return 1;
        }

        static int UDPServerRemovePeer(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            int peerId = luaL_checkinteger(state, 2);
            NetState& ns = getNetState();
            auto it = ns.udpServers.find(id);
            if (it != ns.udpServers.end()) {
                it->second->removePeer(peerId);
            }
            return 0;
        }

        static int UDPServerGetPeerCount(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.udpServers.find(id);
            if (it != ns.udpServers.end()) {
                lua_pushinteger(state, it->second->getPeerCount());
            } else {
                lua_pushinteger(state, 0);
            }
            return 1;
        }

        static int UDPServerIsPeerKnown(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            int peerId = luaL_checkinteger(state, 2);
            NetState& ns = getNetState();
            auto it = ns.udpServers.find(id);
            if (it != ns.udpServers.end()) {
                lua_pushboolean(state, it->second->isPeerKnown(peerId));
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        // ==================== UDP CLIENT ====================

        static int CreateUDPClient(lua_State* state) {
            NetState& ns = getNetState();
            auto client = std::make_shared<BlazeBolt::Net::UdpClient>();
            int id = ns.nextId++;
            ns.udpClients[id] = client;
            lua_pushinteger(state, id);
            return 1;
        }

        static int UDPClientConnect(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            const char* host = luaL_checkstring(state, 2);
            int port = luaL_checkinteger(state, 3);
            NetState& ns = getNetState();
            auto it = ns.udpClients.find(id);
            if (it != ns.udpClients.end()) {
                lua_pushboolean(state, it->second->connect(host, static_cast<uint16_t>(port)));
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        static int UDPClientSend(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            const char* data = luaL_checkstring(state, 2);
            NetState& ns = getNetState();
            auto it = ns.udpClients.find(id);
            if (it != ns.udpClients.end()) {
                lua_pushboolean(state, it->second->send(std::string(data)));
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }

        static int UDPClientReceive(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.udpClients.find(id);
            if (it != ns.udpClients.end()) {
                std::string data;
                if (it->second->receive(data)) {
                    lua_pushstring(state, data.c_str());
                    return 1;
                }
            }
            lua_pushnil(state);
            return 1;
        }

        static int UDPClientDisconnect(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.udpClients.find(id);
            if (it != ns.udpClients.end()) {
                it->second->disconnect();
                ns.udpClients.erase(it);
            }
            return 0;
        }

        static int UDPClientIsConnected(lua_State* state) {
            int id = luaL_checkinteger(state, 1);
            NetState& ns = getNetState();
            auto it = ns.udpClients.find(id);
            if (it != ns.udpClients.end()) {
                lua_pushboolean(state, it->second->isConnected());
            } else {
                lua_pushboolean(state, false);
            }
            return 1;
        }
    };
}
