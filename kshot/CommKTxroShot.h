#pragma once

#include "ServerSocket.h"

#include "CommBase.h"

#include <map>

#include "tinyxml2/tinyxml2.h"

class XroshotSendInfo;

// ���� ���� 1M
#define XROSHOT_RECEIVE_MAX_SIZE	(1024*1024*1)

// MMS���� �ִ� ũ�� 5M
#define	MMS_MAX_SIZE				(1024*1024*5)

class CommKTxroShot : public CommBase
{
public:
	CommKTxroShot(CString id, CString pwd, CString certFile, CString serverUrl, CommManager *manager);
	virtual ~CommKTxroShot(void);


///////////////////////////////////////////////////////////////////////////////// MEMBER DATA

	// ��ȣȭ Ű
	unsigned char		_secretKeySHA1[20];

	// ���� ����
	CString				_serverInfo;
	CString				_serverIP;
	CString				_serverPort;

	// ���� ����
	CString				_center_url;
	CString				_id;
	CString				_password;
	CString				_certFile;

	// �����ð�
	CString				_serverTime;

	// ��ȣȭ�� AuthTicket
	//std::string		base64encodedciphertext;
	std::string			_encryptAuthTicket;

	// ���� ���� ����
	BOOL				_isConnnted;

	// �α��� ����
	BOOL				_isLogin;

	// ping xml����Ÿ, �ֱ������� �߼��ϹǷ� ���ۿ� ����
	TCHAR				_xmlPingBuf[256];

	BOOL				_activate;
	CString			_activateDate;

	BOOL				_bNonePingPacket;
		

	// �޼��� Ÿ�� ����( �����԰ݿ� ���� )

	const char* _MSG_TYPE_SMS;			//	1
	const char* _MSG_TYPE_LMS;			//	4
	const char* _MSG_TYPE_FAX;			//	3
	const char* _MSG_TYPE_MMS;			//	4
	const char* _MSG_SUBTYPE_TEXT;		//	1
	const char* _MSG_SUBTYPE_IMAGE;		//	3

	// �޼��� ���� ����
	map<CString, XroshotSendInfo*>	_sendList;

	// ���� ���ε� ����
	CString				_fileServerIP;
	CString				_fileServerPort;

	// MMS ������(����ڰ��ø� MMS���� ����)
	CString				_mmsServerIP;
	CString				_mmsServerPort;

	char				_receiveBuf[XROSHOT_RECEIVE_MAX_SIZE];
	int					_receiveBufSize;

	CCriticalSection	_cs;

	byte _iv[16];
	byte _key[16];

	tinyxml2::XMLDocument _xmldoc;

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

	virtual BOOL	SendCoreMMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, byte* imageData, int size, CString messageType, CString messageSubType);

	virtual void	OnClose(int nErrorCode);
	virtual void	OnConnect(int nErrorCode);
	
	virtual void	OnSend(int nErrorCode);

	virtual void	OnReceive(TCHAR *buf, int size);

/////////////// GENERAL
	void			ProcessReceive(char* buf);
	void			AttachToReceiveBuf(BYTE* buf, int len);
	void			DetachFromReceiveBuf(int detachLen, int restLen);
	int				AnalysisPacket(int* msgId, short int* msglen);


	// SHA1 Ű ����
	void			MakeShaKey();

	// ���� ���� ã��
	CString			FindMCSServer(CString URL, CString URI);

	// ���� ���� ȹ��
	BOOL			GetServerInfo(CString serverInfo);

	// ����(�ð�)���
	void			RequestAuth();
	void			MakeAuth(TCHAR* xmlbuf);
	void			GetTime(TCHAR *buf);

	// ���� �м�
	int				AnalysisReceive(TCHAR* buf);

	// �α���(����)
	void			RequestLogin();
	void			MakeLogin(TCHAR* xmlbuf);
	BOOL			MakeAuthLogin();
	void			hex2byte(const char *in, unsigned int len, byte *out);
	void			confirmLogin(TCHAR *buf);

	// �� üũ
	void			MakePing(TCHAR* xmlbuf);

	//////// SMS ����
	// SMS ���ȵ���Ÿ ����
	void			MakeSMSData(XroshotSendInfo *info, char *pxmldata);
	void			MakeSMSDataFromFile(XroshotSendInfo *info, char *pxmldata);
	void			MakeCryptData(const char *OrgData,char *CryptData);

	//////// MMS ����
	// ����� ��ġ ã��
	void			MakeStorage(CString sendId, CString filename, char *xmlData);
	BOOL			GetAuthKeyNPath(char *buf, XroshotSendInfo **info);

	// ���ϼ��� ã��� ����
	void			GetUploadServerInfo(XroshotSendInfo *info);

	// ���� ���ε�
	void			UpLoadFile();
	void			UpLoadFile(XroshotSendInfo *info);
	void			MakeFileEncryptData(XroshotSendInfo* info);
	BOOL			UploadFileToSocket(XroshotSendInfo* info, CString URI);
	void			GetUploadResult(char *resbuf);
	void			RequestUploadDone(XroshotSendInfo* info);
	void			GetUploadComplete(char *resbuf);

	void			SendMMSFileData(char *buf, int size);

	// MMS ���� ����
	void			SendMMSByUploadDone(XroshotSendInfo *info);
	void			MakeMMSData(XroshotSendInfo *info, char *pxmldata);

	// ���� �ڷ� ã��
	XroshotSendInfo*	FindMMSInfo(CString descrytPath);
	XroshotSendInfo*	FindMMSInfoById(CString sendId);


	// ��� ��������(response)
	void			GetResponse(char *buf);

	// ��� ���۰��
	void			GetResult(char *buf);
	bool			ParsingResult(char *xmlbuf, char* jobId);

	// ���۰�� ���� �����
	void			MakeXMLResReport(char *jobID, char *pxmldata);

	// ���� ���� �߰�
	XroshotSendInfo* AddSendInfo(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString msgtype, CString msgsubtype);

	// �������� ��ȣȭ
	BOOL			MakeEncryptionData(XroshotSendInfo* info);

	// ������ ������ �ٿ�ε�
	int				getFileFromHttp(char* pszUrl, int filesize, char* pszFileBuffer);

	void			SendReqConvert(XroshotSendInfo *info);
	void			GetResConvert(char *buf);
	void			SendFAXByDoneConvert(XroshotSendInfo *info);

	void			SetActivate(BOOL b);
	void			SetActivateDate(CString date);
};

