#include "client.h"
#include <iostream>
#include "NetworkBase.h"

#include <sstream>
#include <vector>
#include <stdexcept>
#include <cstdlib>

ClientPacketReceiver::ClientPacketReceiver(const std::string& name)
    : name(name) {}

//void ClientPacketReceiver::ReceivePacket(int type, GamePacket* payload, int source)
//{
//    if (type == String_Message) {
//        // Cast the payload to StringPacket
//		StringPacket* realPacket = static_cast<StringPacket*>(payload);
//        if (realPacket) {
//            std::string message = realPacket->GetStringFromData();
//            std::cout << name << " received message: " << message << " from source: " << source << std::endl;
//        }
//        else {
//            std::cerr << name << " received an invalid String_Message packet!" << std::endl;
//        }
//    }
//    else {
//        std::cerr << name << " received an unknown packet type: " << type << std::endl;
//    }
//}

Client::Client(const std::string& name, const std::string& serverIP, int serverPort)
    : name(name){
    // Create the GameClient instance
    gameClient = new GameClient();

    // Parse the IP address into uint8_t components
    std::vector<uint8_t> ipBytes = ParseIPAddress(serverIP);
    if (ipBytes.size() != 4) {
        throw std::invalid_argument("Invalid IP address format: " + serverIP);
    }

    // Connect the client to the server
    if (!gameClient->Connect(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3], serverPort)) {
        throw std::runtime_error("Failed to connect to server at " + serverIP + ":" + std::to_string(serverPort));
    }

    // Register the packet receiver
    //gameClient->RegisterPacketHandler(String_Message, &packetReceiver);
}

Client::~Client() {
    delete gameClient;
}

void Client::SendPacket(GamePacket* packet) {
    gameClient->SendPacket(*packet);
}

void Client::Update() {
    gameClient->UpdateClient();
}

// Helper method to parse an IP address string into uint8_t components
std::vector<uint8_t> Client::ParseIPAddress(const std::string& ipAddress) {
    std::vector<uint8_t> bytes;
    std::istringstream stream(ipAddress);
    std::string segment;

    while (std::getline(stream, segment, '.')) {
        int byte = std::stoi(segment);
        if (byte < 0 || byte > 255) {
            throw std::invalid_argument("Invalid IP address byte: " + segment);
        }
        bytes.push_back(static_cast<uint8_t>(byte));
    }

    return bytes;
}
