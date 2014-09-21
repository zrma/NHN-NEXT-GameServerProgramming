#pragma once

#define MAX_NAME_LEN	32
#define MAX_COMMENT_LEN	256

class ClientSession;

class Player
{
public:
	Player( ClientSession* session );
	~Player();

	bool IsValid() { return mPlayerId > 0; }
	int  GetPlayerId() { return mPlayerId; }

	void RequestLogin( int pid );
	void ResponseLogin( int pid, float x, float y, float z, wchar_t* name, bool valid );

	void RequestUpdatePosition( float x, float y, float z );
	void ResponseUpdatePosition( float x, float y, float z );

	void RequestChat( const char* comment );
	void ResponseChat( const char* name, const char* comment );
	
private:
	void PlayerReset();
	
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

