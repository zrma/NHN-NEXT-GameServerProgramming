#pragma once

#define MAX_IO_THREAD	4

enum THREAD_TYPE
{
	THREAD_MAIN,
	THREAD_IO_WORKER,
	THREAD_DB_WORKER
};


class Timer;
class LockOrderChecker;

//TLS를 활용하는 것
//TLS에 각각의 정보를 담아서 사용하는 것입니다.
//http://kimsk99.blog.me/50002386480 참고

extern __declspec(thread) int LThreadType;
extern __declspec(thread) int LIoThreadId;
extern __declspec(thread) Timer* LTimer;
extern __declspec(thread) int64_t LTickCount;
extern __declspec(thread) LockOrderChecker* LLockOrderChecker;

