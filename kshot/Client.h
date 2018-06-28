#pragma once



// Client

#include "ClientSocket.h"

#include "msgPacket.h"

class SendManager;
class ClientManager;

// 기본 수신 버퍼 1M
#define CLIENT_RECEIVE_MAX_SIZE (1024*1024*1)

class Client : public CWinThread
{
	DECLARE_DYNCREATE(Client)

protected:
	Client();           // 동적 만들기에 사용되는 protected 생성자입니다.
	virtual ~Client();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();


protected:
	DECLARE_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////// DATA

public:
	// 부여받은 번호
	int					_no;

	// 사용유무
	bool				_bUse;

	// BIND 유무
	bool				_bBind;

	// 로그인(BIND)성공시 부여됨.
	CString				_id;

	// 마지막 체크 시간
	CTime				_lastLinkCheckTime;
	// 마지막 전송 시간
	CTime				_lastSendTime;

	ClientManager		*_manager;

	// 현재 일자
	static CString			_curDate;
	static CString			_curDateAck;

	// 전송원시데이타 저장
	static CStdioFile		_sendLogFile;

	// reportAck 저장
	static CStdioFile		_reportAckFile;

protected:

	// 각 패킷데이타
	PKT_LINKCHK			_pkt_linkchk;
	PKT_LINKCHK_ACK		_pkt_linkchk_ack;
	PKT_GRANT			_pkt_grant;
	PKT_GRANT_ACK		_pkt_grant_ack;
	PKT_BIND			_pkt_bind;
	PKT_BIND_ACK		_pkt_bind_ack;
	PKT_MESSAGE			_pkt_message;
	PKT_MESSAGE_ACK		_pkt_message_ack;
	PKT_REPORT			_pkt_report;
	PKT_REPORT_ACK		_pkt_report_ack;


public:
	//통신 담당 소켓
	ClientSocket		_socket;

	// 발송 관리
	SendManager*		_sendManager;			

	struct CLIENT_SEND_INFO
	{
		CString send_number;
		CString receiveNo;
		int doneflag;			// 1: done
	};

	vector<CLIENT_SEND_INFO*>	_vtSend;

	// 수신 버퍼
	//char				_receiveBuf[CLIENT_RECEIVE_MAX_SIZE];
	char*				_receiveBuf;

	// 수신버퍼의 현재 위치
	int					_receiveBufSize;

	// 할당된 수신버퍼 크기(재할당된 크기)
	int					_assigned_size;


/////////////////////////////////////////////////////////////////////////// METHOD
public:
	// 사용중인지 확인
	virtual	bool	isUse();

	// USER 변경
	virtual void	SetUse(bool b);

	// NO설정
	virtual void	SetNo(int no);
	virtual void	SetParent(ClientManager *manager);

	// 서버로 부터 연결종료
	virtual void	OnClose(int nErrorCode);

	// 수신
	virtual void	OnReceive(byte* buf, int len);

	bool ProcessReceive(BYTE* buf);
	void AttachToReceiveBuf(BYTE* buf, int len);
	void DetachFromReceiveBuf(int detachLen, int restLen);
	int AnalysisPacket(int* msgId, int* msglen);

	void Send_LinkChk();
	void Get_LinkChkAck();
	void Send_LinkChkAck();

	void ParseGrant(BYTE* buf, int len);
	void Send_GrantAck(int result, char* bind_id, char* serverIP, int port);

	void Parse_Bind(BYTE* buf, int len);
	void Send_BindAck(int result, int maxofsec);

	void ParseMessage(BYTE* buf, int len);
	void Send_MessageAck(int result, int send_number);

	void ParseReportAck(BYTE* buf, int len);
	void Send_Report(int result, int send_number, char* target, char* done_time, int telecom);

	afx_msg void OnNewConnection(WPARAM wParam, LPARAM lParam);
	afx_msg void OnCloseForNoReply(WPARAM wParam, LPARAM lParam);

	// 윈시데이타 기록
	static void WriteSendRawData(CString formatedMsg);

	// report_ack 기록
	static void WriteResultAck(CString id, CString ack_list);

	// 예약일자형식 체크
	bool	CheckInvalidDateFormat(char* dateFormat, COleDateTime *oleTime);
};


