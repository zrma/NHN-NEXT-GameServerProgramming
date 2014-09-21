#pragma once

#define MAX_IO_THREAD	4

enum THREAD_TYPE
{
	THREAD_MAIN,
	THREAD_IO_WORKER,
	THREAD_DB_WORKER
};

class ThreadCallHistory;
class ThreadCallElapsedRecord;

extern __declspec(thread) int LThreadType;
extern __declspec(thread) int LIoThreadId;

extern __declspec( thread ) ThreadCallHistory* LThreadCallHistory;
extern __declspec( thread ) ThreadCallElapsedRecord* LThreadCallElapsedRecord;
extern __declspec( thread ) void* LRecentThisPointer;

extern ThreadCallHistory* GThreadCallHistory[MAX_IO_THREAD];
extern ThreadCallElapsedRecord* GThreadCallElapsedRecord[MAX_IO_THREAD];