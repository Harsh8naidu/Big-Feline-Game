//#pragma once
//#include "NetworkBase.h"
//#include <iostream>
//#include <string>
//
//namespace NCL {
//	namespace CSC8503 {
//		class NetworkedGame;
//		class TestPacketReceiver : public PacketReceiver {
//		public:
//			TestPacketReceiver(std::string name) {
//				this->name = name;
//			}
//
//			void ReceivePacket(int type, GamePacket* payload, int source) {
//				if (type == String_Message) {
//					StringPacket* realPacket = (StringPacket*)payload;
//
//					std::string message = realPacket->GetStringFromData();
//
//					std::cout << name << " received message: " << message << std::endl;
//				}
//
//				if (type == Client_Connected) {
//					std::cout << name << " received client connected message" << std::endl;
//					
//					ClientConnectedPacket* clientPacket = (ClientConnectedPacket*)payload;
//					connectedClientID = clientPacket->clientID;
//					source = connectedClientID;
//					
//				}
//
//				if (type == Client_Disconnected) {
//					std::cout << name << " received client disconnected message" << std::endl;
//
//					ClientDisconnectedPacket* clientPacket = (ClientDisconnectedPacket*)payload;
//					int disconnectedPlayerID = clientPacket->playerID;
//					source = disconnectedPlayerID;
//				}
//
//				if (type == Server_Connected) {
//					std::cout << name << " received server connected message" << std::endl;
//
//					ServerConnectedPacket* serverPacket = (ServerConnectedPacket*)payload;
//					int serverID = serverPacket->serverID; // Assuming server packet contains an ID or other data
//
//					std::cout << "Connected to server with ID: " << serverID << std::endl;
//				}
//
//				if (type == Server_Disconnected) {
//					// CleanupServerConnection() // not implemeneted yet
//				}
//
//				if (type == Player_Update) {
//					PlayerUpdatePacket* updatePacket = (PlayerUpdatePacket*)payload;
//
//					int playerID = updatePacket->playerID;
//					Vector3 newPosition = updatePacket->position;
//					Quaternion newOrientation = updatePacket->orientation;
//
//					std::cout << name << " received Player_Update for Player ID: " << playerID << std::endl;
//
//					// Locate the player object in the game world
//					auto playerIter = game->playerPeerMap.find(playerID);	
//					if (playerIter != game->playerPeerMap.end()) {
//						NetworkPlayer* player = playerIter->second;
//
//						// Update the player's position and orientation
//						player->GetTransform().SetPosition(newPosition);
//						player->GetTransform().SetOrientation(newOrientation);
//
//						std::cout << "Updated Player ID " << playerID
//							<< " to Position: " << newPosition.x << ", " << newPosition.y << ", " << newPosition.z
//							<< " and Orientation: " << newOrientation.x << newOrientation.y << newOrientation.z << std::endl;
//					}
//					else {
//						std::cerr << "Player ID " << playerID << " not found in playerPeerMap!" << std::endl;
//					}
//				}
//
//			}
//
//		protected:
//			std::string name;
//			int connectedClientID;
//			int connectedServerID;
//		};
//	}
//}