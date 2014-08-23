#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "FastSpinlock.h"
#include "LockOrderChecker.h"

LockOrderChecker::LockOrderChecker(int tid) : mWorkerThreadId(tid), mStackTopPos(0)
{
	memset(mLockStack, 0, sizeof(mLockStack));
}

void LockOrderChecker::Push(FastSpinlock* lock)
{
	CRASH_ASSERT(mStackTopPos < MAX_LOCK_DEPTH);

	if (mStackTopPos > 0)
	{
		/// 현재 락이 걸려 있는 상태에 진입한경우는 반드시 이전 락의 우선순위가 높아야 한다.
		//TODO: 그렇지 않은 경우 CRASH_ASSERT gogo
		//CRASH_ASSERT( mLockStack[mStackTopPos - 1]->mLockOrder <= lock->mLockOrder );
		CRASH_ASSERT(mLockStack[mStackTopPos - 1]->mLockOrder < lock->mLockOrder); ///# 등호 빠져야지 by sm9
		
	}

	mLockStack[mStackTopPos++] = lock;
}

void LockOrderChecker::Pop(FastSpinlock* lock)
{

	/// 최소한 락이 잡혀 있는 상태여야 할 것이고
	CRASH_ASSERT(mStackTopPos > 0);
	
	//TODO: 당연히 최근에 push했던 녀석이랑 같은지 체크.. 틀리면 CRASH_ASSERT
	//stack을 놓고 stack에서 넣었다가 빼는 것
	//push와 pop이 쌍으로 되어야 함
	CRASH_ASSERT( mLockStack[mStackTopPos-1] == lock );

	//전항으로 돌리면서 동시에 nullptr 입력
	mLockStack[--mStackTopPos] = nullptr;

}