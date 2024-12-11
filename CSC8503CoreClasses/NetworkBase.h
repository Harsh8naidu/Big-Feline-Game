#pragma once
//#include "./enet/enet.h"

#include <string>
#include <map>

struct _ENetHost;
struct _ENetPeer;
struct _ENetEvent;

enum BasicNetworkMessages {
	None,
	Hello,
	Message,
	String_Message,
	Delta_State,	//1 byte per channel since the last state
	Full_State,		//Full transform etc
	Received_State, //received from a client, informs that its received packet n
	Player_Connected,
	Player_Disconnected,
	Client_Connected,
	Client_Disconnected,
	Server_Connected,
	Server_Disconnected,
	Player_Update,
	Shutdown,

	Ack_State // Added by me for acknowledgment packets
};

struct GamePacket {
	short size;
	short type;

	GamePacket() {
		type		= BasicNetworkMessages::None;
		size		= 0;
	}

	GamePacket(short type) : GamePacket() {
		this->type	= type;
	}

	int GetTotalSize() {
		return sizeof(GamePacket) + size;
	}
};

struct StringPacket : public GamePacket {
	char stringData[256];

	StringPacket(const std::string& message) {
		type = BasicNetworkMessages::String_Message;
		size = (short)message.length();
		
		memcpy(stringData, message.data(), size);
	}

	std::string GetStringFromData() {
		std::string realString(stringData);
		realString.resize(size);
		return realString;
	}

	char data[1400 - sizeof(GamePacket)];
};

struct ClientConnectedPacket : public GamePacket {
	int clientID;

	ClientConnectedPacket(int id) {
		type = BasicNetworkMessages::Client_Connected;
		clientID = id;
		size = sizeof(int);
	}
};

struct ClientDisconnectedPacket : public GamePacket {
	int playerID;

	ClientDisconnectedPacket() {
		type = Client_Disconnected;
		size = sizeof(ClientDisconnectedPacket);
	}
};

struct ServerConnectedPacket : public GamePacket {
	int serverID;

	ServerConnectedPacket() {
		type = Server_Connected;
		size = sizeof(ServerConnectedPacket);
	}
};


class PacketReceiver {
public:
	virtual void ReceivePacket(int type, GamePacket* payload, int source = -1) = 0;
};

class NetworkBase	{
public:
	static void Initialise();
	static void Destroy();

	static int GetDefaultPort() {
		return 1234;
	}

	void RegisterPacketHandler(int msgID, PacketReceiver* receiver) {
		packetHandlers.insert(std::make_pair(msgID, receiver));
	}
protected:
	NetworkBase();
	~NetworkBase();

	bool ProcessPacket(GamePacket* p, int peerID = -1);

	typedef std::multimap<int, PacketReceiver*>::const_iterator PacketHandlerIterator;

	bool GetPacketHandlers(int msgID, PacketHandlerIterator& first, PacketHandlerIterator& last) const {
		auto range = packetHandlers.equal_range(msgID);

		if (range.first == packetHandlers.end()) {
			return false; //no handlers for this message type!
		}
		first	= range.first;
		last	= range.second;
		return true;
	}

	_ENetHost* netHandle;

	std::multimap<int, PacketReceiver*> packetHandlers;
};