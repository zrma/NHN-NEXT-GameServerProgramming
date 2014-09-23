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
	
	void RequestCrypt( MyPacket::CryptRequest cryptRequest );
	void ResponseCrypt();

	void RequestLogin( MyPacket::LoginRequest loginRequest );
	void ResponseLogin();

	void RequestUpdatePosition( MyPacket::MoveRequest moveRequest );
	void ResponseUpdatePosition();

	void RequestChat( MyPacket::ChatRequest chatRequest );
	void ResponseChat();



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