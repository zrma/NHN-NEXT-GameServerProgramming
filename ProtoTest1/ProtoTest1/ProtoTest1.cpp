// ProtoTest1.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

#include "mypacket.pb.h"
#include "google\protobuf\io\coded_stream.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

#define MAX_BUFFER_SIZE 2048

struct MessageHeader
{
	google::protobuf::uint32 size;
	MyPacket::MessageType type;
};


void WriteMessageToStream(
	MyPacket::MessageType msgType,
	const google::protobuf::MessageLite& message,
	google::protobuf::io::CodedOutputStream& stream )

{
	MessageHeader messageHeader;
	messageHeader.size = message.ByteSize();
	messageHeader.type = msgType;
	stream.WriteRaw( &messageHeader, sizeof( MessageHeader ) );
	message.SerializeToCodedStream( &stream );
}

const int MessageHeaderSize = sizeof( MessageHeader );




int _tmain(int argc, _TCHAR* argv[])
{
	google::protobuf::uint8 m_SessionBuffer[MAX_BUFFER_SIZE];
	google::protobuf::io::ArrayOutputStream* m_pArrayOutputStream = new google::protobuf::io::ArrayOutputStream( m_SessionBuffer, MAX_BUFFER_SIZE );
	google::protobuf::io::CodedOutputStream* m_pCodedOutputStream = new google::protobuf::io::CodedOutputStream( m_pArrayOutputStream );

	MyPacket::LoginResult loginResult;

	loginResult.set_playerid( 1234 );


	char* name = "ProfessorNTR";
	loginResult.set_playername( name );

	MyPacket::Position position;
	position.set_x( 1.0f );
	position.set_y( 2.0f );
	position.set_z( 3.0f );


	loginResult.mutable_playerpos()->set_x( 1.0f );
	loginResult.mutable_playerpos()->set_y( 2.0f );
	loginResult.mutable_playerpos()->set_z( 3.0f );

	WriteMessageToStream( MyPacket::MessageType::PKT_SC_LOGIN, loginResult, *m_pCodedOutputStream );



	//////////////////////////////////////////////////////////////////////////

	MessageHeader messageHeader;
	memcpy( &messageHeader, m_SessionBuffer, MessageHeaderSize );
	


	google::protobuf::io::ArrayInputStream payloadArrayStream( m_SessionBuffer, messageHeader.size + MessageHeaderSize );
	payloadArrayStream.Skip( MessageHeaderSize );

	switch ( messageHeader.type )
	{
		case MyPacket::MessageType::PKT_CS_LOGIN:
			printf_s( "PKT_CS_LOGIN \n" );
			break;
		case MyPacket::MessageType::PKT_CS_CHAT:
			printf_s( "PKT_CS_CHAT \n" );
			break;
		case MyPacket::MessageType::PKT_CS_MOVE:
			printf_s( "PKT_CS_MOVE \n" );
			break;


		// test
		case MyPacket::MessageType::PKT_SC_LOGIN:
		{
			printf_s( "PKT_SC_LOGIN \n" );


			MyPacket::LoginResult message;
			if ( false == message.ParseFromZeroCopyStream( &payloadArrayStream ) )
			{
				break;
			}

			int id = message.playerid();
			std::string name = message.playername();

			float x = message.playerpos().x();
			float y = message.playerpos().y();
			float z = message.playerpos().z();

			printf_s( "ID:%d Name:%s [%f][%f][%f] \n", id, name.c_str(), x, y, z );
			break;
		}



			
		case MyPacket::MessageType::PKT_SC_CHAT:
			printf_s( "PKT_SC_CHAT \n" );
			break;
		case MyPacket::MessageType::PKT_SC_MOVE:
			printf_s( "PKT_SC_MOVE \n" );
			break;
		default:
			printf_s( "default \n" );
			break;
	}


	getchar();
	return 0;
}

