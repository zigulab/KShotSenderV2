// ListenSocket.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "ListenSocket.h"

#include "ClientManager.h"


// ListenSocket

ListenSocket::ListenSocket(ClientManager* manager)
{
	_manager = manager;
}

ListenSocket::~ListenSocket()
{
}


// ListenSocket 멤버 함수


void ListenSocket::OnAccept(int nErrorCode)
{
	_manager->OnAccept();

	CAsyncSocket::OnAccept(nErrorCode);
}


void ListenSocket::OnClose(int nErrorCode)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CAsyncSocket::OnClose(nErrorCode);
}


void ListenSocket::OnReceive(int nErrorCode)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CAsyncSocket::OnReceive(nErrorCode);
}


void ListenSocket::OnSend(int nErrorCode)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CAsyncSocket::OnSend(nErrorCode);
}