#pragma once
#include "PacketHeader.h"

class ClientSession;

class PacketHandler
{
public:
	PacketHandler();
	~PacketHandler();

public:
	void StreamSetInit();
	void StreamSetClear();



	void WriteMessageToStream(MyPacket::MessageType msgType, const google::protobuf::MessageLite& message, google::protobuf::io::CodedOutputStream& stream );
	void InputPacketJunction( ClientSession* currSession);

	
private:
	google::protobuf::uint8 mSessionBuffer[MAX_BUFFER_SIZE];
	google::protobuf::io::ArrayOutputStream* mArrayOutputStream;
	google::protobuf::io::CodedOutputStream* mCodedOutputStream;
	
};

