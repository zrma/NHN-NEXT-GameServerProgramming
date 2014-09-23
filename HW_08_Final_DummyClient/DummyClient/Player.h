#pragma once

#include "ProtoHeader.h"

#define MAX_NAME_LEN	32
#define MAX_COMMENT_LEN	256

class ClientSession;

class Player
{
public:
	Player( ClientSession* session );
	~Player();

	bool	IsValid() { return mPlayerId > 0; }
	int		GetPlayerId() { return mPlayerId; }

	void	RequestLogin( int pid );

	void	ResponseLogin( MyPacket::LoginResult& loginResult );
	void	ResponseUpdatePosition( MyPacket::MoveResult& moveResult );
	void	ResponseChat( MyPacket::ChatResult& chatResult );
	
private:
	void	PlayerReset();
	void	RequestUpdatePosition();
	void	RequestChat( const char* comment );

private:

	int		mPlayerId;
	float	mPosX;
	float	mPosY;
	float	mPosZ;
	bool	mIsValid;
	char	mPlayerName[MAX_NAME_LEN];

	int		mHP;

	ClientSession* const mSession;
	friend class ClientSession;
};

