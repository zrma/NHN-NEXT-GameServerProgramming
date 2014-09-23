#include "stdafx.h"
#include "Exception.h"
#include "Log.h"
#include "EduServer_IOCP.h"
#include "OverlappedIOContext.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "ClientSessionManager.h"
#include "PacketHeader.h"

#define CLIENT_BUFSIZE	65536

ClientSession::ClientSession(): Session( CLIENT_BUFSIZE, CLIENT_BUFSIZE ), mPlayer( this )
{
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
}

ClientSession::~ClientSession()
{
}

void ClientSession::SessionReset()
{
	TRACE_THIS;
	TRACE_PERF;

	mConnected = 0;
	mRefCount = 0;
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

	mRecvBuffer.BufferReset();

	mSendBufferLock.EnterLock();
	mSendBuffer.BufferReset();
	mSendBufferLock.LeaveLock();

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

	mPlayer.PlayerReset();
}

bool ClientSession::PostAccept()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	OverlappedAcceptContext* acceptContext = new OverlappedAcceptContext(this);
	DWORD bytes = 0;
	DWORD flags = 0;
	acceptContext->mWsaBuf.len = 0;
	acceptContext->mWsaBuf.buf = nullptr;

	if (FALSE == AcceptEx(*GIocpManager->GetListenSocket(), mSocket, GIocpManager->mAcceptBuf, 0,
		sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &bytes, (LPOVERLAPPED)acceptContext))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(acceptContext);
			printf_s("AcceptEx Error : %d\n", GetLastError());

			return false;
		}
	}

	return true;
}

void ClientSession::AcceptCompletion()
{
	TRACE_THIS;
	TRACE_PERF;

	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);
	
	if (1 == InterlockedExchange(&mConnected, 1))
	{
		/// already exists?
		CRASH_ASSERT(false);
		return;
	}

	bool resultOk = true;
	do 
	{
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)GIocpManager->GetListenSocket(), sizeof(SOCKET)))
		{
			printf_s("[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		int opt = 1;
		if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)))
		{
			printf_s("[DEBUG] TCP_NODELAY error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		opt = 0;
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)))
		{
			printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		int addrlen = sizeof(SOCKADDR_IN);
		if (SOCKET_ERROR == getpeername(mSocket, (SOCKADDR*)&mClientAddr, &addrlen))
		{
			printf_s("[DEBUG] getpeername error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		HANDLE handle = CreateIoCompletionPort((HANDLE)mSocket, GIocpManager->GetComletionPort(), (ULONG_PTR)this, 0);
		if (handle != GIocpManager->GetComletionPort())
		{
			printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

	} while (false);


	if (!resultOk)
	{
		DisconnectRequest(DR_ONCONNECT_ERROR);
		return;
	}

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	if (false == PreRecv())
	{
		printf_s("[DEBUG] PreRecv error: %d\n", GetLastError());
	}

	/*
	//TEST: 요놈의 위치는 원래 C_LOGIN 핸들링 할 때 해야하는거지만 지금은 접속 완료 시점에서 테스트 ㄱㄱ

	//todo: 플레이어 id는 여러분의 플레이어 테이블 상황에 맞게 적절히 고쳐서 로딩하도록 
	static int id = 101;
	mPlayer.ResponseCreatePlayerData( L"newPlayer 뿅" );
 	mPlayer.RequestLoad(id);
	mPlayer.ResponseDeletePlayerData( id++ );
	*/
	//DB out
}




void ClientSession::OnDisconnect(DisconnectReason dr)
{
	TRACE_THIS;
	TRACE_PERF;

	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
}

void ClientSession::OnRelease()
{
	TRACE_THIS;
	TRACE_PERF;


	GClientSessionManager->ReturnClientSession(this);
}

void ClientSession::SetReceiveKeySet( MyPacket::SendingKeySet keySet )
{
	mReceiveKeySet.dwDataLen = keySet.datalen();
	memcpy( mKeyBlob, keySet.keyblob().c_str(), sizeof( BYTE ) * 8 );
	mReceiveKeySet.pbKeyBlob = mKeyBlob;

	mCrypt.GetSessionKey( &mPrivateKeySet, &mReceiveKeySet );

	mIsEncrypt = true;
}

void ClientSession::KeyInit()
{
	mCrypt.GenerateKey( &mPrivateKeySet, &mServerSendKeySet );
}

DWORD ClientSession::GetKeyDataLen()
{
	return mServerSendKeySet.dwDataLen;
}

char* ClientSession::GetKeyBlob()
{
	return (char*)mServerSendKeySet.pbKeyBlob;
}

void ClientSession::CryptAction( BYTE* original, int originalSize, BYTE* crypted )
{
	if (IsEncrypt())
	{
		if (!mCrypt.EncryptData(mPrivateKeySet.hSessionKey, original, originalSize, crypted))
		{
			printf_s( "Encrypt failed error \n" );
		}
	}
}

void ClientSession::DecryptAction( BYTE* crypted, int crypedSize )
{
	if ( IsEncrypt() )
		if ( !mCrypt.DecryptData( mPrivateKeySet.hSessionKey,crypted, crypedSize ) )
			printf_s( "decrypt failed error \n" );
}

bool ClientSession::SendRequest( short packetType, const google::protobuf::MessageLite& payload )
{
	TRACE_THIS;

	if ( !IsConnected() )
		return false;

	FastSpinlockGuard criticalSection( mSendBufferLock );

	int totalSize = payload.ByteSize() + HEADER_SIZE;
	if ( mSendBuffer.GetFreeSpaceSize() < (size_t)( totalSize ) )
		return false;

	google::protobuf::io::ArrayOutputStream arrayOutputStream( mSendBuffer.GetBuffer(), totalSize );
	google::protobuf::io::CodedOutputStream codedOutputStream( &arrayOutputStream );

	PacketHeader packetheader;
	packetheader.mSize = payload.ByteSize();
	packetheader.mType = packetType;

	codedOutputStream.WriteRaw( &packetheader, HEADER_SIZE );
	payload.SerializeToCodedStream( &codedOutputStream );

	mSendBuffer.Commit( totalSize );

	if ( packetType == MyPacket::PKT_SC_CRYPT )
	{
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

		mSendPendingCount++;
	}
	else
	{
		CryptAction( (BYTE*)mSendBuffer.GetBufferStart(), totalSize, (BYTE*)mSendBuffer.GetBufferStart() );

		/// flush later...
		LSendRequestSessionList->push_back( this );
	}

	return true;
}