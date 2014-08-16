#pragma once

#include "TypeTraits.h"
#include "FastSpinlock.h"
#include "Timer.h"


class SyncExecutable : public std::enable_shared_from_this<SyncExecutable>
{
public:
	SyncExecutable() : mLock(LO_BUSINESS_CLASS)
	{}
	virtual ~SyncExecutable() {}

	//호출 코드
	//mPlayer->DoSync(&Player::PlayerReset);

	template <class R, class T, class... Args>
	R DoSync(R (T::*memfunc)(Args...), Args... args)
	{
		//예외 조건 평가를 컴파일 타임에서 확인해 버그를 잡아 내도록 함
		//http://frompt.egloos.com/viewer/2742204
		//SyncExecutable을 상속 받은 녀석만 사용 가능하도록 함
		static_assert(true == std::is_convertible<T, SyncExecutable>::value, "T should be derived from SyncExecutable");

		//TODO: mLock으로 보호한 상태에서, memfunc를 실행하고 결과값 R을 리턴
		FastSpinlockGuard criticalSection( mLock );
		
		//대입은 될리가 없었다...
		//auto temp = std::bind( memfunc, static_cast<T*>( this ), std::forward<Args>( args )... )( );
		//std::bind( memfunc, static_cast<T*>( this ), std::forward<Args>( args )... )( );

		return std::bind( memfunc, static_cast<T*>( this ), std::forward<Args>( args )... )( );
	}
	
	void EnterLock() { mLock.EnterWriteLock(); }
	void LeaveLock() { mLock.LeaveWriteLock(); }
	
	//호출코드
	//mPlayerId = GPlayerManager->RegisterPlayer(GetSharedFromThis<Player>());

 	template <class T>
	std::shared_ptr<T> GetSharedFromThis()
 	{
		static_assert(true == std::is_convertible<T, SyncExecutable>::value, "T should be derived from SyncExecutable");
 		
		//TODO: this 포인터를 std::shared_ptr<T>형태로 반환.
		//(HINT: 이 클래스는 std::enable_shared_from_this에서 상속받았다. 그리고 static_pointer_cast 사용)

		//return std::shared_ptr<T>((Player*)this); ///< 이렇게 하면 안될걸???
		//바로 this로 반환하게 되면 포인터 2번 삭제 오류 발생
		//weak pointer로 넘겨줘야 하는데 그 방법이 enable_shared_from_this의 shared_from_this()를 사용하는 것
		//자세한 설명은 링크
		//https://www.evernote.com/shard/s335/sh/ff59ace2-9cea-42ae-8307-e881c1df5edc/f7e65e4901b33fe3a9cf26cd6dcc3244
		//return static_pointer_cast<T>( T::shared_from_this() );
		return std::static_pointer_cast<T>( shared_from_this() );
 	}

private:

	FastSpinlock mLock;
};

//호출문 예제 DoSyncAfter(10, mPlayer, &Player::Start, 1000);

template <class T, class F, class... Args>
void DoSyncAfter(uint32_t after, T instance, F memfunc, Args&&... args)
{
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");
	static_assert(true == std::is_convertible<T, std::shared_ptr<SyncExecutable>>::value, "T should be shared_ptr SyncExecutable");

	//TODO: instance의 memfunc를 bind로 묶어서 LTimer->PushTimerJob() 수행
	//std::bind는 functor로 묶어주는 함수였지요
	//http://la-stranger.tistory.com/32

	//std::forward()는 원래 좌측값인 것은 좌측값으로, 원래 우측값인 것은 우측값으로 캐스킨 해주는 역할
	//http://frompt.egloos.com/viewer/2765424
	
	//함수 호출성 개체는 두 종류가 있다
	//우리가 흔히 사용하는 전역 함수 포인터 / 멤버 함수 포인터
	//여기서 사용하는 것은 멤버 함수 포인터기 때문에 2번째 인자로 객체가 들어간다
	//http://en.cppreference.com/w/cpp/utility/functional/bind

	LTimer->PushTimerJob( instance, std::bind( memfunc, instance, std::forward<Args>( args )... ), after );
}