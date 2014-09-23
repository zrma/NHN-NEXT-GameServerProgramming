#include "stdafx.h"
#include "Exception.h"
#include "Log.h"
#include "PacketHeader.h"
#include "ClientSession.h"
#include "Player.h"
#include "mypacket.pb.h"

typedef void( *HandlerFunc )( ClientSession* session, PacketHeader& pktBase, google::protobuf::io::CodedInputStream& payloadStream );

static HandlerFunc HandlerTable[MAX_PKT_TYPE];

static void DefaultHandler( ClientSession* session, PacketHeader& pktBase, google::protobuf::io::CodedInputStream& payloadStream )
{
	//일단 미정의된 행동을 방지하도록 함
	//CRASH_ASSERT( false );
	//session->DisconnectRequest( DR_NONE );
	printf_s( "Default Handler...PKT ID: %d\n", pktBase.mType );
}

//handlerTable 초기화
struct InitializeHandlers
{
	InitializeHandlers()
	{
		for ( int i = 0; i < MAX_PKT_TYPE; ++i )
		{
			HandlerTable[i] = DefaultHandler;
		}
	}

}_init_handlers_;


//해당 패킷에 대한 행동 정의 작업
struct RegisterHandler
{
	RegisterHandler( int pktType, HandlerFunc handler )
	{
		HandlerTable[pktType] = handler;
	}
};

#define REGISTER_HANDLER(PKT_TYPE)	\
	static void Handler_##PKT_TYPE(ClientSession* session, PacketHeader& pktBase, google::protobuf::io::CodedInputStream& payloadStream); \
	static RegisterHandler _register_##PKT_TYPE(PKT_TYPE, Handler_##PKT_TYPE); \
	static void Handler_##PKT_TYPE(ClientSession* session, PacketHeader& pktBase, google::protobuf::io::CodedInputStream& payloadStream)



void ClientSession::OnReceive( size_t len )
{
	TRACE_THIS;

	google::protobuf::io::ArrayInputStream arrayInputStream( mRecvBuffer.GetBufferStart(), mRecvBuffer.GetContiguiousBytes() );
	google::protobuf::io::CodedInputStream codedInputStream( &arrayInputStream );

	PacketHeader packetheader;

	while ( codedInputStream.ReadRaw(&packetheader, HEADER_SIZE) )
	{
		const void* payloadPos = nullptr;
		int payloadSize = 0;

		codedInputStream.GetDirectBufferPointer( &payloadPos, &payloadSize );

		if (payloadSize < packetheader.mSize)
		{
			break;
		}

		if (packetheader.mType >= MAX_PKT_TYPE || packetheader.mType <= 0)
		{
			DisconnectRequest( DR_ACTIVE );
			break;
		}

		google::protobuf::io::ArrayInputStream payloadArrayStream( payloadPos, packetheader.mSize );
		google::protobuf::io::CodedInputStream payloadInputstream( &payloadArrayStream );


		//받은 패킷에 대한 미리 정의된 행동 수행 부분
		HandlerTable[packetheader.mType]( this, packetheader, payloadInputstream );


		//처리한 만큼 스트림에서 진행
		//세션 버퍼에서는 처리한 내역을 제거함
		codedInputStream.Skip( packetheader.mSize );
		mRecvBuffer.Remove( HEADER_SIZE + packetheader.mSize );
	}
}



//////////////////////////////////////////////////////////////////////////
//실제 핸들러 등록 부분

using namespace MyPacket;

REGISTER_HANDLER( PKT_CS_CRYPT )
{
	//패킷 검증 단계
	CryptRequest cryptResquest;
	if (false == cryptResquest.ParseFromCodedStream(&payloadStream))
	{
		session->DisconnectRequest( DR_ACTIVE );
		return;
	}

	//실제 작업 진행
	
}

REGISTER_HANDLER( PKT_CS_LOGIN )
{
	LoginRequest loginRequest;
	if ( false == loginRequest.ParseFromCodedStream( &payloadStream ) )
	{
		session->DisconnectRequest( DR_ACTIVE );
		return;
	}


	session->mPlayer.RequestLogin( loginRequest );
}

REGISTER_HANDLER( PKT_CS_MOVE )
{
	MoveRequest moveRequest;
	if ( false == moveRequest.ParseFromCodedStream( &payloadStream ) )
	{
		session->DisconnectRequest( DR_ACTIVE );
		return;
	}

	session->mPlayer.RequestUpdatePosition( moveRequest );
}

REGISTER_HANDLER( PKT_CS_CHAT )
{
	ChatRequest chatRequest;
	if ( false == chatRequest.ParseFromCodedStream( &payloadStream ) )
	{
		session->DisconnectRequest( DR_ACTIVE );
		return;
	}

	session->mPlayer.RequestChat( chatRequest );
}