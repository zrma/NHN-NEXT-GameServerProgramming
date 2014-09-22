#pragma once
#include "ContentsConfig.h"

class ClientSession;

class Player
{
public:
	Player(ClientSession* session);
	~Player();

	bool IsLoaded() { return mPlayerId > 0; }
	
	void UpdateID( int id );
	
	void SetPosition(float x, float y, float z);
	void UpdateChat( std::string reqStirng );



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