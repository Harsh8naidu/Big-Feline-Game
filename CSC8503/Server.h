#pragma once  
#include "NetworkBase.h"  

namespace NCL {
	namespace CSC8503 {
		class GameServer;
		class TestPacketReceiver;
		class Server {
		public:
			Server(int port, int maxClients);
			~Server();

			void Start();
			void Update();
			void Stop();

		private:
			GameServer* gameServer;
			TestPacketReceiver* serverReceiver;
			int port;
		};
	}
}