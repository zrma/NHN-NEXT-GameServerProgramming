#include "stdafx.h"
#include "Log.h"
#include "ClientSession.h"
#include "Player.h"
#include "ProtoHeader.h"

#define PKT_NONE	0
#define PKT_MAX		1024

#define PKT_SC_CRYPT	MyPacket::MessageType::PKT_SC_CRYPT
#define PKT_SC_LOGIN	MyPacket::MessageType::PKT_SC_LOGIN
#define PKT_SC_CHAT		MyPacket::MessageType::PKT_SC_CHAT
#define PKT_SC_MOVE		MyPacket::MessageType::PKT_SC_MOVE


typedef void(*HandlerFunc)(ClientSession* session, int size);

static HandlerFunc HandlerTable[PKT_MAX];

static void DefaultHandler(ClientSession* session, int size)
{
	LoggerUtil::EventLog("invalid packet handler", session->mPlayer->GetPlayerId());
	session->DisconnectRequest(DR_ACTIVE);
}

struct InitializeHandlers
{
	InitializeHandlers()
	{
		for (int i = 0; i < PKT_MAX; ++i)
			HandlerTable[i] = DefaultHandler;
	}
} _init_handlers_;

struct RegisterHandler
{
	RegisterHandler(int pktType, HandlerFunc handler)
	{
		HandlerTable[pktType] = handler;
	}
};

#define REGISTER_HANDLER(PKT_TYPE)	\
	static void Handler_##PKT_TYPE(ClientSession* session, int size); \
	static RegisterHandler _register_##PKT_TYPE(PKT_TYPE, Handler_##PKT_TYPE); \
	static void Handler_##PKT_TYPE(ClientSession* session, int size)


void ClientSession::OnRead(size_t len)
{
	/// 패킷 파싱하고 처리
	while (true)
	{
		/// 패킷 헤더 크기 만큼 읽어와보기
		MessageHeader header;
		
		if (false == mRecvBuffer.Peek((char*)&header, MessageHeaderSize))
			return;
		
		/// 패킷 완성이 되는가? 

		// 구 교수님 코드와는 다르다!
		// Easy Server에서는 헤더에 담긴 사이즈 = 패킷 전체 사이즈 였지만
		// 여기서는 프로토버프의 몸통 사이즈(페이로드)만 담겨 있음
		if ( mRecvBuffer.GetStoredSize() < header.size - MessageHeaderSize )
			return;
		

		if (header.type >= PKT_MAX || header.type <= PKT_NONE)
		{
			// 서버에서 보낸 패킷이 이상하다?!
			LoggerUtil::EventLog("packet type error", mPlayer->GetPlayerId());
			
			DisconnectRequest(DR_ACTIVE);
			return;
		}

		/// packet dispatch...
		HandlerTable[header.type](this, header.size);
	}
}

REGISTER_HANDLER( PKT_SC_CRYPT )
{
	//////////////////////////////////////////////////////////////////////////
	// 서버로부터 키를 받았다!
	if ( session->IsEncrypt() )
	{
		// 이미 키 교환을 해서 암호화 하는 도중이므로 이상하다!
		return;
	}

	char* packetTemp = new char[MessageHeaderSize + size];

	if ( false == session->ParsePacket( packetTemp, MessageHeaderSize + size ) )
	{
		LoggerUtil::EventLog( "packet parsing error", PKT_SC_CRYPT );
		return;
	}

	// 디시리얼라이즈
	google::protobuf::io::ArrayInputStream is( packetTemp, MessageHeaderSize + size );
	is.Skip( MessageHeaderSize );

	MyPacket::CryptResult inPacket;
	inPacket.ParseFromZeroCopyStream( &is );

	delete packetTemp;

	MyPacket::SendingKeySet keySet = inPacket.sendkey();
	session->SetReceiveKeySet( keySet );
}

REGISTER_HANDLER( PKT_SC_LOGIN )
{
	if ( !session->IsEncrypt() )
	{
		LoggerUtil::EventLog( "uncrypted session error", PKT_SC_LOGIN );
		return;
	}

	char* packetTemp = new char[MessageHeaderSize + size];

	if ( false == session->ParsePacket( packetTemp, MessageHeaderSize + size ) )
	{
		LoggerUtil::EventLog( "packet parsing error", PKT_SC_LOGIN );
		return;
	}

	// 디시리얼라이즈
	google::protobuf::io::ArrayInputStream is( packetTemp, MessageHeaderSize + size );
	is.Skip( MessageHeaderSize );

	MyPacket::LoginResult inPacket;
	inPacket.ParseFromZeroCopyStream( &is );

	delete packetTemp;

	MyPacket::Position pos = inPacket.playerpos();
	session->mPlayer->ResponseLogin( inPacket.playerid(), pos.x(), pos.y(), pos.z(),
									 inPacket.playername().c_str(), true );
}

REGISTER_HANDLER( PKT_SC_MOVE )
{
	//////////////////////////////////////////////////////////////////////////
	// 서버로부터 이동 허가가 떨어진 것이다.
	if ( !session->IsEncrypt() )
	{
		LoggerUtil::EventLog( "uncrypted session error", PKT_SC_MOVE );
		return;
	}

	char* packetTemp = new char[MessageHeaderSize + size];

	if ( false == session->ParsePacket( packetTemp, MessageHeaderSize + size ) )
	{
		LoggerUtil::EventLog( "packet parsing error", PKT_SC_MOVE );
		return;
	}

	// 디크립트


	// 디시리얼라이즈
	google::protobuf::io::ArrayInputStream is( packetTemp, MessageHeaderSize + size );
	is.Skip( MessageHeaderSize );

	MyPacket::MoveResult inPacket;
	inPacket.ParseFromZeroCopyStream( &is );

	delete packetTemp;

	MyPacket::Position pos = inPacket.playerpos();
	session->mPlayer->ResponseUpdatePosition( pos.x(), pos.y(), pos.z() );
}

REGISTER_HANDLER( PKT_SC_CHAT )
{
	//////////////////////////////////////////////////////////////////////////
	// 누군가 내게 채팅을 했다
	char* packetTemp = new char[MessageHeaderSize + size];
		
	if ( false == session->ParsePacket( packetTemp, MessageHeaderSize + size ) )
	{
		LoggerUtil::EventLog( "packet parsing error", PKT_SC_CHAT );
		return;
	}
	
	// 디크립트


	// 디시리얼라이즈
	google::protobuf::io::ArrayInputStream is( packetTemp, MessageHeaderSize + size );
	is.Skip( MessageHeaderSize );

	MyPacket::ChatResult inPacket;
	inPacket.ParseFromZeroCopyStream( &is );

	delete packetTemp;

	session->mPlayer->ResponseChat( inPacket.playername().c_str() , inPacket.playermessage().c_str() );
}

/////////////////////////////////////////////////////////
// REGISTER_HANDLER(PKT_SC_LOGIN)
// {
// 	LoginRequest inPacket;
// 	if (false == session->ParsePacket(inPacket))
// 	{
// 		LoggerUtil::EventLog("packet parsing error", inPacket.mType);
// 		return;
// 	}
// 	
// 	/// 테스트를 위해 10ms후에 로딩하도록 ㄱㄱ
// 	DoSyncAfter(10, session->mPlayer, &Player::RequestLoad, inPacket.mPlayerId);
// 
// }

// REGISTER_HANDLER(PKT_CS_MOVE)
// {
// 	MoveRequest inPacket;
// 	if (false == session->ParsePacket(inPacket))
// 	{
// 		LoggerUtil::EventLog("packet parsing error", inPacket.mType);
// 		return;
// 	}
// 
// 	if (inPacket.mPlayerId != session->mPlayer->GetPlayerId())
// 	{
// 		LoggerUtil::EventLog("PKT_CS_MOVE: invalid player ID", session->mPlayer->GetPlayerId());
// 		return;
// 	}
// 
// 	/// 지금은 성능 테스트를 위해 DB에 업데이트하고 통보하도록 하자.
// 	session->mPlayer->DoSync(&Player::RequestUpdatePosition, inPacket.mPosX, inPacket.mPosY, inPacket.mPosZ);
// }

// REGISTER_HANDLER(PKT_CS_CHAT)
// {
// 	ChatBroadcastRequest inPacket;
// 	if (false == session->ParsePacket(inPacket))
// 	{
// 		LoggerUtil::EventLog("packet parsing error", inPacket.mType);
// 		return;
// 	}
// 
// 	if (inPacket.mPlayerId != session->mPlayer->GetPlayerId())
// 	{
// 		LoggerUtil::EventLog("PKT_CS_CHAT: invalid player ID", session->mPlayer->GetPlayerId());
// 		return;
// 	}
// 
// 	/// chatting의 경우 여기서 바로 방송
// 	ChatBroadcastResult outPacket;
// 		
// 	outPacket.mPlayerId = inPacket.mPlayerId;
// 	wcscpy_s(outPacket.mChat, inPacket.mChat);
// 	GBroadcastManager->BroadcastPacket(&outPacket);
// 	
// }

