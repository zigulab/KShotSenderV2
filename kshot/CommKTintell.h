#pragma once

#include "CommBase.h"

#include "Intelli_PacketData.h"

#include "ServerSocket.h"

// 수신 버퍼 1M
#define INTELLI_RECEIVE_MAX_SIZE (1024*1024*1)

class CommKTintell : public CommBase
{
public:
	CommKTintell(CommManager *manager);
	virtual ~CommKTintell(void);

///////////////////////////////////////////////////////////////////////////////// MEMBER DATA

	// grant 서버로 권한 획득을 위해 사용
	ServerSocket	_grant_socket;

	CString			_grantIP;
	CString			_grantPort;
	CString			_id;
	CString			_password;

	CString			_bindServerIP;
	int				_bindServerPort;
	CString			_bindId;

	CTime			_lastLinkCheckTime;

	typedef enum RUN_STATE
	{
		STATE_READY,
		STATE_GRANTING,
		STATE_BINDING,
		STATE_BINDED,
	}RUN_STATE;

	RUN_STATE				_run_state;


	struct SMS_GRANT		_pkt_grant;
    struct SMS_GRANTACK		_pkt_grantack;

	struct SMS_BIND			_pkt_bind;
	struct SMS_BINDACK		_pkt_bindack;
		
	struct SMS_DELIVER		_pkt_deliver;
	struct SMS_DELIVERACK	_pkt_deliverack;

	struct SMS_REPORT		_pkt_report;
    struct SMS_REPORTACK	_pkt_reportack;

	struct  SMS_LINESEND	_pkt_linesend;
    struct  SMS_LINESENDACK	_pkt_linesendack;

	char					_receiveBuf[INTELLI_RECEIVE_MAX_SIZE];
	int						_receiveBufSize;

	BOOL					_isBind;

///////////////////////////////////////////////////////////////////////////////// METHOD
/////////////// INTERFACE

	virtual BOOL	Init();
	virtual BOOL	Uninit();

	virtual BOOL	Connect();

	virtual void	Ping(int tid);

	virtual BOOL	SendSMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg);
	virtual BOOL	SendLMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg);
	virtual BOOL	SendMMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size);
	virtual BOOL	SendFAX(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size);


	virtual void	OnClose(int nErrorCode);
	virtual void	OnConnect(int nErrorCode);

	virtual void	OnSend(int nErrorCode);

	virtual void	OnReceive(TCHAR *buf, int size);

/////////////// GENERAL
	bool			ProcessReceive(BYTE* buf);
	void			AttachToReceiveBuf(BYTE* buf, int len);
	void			DetachFromReceiveBuf(int detachLen, int restLen);
	int				AnalysisPacket(int* msgId, short int* msglen);
	
	// grant 서버 연결
	BOOL			ConnectGrantServer();

	// bind 서버 연결
	BOOL			ConnectBindServer();

	void			GrantReq();
	void			BindReq();
	void			LinkReq();
	void			LinkAck(int request_id);
	void			ReportAck(int request_id);

	// 서버 응답 분석
	int				ResponseAnalysis(char * resbuf, int *msgid);
};

