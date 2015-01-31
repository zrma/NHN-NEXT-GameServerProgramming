#pragma once
#include "CircularBuffer.h"
#include "OverlappedIOContext.h"
#include "PacketHeader.h"
#include "Crypter.h"

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
	

	void	CrypterInit();

	typedef std::vector < char > KeySet;
	KeySet&	GetExchangeKey();
	void	SetReceiveKey( KeySet& receiveKey );

	void	Crypt( char* originalData, int dataSize );
	void	Decrypt( char* encryptedData, int dataSize );

	bool	IsEncrypt() const { return mIsEncrypt; }

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
	Crypter			mCrypter;
	//////////////////////////////////////////////////////////////////////////
};


extern __declspec( thread ) std::deque<Session*>* LSendRequestSessionList;
extern __declspec( thread ) std::deque<Session*>* LSendRequestFailedSessionList;