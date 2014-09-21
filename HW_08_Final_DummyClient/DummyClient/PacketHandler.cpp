#include "stdafx.h"
#include "Log.h"
#include "ClientSession.h"
#include "Player.h"

#define PKT_NONE	0
#define PKT_MAX		1024

typedef void(*HandlerFunc)(ClientSession* session);

static HandlerFunc HandlerTable[PKT_MAX];

static void DefaultHandler(ClientSession* session)
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
	static void Handler_##PKT_TYPE(ClientSession* session); \
	static RegisterHandler _register_##PKT_TYPE(PKT_TYPE, Handler_##PKT_TYPE); \
	static void Handler_##PKT_TYPE(ClientSession* session)

//@}


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
		if (mRecvBuffer.GetStoredSize() < header.size)
			return;


		if (header.type >= PKT_MAX || header.type <= PKT_NONE)
		{
			// LoggerUtil::EventLog("packet type error", mPlayer->GetPlayerId());

			// 서버에서 보낸 패킷이 이상하다?!

			DisconnectRequest(DR_ACTIVE);
			return;
		}

		/// packet dispatch...
		HandlerTable[header.type](this);
	}
}

/////////////////////////////////////////////////////////
// REGISTER_HANDLER(PKT_CS_LOGIN)
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

