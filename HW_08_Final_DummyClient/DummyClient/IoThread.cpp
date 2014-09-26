#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "IOThread.h"
#include "ClientSession.h"
#include "IocpManager.h"

IOThread::IOThread( HANDLE hThread, HANDLE hCompletionPort ): mThreadHandle( hThread ), mCompletionPort( hCompletionPort )
{
}


IOThread::~IOThread()
{
	CloseHandle( mThreadHandle );
}

DWORD IOThread::Run()
{
	while ( mIsContinue )
	{
		DoIocpJob();

		DoSendJob(); ///< aggregated sends

		//... ...
	}

	printf_s( "찍!" );

	return 1;
}

void IOThread::DoIocpJob()
{
	DWORD dwTransferred = 0;
	OverlappedIOContext* context = nullptr;

	ULONG_PTR completionKey = 0;

	int ret = GetQueuedCompletionStatus( mCompletionPort, &dwTransferred, (PULONG_PTR)&completionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT );
	
	if ( completionKey == THREAD_QUIT_KEY )
	{
		mIsContinue = false;
		return;
	}

	ClientSession* session = context ? context->mSessionObject : nullptr;

	if ( ret == 0 || dwTransferred == 0 )
	{
		int gle = GetLastError();

		/// check time out first 
		if ( gle == WAIT_TIMEOUT && context == nullptr )
		{
			return;
		}

		if ( context->mIoType == IO_RECV || context->mIoType == IO_SEND )
		{
			CRASH_ASSERT( nullptr != session );

			/// In most cases in here: ERROR_NETNAME_DELETED(64)

			session->DisconnectRequest( DR_COMPLETION_ERROR );

			DeleteIoContext( context );

			return;
		}
	}

	CRASH_ASSERT( nullptr != session );

	bool completionOk = false;
	switch ( context->mIoType )
	{
		case IO_CONNECT:
			session->ConnectCompletion();
			completionOk = true;
			break;

		case IO_DISCONNECT:
			session->DisconnectCompletion( static_cast<OverlappedDisconnectContext*>( context )->mDisconnectReason );
			completionOk = true;
			break;

		case IO_RECV_ZERO:
			completionOk = session->PostRecv();
			break;

		case IO_SEND:
			session->SendCompletion( dwTransferred );

			if ( context->mWsaBuf.len != dwTransferred )
				printf_s( "Partial SendCompletion requested [%d], sent [%d]\n", context->mWsaBuf.len, dwTransferred );
			else
				completionOk = true;
			
			break;
			
		case IO_RECV:
			session->RecvCompletion( dwTransferred );

			// 일단 테스트용 에코!
			// session->EchoBack();

			completionOk = session->PreRecv();

			break;

		default:
			printf_s( "Unknown I/O Type: %d\n", context->mIoType );
			CRASH_ASSERT( false );
			break;
	}

	if ( !completionOk )
	{
		/// connection closing
		session->DisconnectRequest( DR_IO_REQUEST_ERROR );
	}

	DeleteIoContext( context );

}


void IOThread::DoSendJob()
{
	while ( !LSendRequestSessionList->empty() )
	{
		auto& session = LSendRequestSessionList->front();
		LSendRequestSessionList->pop_front();

		if ( !(session->FlushSend()) )
		{
			// false 되면 가비지 수집한다
			LSendRequestFailedSessionList->push_back( session );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Swap!
	
	// 비어 있는 녀석이다
	std::deque<ClientSession*>* tempDeq = LSendRequestSessionList;

	// 남아 있는 녀석을 다음번에 이어서 쓰고
	LSendRequestSessionList = LSendRequestFailedSessionList;
	
	// 가비지 수집할 녀석을 비어 있는 녀석으로 교체
	LSendRequestFailedSessionList = tempDeq;
}