#pragma once

namespace NCL::CSC8503 {
	class Player {
	public:
		Player(int playerNum) : playerNum(playerNum), lastAcknowledgedID(0) {}

		int GetPlayerNum() const {
			return playerNum;
		}

		void AcknowledgePacket(int stateID) {
			lastAcknowledgedID = stateID;
		}

		int GetLastAcknowledgedID() const {
			return lastAcknowledgedID;
		}

	private:
		int playerNum;
		int lastAcknowledgedID; // Tracks the last acknowledged state ID
	};
}
