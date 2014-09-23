#include "stdafx.h"
#include "Log.h"
#include "ClientSession.h"
#include "Player.h"

#define PKT_NONE	0
#define PKT_MAX		1024

typedef void( *HandlerFunc )( ClientSession* session, MessageHeader& pktBase, google::protobuf::io::CodedInputStream& payloadStream );

static HandlerFunc HandlerTable[PKT_MAX];

static void DefaultHandler( ClientSession* session, MessageHeader& pktBase, google::protobuf::io::CodedInputStream& payloadStream )
{
	printf_s( "Default Handler...PKT ID: %d\n", pktBase.mType );
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
	static void Handler_##PKT_TYPE(ClientSession* session, MessageHeader& pktBase, google::protobuf::io::CodedInputStream& payloadStream ); \
	static RegisterHandler _register_##PKT_TYPE(PKT_TYPE, Handler_##PKT_TYPE); \
	static void Handler_##PKT_TYPE(ClientSession* session, MessageHeader& pktBase, google::protobuf::io::CodedInputStream& payloadStream )


void ClientSession::OnRead(size_t len)
{
	/// 패킷 파싱하고 처리
	google::protobuf::io::ArrayInputStream arrayInputStream( mRecvBuffer.GetBufferStart(), mRecvBuffer.GetContiguiousBytes() );
	google::protobuf::io::CodedInputStream codedInputStream( &arrayInputStream );

	MessageHeader packetheader;
	
	while ( codedInputStream.ReadRaw( &packetheader, MessageHeaderSize ) )
	{
		const void* payloadPos = nullptr;
		int payloadSize = 0;

		codedInputStream.GetDirectBufferPointer( &payloadPos, &payloadSize );
		
		if ( payloadSize < packetheader.mSize ) ///< 패킷 본체 사이즈 체크
			break;
		
		if ( packetheader.mType >= PKT_MAX || packetheader.mType <= PKT_NONE )
		{
			// 서버에서 보낸 패킷이 이상하다?!
			LoggerUtil::EventLog( "packet type error", mPlayer->GetPlayerId() );

			DisconnectRequest( DR_ACTIVE );
			break;;
		}
		
		/// payload 읽기
		google::protobuf::io::ArrayInputStream payloadArrayStream( payloadPos, packetheader.mSize );
		google::protobuf::io::CodedInputStream payloadInputStream( &payloadArrayStream );
		
		/// packet dispatch...
		HandlerTable[packetheader.mType]( this, packetheader, payloadInputStream );

		/// 읽은 만큼 전진 및 버퍼에서 제거
		codedInputStream.Skip( packetheader.mSize ); ///< ReadRaw에서 헤더 크기만큼 미리 전진했기 때문
		mRecvBuffer.Remove( MessageHeaderSize + packetheader.mSize );
	}
}

using namespace MyPacket;

REGISTER_HANDLER( PKT_SC_CRYPT )
{
	//////////////////////////////////////////////////////////////////////////
	// 서버로부터 키를 받았다!
	if ( session->IsEncrypt() )
	{
		// 이미 키 교환을 해서 암호화 하는 도중이므로 이상하다!
		return;
	}

	MyPacket::CryptResult inPacket;
	if ( false == inPacket.ParseFromCodedStream( &payloadStream ) )
	{
		session->DisconnectRequest( DR_ACTIVE );
		return;
	}

	MyPacket::SendingKeySet keySet = inPacket.sendkey();
	session->SetReceiveKeySet( keySet );

	printf_s( "서버의 키를 받음 \n" );

	// 대략 100~9100번의 아이디로 로그인을 시도 해 보자
	session->mPlayer->RequestLogin( rand() % 9000 + 100 );
}

REGISTER_HANDLER( PKT_SC_LOGIN )
{
	//////////////////////////////////////////////////////////////////////////
	// 서버로부터 정상적으로 로그인 허락을 받은 것이다.
	if ( !session->IsEncrypt() )
	{
		LoggerUtil::EventLog( "uncrypted session error", PKT_SC_LOGIN );
		return;
	}

	MyPacket::LoginResult inPacket;
	if ( false == inPacket.ParseFromCodedStream( &payloadStream ) )
	{
		session->DisconnectRequest( DR_ACTIVE );
		return;
	}

	session->mPlayer->ResponseLogin( inPacket );
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

	MyPacket::MoveResult inPacket;
	if ( false == inPacket.ParseFromCodedStream( &payloadStream ) )
	{
		session->DisconnectRequest( DR_ACTIVE );
		return;
	}

	session->mPlayer->ResponseUpdatePosition( inPacket );
}

REGISTER_HANDLER( PKT_SC_CHAT )
{
	//////////////////////////////////////////////////////////////////////////
	// 누군가 내게 채팅을 했다
	if ( !session->IsEncrypt() )
	{
		LoggerUtil::EventLog( "uncrypted session error", PKT_SC_MOVE );
		return;
	}

	MyPacket::ChatResult inPacket;
	if ( false == inPacket.ParseFromCodedStream( &payloadStream ) )
	{
		session->DisconnectRequest( DR_ACTIVE );
		return;
	}

	session->mPlayer->ResponseChat( inPacket );
}