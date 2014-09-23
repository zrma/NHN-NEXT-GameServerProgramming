#include "stdafx.h"
#include "Player.h"
#include "ClientSession.h"

Player::Player( ClientSession* session ): mSession( session )
{
	PlayerReset();
}

Player::~Player()
{
}

void Player::PlayerReset()
{
	memset( mPlayerName, 0, sizeof( mPlayerName ) );
	mPlayerId = -1;
	mIsValid = false;
	mPosX = mPosY = mPosZ = 0;
	mHP = 0;
}

void Player::RequestLogin( int pid )
{
	MyPacket::LoginRequest loginRequest;
	loginRequest.set_playerid( pid );

	mSession->SendRequest( MyPacket::PKT_CS_LOGIN, loginRequest );
}

void Player::ResponseLogin( int pid, float x, float y, float z, const char* name, bool valid )
{
	mPlayerId = pid;
	mPosX = x, mPosY = y, mPosZ = z;
	strcpy_s( mPlayerName, name );
	mIsValid = valid;

	// 로그인 했으면 이동 시작
	RequestUpdatePosition();
}

void Player::RequestUpdatePosition()
{
	MyPacket::MoveRequest moveRequest;
	moveRequest.set_playerid( mPlayerId );
	moveRequest.mutable_playerpos()->set_x( mPosX + ( rand() % 10 - 5 ) / 10 );
	moveRequest.mutable_playerpos()->set_y( mPosY + ( rand() % 10 - 5 ) / 10 );
	moveRequest.mutable_playerpos()->set_z( mPosZ + ( rand() % 10 - 5 ) / 10 );

	mSession->SendRequest( MyPacket::PKT_CS_MOVE, moveRequest );
}

void Player::ResponseUpdatePosition( float x, float y, float z )
{
	mPosX = x, mPosY = y, mPosZ = z;

	//////////////////////////////////////////////////////////////////////////
	// 이동이 끝났으면 채팅을 시작하지

	char chatMessage[80];
	sprintf_s( chatMessage, "Test Chatting Message from %s (%d) ", mPlayerName, mPlayerId );

	RequestChat( chatMessage );
}

void Player::RequestChat( const char* comment )
{
	MyPacket::ChatRequest chatRequest;
	chatRequest.set_playerid( mPlayerId );
	chatRequest.set_playermessage( comment );

	mSession->SendRequest( MyPacket::PKT_CS_CHAT, chatRequest );
}

void Player::ResponseChat( const char* name, const char* comment )
{
	//////////////////////////////////////////////////////////////////////////
	// 채팅을 받았으면 카운트 해서 100회 초과 되면 접속 종료
	// 아니면 이동
	if ( mHP > 100 )
	{
		mSession->DisconnectRequest( DR_ACTIVE );
	}
	else
	{
		RequestUpdatePosition();
	}
}
