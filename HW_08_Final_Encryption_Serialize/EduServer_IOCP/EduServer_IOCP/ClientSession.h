#pragma once

#include "Session.h"
#include "Player.h"
#include "mypacket.pb.h"

class ClientSessionManager;
class Player;

class ClientSession : public Session, public ObjectPool<ClientSession>
{
public:
	ClientSession();
	virtual ~ClientSession();

	void SessionReset();

	bool PostAccept();
	void AcceptCompletion();
	
	virtual bool SendRequest( short packetType, const google::protobuf::MessageLite& payload );
	virtual void OnReceive( size_t len );
	virtual void OnDisconnect(DisconnectReason dr);
	virtual void OnRelease();

	
	
public:
	Player			mPlayer;

private:
	
	SOCKADDR_IN		mClientAddr ;

	
	friend class ClientSessionManager;
} ;



