// ClientAcceptor.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "ClientAcceptor.h"
#include "ClientManager.h"

// ClientAcceptor

IMPLEMENT_DYNCREATE(ClientAcceptor, CWinThread)

ClientAcceptor::ClientAcceptor()
{
	_manager = NULL;
	_socket = NULL;

}

ClientAcceptor::~ClientAcceptor()
{
}

BOOL ClientAcceptor::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	_socket = new ListenSocket(_manager);

	if (!_socket->Create(_port)) {
		Log(CMdlLog::LEVEL_ERROR, _T("KSHOTSender Lintening Socket 생성 실패"));
		return false;
	}

	Log(CMdlLog::LEVEL_EVENT, _T("ClietManager Linten Socket, Listening..."));

	_socket->Listen();

	return TRUE;
}

int ClientAcceptor::ExitInstance()
{
	_socket->ShutDown();
	_socket->Close();

	delete _socket;

	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(ClientAcceptor, CWinThread)
END_MESSAGE_MAP()


// ClientAcceptor 메시지 처리기입니다.

void ClientAcceptor::SetManager(ClientManager*		manager)
{
	_manager = manager;
}

void ClientAcceptor::Init(ClientManager*	manager, int port)
{
	_manager = manager;
	_port = port;
}

void ClientAcceptor::Uninit()
{
	PostThreadMessage(WM_QUIT, 0, 0);

	Log(CMdlLog::LEVEL_EVENT, _T("ClientAcceptor, _socket close()"));
}

void ClientAcceptor::Accept(CAsyncSocket& socket)
{
	_socket->Accept(socket);
}