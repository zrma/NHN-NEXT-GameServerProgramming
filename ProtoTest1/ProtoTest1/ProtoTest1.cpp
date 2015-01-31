// ProtoTest1.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

#include "mypacket.pb.h"
#include "google\protobuf\io\coded_stream.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>
#include "Crypter.h"

#define MAX_BUFFER_SIZE 4096

struct MessageHeader
{
	google::protobuf::uint32 mSize;
	MyPacket::MessageType mType;
};

const int MessageHeaderSize = sizeof( MessageHeader );

void WriteMessageToStream(
	MyPacket::MessageType msgType,
	const google::protobuf::MessageLite& message,
	google::protobuf::io::CodedOutputStream& stream )
{
	MessageHeader messageHeader;
	messageHeader.mSize = message.ByteSize();
	messageHeader.mType = msgType;
	
	stream.WriteRaw( &messageHeader, sizeof( MessageHeader ) );
	message.SerializeToCodedStream( &stream );
}

int _tmain(int argc, _TCHAR* argv[])
{
	//////////////////////////////////////////////////////////////////////////
	// 밥 = 세션1
	// 엘리스 = 세션2
	Crypter alice;
	Crypter bob;
	
	alice.GenerateExchangeKey();
	bob.GenerateExchangeKey();

	std::vector<char>& bobSendThisKey = bob.GetExchangeKey();

	MyPacket::CryptRequest cryptRequest;
	cryptRequest.mutable_sendkey()->set_datalen( bobSendThisKey.size() );
	
	char* key = new char[bobSendThisKey.size()];
	char* keyCurPtr = key;

	for each( char c in bobSendThisKey )
	{
		*( keyCurPtr++ ) = c;
	}

	cryptRequest.mutable_sendkey()->set_keyblob( key );
	delete key;

	google::protobuf::uint8 buferA[MAX_BUFFER_SIZE];
	google::protobuf::io::ArrayOutputStream* aos = new google::protobuf::io::ArrayOutputStream( buferA, MAX_BUFFER_SIZE );
	google::protobuf::io::CodedOutputStream* cos = new google::protobuf::io::CodedOutputStream( aos );

	WriteMessageToStream( MyPacket::MessageType::PKT_SC_CRYPT, cryptRequest, *cos );
	
	google::protobuf::io::ArrayInputStream ais( buferA, sizeof( buferA ) );
	google::protobuf::io::CodedInputStream cis( &ais );

	MessageHeader header;
	cis.ReadRaw( &header, MessageHeaderSize );

	if ( header.mType != MyPacket::MessageType::PKT_SC_CRYPT )
	{
		printf_s( "Packet Type Error! \n" );
	}

	const void* payloadPos = nullptr;
	int payloadSize = 0;

	cis.GetDirectBufferPointer( &payloadPos, &payloadSize );

	// payload 읽기
	google::protobuf::io::ArrayInputStream payloadArrayStream( payloadPos, header.mSize );
	google::protobuf::io::CodedInputStream payloadInputStream( &payloadArrayStream );

	MyPacket::CryptRequest message;
	if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
	{
		printf_s( "Packet Parse Error! \n" );
	}

	std::vector<char> aliceReciveThisKey;

	for ( int i = 0; i < message.sendkey().datalen(); ++i )
	{
		aliceReciveThisKey.push_back( message.sendkey().keyblob().c_str()[i] );
	}

	alice.CreateSharedKey( aliceReciveThisKey );
	bob.CreateSharedKey( alice.GetExchangeKey() );
	
	printf_s( "----------------------------\n" );
	bob.PrintExchangeKey();
	printf_s( "----------------------------\n" );
	bob.PrintBuffer( aliceReciveThisKey );

	printf_s( "----------------------------\n\n" );
	alice.PrintSharedKey();
	printf_s( "----------------------------\n" );
	bob.PrintSharedKey();

	printf_s( "----------------------------\n" );

	if ( alice.GetSharedKey() == bob.GetSharedKey() )
		printf_s( "Save \n" );
	else
		printf_s( "Out! \n" );
	
	getchar();

	//////////////////////////////////////////////////////////////////////////

	google::protobuf::uint8 buferB[MAX_BUFFER_SIZE];
	google::protobuf::io::ArrayOutputStream* aos2 = new google::protobuf::io::ArrayOutputStream( buferB, MAX_BUFFER_SIZE );
	google::protobuf::io::CodedOutputStream* cos2 = new google::protobuf::io::CodedOutputStream( aos2 );

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

	WriteMessageToStream( MyPacket::MessageType::PKT_SC_LOGIN, loginResult, *cos2 );
	
	//////////////////////////////////////////////////////////////////////////
	alice.Encrypt( (char*)buferB, sizeof( buferB ) );
	bob.Encrypt( (char*)buferB, sizeof( buferB ) );
	//////////////////////////////////////////////////////////////////////////

	google::protobuf::io::ArrayInputStream ais2( buferB, sizeof( buferB ) );
	google::protobuf::io::CodedInputStream cis2( &ais2 );

	MessageHeader messageHeader;
	cis2.ReadRaw( &messageHeader, MessageHeaderSize );

	switch ( messageHeader.mType )
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
			
			const void* payloadPos = nullptr;
			int payloadSize = 0;

			cis2.GetDirectBufferPointer( &payloadPos, &payloadSize );

			// payload 읽기
			google::protobuf::io::ArrayInputStream payloadArrayStream( payloadPos, messageHeader.mSize );
			google::protobuf::io::CodedInputStream payloadInputStream( &payloadArrayStream );

			MyPacket::LoginResult message;
			if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
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

