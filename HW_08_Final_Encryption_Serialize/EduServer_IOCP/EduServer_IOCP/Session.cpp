#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "Log.h"
#include "EduServer_IOCP.h"
#include "OverlappedIOContext.h"
#include "Session.h"
#include "IocpManager.h"
#include "PacketHeader.h"

__declspec( thread ) std::deque<Session*>* LSendRequestSessionList = nullptr;
__declspec( thread ) std::deque<Session*>* LSendRequestFailedSessionList = nullptr;

Session::Session(size_t sendBufSize, size_t recvBufSize) 
: mSendBuffer(sendBufSize), mRecvBuffer(recvBufSize), mConnected(0), mRefCount(0), mSendPendingCount(0)
{
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}


void Session::DisconnectRequest(DisconnectReason dr)
{
	TRACE_THIS;

	/// 이미 끊겼거나 끊기는 중이거나
	if (0 == InterlockedExchange(&mConnected, 0))
		return;

	OverlappedDisconnectContext* context = new OverlappedDisconnectContext(this, dr);

	if (FALSE == DisconnectEx(mSocket, (LPWSAOVERLAPPED)context, TF_REUSE_SOCKET, 0))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(context);
			printf_s("Session::DisconnectRequest Error : %d\n", GetLastError());
		}
	}

}


bool Session::PreRecv()
{
	TRACE_THIS;

	if (!IsConnected())
		return false;

	OverlappedPreRecvContext* recvContext = new OverlappedPreRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = 0;
	recvContext->mWsaBuf.buf = nullptr;

	/// start async recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(recvContext);
			printf_s("Session::PreRecv Error : %d\n", GetLastError());
			return false;
		}
	}

	return true;
}

bool Session::PostRecv()
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
			printf_s("Session::PostRecv Error : %d\n", GetLastError());
			return false;
		}

	}

	return true;
}


bool Session::PostSend(const char* data, size_t len)
{
	TRACE_THIS;

	if (!IsConnected())
		return false;

	FastSpinlockGuard criticalSection(mSendBufferLock);

	if (mSendBuffer.GetFreeSpaceSize() < len)
		return false;

	/// flush later...
	LSendRequestSessionList->push_back(this);
	
	char* destData = mSendBuffer.GetBuffer();

	memcpy(destData, data, len);

	mSendBuffer.Commit(len);

	return true;
}


bool Session::FlushSend()
{
	TRACE_THIS;

	if (!IsConnected())
	{
		DisconnectRequest(DR_SENDFLUSH_ERROR);
		return true;
	}
		

	FastSpinlockGuard criticalSection(mSendBufferLock);

	/// 보낼 데이터가 없는 경우
	if (0 == mSendBuffer.GetContiguiousBytes())
	{
		/// 보낼 데이터도 없는 경우
		if (0 == mSendPendingCount)
			return true;
		
		return false;
	}

	/// 이전의 send가 완료 안된 경우
	if (mSendPendingCount > 0)
		return false;

	
	OverlappedSendContext* sendContext = new OverlappedSendContext(this);

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = (ULONG)mSendBuffer.GetContiguiousBytes();
	sendContext->mWsaBuf.buf = mSendBuffer.GetBufferStart();

	//암호화
	CryptAction( (BYTE*)sendContext->mWsaBuf.buf, sendContext->mWsaBuf.len, (BYTE*)sendContext->mWsaBuf.buf );
		
	/// start async send
	if (SOCKET_ERROR == WSASend(mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags, (LPWSAOVERLAPPED)sendContext, NULL))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(sendContext);
			printf_s("Session::FlushSend Error : %d\n", GetLastError());

			DisconnectRequest(DR_SENDFLUSH_ERROR);
			return true;
		}

	}

	mSendPendingCount++;

	return mSendPendingCount == 1;
}

void Session::DisconnectCompletion(DisconnectReason dr)
{
	TRACE_THIS;

	OnDisconnect(dr);

	/// release refcount when added at issuing a session
	ReleaseRef();
}


void Session::SendCompletion(DWORD transferred)
{
	TRACE_THIS;

	FastSpinlockGuard criticalSection(mSendBufferLock);

	mSendBuffer.Remove(transferred);

	mSendPendingCount--;
}

void Session::RecvCompletion(DWORD transferred)
{
	TRACE_THIS;

	mRecvBuffer.Commit(transferred);

	//암호 해제 구간
	DecryptAction( (BYTE*)mRecvBuffer.GetBufferStart(), transferred );
	
	//받고서 바로 패킷 처리 작업 진행
	OnReceive( transferred );
}



void Session::AddRef()
{
	CRASH_ASSERT(InterlockedIncrement(&mRefCount) > 0);
}

void Session::ReleaseRef()
{
	long ret = InterlockedDecrement(&mRefCount);
	CRASH_ASSERT(ret >= 0);

	if (ret == 0)
	{
		OnRelease();
	}
}

void Session::EchoBack()
{
	TRACE_THIS;

	size_t len = mRecvBuffer.GetContiguiousBytes();

	if (len == 0)
		return;

	if (false == PostSend(mRecvBuffer.GetBufferStart(), len))
		return;

	mRecvBuffer.Remove(len);

}

void Session::SetReceiveKeySet( MyPacket::SendingKeySet keySet )
{
	mReceiveKeySet.dwDataLen = keySet.datalen();
	mReceiveKeySet.pbKeyBlob = mKeyBlob;

	if ( mReceiveKeySet.dwDataLen == 0 )
	{
		printf_s( "Key Length error - 0" );
		return;
	}

	if ( mKeyBlob )
	{
		delete mKeyBlob;
		mKeyBlob = nullptr;
	}

	mKeyBlob = new BYTE[mReceiveKeySet.dwDataLen];
	memcpy( mKeyBlob, keySet.keyblob().data(), mReceiveKeySet.dwDataLen );

	// 널문자 때문에 +1 더해준 것 -1
	for ( size_t i = 0; i < mReceiveKeySet.dwDataLen; ++i )
		mKeyBlob[i]--;

	mCrypt.GetSessionKey( &mPrivateKeySet, &mReceiveKeySet );

	mIsEncrypt = true;
}

void Session::KeyInit()
{
	memset( &mPrivateKeySet, 0, sizeof( KeyPrivateSets ) );
	memset( &mReceiveKeySet, 0, sizeof( KeySendingSets ) );
	memset( &mServerSendKeySet, 0, sizeof( KeySendingSets ) );

	memset( &mKeyBlob, 0, sizeof( BYTE ) * 8 );

	mCrypt.GenerateKey( &mPrivateKeySet, &mServerSendKeySet );

	bool flag = false;

	// 키가 유효할 때까지(255가 없을 때까지) 계속 뽑아준다.
	// Why -> 널 문자 때문에 제대로 안 들어간다. 그래서 밑에서 꼼수로 +1 해줌
	// 하지만 unsigned char 255인 녀석(signed char -1)은 오버플로우 되면서 0이 되므로... risk!
	while ( !flag )
	{
		for ( DWORD i = 0; i < mServerSendKeySet.dwDataLen; ++i )
		{
			if ( mServerSendKeySet.pbKeyBlob[i] == (UCHAR)255 )
			{
				printf_s( "키 재생성! \n" );
				mCrypt.GenerateKey( &mPrivateKeySet, &mServerSendKeySet );
				break;
			}

			flag = true;
		}
	}
}

DWORD Session::GetKeyDataLen()
{
	return mServerSendKeySet.dwDataLen;
}

char* Session::GetKeyBlob()
{
	return (char*)mServerSendKeySet.pbKeyBlob;
}

void Session::CryptAction( BYTE* original, int originalSize, BYTE* crypted )
{
	if ( IsEncrypt() )
		if ( !mCrypt.EncryptData( mPrivateKeySet.hSessionKey, original, originalSize, crypted ) )
			printf_s( "Encrypt failed error \n" );
}

void Session::DecryptAction( BYTE* crypted, int crypedSize )
{
	if ( IsEncrypt() )
		if ( !mCrypt.DecryptData( mPrivateKeySet.hSessionKey, crypted, crypedSize ) )
			printf_s( "decrypt failed error \n" );
}