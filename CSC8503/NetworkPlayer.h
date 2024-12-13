#pragma once
#include "GameObject.h"
#include "GameClient.h"

struct PlayerUpdatePacket : public GamePacket {
	int playerID;
	NCL::Maths::Vector3 position;
	Quaternion orientation;

	PlayerUpdatePacket() {
		type = Player_Update;
		size = sizeof(PlayerUpdatePacket);
	}
};

namespace NCL {
	namespace CSC8503 {
		class NetworkedGame;
		class NetworkObject;

		class NetworkPlayer : public GameObject {
		public:
			bool controllerByServer;

			NetworkPlayer(std::string playerName);
			~NetworkPlayer();

			NetworkPlayer() : controllerByServer(false) {}

			void OnCollisionBegin(GameObject* otherObject) override;

			int GetPlayerNum() const {
				return playerNum;
			}

			void SetNetworkObject(NetworkObject* obj) {
				networkObject = obj;
			}

		protected:
			NetworkedGame* game;
			int playerNum;
		};
	}
}

