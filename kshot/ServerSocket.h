#pragma once

// ServerSocket 명령 대상입니다.

class CommBase;
class ServerSocket : public CAsyncSocket
{
public:
	ServerSocket(CommBase *commObj);
	virtual ~ServerSocket();

///////////////////////////////////////////////////////////////////////////////// MEMBER DATA
	CommBase*		_commObj;

	// 전송 버퍼
	char            _sendBuffer[SOCKET_SEND_BUF_SIZE];

	// 전송 해야할 버퍼 크기
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


