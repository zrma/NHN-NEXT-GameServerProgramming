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

	void RequestChat( const wchar_t* comment );
	void ResponseChat( const wchar_t* comment );

	void RequestUpdateValidation( bool isValid );
	void ResponseUpdateValidation( bool isValid );

private:
	void PlayerReset();
	
private:

	int		mPlayerId;
	float	mPosX;
	float	mPosY;
	float	mPosZ;
	bool	mIsValid;
	wchar_t	mPlayerName[MAX_NAME_LEN];

	ClientSession* const mSession;
	friend class ClientSession;
};

