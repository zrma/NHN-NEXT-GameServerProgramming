#include "stdafx.h"
#include "ThreadLocal.h"
#include "Exception.h"
#include "SyncExecutable.h"
#include "Timer.h"



Timer::Timer()
{
	LTickCount = GetTickCount64();
}

//호출코드: LTimer->PushTimerJob( instance, std::bind( memfunc, std::forward<Args>( args )... ), after );

void Timer::PushTimerJob(SyncExecutablePtr owner, const TimerTask& task, uint32_t after)
{
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);

	//TODO: mTimerJobQueue에 TimerJobElement를 push..

	// LTickCount는 해당 IOCP Worker Thread에서 매 주기마다
	// DoTimerJob()을 수행 할 때 갱신하는 대표적인 표준 시간의 개념이다.

	int64_t timeStamped = after + LTickCount; ///# LTickCount 사용하는게 의도 by sm9
	
	TimerJobElement temp(owner, task, timeStamped);
	mTimerJobQueue.push( temp );
}


void Timer::DoTimerJob()
{
	/// thread tick update
	LTickCount = GetTickCount64();

	while (!mTimerJobQueue.empty())
	{
		//제일 우선 되는 것을 가져오지만 빼는 것은 아니다
		const TimerJobElement& timerJobElem = mTimerJobQueue.top(); 

		if (LTickCount < timerJobElem.mExecutionTick)
			break;

		
		timerJobElem.mOwner->EnterLock();
		
		const TimerJobElement& timerJobElem2 = mTimerJobQueue.top();

		timerJobElem.mTask();

		const TimerJobElement& timerJobElem3 = mTimerJobQueue.top();

		// timerJobElem.mOwner->LeaveLock();
		timerJobElem3.mOwner->LeaveLock();

		//처리 했으니깐 제일 우선 되는 것을 제거
		mTimerJobQueue.pop();
	}


}

