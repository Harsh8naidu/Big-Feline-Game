#pragma once
#include "NetworkBase.h"
#include <iostream>
#include <string>

namespace NCL {
	namespace CSC8503 {
		class TestPacketReceiver : public PacketReceiver {
		public:
			TestPacketReceiver(std::string name) {
				this->name = name;
			}

			void ReceivePacket(int type, GamePacket* payload, int source) {
				if (type == String_Message) {
					StringPacket* realPacket = (StringPacket*)payload;

					std::string message = realPacket->GetStringFromData();

					std::cout << name << " received message: " << message << std::endl;
				}
			}
		protected:
			std::string name;
		};
	}
}