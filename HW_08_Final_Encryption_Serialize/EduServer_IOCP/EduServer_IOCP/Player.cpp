#include "stdafx.h"
#include "ClientSession.h"
#include "Player.h"

float RandomFloat( float a, float b );

Player::Player(ClientSession* session) : mSession(session)
{
	PlayerReset();
}

Player::~Player()
{
}

void Player::PlayerReset()
{
	FastSpinlockGuard criticalSection(mPlayerLock);

	mPlayerId = -1;
	mPosX = mPosY = mPosZ = 0;
}

void Player::RequestLogin( int pid )
{
	if ( mPlayerId != -1 )
	{
		//중복 로그인
		CRASH_ASSERT( false );

		printf_s( "this session already logged in \n" );
	}

	mPlayerId = pid;
}

void Player::ResponseLogin( MyPacket::LoginResult& result )
{
	int nameTag = 0;

}









//세 번째에 map 크기 인자를 넣어서 크기를 넘지 않도록 해야되지 않을까...
float RandomFloat( float a, float b )
{
	float random = ( (float)rand() ) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}