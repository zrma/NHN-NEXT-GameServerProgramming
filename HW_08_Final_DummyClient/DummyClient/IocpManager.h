#pragma once
#include "FastSpinlock.h"

#define GQCS_TIMEOUT			20 // INFINITE
#define THREAD_QUIT_KEY			9999


class ClientSession;
class IOThread;

struct OverlappedSendContext;
struct OverlappedPreRecvContext;
struct OverlappedRecvContext;
struct OverlappedDisconnectContext;
struct OverlappedAcceptContext;

class IocpManager
{
public:
	IocpManager();
	~IocpManager();

	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	void StartAccept();
	void StartConnect();

	HANDLE GetComletionPort()	{ return mCompletionPort; }
	
	static char mConnectBuf[64];
	static LPFN_CONNECTEX mFnConnectEx;
	static LPFN_DISCONNECTEX mFnDisconnectEx;
	
	static BOOL ConnectEx( SOCKET hSocket, const struct sockaddr *name, int nameLen,
						   PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped );
	static BOOL DisconnectEx( SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved );

private:
	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

private:
	HANDLE	mCompletionPort;
	volatile long	mIoThreadCount;
		
	SOCKET	mListenSocket;
	IOThread* mIoWorkerThread[MAX_IO_THREAD];

	HANDLE*	mThreadHandle = nullptr;
};

extern IocpManager* GIocpManager;