#include "NetworkedGame.h"
#include "NetworkPlayer.h"
#include "NetworkObject.h"
#include "GameServer.h"
#include "GameClient.h"
#include "AckPacket.h"

#define COLLISION_MSG 30

struct MessagePacket : public GamePacket {
	short playerID;
	short messageID;

	MessagePacket() {
		type = Message;
		size = sizeof(short) * 2;
	}
};

NetworkedGame::NetworkedGame()	{
	thisServer = nullptr;
	thisClient = nullptr;

	NetworkBase::Initialise();
	timeToNextPacket  = 0.0f;
	packetsToSnapshot = 0;
	std::cout << "NetworkedGame Created!" << std::endl;
	StartLevel();
	AddPlayerToWorld(Vector3(0, 2, 0));
}

NetworkedGame::~NetworkedGame()	{
	delete thisServer;
	delete thisClient;
}

void NetworkedGame::StartAsServer() {
	thisServer = new GameServer(NetworkBase::GetDefaultPort(), 4);

	thisServer->RegisterPacketHandler(Received_State, this);

	StartLevel();
}

void NetworkedGame::StartAsClient(char a, char b, char c, char d) {
	thisClient = new GameClient();
	thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort());

	thisClient->RegisterPacketHandler(Delta_State, this);
	thisClient->RegisterPacketHandler(Full_State, this);
	thisClient->RegisterPacketHandler(Player_Connected, this);
	thisClient->RegisterPacketHandler(Player_Disconnected, this);

	StartLevel();
}

void NetworkedGame::UpdateGame(float dt) {
	timeToNextPacket -= dt;
	if (timeToNextPacket < 0) {
		if (thisServer) {
			UpdateAsServer(dt);
		}
		else if (thisClient) {
			UpdateAsClient(dt);
		}
		timeToNextPacket += 1.0f / 20.0f; //20hz server/client update
	}

	if (!thisServer && Window::GetKeyboard()->KeyPressed(KeyCodes::F9)) {
		StartAsServer();
	}
	if (!thisClient && Window::GetKeyboard()->KeyPressed(KeyCodes::F10)) {
		StartAsClient(127,0,0,1);
	}

	TutorialGame::UpdateGame(dt);
}

void NetworkedGame::UpdateAsServer(float dt) {
	packetsToSnapshot--;
	if (packetsToSnapshot < 0) {
		BroadcastSnapshot(false);
		packetsToSnapshot = 5;
	}
	else {
		BroadcastSnapshot(true);
	}
}

void NetworkedGame::UpdateAsClient(float dt) {
	ClientPacket newPacket;

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
		//fire button pressed!
		newPacket.buttonstates[0] = 1;
		newPacket.lastID = 0; //You'll need to work this out somehow...
	}
	thisClient->SendPacket(newPacket);
}

void NetworkedGame::BroadcastSnapshot(bool deltaFrame) {
	for (auto& [player, peer] : playerPeerMap) {
		std::vector<GameObject*>::const_iterator first;
		std::vector<GameObject*>::const_iterator last;

		world->GetObjectIterators(first, last);

		for (auto i = first; i != last; ++i) {
			NetworkObject* o = (*i)->GetNetworkObject();
			if (!o) {
				continue;
			}

			Player* player = o->GetPlayer();
			int playerState = 0;

			if (player) {
				playerState = player->GetLastAcknowledgedID();
			}

			GamePacket* newPacket = nullptr;
			if (o->WritePacket(&newPacket, deltaFrame, playerState)) {
				// Get the peerID from the NetworkPlayer
				int peerID = player->GetPlayerNum();

				std::cout << "Sending packet to player " << peerID << std::endl;
				// Send the packet to the specific player via ENet
				thisServer->SendPacket(*newPacket, peerID);
				delete newPacket;
			}
		}
	}
}

void NetworkedGame::UpdateMinimumState() {
	//Periodically remove old data from the server
	int minID = INT_MAX;
	int maxID = 0; //we could use this to see if a player is lagging behind?

	for (auto i : stateIDs) {
		minID = std::min(minID, i.second);
		maxID = std::max(maxID, i.second);
	}
	//every client has acknowledged reaching at least state minID
	//so we can get rid of any old states!
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	world->GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i) {
		NetworkObject* o = (*i)->GetNetworkObject();
		if (!o) {
			continue;
		}
		o->UpdateStateHistory(minID); //clear out old states so they arent taking up memory...
	}
}

void NetworkedGame::SpawnPlayer() {
	AddPlayerToWorld(Vector3(0, 2, 0));
}

void NetworkedGame::StartLevel() {
	world->ClearAndErase();
	physics->Clear();

	AddFloorToWorld(Vector3(0, 0, 0));
}

void NetworkedGame::ReceivePacket(int type, GamePacket* payload, int source) {
	if (type == Ack_State) {
		AckPacket* ack = static_cast<AckPacket*>(payload); // Safer cast
		auto it = players.find(source); // Explicit lookup
		if (it != players.end()) {
			Player* player = it->second;
			player->AcknowledgePacket(ack->stateID); // Update acknowledgment in Player object
		}
	}
}


void NetworkedGame::OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b) {
	if (thisServer) { //detected a collision between players!
		MessagePacket newPacket;
		newPacket.messageID = COLLISION_MSG;
		newPacket.playerID  = a->GetPlayerNum();

		thisClient->SendPacket(newPacket);

		newPacket.playerID = b->GetPlayerNum();
		thisClient->SendPacket(newPacket);
	}
}

void NetworkedGame::OnPlayerConnected(int playerID) {
	NetworkPlayer* newPlayer = new NetworkPlayer(this, playerID);

	//world->AddGameObject(newPlayer); // Add the new player to the game world

	// Add the new player to the player-peer map
	playerPeerMap[playerID] = newPlayer;

	std::cout << "Player " << playerID << " connected!" << std::endl;
}
