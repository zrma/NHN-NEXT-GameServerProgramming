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

void Player::RequestCrypt( MyPacket::CryptRequest cryptRequest )
{
	mSession->KeyInit();

	MyPacket::SendingKeySet keySet = cryptRequest.sendkey();
	mSession->SetReceiveKeySet( keySet );

	// printf_s( "Crypting \n" );

	ResponseCrypt();
}

void Player::ResponseCrypt()
{
	MyPacket::CryptResult cryptResult;

	cryptResult.mutable_sendkey()->set_datalen(mSession->GetKeyDataLen());
	
	uint64_t key[8] = { 0, };
	memcpy( key, mSession->GetKeyBlob(), sizeof( char ) * 64 );

	cryptResult.mutable_sendkey()->set_keyblob0( key[0] );
	cryptResult.mutable_sendkey()->set_keyblob1( key[1] );
	cryptResult.mutable_sendkey()->set_keyblob2( key[2] );
	cryptResult.mutable_sendkey()->set_keyblob3( key[3] );
	cryptResult.mutable_sendkey()->set_keyblob4( key[4] );
	cryptResult.mutable_sendkey()->set_keyblob5( key[5] );
	cryptResult.mutable_sendkey()->set_keyblob6( key[6] );
	cryptResult.mutable_sendkey()->set_keyblob7( key[7] );

	printf_s( "쏘는 키는 %d 라호! \n", *mSession->GetKeyBlob() );
	
	// printf_s( "Crypting_1 \n" );

	mSession->SendRequest( MyPacket::PKT_SC_CRYPT, cryptResult );
}


void Player::RequestLogin( MyPacket::LoginRequest loginRequest )
{
	if ( !mSession->IsEncrypt() )
	{
		printf_s( "uncrypted session error" );
		return;
	}

	if ( mPlayerId != -1 )
	{
		//이미 사용하고 있는 녀석에게 또 로그인이 시도
		CRASH_ASSERT( false );

		printf_s( "this session already used \n" );
	}

	mPlayerId = loginRequest.playerid();
	printf_s( "Login!!" );

	ResponseLogin();
}


void Player::ResponseLogin()
{
	int nameTag = 0;
	std::string name( "Player" + std::to_string( nameTag ) );
	mPlayerName = name;

	//맵 크기를 정해서 숫자 변경 필요
	//맵 내 랜덤하게 뿌려지도록 할 것
	mPosX += RandomFloat( -1.0f, 1.0f );
	mPosY += RandomFloat( -1.0f, 1.0f );
	mPosZ += RandomFloat( -1.0f, 1.0f );

	MyPacket::LoginResult loginResult;
	
	loginResult.set_playerid( mPlayerId );
	loginResult.set_playername( mPlayerName );
	loginResult.mutable_playerpos()->set_x( mPosX );
	loginResult.mutable_playerpos()->set_y( mPosY );
	loginResult.mutable_playerpos()->set_z( mPosZ );

	//암호화
	//mSession->CryptAction( (BYTE*)&loginResult, sizeof( loginResult ), (BYTE*)&loginResult );

	mSession->SendRequest( MyPacket::PKT_SC_LOGIN, loginResult );
	++nameTag;
}

void Player::RequestUpdatePosition( MyPacket::MoveRequest moveRequest )
{
	if ( !mSession->IsEncrypt() )
	{
		printf_s( "uncrypted session error" );
		return;
	}

	MyPacket::Position pos = moveRequest.playerpos();

	mPosX = pos.x();
	mPosY = pos.y();
	mPosZ = pos.z();

	ResponseUpdatePosition();
}

void Player::ResponseUpdatePosition()
{
	MyPacket::MoveResult moveResult;

	moveResult.set_playerid( mPlayerId );
	moveResult.mutable_playerpos()->set_x( mPosX );
	moveResult.mutable_playerpos()->set_y( mPosY );
	moveResult.mutable_playerpos()->set_z( mPosZ );

	//mSession->CryptAction( (BYTE*)&moveResult, sizeof( moveResult ), (BYTE*)&moveResult );

	mSession->SendRequest( MyPacket::PKT_SC_MOVE, moveResult );
}

void Player::RequestChat( MyPacket::ChatRequest chatRequest )
{
	if ( !mSession->IsEncrypt() )
	{
		printf_s( "uncrypted session error" );
		return;
	}

	mChat = chatRequest.playermessage();

	ResponseChat();
}

void Player::ResponseChat()
{
	MyPacket::ChatResult chatResult;

	chatResult.set_playername( mPlayerName );
	chatResult.set_playermessage( mChat );

	//같은 영역에 있는 플레이어를 순회하며 채팅을 날릴 수 있도록 해야 하는데...
	//아직 영역 관련 코드가 없네
	//그래서 테스트로 자신에게 리턴하는 것

	//mSession->CryptAction( (BYTE*)&chatResult, sizeof( chatResult ), (BYTE*)&chatResult );

	mSession->SendRequest( MyPacket::PKT_SC_CHAT, chatResult );
}






//세 번째에 map 크기 인자를 넣어서 크기를 넘지 않도록 해야되지 않을까...
float RandomFloat( float a, float b )
{
	float random = ( (float)rand() ) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}