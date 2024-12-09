#include "NetworkedGame.h"
#include "NetworkPlayer.h"
#include "NetworkObject.h"
#include "GameServer.h"
#include "GameClient.h"
#include "AckPacket.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"
#include "TestPacketReceiver.h"

#define COLLISION_MSG 30

struct MessagePacket : public GamePacket {
	short playerID;
	short messageID;

	MessagePacket() {
		type = Message;
		size = sizeof(short) * 2;
	}
};

NetworkedGame::NetworkedGame(bool isServer)	{
	thisServer = nullptr;
	thisClient = nullptr;

	NetworkBase::Initialise();
	timeToNextPacket  = 0.0f;
	packetsToSnapshot = 0;
	std::cout << "NetworkedGame Created!" << std::endl;
	StartLevel();
	TestPathfinding();

	TestPacketReceiver clientReceiver("Client");
	TestPacketReceiver serverReceiver("Server");

	int port = NetworkBase::GetDefaultPort();


	if (isServer) {
		server = new GameServer(port, 1);
		//this->StartAsServer();
		server->RegisterPacketHandler(String_Message, &serverReceiver);
		server->RegisterPacketHandler(Received_State, &serverReceiver);
	}
	else {
		client = new GameClient();
		//this->StartAsClient(127, 0, 0, 1);
		
		client->RegisterPacketHandler(String_Message, &clientReceiver);
		client->RegisterPacketHandler(Delta_State, &clientReceiver);
		client->RegisterPacketHandler(Full_State, &clientReceiver);
		client->RegisterPacketHandler(Player_Connected, &clientReceiver);
		client->RegisterPacketHandler(Player_Disconnected, &clientReceiver);

		client->Connect(127, 0, 0, 1, port);
	}

	StartLevel();
	
}

NetworkedGame::~NetworkedGame()	{
	delete thisServer;
	delete thisClient;
}

//void NetworkedGame::StartAsServer() {
//	thisServer = new GameServer(NetworkBase::GetDefaultPort(), 4);
//
//	thisServer->RegisterPacketHandler(Received_State, this);
//
//	StartLevel();
//}
//
//void NetworkedGame::StartAsClient(char a, char b, char c, char d) {
//	thisClient = new GameClient();
//	thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort());
//
//	thisClient->RegisterPacketHandler(Delta_State, this);
//	thisClient->RegisterPacketHandler(Full_State, this);
//	thisClient->RegisterPacketHandler(Player_Connected, this);
//	thisClient->RegisterPacketHandler(Player_Disconnected, this);
//
//	StartLevel();
//}

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

	DisplayPathfinding();

	if (client) {
		client->UpdateClient();
	}
	if (server) {
		server->UpdateServer();
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

void NetworkedGame::TestPathfinding() {
	NavigationGrid grid("TestGrid1.txt");

	// Generate the maze
	game->GenerateMaze(grid);

	// Add the maze to the game world
	

	NavigationPath outPath;

	Vector3 startPos(80, 0, 10);
	Vector3 endPos(80, 0, 80);

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes.push_back(pos);
	}
}

void NetworkedGame::DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];
		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}
}

void NetworkedGame::AddMazeToWorld() {
	std::cout << "Adding maze to world..." << std::endl;
	Vector3 cubeSize = Vector3(5, 5, 5);
	const auto& wallPositions = game->GetWallPositions();

	for (const Vector3& pos : wallPositions) {
		AddCubeToWorld(pos, cubeSize, 0);
	}
}

GameObject* NetworkedGame::SpawnPlayer() {

	return AddPlayerToWorld(Vector3(0, 2, -30));
}

void NetworkedGame::StartLevel() {
	world->ClearAndErase();
	physics->Clear();

	AddFloorToWorld(Vector3(0, -2, 0));

	//AddEnemyToWorld(Vector3(30, 2, -30));

	//testStateObject = AddStateObjectToWorld(Vector3(0, 10, -10));

	gooseEnemy = AddAngryGooseToWorld(Vector3(50, 2, -50));

	angryGooseAI = new AngryGoose(gooseEnemy);

	AddFlyingStairs();

	bonus1 = AddBonusToWorld(Vector3(-30, 2, 0));

	AddMazeToWorld();

	player = SpawnPlayer();

	//networkObject = new NetworkObject(playerToControl, 1, player1);

}

//void NetworkedGame::ReceivePacket(int type, GamePacket* payload, int source) {
//	if (type == Ack_State) {
//		AckPacket* ack = static_cast<AckPacket*>(payload); // Safer cast
//		auto it = players.find(source); // Explicit lookup
//		if (it != players.end()) {
//			Player* player = it->second;
//			player->AcknowledgePacket(ack->stateID); // Update acknowledgment in Player object
//		}
//	}
//}


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

void NetworkedGame::TestBehaviourTree() {
	float behaviourTimer;
	float distanceToTarget;
	BehaviourAction* findKey = new BehaviourAction("Find Key", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for key..." << std::endl;
			behaviourTimer = rand() % 100;
			return Ongoing;
		}
		else if (state == Ongoing) {
			behaviourTimer -= dt;
			if (behaviourTimer <= 0.0f) {
				std::cout << "Found a key!" << std::endl;
				return Success;
			}
		}
		return state; // will be ongoing until success
		});

	BehaviourAction* goToRoom = new BehaviourAction("Go To Room", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Going to the loot room..." << std::endl;
			state = Ongoing;
		}
		else if (state == Ongoing) {
			distanceToTarget -= dt;
			if (distanceToTarget <= 0.0f) {
				std::cout << "Reached room!" << std::endl;
				return Success;
			}
		}
		return state; // will be ongoing until success
		});

	BehaviourAction* openDoor = new BehaviourAction("Open Door", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Opening door..." << std::endl;
			return Success;
		}
		return state; // will be ongoing until success
		});

	BehaviourAction* lookForTreasure = new BehaviourAction("Look For Treasure", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for treasure..." << std::endl;
			return Ongoing;
		}
		else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "I found some treasure!" << std::endl;
				return Success;
			}
			std::cout << "No treasure here..." << std::endl;
			return Failure;
		}
		return state; // will be ongoing until success
		});

	BehaviourAction* lookForItems = new BehaviourAction("Look For Items", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for items..." << std::endl;
			return Ongoing;
		}
		else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "I found some items!" << std::endl;
				return Success;
			}
			std::cout << "No items here..." << std::endl;
			return Failure;
		}
		return state; // will be ongoing until success
		});

	BehaviourSequence* sequence = new BehaviourSequence("Room Sequence");
	sequence->AddChild(findKey);
	sequence->AddChild(goToRoom);
	sequence->AddChild(openDoor);

	BehaviourSelector* selection = new BehaviourSelector("Loot Selection");
	selection->AddChild(lookForTreasure);
	selection->AddChild(lookForItems);

	BehaviourSequence* rootSequence = new BehaviourSequence("Root Sequence");
	rootSequence->AddChild(sequence);
	rootSequence->AddChild(selection);

	for (int i = 0; i < 5; ++i) {
		rootSequence->Reset();
		behaviourTimer = 0.0f;
		distanceToTarget = rand() % 250;
		BehaviourState state = Ongoing;
		std::cout << "We're going on an adventure!" << std::endl;
		while (state == Ongoing) {
			state = rootSequence->Execute(1.0f); // fake dt
		}
		if (state == Success) {
			std::cout << "What a successful adventure!" << std::endl;
		}
		else if (state == Failure) {
			std::cout << "What a waste of time!" << std::endl;
		}
	}

	std::cout << "All done!" << std::endl;
}
