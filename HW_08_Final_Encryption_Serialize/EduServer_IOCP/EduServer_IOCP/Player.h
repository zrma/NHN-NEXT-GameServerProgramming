#pragma once
#include "ContentsConfig.h"


class ClientSession;

class Player 
{
public:
	Player(ClientSession* session);
	~Player();

	bool IsLoaded() { return mPlayerId > 0; }
	int GetPlayerID() { return mPlayerId; }
	
	void RequestLogin( int pid );
	void ResponseLogin( int pid, float x, float y, float z, const char* name);

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