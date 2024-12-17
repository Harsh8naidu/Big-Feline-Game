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

#include "NetworkPlayer.h"
#include <NavigationMesh.h>

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
	NetworkBase::Initialise();
	timeToNextPacket  = 0.0f;
	packetsToSnapshot = 0;
	std::cout << "NetworkedGame Created!" << std::endl;
	StartLevel();
	TestPathfinding();

	// Needs to be part of NetworkedGame class
	clientReceiver = new TestPacketReceiver("Client");
	clientReceiver->game = this;
	serverReceiver = new TestPacketReceiver("Server");
	serverReceiver->game = this;

	int port = NetworkBase::GetDefaultPort();

	client = nullptr;
	server = nullptr;
	if (isServer) {
		server = new GameServer(port, 1);

		//this->StartAsServer();
		//server = thisServer; // initialize the server
		server->RegisterPacketHandler(String_Message, serverReceiver);
		server->RegisterPacketHandler(Received_State, serverReceiver);
		server->RegisterPacketHandler(Client_Connected, serverReceiver);
		server->RegisterPacketHandler(Client_Disconnected, serverReceiver);
		server->RegisterPacketHandler(Player_Update, serverReceiver);
	}
	else {
		client = new GameClient();
		//this->StartAsClient(127, 0, 0, 1);
		//client = thisClient; // initialize the client
		client->RegisterPacketHandler(String_Message, clientReceiver);
		client->RegisterPacketHandler(Delta_State, clientReceiver);
		client->RegisterPacketHandler(Full_State, clientReceiver);
		client->RegisterPacketHandler(Server_Connected, clientReceiver);
		client->RegisterPacketHandler(Server_Disconnected, clientReceiver);
		client->RegisterPacketHandler(Player_Update, clientReceiver);

		client->Connect(127, 0, 0, 1, port);
	}

	StartLevel();
	
}

NetworkedGame::~NetworkedGame()	{
	delete server;
	delete client;
}

void TestPacketReceiver::ReceivePacket(int type, GamePacket* payload, int source) {
	if (type < 0) {
		std::cerr << "Invalid packet type received: " << type << std::endl;
		return;
	}

	std::cout << "Received packet of type: " << type << " from source: " << source << " Size: " << payload->size << std::endl;
	if (type == String_Message) {
		StringPacket* realPacket = (StringPacket*)payload;

		std::string message = realPacket->GetStringFromData();

		std::cout << name << " received message: " << message << std::endl;
	}

	if (type == Client_Connected) {
		std::cout << name << " received client connected message" << std::endl;

		game->OnPlayerConnected(source);

	}

	if (type == Client_Disconnected) {
		std::cout << name << " received client disconnected message" << std::endl;

	}

	if (type == Server_Connected) {
		std::cout << name << " received server connected message" << std::endl;

		ServerConnectedPacket* serverPacket = (ServerConnectedPacket*)payload;
		int serverID = serverPacket->serverID; // Assuming server packet contains an ID or other data

		std::cout << "Connected to server with ID: " << serverID << std::endl;
	}

	if (type == Server_Disconnected) {
		std::cout << name << " received server disconnected message" << std::endl;
	}

	if (type == Player_Update) {
		PlayerUpdatePacket* updatePacket = (PlayerUpdatePacket*)payload;

		Vector3 newPosition = updatePacket->position;
		Quaternion newOrientation = updatePacket->orientation;

		std::cout << name << " received Player_Update for Player ID: " << source << std::endl;

		// Locate the player object in the game world
		auto playerIter = game->playerPeerMap.find(source);
		if (playerIter != game->playerPeerMap.end()) {
			NetworkPlayer* player = playerIter->second;

			// Update the player's position and orientation
			player->GetTransform().SetPosition(newPosition);
			player->GetTransform().SetOrientation(newOrientation);

			/*std::cout << "Updated Player ID " << source
				<< " to Position: " << newPosition.x << ", " << newPosition.y << ", " << newPosition.z
				<< " and Orientation: " << newOrientation.x << newOrientation.y << newOrientation.z << std::endl;*/
		}
		else {
			std::cerr << "Player ID " << source << " not found in playerPeerMap!" << std::endl;
		}
	}

	if (type == Full_State) {
		FullPacket* fullPacket = (FullPacket*)payload;

		int objectID = fullPacket->objectID; // Extract the ID of the networked object
		NetworkState fullState = fullPacket->fullState; // Extract the state data

		std::cout << name << " received Full_State for Object ID: " << objectID << std::endl;

		// Locate the network object in the game
		auto objectIter = game->networkObjects.find(objectID);
		if (objectIter != game->networkObjects.end()) {
			NetworkObject* networkObject = objectIter->second;

			std::cout << "Updated Object ID " << objectID
				<< " to Position: " << fullState.position.x << ", "
				<< fullState.position.y << ", " << fullState.position.z
				<< " and Orientation: " << fullState.orientation.x << ", "
				<< fullState.orientation.y << ", " << fullState.orientation.z
				<< ", " << fullState.orientation.w << std::endl;

			// Apply the full state to the network object
			networkObject->ReadFullPacket(*fullPacket);
		}
		else {
			std::cerr << "Object ID " << objectID << " not found in networkObjects!" << std::endl;
		}
	}

	if (type == Delta_State) {
		DeltaPacket* deltaPacket = (DeltaPacket*)payload;

		int objectID = deltaPacket->objectID; // Extract the ID of the networked object

		std::cout << name << " received Delta_State for Object ID: " << objectID << std::endl;

		// Locate the network object in the game
		auto objectIter = game->networkObjects.find(objectID);
		if (objectIter != game->networkObjects.end()) {
			NetworkObject* networkObject = objectIter->second;

			// Apply the delta state to the network object
			networkObject->ReadDeltaPacket(*deltaPacket);

			std::cout << "Updated Object ID " << objectID
				<< " to Position: " << networkObject->GetLatestNetworkState().position.x << ", "
				<< networkObject->GetLatestNetworkState().position.y << ", "
				<< networkObject->GetLatestNetworkState().position.z
				<< " and Orientation: " << networkObject->GetLatestNetworkState().orientation.x << ", "
				<< networkObject->GetLatestNetworkState().orientation.y << ", "
				<< networkObject->GetLatestNetworkState().orientation.z << ", "
				<< networkObject->GetLatestNetworkState().orientation.w << std::endl;
		}
		else {
			std::cerr << "Object ID " << objectID << " not found in networkObjects!" << std::endl;
		}
	}
}

void NetworkedGame::UpdateGame(float dt) {
	timeToNextPacket -= dt;
	if (timeToNextPacket < 0) {
		if (server) {
			UpdateAsServer(dt);
		}
		else if (client) {
			UpdateAsClient(dt);
		}
		timeToNextPacket += 1.0f / 20.0f; //20hz server/client update
	}

	DisplayPathfinding();

	if (client) {
		Debug::Print("This is Client Player", Vector2(2, 15), Debug::MAGENTA);
		client->UpdateClient();
	}
	if (server) {
		Debug::Print("This is Server Player", Vector2(2, 15), Debug::MAGENTA);
		server->UpdateServer();
	}
	
	if (angryGoose) {
		angryGoose->Update(dt);
	}

	if (kitten1 != nullptr && kitten2 != nullptr && kitten3 != nullptr) {
		kitten1->Update(dt);
		kitten2->Update(dt);
		kitten3->Update(dt);
	}

	if (!player) {
		SpawnPlayer(Vector3(0, 2, -30), "player");
	}

	TutorialGame::UpdateGame(dt);
}

void NetworkedGame::UpdateAsServer(float dt) {
	packetsToSnapshot--;
	for (auto& [playerID, player] : playerPeerMap) {
		if (player->controllerByServer) {
			NetworkObject* networkObj = player->GetNetworkObject();
			if (networkObj) {
				GamePacket* newPacket = nullptr;
				if (networkObj->WritePacket(&newPacket, packetsToSnapshot < 0, 0)) {
					server->SendPacket(*newPacket, playerID);
					delete newPacket;
				}
			}
		}
	}
	if (packetsToSnapshot < 0) {
		packetsToSnapshot = 5; // Reset the snapshot counter

		// Broadcast the snapshot to all clients
		// Send snapshot data to each player
		BroadcastSnapshot(true);
	}
}

void NetworkedGame::UpdateAsClient(float dt) {
	if (!client) {
		std::cout << "Client is not initialized!" << std::endl;
		return;
	}

	if (!player2->controllerByServer) {
		// Update player inputs and send them to the server
		ClientPacket newPacket;
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
			newPacket.buttonstates[0] = 1;
		}
		client->SendPacket(newPacket);
	}
	client->UpdateClient();
}

void NetworkedGame::BroadcastSnapshot(bool deltaFrame) {
	for (auto& [playerID, player] : playerPeerMap) {
		std::vector<GameObject*>::const_iterator first, last;
		world->GetObjectIterators(first, last);

		for (auto i = first; i != last; ++i) {
			NetworkObject* o = (*i)->GetNetworkObject();
			if (o) {
				GamePacket* newPacket = nullptr;
				if (o->WritePacket(&newPacket, deltaFrame, 0)) {
					server->SendPacket(*newPacket, playerID);
					delete newPacket;
				}
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
	//game->GenerateMaze(grid);

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

void NetworkedGame::SetupEnemyPath() {
	NavigationGrid grid("TestGrid1.txt");

	// Generate maze and find path
	game->GenerateMaze(grid);

	NavigationPath outPath;
	Vector3 startPos(80, 0, 10); // Starting position
	Vector3 endPos(80, 0, 80);   // End position

	bool found = grid.FindPath(startPos, endPos, outPath);

	std::vector<Vector3> path;
	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		path.push_back(pos);
	}

	if (path.empty()) {
		std::cout << "No path found for enemy!" << std::endl;
		return;
	}

	// Add the enemy to the world
	angryGoose = AddAngryGooseToWorld(startPos, path, player, world);
}

void NetworkedGame::NavMeshPathFinding() {
	NavigationMesh navMesh("test.navmesh");

	std::cout << "Visualizing Navigation Mesh:" << std::endl;
	for (const auto& tri : navMesh.GetTriangles()) { // Assuming GetTriangles() is implemented
		// Retrieve vertices of the triangle
		Vector3 v0 = navMesh.GetVertex(tri.indices[0]);
		Vector3 v1 = navMesh.GetVertex(tri.indices[1]);
		Vector3 v2 = navMesh.GetVertex(tri.indices[2]);

		//std::cout << "Triangle: " << v0 << ", " << v1 << ", " << v2 << std::endl;

		// Draw edges of the triangle
		Debug::DrawLine(v0, v1, Vector4(0, 0, 1, 1)); // Blue for edges
		Debug::DrawLine(v1, v2, Vector4(0, 0, 1, 1));
		Debug::DrawLine(v2, v0, Vector4(0, 0, 1, 1));

		// Optionally, draw the centroid
		Debug::DrawLine(tri.centroid, Vector4(1, 0, 0, 1)); // Red for centroid
	}

	NavigationPath outPath;

	/* I could not find a startPos and endPos for the navmesh. I tried
	logging the data vertices to the console and finding a start pos
	but the program wouldn't load at all, so I couldn't demonstrate it.
	Please provide me some feedback regarding this while grading, on how
	I could have chosen startpos and endPos
	*/

	// This is the commented out startPos and endPos I tried to use which made it crash
	//Vector3 startPos(64.0, 15.7834, 46.5556);   // Centroid of a triangle in the positive region
	//Vector3 endPos(-21.7222, 21.0334, -45.1111); // Centroid of a triangle in the negative region

	// This is tutorial's startPos and endPos for the navigation grid
	Vector3 startPos(80, 0, 10);
	Vector3 endPos(80, 0, 80);

	bool found = navMesh.FindPath(startPos, endPos, outPath);

	std::vector<Vector3> path;
	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		path.push_back(pos);
	}

	if (path.empty()) {
		std::cout << "No path found for NavMesh!" << std::endl;
		return;
	}

	std::cout << "NavMesh Path Found:" << std::endl;
	for (const auto& waypoint : path) {
		std::cout << "Waypoint: " << waypoint << std::endl;
	}

	// Display the path
	for (int i = 1; i < navMeshNodes.size(); ++i) {
		Vector3 a = navMeshNodes[i - 1];
		Vector3 b = navMeshNodes[i];
		std::cout << "Drawing line from " << a << " to " << b << std::endl;
		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}
}


void NetworkedGame::AddMazeToWorld() {
	Vector3 cubeSize = Vector3(5, 5, 5);
	const auto& wallPositions = game->GetWallPositions();

	for (const Vector3& pos : wallPositions) {
		Debug::DrawLine(pos, pos + Vector3(0, 10, 0), Vector4(1, 0, 0, 1));
		AddCubeToWorld(pos, cubeSize, 0);
	}
}

void NetworkedGame::AddDoorPuzzle()
{	
	for (int z = -60; z <= -12; z += 6) {
		AddCubeToWorld(Vector3(-130, 2, z), Vector3(3, 3, 3), 0);
		AddCubeToWorld(Vector3(-90, 2, z), Vector3(3, 3, 3), 0);
		AddCubeToWorld(Vector3(-130, 8, z), Vector3(3, 3, 3), 0);
		AddCubeToWorld(Vector3(-90, 8, z), Vector3(3, 3, 3), 0);
	}

	for (int x = -130; x <= -90; x += 6) {
		AddCubeToWorld(Vector3(x, 2, -60), Vector3(3, 3, 3), 0);
		AddCubeToWorld(Vector3(x, 8, -60), Vector3(3, 3, 3), 0);
	}

	// Add ceiling at y = 14
	for (int z = -63; z <= -15; z += 6) {
		for (int x = -130; x <= -90; x += 6) {
			AddCubeToWorld(Vector3(x + 2, 14, z), Vector3(3, 3, 3), 0);
		}
	}
}

void NetworkedGame::AddCubesAroundSphere() {
	Vector3 spherePosition(-60, 10, -80);
	float sphereRadius = 5.0f;
	float cubeSize = 3.0f;  // Half-size of the cube (cube dimensions = 4x4x4)

	// Offsets to arrange cubes in a square around the sphere
	float offset = sphereRadius + cubeSize - 1;

	// Top and bottom layer of cubes around the sphere
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			if (x == 0 && z == 0) continue; // Skip the center position
			AddOBBCubeToWorld(spherePosition + Vector3(x * offset, 10, z * offset * 2), Vector3(cubeSize, cubeSize, cubeSize), 0);
		}
	}

	// Middle layer of cubes around the sphere
	for (int x = -1; x <= 1; ++x) {
		for (int z = -1; z <= 1; ++z) {
			if (x == 0 && z == 0) continue; // Skip the center position
			AddOBBCubeToWorld(spherePosition + Vector3(x * offset * 1.5, 0, z * offset * 2), Vector3(cubeSize, cubeSize, cubeSize), 0);
		}
	}
}

void NetworkedGame::AddCubesAroundHome() {
	// Adjusted z range to start at -150 and align accordingly
	for (int z = -150; z <= -102; z += 6) {
		AddCubeToWorld(Vector3(0, 2, z), Vector3(3, 3, 3), 0);
		AddCubeToWorld(Vector3(40, 2, z), Vector3(3, 3, 3), 0);
		AddCubeToWorld(Vector3(0, 8, z), Vector3(3, 3, 3), 0);
		AddCubeToWorld(Vector3(40, 8, z), Vector3(3, 3, 3), 0);
	}

	// Adjusted x range to start at 0 and align accordingly
	for (int x = 0; x <= 40; x += 6) {
		AddCubeToWorld(Vector3(x, 2, -150), Vector3(3, 3, 3), 0);
		AddCubeToWorld(Vector3(x, 8, -150), Vector3(3, 3, 3), 0);
	}

	// Adjust ceiling placement with new x and z ranges
	for (int z = -153; z <= -105; z += 6) {
		for (int x = 0; x <= 40; x += 6) {
			AddCubeToWorld(Vector3(x + 2, 14, z), Vector3(3, 3, 3), 0);
		}
	}

	// Add the front wall and leave a gap for the door
	AddCubeToWorld(Vector3(6.5, 2, -102), Vector3(3, 3, 3), 0);
	AddCubeToWorld(Vector3(13, 2, -102), Vector3(3, 3, 3), 0);
	AddCubeToWorld(Vector3(33, 2, -102), Vector3(3, 3, 3), 0);
	AddCubeToWorld(Vector3(26.5, 2, -102), Vector3(3, 3, 3), 0);
}

void NetworkedGame::SpawnBonus() {
	// Add bonus to the world
	bonus1 = AddBonusToWorld(Vector3(-30, 2, 0));
	bonus2 = AddBonusToWorld(Vector3(-50, 60.5, 55));
	bonus3 = AddBonusToWorld(Vector3(-130, 61, 60));
	bonus4 = AddBonusToWorld(Vector3(10, 3, 75));
	bonus5 = AddBonusToWorld(Vector3(-110, 2, -40));
	bonus6 = AddBonusToWorld(Vector3(-60, 2, -90));
	bonus7 = AddBonusToWorld(Vector3(-140, 95, 55));
}

void NetworkedGame::PlaneVsAABB() {
	AddCubeToWorld(Vector3(-30, 2, 20), Vector3(10, 0.4, 10), 0);
	AddCubeToWorld(Vector3(-30, 2, 20), Vector3(5, 1, 5), 0);
	AddPlaneToWorld(Vector3(-30, 10, 20), Vector3(2, 2, 2));
	AddCubeToWorld(Vector3(-30, 20, 20), Vector3(5, 5, 5), 2);
}

void NetworkedGame::CapsuleVsAABB() {
	capsule = AddCapsuleToWorld(Vector3(-30, 100, 60), 5, 5, 20);
	AddCubeToWorld(Vector3(-30, 2, 60), Vector3(6, 6, 6), 0);
	AddCubeToWorld(Vector3(-30, 2, 72.5), Vector3(6, 6, 6), 0);
	AddCubeToWorld(Vector3(-30, 2, 85), Vector3(6, 6, 6), 0);
}

void NetworkedGame::PenaltyMethodOnSphere() {
	AddCubesAroundSphere();
	AddSphereToWorld(Vector3(-60, 10, -80), 5, 1, true);
}

void NetworkedGame::OBBvsOBB() {
	AddPlaneToWorld(Vector3(-30, 10, -20), Vector3(2, 2, 2));
	AddPlaneToWorld(Vector3(-30, 13, -20), Vector3(2, 2, 2));
	AddPlaneToWorld(Vector3(-30, 16, -20), Vector3(2, 2, 2));
	AddPlaneToWorld(Vector3(-30, 18, -20), Vector3(2, 2, 2));
	AddOBBCubeToWorld(Vector3(-30, 60, -20), Vector3(5, 5, 5), 1);
	AddOBBCubeToWorld(Vector3(-30, 20, -20), Vector3(5, 5, 5), 1);
}

void NetworkedGame::CapsuleVsCapsule() {
	AddCapsuleToWorld(Vector3(60, 60, -60), 3, 4, 1);
	AddCapsuleToWorld(Vector3(60, 30, -60), 3, 4, 1);
	AddCubeToWorld(Vector3(60, 5, -60), Vector3(5, 5, 5), 1);
	
}

NetworkPlayer* NetworkedGame::SpawnPlayer(Vector3 playerSpawnPosition, std::string playerName) {

	return AddPlayerToWorld(playerSpawnPosition, playerName);
}

void NetworkedGame::StartLevel() {
	world->ClearAndErase();
	physics->Clear();
	networkObjects.clear();

	AddFloorToWorld(Vector3(0, -2, 0));
	AddFlyingStairs();

	AddDoorPuzzle();
	SpawnBonus();
	AddMazeToWorld();

	player = SpawnPlayer(Vector3(0, 2, -30), "player");
	player2 = SpawnPlayer(Vector3(0, 2, -60), "player2");

	home = AddKittenHome();
	AddCubesAroundHome();

	kitten1 = AddKittenToWorld(Vector3(20, 2, -30), player, world, home);
	kitten2 = AddKittenToWorld(Vector3(-110, 2, -30), player, world, home);
	kitten3 = AddKittenToWorld(Vector3(10, 2, 30), player, world, home);

	// local player
	NetworkObject* networkObj = new NetworkObject(*player, 1);
	player->SetNetworkObject(networkObj);
	networkObjects.insert(std::make_pair(1, networkObj));

	NetworkObject* networkObj2 = new NetworkObject(*player2, 2);
	player2->SetNetworkObject(networkObj2);
	networkObjects.insert(std::make_pair(2, networkObj2));

	PositionBridgeConstraint();
	OrientedBridgeConstraint();

	AddDoorPuzzle();

	plane = AddBlackObstacleToWorld(Vector3(-110, 50, 30), Vector3(1,1,1), 20);
	plane2 = AddBlackObstacleToWorld(Vector3(-10, 50, 0), Vector3(1, 1, 1), 20);

	PlaneVsAABB();
	CapsuleVsAABB();
	OBBvsOBB();
	CapsuleVsCapsule();

	PenaltyMethodOnSphere();

	// Add the nav mesh pathfinding
	NavMeshPathFinding();

	if (server) {
		player->controllerByServer = true; // Server controller by player
	}

	if (client) {
		player2->controllerByServer = false; // Client controller by player2
	}

	SetupEnemyPath();
	door = AddDoorToWorld(Vector3(-110, 2, -10));
}


void NetworkedGame::OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b) {
	if (server) { //detected a collision between players!
		MessagePacket newPacket;
		newPacket.messageID = COLLISION_MSG;
		newPacket.playerID  = a->GetPlayerNum();

		server->SendGlobalPacket(newPacket);

		newPacket.playerID = b->GetPlayerNum();
		server->SendGlobalPacket(newPacket);
	}
}

void NetworkedGame::OnPlayerConnected(int playerID) {
	std::cout << "This is player ID: " << playerID << " connected!" << std::endl;
	newPlayer = new NetworkPlayer();

	if (playerPeerMap.empty()) {
		// First connection is player
		newPlayer = player;
		player->controllerByServer = true;
	}
	else {
		// Second connection is player2
		newPlayer = player2;
		player2->controllerByServer = false;
	}

	world->AddGameObject(newPlayer);
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
