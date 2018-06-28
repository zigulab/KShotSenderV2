#pragma once

// ClientSocket 명령 대상입니다.

class Client;

class ClientSocket : public CAsyncSocket
{
public:
	ClientSocket(Client* client);
	virtual ~ClientSocket();

/////////////////////////////////////////////////////////////////////////// DATA

	Client*			_client;

	// 전송버퍼
	byte			_sendBuffer[SOCKET_SEND_BUF_SIZE];

	// 전송 해야할 버퍼 크기
	int				_toSendBufSize;

	// 연결유무
	BOOL			_bConnected;

	// 전송버퍼를 정리하는 영역을 보호
	CCriticalSection _cs;

/////////////////////////////////////////////////////////////////////////// METHOD

	virtual void OnAccept(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);

	void OnConnectByAccept();

	void SendBuffer(TCHAR* buf);
	void SendBuffer(byte* buf, int len);
	void SendInternal();

	void FlushBuffer(int sentByte);
};


