#pragma once
#include "TutorialGame.h"
#include "NetworkBase.h"
#include "Player.h"

#include <vector>


namespace NCL {
	namespace CSC8503 {
		class GameServer;
		class GameClient;
		class NetworkPlayer;
		class Player;
		class NetworkObject;
		class TestPacketReceiver;

		class NetworkedGame : public TutorialGame {
		public:
			NetworkedGame();
			NetworkedGame(bool isServer);
			~NetworkedGame();

			void StartAsServer();
			void StartAsClient(char a, char b, char c, char d);

			void UpdateGame(float dt) override;

			GameObject* SpawnPlayer(Vector3 playerSpawnPosition);

			void StartLevel();

			void OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b);

			void OnPlayerConnected(int playerID);

			void TestBehaviourTree();

			void AddMazeToWorld();

			void TestPathfinding();
			void DisplayPathfinding();

			void SetupEnemyPath();

			std::map<int, NetworkPlayer*> playerPeerMap;

			TestPacketReceiver* clientReceiver;
			TestPacketReceiver* serverReceiver;

			std::unordered_map<int, NetworkObject*> networkObjects;

		protected:
			void UpdateAsServer(float dt);
			void UpdateAsClient(float dt);

			void BroadcastSnapshot(bool deltaFrame);
			void UpdateMinimumState();

			std::map<int, int> stateIDs;

			float timeToNextPacket;
			int packetsToSnapshot;

			

			std::map<int, GameObject*> serverPlayers;
			GameObject* localPlayer;

			std::map<int, Player*> players;
			

			vector<Vector3> testNodes;

			NavigationGrid* gridForMaze;
			TutorialGame* game = new TutorialGame();

			StateGameObject* gooseEnemy;

			GameClient* client = nullptr;
			GameServer* server = nullptr;

			NetworkObject* networkObject = nullptr;

			int score = 0;
		};

		class TestPacketReceiver : public PacketReceiver {
		public:
			TestPacketReceiver(std::string name) {
				this->name = name;
			}

			void ReceivePacket(int type, GamePacket* payload, int source);

			NetworkedGame* game;
		protected:
			std::string name;
			int connectedClientID;
			int connectedServerID;
		};
	}
}