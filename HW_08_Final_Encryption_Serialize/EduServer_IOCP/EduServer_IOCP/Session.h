#pragma once
#include "CircularBuffer.h"
#include "OverlappedIOContext.h"
#include "PacketHeader.h"
#include "KeyChanger.h"


class Session
{
public:
	Session(size_t sendBufSize, size_t recvBufSize);
	virtual ~Session() {}

	bool IsConnected() const { return !!mConnected; }

	void DisconnectRequest(DisconnectReason dr) ;

	bool PreRecv(); ///< zero byte recv
	bool PostRecv() ;

	bool PostSend(const char* data, size_t len);
	bool FlushSend() ;

	void DisconnectCompletion(DisconnectReason dr) ;
	void SendCompletion(DWORD transferred) ;
	void RecvCompletion(DWORD transferred) ;

	void AddRef();
	void ReleaseRef();

	virtual bool SendRequest( short packetType, const google::protobuf::MessageLite& payload ) = 0;
	virtual void OnReceive( size_t len ) {}
	virtual void OnDisconnect(DisconnectReason dr) {}
	virtual void OnRelease() {}

	void	SetSocket(SOCKET sock) { mSocket = sock; }
	SOCKET	GetSocket() const { return mSocket; }

	CircularBuffer GetRecvBuffer() const { return mRecvBuffer; }
	CircularBuffer GetSendBuffer() const { return mSendBuffer; }

	void EchoBack();


	void KeyInit();
	DWORD GetKeyDataLen();
	char* GetKeyBlob();
	void CryptAction( PBYTE original, int originalSize, PBYTE crypted );
	void DecryptAction( PBYTE crypted, int crypedSize );

	void SetReceiveKeySet( MyPacket::SendingKeySet keySet );

	bool IsEncrypt() const { return mIsEncrypt; }

protected:

	SOCKET			mSocket;

	CircularBuffer	mRecvBuffer;
	CircularBuffer	mSendBuffer;
	FastSpinlock	mSendBufferLock;
	int				mSendPendingCount;

	volatile long	mRefCount;
	volatile long	mConnected;

	//////////////////////////////////////////////////////////////////////////
	// 암호화 관련
	bool			mIsEncrypt = false;

	// 키 생성 및 암호화 클래스
	KeyChanger		mEncrypt;
	// 키 생성 및 복호화 클래스
	KeyChanger		mDecrypt;

	// 내가 사용할 비밀키
	KeyPrivateSets	mPrivateKeySet;
	// 내가 상대방에게 보낼 공개키
	KeySendingSets	mServerSendKeySet;
	// 상대가 나한테 보내준 공개키
	KeySendingSets	mReceiveKeySet;

	PBYTE			mKeyBlob = nullptr;

	//////////////////////////////////////////////////////////////////////////


};


extern __declspec( thread ) std::deque<Session*>* LSendRequestSessionList;
extern __declspec( thread ) std::deque<Session*>* LSendRequestFailedSessionList;