#include "StdAfx.h"
#include "CommBase.h"


CommBase::CommBase(CommManager *manager)
	: _socket(this)
{
	_manager = manager;
}


CommBase::~CommBase(void)
{
}

BOOL CommBase::Uninit() 
{
	_socket.ShutDown();
	_socket.Close();

	Log(CMdlLog::LEVEL_EVENT, _T("CommBase, 서버 소켓 close()"));

	return TRUE;
}

/*
BOOL CommBase::Init()
{
	return TRUE;
}

BOOL CommBase::Connect()
{
	return TRUE;
}

BOOL CommBase::Login()
{
	return TRUE;
}

BOOL CommBase::Send(byte *msg, int len)
{
	return TRUE;
}

BOOL CommBase::Receive(byte *msg, int len)
{
	return TRUE;
}
*/