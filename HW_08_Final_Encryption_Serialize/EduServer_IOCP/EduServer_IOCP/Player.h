#pragma once
#include "ContentsConfig.h"
#include "mypacket.pb.h"


class ClientSession;

class Player 
{
public:
	Player(ClientSession* session);
	~Player();

	bool IsLoaded() { return mPlayerId > 0; }
	int GetPlayerID() { return mPlayerId; }
	
	void RequestLogin( int pid );
	void ResponseLogin( MyPacket::LoginResult& result);

	void RequestUpdatePosition( float x, float y, float z );
	void ResponseUpdatePosition( float x, float y, float z );

	void RequestChat( const char* comment );
	void ResponseChat( const char* name, const char* comment );



private:

	void PlayerReset();
	

private:

	FastSpinlock mPlayerLock;

	int		mPlayerId;
	float	mPosX;
	float	mPosY;
	float	mPosZ;
	std::string	mPlayerName;
	std::string	mChat;

	ClientSession* const mSession;
	friend class ClientSession;
};