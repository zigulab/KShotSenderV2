// ListenSocket.cpp : ���� �����Դϴ�.
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


// ListenSocket ��� �Լ�


void ListenSocket::OnAccept(int nErrorCode)
{
	_manager->OnAccept();

	CAsyncSocket::OnAccept(nErrorCode);
}


void ListenSocket::OnClose(int nErrorCode)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	CAsyncSocket::OnClose(nErrorCode);
}


void ListenSocket::OnReceive(int nErrorCode)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	CAsyncSocket::OnReceive(nErrorCode);
}


void ListenSocket::OnSend(int nErrorCode)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	CAsyncSocket::OnSend(nErrorCode);
}