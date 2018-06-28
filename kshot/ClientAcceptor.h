#pragma once

#include "ListenSocket.h"

// ClientAcceptor

class ClientAcceptor : public CWinThread
{
	DECLARE_DYNCREATE(ClientAcceptor)

protected:
	ClientAcceptor();           // 동적 만들기에 사용되는 protected 생성자입니다.
	virtual ~ClientAcceptor();

	ClientManager*	_manager;
	ListenSocket*		_socket;

	int						_port;
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	void SetManager(ClientManager*		manager);
	void Init(ClientManager* manager, int port);
	void Uninit();

	void Accept(CAsyncSocket& socket);


protected:
	DECLARE_MESSAGE_MAP()
};


