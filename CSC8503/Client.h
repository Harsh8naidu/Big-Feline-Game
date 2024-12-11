#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "GameClient.h"
#include "NetworkBase.h"
#include "GameClient.h"
#include "NetworkedGame.h"

class ClientPacketReceiver : public PacketReceiver {
public:
    ClientPacketReceiver(const std::string& name);

    //void ReceivePacket(int type, GamePacket* payload, int source) override;

protected:
    std::string name;
};

class Client {
public:
    Client(const std::string& name, const std::string& serverIP, int serverPort);
    ~Client();

    void SendPacket(GamePacket* packet);
    void Update();

private:
    std::string name;
    GameClient* gameClient;
    //ClientPacketReceiver packetReceiver;

    std::vector<uint8_t> ParseIPAddress(const std::string& ipAddress);
};

#endif
