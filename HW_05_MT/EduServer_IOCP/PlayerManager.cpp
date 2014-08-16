#include "stdafx.h"
#include "Player.h"
#include "PlayerManager.h"

PlayerManager* GPlayerManager = nullptr;

PlayerManager::PlayerManager() : mLock(LO_ECONOMLY_CLASS), mCurrentIssueId(0)
{

}

int PlayerManager::RegisterPlayer(std::shared_ptr<Player> player)
{
	FastSpinlockGuard exclusive(mLock);

	mPlayerMap[++mCurrentIssueId] = player;

	return mCurrentIssueId;
}

void PlayerManager::UnregisterPlayer(int playerId)
{
	FastSpinlockGuard exclusive(mLock);

	mPlayerMap.erase(playerId);
}


int PlayerManager::GetCurrentPlayers(PlayerList& outList)
{
	//TODO: mLock을 read모드로 접근해서 outList에 현재 플레이어들의 정보를 담고 total에는 현재 플레이어의 총 수를 반환.

	int total = 0;
	FastSpinlockGuard readMode( mLock, false );
	
	
	for ( auto iter = mPlayerMap.begin(); iter != mPlayerMap.end(); ++iter )
	{
		//outList.reserve( mPlayerMap.size() );
		//outList[total] = iter->second;
		outList.push_back( iter->second );
		++total;
	}

	return total;
}