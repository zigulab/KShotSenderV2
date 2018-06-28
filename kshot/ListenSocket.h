#pragma once

// ListenSocket 명령 대상입니다.

class ClientManager;
class ListenSocket : public CAsyncSocket
{
public:
	ListenSocket(ClientManager* _manager);
	virtual ~ListenSocket();

/////////////////////////////////////////////////////////////////////////// DATA

	ClientManager* _manager;


/////////////////////////////////////////////////////////////////////////// METHOD

	virtual void OnAccept(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);
};


