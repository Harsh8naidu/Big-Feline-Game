#pragma once
#include "TutorialGame.h"
#include "NetworkBase.h"
#include "Player.h"

#include <vector>
#include "AngryGooseAI.h"

namespace NCL {
	namespace CSC8503 {
		class GameServer;
		class GameClient;
		class NetworkPlayer;
		class Player;

		class NetworkedGame : public TutorialGame {
		public:
			NetworkedGame(bool isServer);
			~NetworkedGame();

			void StartAsServer();
			void StartAsClient(char a, char b, char c, char d);

			void UpdateGame(float dt) override;

			void SpawnPlayer();

			void StartLevel();

			void OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b);

			void OnPlayerConnected(int playerID);

			void TestBehaviourTree();

			void AddMazeToWorld();

			void TestPathfinding();
			void DisplayPathfinding();

		protected:
			void UpdateAsServer(float dt);
			void UpdateAsClient(float dt);

			void BroadcastSnapshot(bool deltaFrame);
			void UpdateMinimumState();

			std::map<int, int> stateIDs;

			GameServer* thisServer;
			GameClient* thisClient;
			float timeToNextPacket;
			int packetsToSnapshot;

			std::vector<NetworkObject*> networkObjects;

			std::map<int, GameObject*> serverPlayers;
			GameObject* localPlayer;

			std::map<int, Player*> players;
			std::map<int, NetworkPlayer*> playerPeerMap;

			vector<Vector3> testNodes;

			NavigationGrid* gridForMaze;
			TutorialGame* game = new TutorialGame();

			AngryGoose* angryGooseAI;
			StateGameObject* gooseEnemy;

			GameClient* client = nullptr;
			GameServer* server = nullptr;
		};
	}
}