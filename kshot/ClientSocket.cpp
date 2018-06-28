// ClientSocket.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "kshot.h"
#include "ClientSocket.h"

#include "Client.h"


// ClientSocket

ClientSocket::ClientSocket(Client* client)
{
	_client = client;

	_toSendBufSize = 0;

	_bConnected = FALSE;
}

ClientSocket::~ClientSocket()
{
    
}


// ClientSocket ��� �Լ�


void ClientSocket::OnAccept(int nErrorCode)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	CAsyncSocket::OnAccept(nErrorCode);
}


void ClientSocket::OnClose(int nErrorCode)
{
	_bConnected = FALSE;

	_toSendBufSize = 0;

	_client->OnClose(nErrorCode);

	CAsyncSocket::OnClose(nErrorCode);
}

void ClientSocket::OnConnectByAccept()
{
	_bConnected = TRUE;

	_toSendBufSize = 0;
}

void ClientSocket::OnConnect(int nErrorCode)
{
	// ���� ȣ����� �ʴ´�.
	_bConnected = TRUE;

	_toSendBufSize = 0;

	CAsyncSocket::OnConnect(nErrorCode);
}


void ClientSocket::OnReceive(int nErrorCode)
{
	char  buf[SOCKET_RECEIVE_BUF_SIZE] = {0};

	int nRead = Receive(buf, SOCKET_RECEIVE_BUF_SIZE);

	if(nRead == SOCKET_ERROR)
	{   
		int err;
		if( (err = GetLastError()) == WSAEWOULDBLOCK ) 
			TRACE(_T("ClientSocket::OnReceive() WSAEWOULDBLOCK\n"));
		else 
			Logmsg(CMdlLog::LEVEL_ERROR, _T("ClientSocket::OnReceive(), ClientSocket::OnReceive() SOCKET_ERROR(%d), (client_id:%s)"), err, _client->_id);
	}
	else if(nRead == 0)  //��������
	{
		Logmsg(CMdlLog::LEVEL_DEBUG, _T("ClientSocket::OnReceive(), Client�� ������ ����.(client_id:%s)"), _client->_id);
	}
	else
	{
		/*
		TRACE(_T("receviced data: %d byte, %d,%d\n"), nRead, buf[0],buf[1]);
		if( buf[0] ==  -52 && buf[1] == -52 )
			TRACE(_T("Invalid Data"));

		memcpy(_tempbuf+_tempbufsize, buf, nRead);

		_tempbufsize += nRead;

		int			id;
		short int	len;

		memcpy((short int *)&len, _tempbuf+2, 2);
		memcpy((int *)&id, _tempbuf+4, 4);

		//*msgId = ntohl(id);
		len = ntohs(len);

		while( len <= _tempbufsize ) 
		{
			TRACE(_T("MESSAGEó��: %d byte"), len);

			_tempbufsize -= len;

			TRACE(_T("���� ���� %d, ������ġ:%d,%d\n"), _tempbufsize, _tempbuf[len], _tempbuf[len+1]);
		}
		*/

		_client->OnReceive((byte*)buf, nRead);
	}

	CAsyncSocket::OnReceive(nErrorCode);
}


void ClientSocket::OnSend(int nErrorCode)
{
	_cs.Lock();
	
	//int would_retry = 0;

	int sentByte = 0;
	while(sentByte < _toSendBufSize)
	{
		int dwBytes = 0;
		if((dwBytes = Send((LPCSTR)_sendBuffer + sentByte, 
								_toSendBufSize - sentByte)) == SOCKET_ERROR)
		{
			int err;
			if( (err = GetLastError()) == WSAEWOULDBLOCK ) 
			{
				TRACE(_T("ClientSocket::OnSend(), WSAEWOULDBLOCK\n"));
				//would_retry++;
				//if (would_retry > 3)
				//	break;

				//Sleep(10);
				break;
			}
			else
			{
				Logmsg(CMdlLog::LEVEL_ERROR, _T("ClientSocket::OnSend(), SOCKET_ERROR(%d), (client_id:%s)"), err, _client->_id);
				break;
			}
		}
		else
		{
			sentByte += dwBytes;
			//would_retry = 0;
		}
	}

	if (sentByte > 0) 
	{
		FlushBuffer(sentByte);
	}

	_cs.Unlock();

	CAsyncSocket::OnSend(nErrorCode);
}



void ClientSocket::SendBuffer(TCHAR* buf)
{
	_cs.Lock();

	int len = _tcslen(buf) + 1;
	
	memcpy(_sendBuffer + _toSendBufSize,(LPCSTR)buf, len);
    _toSendBufSize += len;

	if( _bConnected )
		SendInternal();

	_cs.Unlock();
}

void ClientSocket::SendBuffer(byte* buf, int len)
{
	_cs.Lock();

	ASSERT(len > 0);

	// *********************************************************************************//
	// �������� ���� ����ũ��(_toSendBufSize) + ���纸�� ����ũ��(len)�� SOCKET_SEND_BUF_SIZE���� ũ�� 
	// ���̻� ����� ���� �� ���ٴ� ���̴�.
	// �̰��� Ŭ���̾�Ʈ���� �޾��� �� ���� ��Ȳ���Ƿ� ������ ����� �����Ѵ�.
	// *********************************************************************************//
	if (_toSendBufSize + len > SOCKET_SEND_BUF_SIZE)
	{
		memset(_sendBuffer, 0, SOCKET_SEND_BUF_SIZE);
		_toSendBufSize = 0;
	}
	else 
	{
		memcpy(_sendBuffer + _toSendBufSize, buf, len);
		_toSendBufSize += len;

		ASSERT(_toSendBufSize > 0);

		if (_bConnected)
			SendInternal();
	}

	_cs.Unlock();
}

void ClientSocket::SendInternal()
{
	int dwBytes = 0;
	if((dwBytes = Send(_sendBuffer, _toSendBufSize)) == SOCKET_ERROR)
	{
		int err;
		if((err = GetLastError()) == WSAEWOULDBLOCK ) 
			TRACE(_T("ClientSocket::SendInternal(), WSAEWOULDBLOCK(_toSendBufSize=%d)\n"), _toSendBufSize);
		else
			Logmsg(CMdlLog::LEVEL_ERROR, _T("ClientSocket::SendInternal(), SOCKET_ERROR(%d), (client_id:%s)"), err, _client->_id);
	}

	if( dwBytes > 0 ) 
		FlushBuffer(dwBytes);
}

void ClientSocket::FlushBuffer(int sentByte)
{
	if (sentByte > _toSendBufSize) {
		Logmsg(CMdlLog::LEVEL_ERROR, _T("ClientSocket::FlushBuffer(), error(sentByte:%d, _toSendBufSize:%d)"), sentByte, _toSendBufSize);
		ASSERT(0);
	}

	_toSendBufSize -= sentByte;

	if( _toSendBufSize == 0 ) 
		memset(_sendBuffer, 0, SOCKET_SEND_BUF_SIZE);
	else 
	{
		byte *tempbuf = _sendBuffer+sentByte;
		memcpy(_sendBuffer, tempbuf, _toSendBufSize);
		memset(_sendBuffer+_toSendBufSize, 0, SOCKET_SEND_BUF_SIZE-_toSendBufSize);
	}

}