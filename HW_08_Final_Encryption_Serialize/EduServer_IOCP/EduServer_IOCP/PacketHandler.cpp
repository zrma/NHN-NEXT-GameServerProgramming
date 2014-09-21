#include "stdafx.h"
#include "PacketHandler.h"
#include "mypacket.pb.h"
#include "ClientSession.h"

PacketHandler::PacketHandler()
{
}


PacketHandler::~PacketHandler()
{
}

void PacketHandler::StreamSetInit()
{
	mArrayOutputStream = new google::protobuf::io::ArrayOutputStream( mSessionBuffer, MAX_BUFFER_SIZE );
	mCodedOutputStream = new google::protobuf::io::CodedOutputStream( mArrayOutputStream );
}

void PacketHandler::StreamSetClear()
{
	if ( mCodedOutputStream )
	{
		delete mCodedOutputStream;
		mCodedOutputStream = nullptr;
	}

	if ( mArrayOutputStream )
	{
		delete mArrayOutputStream;
		mArrayOutputStream = nullptr;
	}
}

void PacketHandler::WriteMessageToStream( MyPacket::MessageType msgType, const google::protobuf::MessageLite& message, google::protobuf::io::CodedOutputStream& stream )
{
	MessageHeader messageHeader;
	messageHeader.size = message.ByteSize();
	messageHeader.type = msgType;
	stream.WriteRaw( &messageHeader, sizeof( MessageHeader ) );
	message.SerializeToCodedStream( &stream );
}


void PacketHandler::InputPacketJunction( ClientSession* currSession )
{
	MessageHeader messageHeder;

	char* packetStart = currSession->GetRecvBuffer().GetBufferStart();
	memcpy( &messageHeder, packetStart, MessageHeaderSize );
	currSession->GetRecvBuffer().Remove( MessageHeaderSize );

	const void* payloadStart = currSession->GetRecvBuffer().GetBufferStart();
	int remainSize = 0;

	google::protobuf::io::ArrayInputStream payloadArrayStream( packetStart, messageHeder.size );

	switch ( messageHeder.type )
	{
		case MyPacket::MessageType::PKT_CS_LOGIN:
		{
			MyPacket::LoginRequest message;
			if ( false == message.ParseFromZeroCopyStream( &payloadArrayStream ) )
			{
				break;
			}
//			printf_s( "PKT_CS_LOGIN \n" );

			int id = message.playerid();
			printf_s( "Login Success! Player Id: %d \n", id );

			MyPacket::LoginResult loginResultMessage;
			loginResultMessage.set_playerid( id );

			//내부 콘텐츠 필요?
			//이름은?
			//포지션은?
			
			char playerName[] = "who is the Next";
			loginResultMessage.set_playername( playerName );

			loginResultMessage.mutable_playerpos()->set_x(0.0f);
			loginResultMessage.mutable_playerpos()->set_y( 0.0f );
			loginResultMessage.mutable_playerpos()->set_z( 0.0f );

			/*
			if (true)
			{
			}

			WriteMessageToStream(PKT_SC_LOGIN, loginResultMessage, )
			*/

			break;
		}
			

		case MyPacket::MessageType::PKT_CS_CHAT:
		{
			MyPacket::ChatRequest message;
			if ( false == message.ParseFromZeroCopyStream( &payloadArrayStream ) )
			{
				break;
			}

			printf_s( "PKT_CS_CHAT \n" );
			break;
		}
			

		case MyPacket::MessageType::PKT_CS_MOVE:
		{
			MyPacket::MoveRequest message;
			if ( false == message.ParseFromZeroCopyStream( &payloadArrayStream ) )
			{
				break;
			}

			printf_s( "PKT_CS_MOVE \n" );
			break;
		}
			

		default:
			break;
	}
	currSession->GetRecvBuffer().Remove( messageHeder.size );

}


