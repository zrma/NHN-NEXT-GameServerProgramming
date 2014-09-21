#pragma once

class IOThread
{
public:
	IOThread( HANDLE hThread, HANDLE hCompletionPort );
	~IOThread();

	DWORD Run();

	void DoIocpJob();
	void DoSendJob();

	HANDLE GetHandle() { return mThreadHandle; }

private:
	bool	mIsContinue = true;

	HANDLE mThreadHandle;
	HANDLE mCompletionPort;
};