#pragma once

// ClientSocket ��� ����Դϴ�.

class Client;

class ClientSocket : public CAsyncSocket
{
public:
	ClientSocket(Client* client);
	virtual ~ClientSocket();

/////////////////////////////////////////////////////////////////////////// DATA

	Client*			_client;

	// ���۹���
	byte			_sendBuffer[SOCKET_SEND_BUF_SIZE];

	// ���� �ؾ��� ���� ũ��
	int				_toSendBufSize;

	// ��������
	BOOL			_bConnected;

	// ���۹��۸� �����ϴ� ������ ��ȣ
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


