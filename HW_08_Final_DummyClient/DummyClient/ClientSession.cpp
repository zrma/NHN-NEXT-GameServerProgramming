#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"
#include "FastSpinlock.h"
#include "DummyClient.h"
#include "Player.h"
#include "Log.h"

__declspec( thread ) std::deque<ClientSession*>* LSendRequestSessionList = nullptr;
__declspec( thread ) std::deque<ClientSession*>* LSendRequestFailedSessionList = nullptr;

OverlappedIOContext::OverlappedIOContext(ClientSession* owner, IOType ioType) 
: mSessionObject(owner), mIoType(ioType)
{
	memset(&mOverlapped, 0, sizeof(OVERLAPPED));
	memset(&mWsaBuf, 0, sizeof(WSABUF));
	mSessionObject->AddRef();
}

ClientSession::ClientSession() : mRecvBuffer(BUFFER_SIZE), mSendBuffer(BUFFER_SIZE)
, mConnected(0), mRefCount(0), mSendBufferLock(LO_LUGGAGE_CLASS), mSendPendingCount(0)
, mPlayer( new Player( this ) )
{
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	struct sockaddr_in addr;
	ZeroMemory( &addr, sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 0;
	int result = bind( mSocket, (SOCKADDR*)&addr, sizeof( addr ) );

	if ( result != 0 )
	{
		printf( "Bind failed: %d\n", WSAGetLastError() );
	}
	
	// proto

	m_pArrayOutputStream = new google::protobuf::io::ArrayOutputStream( m_SessionBuffer, MAX_BUFFER_SIZE );
	m_pCodedOutputStream = new google::protobuf::io::CodedOutputStream( m_pArrayOutputStream );

}

void ClientSession::SessionReset()
{
	mConnected = 0;
	mRefCount = 0;
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

	mSendBuffer.BufferReset();
	mRecvBuffer.BufferReset();

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)))
	{
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}
	closesocket(mSocket);

	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}


bool ClientSession::PostConnect()
{
	TRACE_THIS;

	CRASH_ASSERT( LThreadType == THREAD_MAIN );

	HANDLE handle = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), ( ULONG_PTR )this, 0 );
	if ( handle != GIocpManager->GetComletionPort() )
	{
		printf_s( "[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError() );
		return false;
	}

	OverlappedConnectContext* connectContext = new OverlappedConnectContext( this );
	DWORD bytes = 0;
	connectContext->mWsaBuf.len = 0;
	connectContext->mWsaBuf.buf = nullptr;
	
	if ( SERVER_PORT <= 4000 || SERVER_PORT > 10000 )
	{
		SERVER_PORT = 9000;
	}

	if ( HOST_NAME.length() == 0 )
	{
		HOST_NAME = "127.0.0.1";
	}

	sockaddr_in addr;
	ZeroMemory( &addr, sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr( HOST_NAME.c_str() );
	addr.sin_port = htons( SERVER_PORT );

	if ( FALSE == IocpManager::ConnectEx( mSocket, (SOCKADDR*)&addr, sizeof( addr ), 
		GIocpManager->mConnectBuf, 0, &bytes, (LPOVERLAPPED)connectContext ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( connectContext );
			printf_s( "ConnectEx Error : %d\n", GetLastError() );

			return false;
		}
	}
	
	return true;
}

void ClientSession::ConnectCompletion()
{
	TRACE_THIS;
	CRASH_ASSERT( LThreadType == THREAD_IO_WORKER );
	if ( 1 == InterlockedExchange( &mConnected, 1 ) )
	{
		/// already exists?
		CRASH_ASSERT( false );
		return;
	}

	bool resultOk = true;

	do
	{
		if ( SOCKET_ERROR == setsockopt( mSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0 ) )
		{
			printf_s( "[DEBUG] SO_UPDATE_CONNECT_CONTEXT error: %d\n", GetLastError() );
			resultOk = false;
			break;
		}

		int opt = 1;
		if ( NO_DELAY )
		{
			int opt = 1;
			if ( SOCKET_ERROR == setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof( int ) ) )
			{
				printf_s( "[DEBUG] TCP_NODELAY error: %d\n", GetLastError() );
				resultOk = false;
				break;
			}
		}

		opt = 0;
		if ( SOCKET_ERROR == setsockopt( mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof( int ) ) )
		{
			printf_s( "[DEBUG] SO_RCVBUF change error: %d\n", GetLastError() );
			resultOk = false;
			break;
		}

		int addrlen = sizeof( SOCKADDR_IN );
		if ( SOCKET_ERROR == getpeername( mSocket, (SOCKADDR*)&mClientAddr, &addrlen ) )
		{
			printf_s( "[DEBUG] getpeername error: %d\n", GetLastError() );
			resultOk = false;
			break;
		}

	} while ( false );
	
	if ( !resultOk )
	{
		DisconnectRequest( DR_ONCONNECT_ERROR );
		return;
	}

	FastSpinlockGuard criticalSection( mSendBufferLock );

	//////////////////////////////////////////////////////////////////////////
	// 여기부터 로그인 리퀘스트 패킷 조합 -> 시리얼라이즈 -> 암호화 해서 전송 요청 시작하면 됨
	//
	// 하단 코드는 날려버릴 예정

	if ( BUFFER_SIZE <= 0 || BUFFER_SIZE > BUFSIZE )
	{
		BUFFER_SIZE = 4096;
	}

	char* temp = new char[BUFFER_SIZE];
	
	ZeroMemory( temp, sizeof( char ) * BUFFER_SIZE );
	for ( int i = 0; i < BUFFER_SIZE - 1; ++i )
	{
		temp[i] = 'a' + ( mSocket % 26 );
	}

	temp[BUFFER_SIZE - 1] = '\0';

	char* bufferStart = mSendBuffer.GetBuffer();
	memcpy( bufferStart, temp, BUFFER_SIZE );

	mSendBuffer.Commit( BUFFER_SIZE );
		
	CRASH_ASSERT( 0 != mSendBuffer.GetContiguiousBytes() );
		


	delete[] temp;


	OverlappedSendContext* sendContext = new OverlappedSendContext( this );

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = (ULONG)mSendBuffer.GetContiguiousBytes();
	sendContext->mWsaBuf.buf = mSendBuffer.GetBufferStart();


	/// start async send
	if ( SOCKET_ERROR == WSASend( mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags, (LPWSAOVERLAPPED)sendContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( sendContext );
			printf_s( "ClientSession::PostSend Error : %d\n", GetLastError() );
		}
	}


	// proto


	MyPacket::LoginRequest loginRequest;
	loginRequest.set_playerid( 1234 );

	WriteMessageToStream( MyPacket::MessageType::PKT_CS_LOGIN, loginRequest, *m_pCodedOutputStream );




	++mUseCount;
}

void ClientSession::DisconnectRequest(DisconnectReason dr)
{
	TRACE_THIS;

	/// 이미 끊겼거나 끊기는 중이거나
	if (0 == InterlockedExchange(&mConnected, 0))
		return ;
	
	OverlappedDisconnectContext* context = new OverlappedDisconnectContext(this, dr);

	if (FALSE == IocpManager::DisconnectEx(mSocket, (LPWSAOVERLAPPED)context, TF_REUSE_SOCKET, 0))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(context);
			printf_s("ClientSession::DisconnectRequest Error : %d\n", GetLastError());
		}
	}
}

void ClientSession::DisconnectCompletion(DisconnectReason dr)
{
	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	/// release refcount when added at issuing a session
	ReleaseRef();
}


bool ClientSession::PreRecv()
{
	TRACE_THIS;

	if ( !IsConnected() )
		return false;

	OverlappedPreRecvContext* recvContext = new OverlappedPreRecvContext( this );

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = 0;
	recvContext->mWsaBuf.buf = nullptr;

	/// start async recv
	if ( SOCKET_ERROR == WSARecv( mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( recvContext );
			printf_s( "Session::PreRecv Error : %d\n", GetLastError() );
			return false;
		}
	}

	return true;
}

bool ClientSession::PostRecv()
{
	TRACE_THIS;

	if (!IsConnected())
		return false;

	if (0 == mRecvBuffer.GetFreeSpaceSize())
		return false;

	OverlappedRecvContext* recvContext = new OverlappedRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = (ULONG)mRecvBuffer.GetFreeSpaceSize();
	recvContext->mWsaBuf.buf = mRecvBuffer.GetBuffer();
	

	/// start real recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(recvContext);
			printf_s("ClientSession::PostRecv Error : %d\n", GetLastError());
			return false;
		}
			
	}

	return true;
}

void ClientSession::RecvCompletion(DWORD transferred)
{
	TRACE_THIS;

	FastSpinlockGuard criticalSection(mSendBufferLock);

	mRecvBuffer.Commit(transferred);
	mRecvBytes += transferred;

	mRecvBuffer.GetBuffer();
	// OnRead(transferred);
}

bool ClientSession::PostSend( const char* data, size_t len )
{
	TRACE_THIS;

	if ( !IsConnected() )
		return false;

	FastSpinlockGuard criticalSection( mSendBufferLock );

	if ( mSendBuffer.GetFreeSpaceSize() < len )
		return false;

	/// flush later...
	LSendRequestSessionList->push_back( this );

	char* destData = mSendBuffer.GetBuffer();

	memcpy( destData, data, len );

	mSendBuffer.Commit( len );

	return true;
}

bool ClientSession::FlushSend()
{
	TRACE_THIS;

	if ( !IsConnected() )
	{
		DisconnectRequest( DR_SENDFLUSH_ERROR );
		return true;
	}


	FastSpinlockGuard criticalSection( mSendBufferLock );

	/// 보낼 데이터가 없는 경우
	if ( 0 == mSendBuffer.GetContiguiousBytes() )
	{
		/// 보낼 데이터도 없는 경우
		if ( 0 == mSendPendingCount )
			return true;

		return false;
	}

	/// 이전의 send가 완료 안된 경우
	if ( mSendPendingCount > 0 )
		return false;


	OverlappedSendContext* sendContext = new OverlappedSendContext( this );

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = (ULONG)mSendBuffer.GetContiguiousBytes();
	sendContext->mWsaBuf.buf = mSendBuffer.GetBufferStart();

	/// start async send
	if ( SOCKET_ERROR == WSASend( mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags, (LPWSAOVERLAPPED)sendContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( sendContext );
			printf_s( "Session::FlushSend Error : %d\n", GetLastError() );

			DisconnectRequest( DR_SENDFLUSH_ERROR );
			return true;
		}

	}

	++mSendPendingCount;

	return mSendPendingCount == 1;
}

void ClientSession::SendCompletion(DWORD transferred)
{
	TRACE_THIS;

	FastSpinlockGuard criticalSection( mSendBufferLock );

	mSendBuffer.Remove( transferred );

	--mSendPendingCount;
}

void ClientSession::AddRef()
{
	CRASH_ASSERT(InterlockedIncrement(&mRefCount) > 0);
}

void ClientSession::ReleaseRef()
{
	long ret = InterlockedDecrement(&mRefCount);
	CRASH_ASSERT(ret >= 0);
	
	if (ret == 0)
	{
		GSessionManager->ReturnClientSession(this);
	}
}

void ClientSession::EchoBack()
{
	TRACE_THIS;

	size_t len = mRecvBuffer.GetContiguiousBytes();

	if ( len == 0 )
		return;

	if ( false == PostSend( mRecvBuffer.GetBufferStart(), len ) )
		return;

	mRecvBuffer.Remove( len );
}

void DeleteIoContext(OverlappedIOContext* context)
{
	if (nullptr == context)
		return;

	context->mSessionObject->ReleaseRef();

	/// ObjectPool's operate delete dispatch
	switch (context->mIoType)
	{
	case IO_SEND:
		delete static_cast<OverlappedSendContext*>(context);
		break;

	case IO_RECV_ZERO:
		delete static_cast<OverlappedPreRecvContext*>(context);
		break;

	case IO_RECV:
		delete static_cast<OverlappedRecvContext*>(context);
		break;

	case IO_DISCONNECT:
		delete static_cast<OverlappedDisconnectContext*>(context);
		break;

	case IO_CONNECT:
		delete static_cast<OverlappedConnectContext*>(context);
		break;

	default:
		CRASH_ASSERT(false);
	}
}



