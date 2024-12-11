#include "Server.h"
#include <iostream>  
#include <thread>  
#include <chrono>
#include "GameServer.h"
#include "NetworkedGame.h"

using namespace NCL;
using namespace CSC8503;

Server::Server(int port, int maxClients)
	: port(port), serverReceiver(new TestPacketReceiver("Server")) {
	gameServer = new GameServer(port, maxClients);
	gameServer->RegisterPacketHandler(String_Message, serverReceiver);
}

Server::~Server() {
	delete gameServer;
	NetworkBase::Destroy();
}

void Server::Start() {
	NetworkBase::Initialise();
	std::cout << "Server started on port " << port << std::endl;
}

void Server::Update() {
	for (int i = 0; i < 100; ++i) {
		StringPacket packet("Server message #" + std::to_string(i));
		gameServer->SendGlobalPacket(packet);

		gameServer->UpdateServer();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void Server::Stop() {
	std::cout << "Server shutting down..." << std::endl;
}