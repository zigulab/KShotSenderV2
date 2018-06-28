#pragma once

// ServerSocket ��� ����Դϴ�.

class CommBase;
class ServerSocket : public CAsyncSocket
{
public:
	ServerSocket(CommBase *commObj);
	virtual ~ServerSocket();

///////////////////////////////////////////////////////////////////////////////// MEMBER DATA
	CommBase*		_commObj;

	// ���� ����
	char            _sendBuffer[SOCKET_SEND_BUF_SIZE];

	// ���� �ؾ��� ���� ũ��
	int				_toSendBufSize;

	BOOL			_bConnected;

///////////////////////////////////////////////////////////////////////////////// MEMBER DATA
	void			SendChar(TCHAR* buf);
	void			SendBin(byte* buf, int len);

	void			SendBuffer();

	virtual void	OnClose(int nErrorCode);
	virtual void	OnConnect(int nErrorCode);
	virtual void	OnReceive(int nErrorCode);
	virtual void	OnSend(int nErrorCode);

	void FlushBuffer(int sentByte);
};


