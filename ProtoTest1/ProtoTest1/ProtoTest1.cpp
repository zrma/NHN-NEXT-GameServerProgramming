// ProtoTest1.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

#include "mypacket.pb.h"
#include "google\protobuf\io\coded_stream.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>
#include "KeyChanger.h"

#define MAX_BUFFER_SIZE 2048

KeyChanger GKeyChanger;
KeyPrivateSets GBobPrivateKeySets;
KeyPrivateSets GAlicePrivateKeySets;

struct MessageHeader
{
	google::protobuf::uint32 size;
	MyPacket::MessageType type;
};

const int MessageHeaderSize = sizeof( MessageHeader );

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

int _tmain(int argc, _TCHAR* argv[])
{
	//////////////////////////////////////////////////////////////////////////
	// 밥 = 세션1
	// 엘리스 = 세션2
	KeySendingSets bobSendingKeySets;
	KeySendingSets aliceSendingKeySets;

	GKeyChanger.GenerateKey( &GBobPrivateKeySets, &bobSendingKeySets );
	GKeyChanger.GenerateKey( &GAlicePrivateKeySets, &aliceSendingKeySets );
	
	//////////////////////////////////////////////////////////////////////////
	// 삽질 테스트
	MyPacket::CryptRequest cryptRequest;
	cryptRequest.mutable_sendkey()->set_datalen( bobSendingKeySets.dwDataLen );
	
	char* key = new char[bobSendingKeySets.dwDataLen];
	memcpy( key, bobSendingKeySets.pbKeyBlob, bobSendingKeySets.dwDataLen );

	// 널문자 때문에 제대로 안 들어가므로 +1씩 더해준다. 뜯을 때 -1 해주자
	for ( size_t i = 0; i < bobSendingKeySets.dwDataLen; ++i )
		key[i]++;
	
	cryptRequest.mutable_sendkey()->set_keyblob( key );
	delete key;

	KeySendingSets copyKeySets;
	copyKeySets.pbKeyBlob = new BYTE[cryptRequest.sendkey().datalen()];

	// 키 복사 중
	copyKeySets.dwDataLen = cryptRequest.sendkey().datalen();
	memcpy( copyKeySets.pbKeyBlob, cryptRequest.sendkey().keyblob().data(),
			copyKeySets.dwDataLen );

	// 널문자 때문에 +1 더해준 것 -1
	for ( size_t i = 0; i < copyKeySets.dwDataLen; ++i )
		copyKeySets.pbKeyBlob[i]--;
		
	GKeyChanger.GetSessionKey( &GBobPrivateKeySets, &aliceSendingKeySets );
	GKeyChanger.GetSessionKey( &GAlicePrivateKeySets, &copyKeySets );
		
	//////////////////////////////////////////////////////////////////////////

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
	
	GKeyChanger.EncryptData( GBobPrivateKeySets.hSessionKey, m_SessionBuffer, sizeof( m_SessionBuffer ), m_SessionBuffer );
	GKeyChanger.DecryptData( GAlicePrivateKeySets.hSessionKey, m_SessionBuffer, sizeof( m_SessionBuffer ) );

	
	//////////////////////////////////////////////////////////////////////////

	GKeyChanger.EncryptData( GAlicePrivateKeySets.hSessionKey, m_SessionBuffer, sizeof( m_SessionBuffer ), m_SessionBuffer );
	GKeyChanger.DecryptData( GBobPrivateKeySets.hSessionKey, m_SessionBuffer, sizeof( m_SessionBuffer ) );

	google::protobuf::io::ArrayInputStream arrayInputStream( m_SessionBuffer, sizeof( m_SessionBuffer ) );
	google::protobuf::io::CodedInputStream codedInputStream( &arrayInputStream );

	MessageHeader messageHeader;
	codedInputStream.ReadRaw( &messageHeader, MessageHeaderSize );

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
			
			const void* payloadPos = nullptr;
			int payloadSize = 0;

			codedInputStream.GetDirectBufferPointer( &payloadPos, &payloadSize );

			// payload 읽기
			google::protobuf::io::ArrayInputStream payloadArrayStream( payloadPos, messageHeader.size );
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

