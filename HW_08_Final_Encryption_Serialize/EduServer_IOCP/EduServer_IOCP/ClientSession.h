#pragma once

#include "Session.h"
#include "Player.h"
#include "KeyChanger.h"
#include "mypacket.pb.h"

class ClientSessionManager;
class Player;

class ClientSession : public Session, public ObjectPool<ClientSession>
{
public:
	ClientSession();
	virtual ~ClientSession();

	void SessionReset();

	bool PostAccept();
	void AcceptCompletion();
	
	virtual void OnReceive( size_t len );
	virtual void OnDisconnect(DisconnectReason dr);
	virtual void OnRelease();

	void KeyInit();
	DWORD GetKeyDataLen();
	char* GetKeyBlob();
	void SetReceiveKeySet( MyPacket::SendingKeySet keySet );
	bool	IsEncrypt() const { return mIsEncrypt; }
	
public:
	Player			mPlayer;

private:
	
	SOCKADDR_IN		mClientAddr ;

	//////////////////////////////////////////////////////////////////////////
	//암호화 관련 이야기!!
	//에니그마로 암호학의 발전을 이끈 히틀러 $@%#$%노무... 시끼

	bool			mIsEncrypt = false;

	// 키 생성 및 암/복호화 클래스
	KeyChanger		mCrypt;
	// 내가 사용할 비밀키
	KeyPrivateSets	mPrivateKeySet;
	// 내가 상대방에게 보낼 공개키
	KeySendingSets	mServerSendKeySet;
	// 상대가 나한테 보내준 공개키
	KeySendingSets	mReceiveKeySet;

	BYTE			mKeyBlob[8];

	//////////////////////////////////////////////////////////////////////////
	
	friend class ClientSessionManager;
} ;



