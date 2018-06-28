#pragma once



// Client

#include "ClientSocket.h"

#include "msgPacket.h"

class SendManager;
class ClientManager;

// �⺻ ���� ���� 1M
#define CLIENT_RECEIVE_MAX_SIZE (1024*1024*1)

class Client : public CWinThread
{
	DECLARE_DYNCREATE(Client)

protected:
	Client();           // ���� ����⿡ ���Ǵ� protected �������Դϴ�.
	virtual ~Client();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();


protected:
	DECLARE_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////// DATA

public:
	// �ο����� ��ȣ
	int					_no;

	// �������
	bool				_bUse;

	// BIND ����
	bool				_bBind;

	// �α���(BIND)������ �ο���.
	CString				_id;

	// ������ üũ �ð�
	CTime				_lastLinkCheckTime;
	// ������ ���� �ð�
	CTime				_lastSendTime;

	ClientManager		*_manager;

	// ���� ����
	static CString			_curDate;
	static CString			_curDateAck;

	// ���ۿ��õ���Ÿ ����
	static CStdioFile		_sendLogFile;

	// reportAck ����
	static CStdioFile		_reportAckFile;

protected:

	// �� ��Ŷ����Ÿ
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
	//��� ��� ����
	ClientSocket		_socket;

	// �߼� ����
	SendManager*		_sendManager;			

	struct CLIENT_SEND_INFO
	{
		CString send_number;
		CString receiveNo;
		int doneflag;			// 1: done
	};

	vector<CLIENT_SEND_INFO*>	_vtSend;

	// ���� ����
	//char				_receiveBuf[CLIENT_RECEIVE_MAX_SIZE];
	char*				_receiveBuf;

	// ���Ź����� ���� ��ġ
	int					_receiveBufSize;

	// �Ҵ�� ���Ź��� ũ��(���Ҵ�� ũ��)
	int					_assigned_size;


/////////////////////////////////////////////////////////////////////////// METHOD
public:
	// ��������� Ȯ��
	virtual	bool	isUse();

	// USER ����
	virtual void	SetUse(bool b);

	// NO����
	virtual void	SetNo(int no);
	virtual void	SetParent(ClientManager *manager);

	// ������ ���� ��������
	virtual void	OnClose(int nErrorCode);

	// ����
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

	// ���õ���Ÿ ���
	static void WriteSendRawData(CString formatedMsg);

	// report_ack ���
	static void WriteResultAck(CString id, CString ack_list);

	// ������������ üũ
	bool	CheckInvalidDateFormat(char* dateFormat, COleDateTime *oleTime);
};


