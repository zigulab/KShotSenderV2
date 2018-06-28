#pragma once

#include "ServerSocket.h"

#include "CommBase.h"

#include <map>

#include "tinyxml2/tinyxml2.h"

class XroshotSendInfo;

// 수신 버퍼 1M
#define XROSHOT_RECEIVE_MAX_SIZE	(1024*1024*1)

// MMS파일 최대 크기 5M
#define	MMS_MAX_SIZE				(1024*1024*5)

class CommKTxroShot : public CommBase
{
public:
	CommKTxroShot(CString id, CString pwd, CString certFile, CString serverUrl, CommManager *manager);
	virtual ~CommKTxroShot(void);


///////////////////////////////////////////////////////////////////////////////// MEMBER DATA

	// 암호화 키
	unsigned char		_secretKeySHA1[20];

	// 서버 정보
	CString				_serverInfo;
	CString				_serverIP;
	CString				_serverPort;

	// 계정 정보
	CString				_center_url;
	CString				_id;
	CString				_password;
	CString				_certFile;

	// 서버시간
	CString				_serverTime;

	// 암호화된 AuthTicket
	//std::string		base64encodedciphertext;
	std::string			_encryptAuthTicket;

	// 서버 연결 유무
	BOOL				_isConnnted;

	// 로그인 유무
	BOOL				_isLogin;

	// ping xml데이타, 주기적으로 발송하므로 버퍼에 저장
	TCHAR				_xmlPingBuf[256];

	BOOL				_activate;
	CString			_activateDate;

	BOOL				_bNonePingPacket;
		

	// 메세지 타입 설정( 연동규격에 정의 )

	const char* _MSG_TYPE_SMS;			//	1
	const char* _MSG_TYPE_LMS;			//	4
	const char* _MSG_TYPE_FAX;			//	3
	const char* _MSG_TYPE_MMS;			//	4
	const char* _MSG_SUBTYPE_TEXT;		//	1
	const char* _MSG_SUBTYPE_IMAGE;		//	3

	// 메세지 정보 관리
	map<CString, XroshotSendInfo*>	_sendList;

	// 파일 업로드 정보
	CString				_fileServerIP;
	CString				_fileServerPort;

	// MMS 웹서버(사용자가올린 MMS파일 저장)
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


	// SHA1 키 생성
	void			MakeShaKey();

	// 서버 정보 찾기
	CString			FindMCSServer(CString URL, CString URI);

	// 서버 정보 획득
	BOOL			GetServerInfo(CString serverInfo);

	// 권한(시간)얻기
	void			RequestAuth();
	void			MakeAuth(TCHAR* xmlbuf);
	void			GetTime(TCHAR *buf);

	// 응답 분석
	int				AnalysisReceive(TCHAR* buf);

	// 로그인(인증)
	void			RequestLogin();
	void			MakeLogin(TCHAR* xmlbuf);
	BOOL			MakeAuthLogin();
	void			hex2byte(const char *in, unsigned int len, byte *out);
	void			confirmLogin(TCHAR *buf);

	// 핑 체크
	void			MakePing(TCHAR* xmlbuf);

	//////// SMS 전송
	// SMS 보안데이타 생성
	void			MakeSMSData(XroshotSendInfo *info, char *pxmldata);
	void			MakeSMSDataFromFile(XroshotSendInfo *info, char *pxmldata);
	void			MakeCryptData(const char *OrgData,char *CryptData);

	//////// MMS 전송
	// 저장소 위치 찾기
	void			MakeStorage(CString sendId, CString filename, char *xmlData);
	BOOL			GetAuthKeyNPath(char *buf, XroshotSendInfo **info);

	// 파일서버 찾기와 연결
	void			GetUploadServerInfo(XroshotSendInfo *info);

	// 파일 업로드
	void			UpLoadFile();
	void			UpLoadFile(XroshotSendInfo *info);
	void			MakeFileEncryptData(XroshotSendInfo* info);
	BOOL			UploadFileToSocket(XroshotSendInfo* info, CString URI);
	void			GetUploadResult(char *resbuf);
	void			RequestUploadDone(XroshotSendInfo* info);
	void			GetUploadComplete(char *resbuf);

	void			SendMMSFileData(char *buf, int size);

	// MMS 내용 전송
	void			SendMMSByUploadDone(XroshotSendInfo *info);
	void			MakeMMSData(XroshotSendInfo *info, char *pxmldata);

	// 전송 자료 찾기
	XroshotSendInfo*	FindMMSInfo(CString descrytPath);
	XroshotSendInfo*	FindMMSInfoById(CString sendId);


	// 모든 전송응답(response)
	void			GetResponse(char *buf);

	// 모든 전송결과
	void			GetResult(char *buf);
	bool			ParsingResult(char *xmlbuf, char* jobId);

	// 전송결과 응답 만들기
	void			MakeXMLResReport(char *jobID, char *pxmldata);

	// 전송 정보 추가
	XroshotSendInfo* AddSendInfo(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString msgtype, CString msgsubtype);

	// 전송정보 암호화
	BOOL			MakeEncryptionData(XroshotSendInfo* info);

	// 웹상의 파일을 다운로드
	int				getFileFromHttp(char* pszUrl, int filesize, char* pszFileBuffer);

	void			SendReqConvert(XroshotSendInfo *info);
	void			GetResConvert(char *buf);
	void			SendFAXByDoneConvert(XroshotSendInfo *info);

	void			SetActivate(BOOL b);
	void			SetActivateDate(CString date);
};

