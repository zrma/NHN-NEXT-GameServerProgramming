#pragma once
#include "ObjectPool.h"
#include "MemoryPool.h"
#include "CircularBuffer.h"
#include "ProtoHeader.h"
#include "KeyChanger.h"

#define BUFSIZE	65536

class ClientSession ;
class SessionManager;
class Player;

enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_DISCONNECT,
	IO_CONNECT
} ;

enum DisconnectReason
{
	DR_NONE,
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
	DR_IO_REQUEST_ERROR,
	DR_COMPLETION_ERROR,
	DR_SENDFLUSH_ERROR
};

struct OverlappedIOContext
{
	OverlappedIOContext(ClientSession* owner, IOType ioType);

	OVERLAPPED		mOverlapped ;
	ClientSession*	mSessionObject ;
	IOType			mIoType ;
	WSABUF			mWsaBuf;
	
} ;

struct OverlappedSendContext : public OverlappedIOContext, public ObjectPool<OverlappedSendContext>
{
	OverlappedSendContext(ClientSession* owner) : OverlappedIOContext(owner, IO_SEND)
	{
	}
};

struct OverlappedRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedRecvContext>
{
	OverlappedRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IO_RECV)
	{
	}
};

struct OverlappedPreRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedPreRecvContext>
{
	OverlappedPreRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IO_RECV_ZERO)
	{
	}
};

struct OverlappedConnectContext: public OverlappedIOContext, public ObjectPool < OverlappedConnectContext >
{
	OverlappedConnectContext( ClientSession* owner ): OverlappedIOContext( owner, IO_CONNECT )
	{
	}
};

struct OverlappedDisconnectContext : public OverlappedIOContext, public ObjectPool<OverlappedDisconnectContext>
{
	OverlappedDisconnectContext(ClientSession* owner, DisconnectReason dr) 
	: OverlappedIOContext(owner, IO_DISCONNECT), mDisconnectReason(dr)
	{}

	DisconnectReason mDisconnectReason;
};

void DeleteIoContext(OverlappedIOContext* context) ;


class ClientSession: public PooledAllocatable
{
public:
	ClientSession();
	~ClientSession() {}

	void	SessionReset();

	bool	IsConnected() const { return !!mConnected; }

	bool	PostConnect();

	void	ConnectCompletion();

	bool	PreRecv(); ///< zero byte recv

	bool	PostRecv();
	void	RecvCompletion( DWORD transferred );

	bool	PostSend( const char* data, size_t len );
	bool	FlushSend();

	void	SendCompletion( DWORD transferred );

	void	EchoBack();
	void	OnRead( size_t len );
	
	bool	ParsePacket( char* packet, int size )
	{
		return mRecvBuffer.Read( packet, size );
	}

	bool	IsEncrypt() const { return mIsEncrypt; }
	bool	SetEncrypt( bool flag = true ) { mIsEncrypt = flag; }

	void	SetReceiveKeySet( MyPacket::SendingKeySet keySet );

	void	DisconnectRequest( DisconnectReason dr );
	void	DisconnectCompletion( DisconnectReason dr );

	void	AddRef();
	void	ReleaseRef();

	void	SetSocket( SOCKET sock ) { mSocket = sock; }
	SOCKET	GetSocket() const { return mSocket; }

	long long	GetSendBytes() { return mSendBytes; }
	long long	GetRecyBytes() { return mRecvBytes; }
	long		GetConnectCount() { return mUseCount; }

public:
	std::shared_ptr<Player> mPlayer;

private:
	SOCKET			mSocket;
	SOCKADDR_IN		mClientAddr;

	FastSpinlock	mSendBufferLock;

	CircularBuffer	mRecvBuffer;
	CircularBuffer	mSendBuffer;

	volatile long	mRefCount;
	volatile long	mConnected;

	int				mSendPendingCount;

	long long		mSendBytes = 0;
	long long		mRecvBytes = 0;
	long			mUseCount = 0;

	bool			mIsEncrypt = false;

	// 키 생성 및 암/복호화 클래스
	KeyChanger		mCrypt;
	// 내가 사용할 비밀키
	KeyPrivateSets	mPrivateKeySet;
	// 내가 상대방에게 보낼 공개키
	KeySendingSets	mCliSendKeySet;
	// 상대가 나한테 보내준 공개키
	KeySendingSets	mReceiveKeySet;

	//////////////////////////////////////////////////////////////////////////
	// for protobuff
	google::protobuf::uint8 m_SessionBuffer[MAX_BUFFER_SIZE];
	google::protobuf::io::ArrayOutputStream* m_pArrayOutputStream;
	google::protobuf::io::CodedOutputStream* m_pCodedOutputStream;

	void WriteMessageToStream(
		MyPacket::MessageType msgType,
		const google::protobuf::MessageLite& message,
		google::protobuf::io::CodedOutputStream& stream )

	{
		MessageHeader messageHeader;
		messageHeader.size = message.ByteSize();
		messageHeader.type = msgType;
		stream.WriteRaw( &messageHeader, sizeof( MessageHeader ) );
		message.SerializeToCodedStream( &stream );
	}
	
	friend class SessionManager;
};

extern __declspec( thread ) std::deque<ClientSession*>* LSendRequestSessionList;
extern __declspec( thread ) std::deque<ClientSession*>* LSendRequestFailedSessionList;
