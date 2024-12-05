#pragma once
#include "NetworkBase.h"

namespace NCL::CSC8503 {
	struct AckPacket : public GamePacket {
		int stateID; // The acknowledged state ID

		AckPacket() {
			type = Ack_State; // Enum value representing acknowledgment packets
			size = sizeof(AckPacket) - sizeof(GamePacket);
		}
	};
}