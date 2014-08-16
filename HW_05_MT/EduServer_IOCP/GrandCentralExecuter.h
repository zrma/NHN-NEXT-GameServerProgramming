#pragma once
#include "Exception.h"
#include "TypeTraits.h"
#include "XTL.h"
#include "ThreadLocal.h"

class GrandCentralExecuter
{
public:
	typedef std::function<void()> GCETask;

	GrandCentralExecuter(): mRemainTaskCount(0)
	{}

	void DoDispatch(const GCETask& task)
	{
		CRASH_ASSERT(LThreadType == THREAD_IO_WORKER); ///< 일단 IO thread 전용

		
		if (InterlockedIncrement64(&mRemainTaskCount) > 1)
		{
			//TODO: 이미 누군가 작업중이면 어떻게?
			mCentralTaskQueue.push( task );
		}
		else
		{
			/// 처음 진입한 놈이 책임지고 다해주자 -.-;

			mCentralTaskQueue.push(task);
			
			while (true)
			{
				GCETask task;
				//concurrent_queue에 대한 설명 참고
				//http://daniel00.tistory.com/43
				if (mCentralTaskQueue.try_pop(task))
				{
					//TODO: task를 수행하고 mRemainTaskCount를 하나 감소 
					// mRemainTaskCount가 0이면 break;
					task();
					
					if ( InterlockedDecrement64( &mRemainTaskCount ) == 0 )
					{
						break;
					}
				}
			}
		}

	}


private:
	typedef concurrency::concurrent_queue<GCETask, STLAllocator<GCETask>> CentralTaskQueue;
	CentralTaskQueue mCentralTaskQueue;
	int64_t mRemainTaskCount;
};

extern GrandCentralExecuter* GGrandCentralExecuter;

//호출
// GCEDispatch( playerEvent, &AllPlayerBuffEvent::DoBuffToAllPlayers, mPlayerId );
// GCEDispatch( playerBuffDecayEvent, &AllPlayerBuffDecay::CheckBuffTimeout );

template <class T, class F, class... Args>
void GCEDispatch(T instance, F memfunc, Args&&... args)
{
	/// shared_ptr이 아닌 녀석은 받으면 안된다. 작업큐에 들어있는중에 없어질 수 있으니..
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");

	//TODO: intance의 memfunc를 std::bind로 묶어서 전달
	GGrandCentralExecuter->DoDispatch( std::bind( memfunc, instance, std::forward<Args>( args )... ) );

	//GGrandCentralExecuter->DoDispatch(bind);
}