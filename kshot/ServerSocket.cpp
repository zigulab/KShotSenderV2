// ClientSocket.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "ServerSocket.h"

#include "CommBase.h"

CCriticalSection	g_csSocket;
// ServerSocket

ServerSocket::ServerSocket(CommBase *commObj)
{
	_commObj = commObj;

	_bConnected = FALSE;

	_toSendBufSize = 0;
}

ServerSocket::~ServerSocket()
{
}


// ServerSocket 멤버 함수

void ServerSocket::SendChar(TCHAR* buf)
{
	g_csSocket.Lock();

	int len = strlen(buf);
	
	ASSERT(len > 0);

	len += 1;

	// *********************************************************************************//
	// 이전까지 보낼 버퍼크기(_toSendBufSize) + 현재보낼 버퍼크기(len)이 SOCKET_SEND_BUF_SIZE보다 크면 
	// 더이상 결과를 보낼 수 없다는 것이다.
	// 이것은 클라이언트에서 받아줄 수 없는 상황으므로 강제로 결과를 삭제한다.
	// *********************************************************************************//
	if (_toSendBufSize + len > SOCKET_SEND_BUF_SIZE)
	{
		memset(_sendBuffer, 0, SOCKET_SEND_BUF_SIZE);
		_toSendBufSize = 0;
	}
	else
	{
		memcpy(_sendBuffer + _toSendBufSize, (LPCSTR)buf, len);
		_toSendBufSize += len;
		
		ASSERT(_toSendBufSize > 0);

		if (_bConnected)
			SendBuffer();
	}

	g_csSocket.Unlock();
}

void ServerSocket::SendBin(byte* buf, int len)
{
	g_csSocket.Lock();

	ASSERT(len > 0);

	// *********************************************************************************//
	// 이전까지 보낼 버퍼크기(_toSendBufSize) + 현재보낼 버퍼크기(len)이 SOCKET_SEND_BUF_SIZE보다 크면 
	// 더이상 결과를 보낼 수 없다는 것이다.
	// 이것은 클라이언트에서 받아줄 수 없는 상황으므로 강제로 결과를 삭제한다.
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
			SendBuffer();
	}

	g_csSocket.Unlock();
}

void ServerSocket::SendBuffer()
{
	int dwBytes = 0;
	if((dwBytes = Send(_sendBuffer, _toSendBufSize)) == SOCKET_ERROR)
	{
		int err;
		if((err = GetLastError()) == WSAEWOULDBLOCK) 
		{
			TRACE(_T("send, WSAEWOULDBLOCK\n"));
		}
		else
		{
			Logmsg(CMdlLog::LEVEL_ERROR, _T("ServerSocket::SendBuffer(), Error(%d)"), err);
		}
	}

	if( dwBytes > 0 ) 
		FlushBuffer(dwBytes);
}

void ServerSocket::OnClose(int nErrorCode)
{
	_bConnected = FALSE;
	
	_toSendBufSize = 0;

	_commObj->OnClose(nErrorCode);

	CAsyncSocket::OnClose(nErrorCode);
}


void ServerSocket::OnConnect(int nErrorCode)
{
	_bConnected = TRUE;

	_toSendBufSize = 0;

	_commObj->OnConnect(nErrorCode);

	CAsyncSocket::OnConnect(nErrorCode);
}


void ServerSocket::OnReceive(int nErrorCode)
{
	char  TBuf[SOCKET_RECEIVE_BUF_SIZE] = {0};

	int nRead = Receive(TBuf, SOCKET_RECEIVE_BUF_SIZE);

	if(nRead == SOCKET_ERROR)
	{   
		int err;
		if( (err=GetLastError()) == WSAEWOULDBLOCK) 
			TRACE(_T("ServerSocket::OnReceive(), socket WSAWOULDBLOCK\n"));
		else
			Logmsg(CMdlLog::LEVEL_ERROR, _T("ServerSocket::OnReceive(), SOCKET_ERROR(%d)"), err);
	}
	else if(nRead == 0)  //연결종료
	{
		// _commObj
		Log(CMdlLog::LEVEL_ERROR, _T("ServerSocket::OnReceive() 서버로 부터 연결이 끊김."));
	}
	else
	{
		_commObj->OnReceive(TBuf, nRead);
	}


	CAsyncSocket::OnReceive(nErrorCode);
}


void ServerSocket::OnSend(int nErrorCode)
{
	g_csSocket.Lock();

	int sentByte = 0;
	while(sentByte < _toSendBufSize)
	{
		int dwBytes;
		if((dwBytes = Send((LPCSTR)_sendBuffer + sentByte, 
								_toSendBufSize - sentByte)) == SOCKET_ERROR)
		{
			int err;
			if((err = GetLastError()) == WSAEWOULDBLOCK) 
			{
				TRACE(_T("ServerSocket::OnSend(), WSAEWOULDBLOCK\n"));
				break;
			}
			else
			{
				Logmsg(CMdlLog::LEVEL_ERROR, _T("ServerSocket::OnSend(), error %d"), err);
				break;
			}
		}
		else
		{
			sentByte += dwBytes;
		}
	}

	if( sentByte > 0 ) 
		FlushBuffer(sentByte);

	g_csSocket.Unlock();

	CAsyncSocket::OnSend(nErrorCode);
}


void ServerSocket::FlushBuffer(int sentByte)
{
	if (sentByte > _toSendBufSize) {
		Logmsg(CMdlLog::LEVEL_ERROR, _T("ServerSocket::FlushBuffer(), error(sentByte:%d, _toSendBufSize:%d)"), sentByte, _toSendBufSize);
		ASSERT(0);
	}

	_toSendBufSize -= sentByte;

	if( _toSendBufSize == 0 ) 
		memset(_sendBuffer, 0, SOCKET_SEND_BUF_SIZE);
	else 
	{
		char *tempbuf = _sendBuffer+sentByte;
		memcpy(_sendBuffer, tempbuf, _toSendBufSize);
		memset(_sendBuffer+_toSendBufSize, 0, SOCKET_SEND_BUF_SIZE-_toSendBufSize);
	}
}