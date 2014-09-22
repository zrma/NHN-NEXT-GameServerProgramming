#include "stdafx.h"
#include "Player.h"

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

}

void Player::ResponseLogin( int pid, float x, float y, float z, const char* name, bool valid )
{

}

void Player::RequestUpdatePosition( float x, float y, float z )
{

}

void Player::ResponseUpdatePosition( float x, float y, float z )
{
	//////////////////////////////////////////////////////////////////////////
	// 이동이 끝났으면 채팅을 시작하지
}

void Player::RequestChat( const char* comment )
{

}

void Player::ResponseChat( const char* name, const char* comment )
{
	//////////////////////////////////////////////////////////////////////////
	// 채팅을 받았으면 카운트 해서 100회 초과 되면 접속 종료
	// 아니면 이동
}
