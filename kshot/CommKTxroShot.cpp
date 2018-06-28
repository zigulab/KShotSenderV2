#include "StdAfx.h"
#include "CommKTxroShot.h"

#include "sha1/sha1.h"
#include "include/cryptopp/cryptlib.h"
#include "include/cryptopp/modes.h"
#include "include/cryptopp/aes.h"
#include "include/cryptopp/filters.h"
#include "include/cryptopp/base64.h"
#include "include/cryptopp/md5.h"
#include "include/cryptopp/hex.h"

//#include "tinyxml2/tinyxml2.h"

#include "XroshotSendInfo.h"
#include "CommManager.h"

#include "sendManager.h"

#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"lib/cryptlib.lib")

using namespace tinyxml2;

#define ONE_TIME_SECRETKEY	_T("kpmobileskey")

//#define XROSHOT_URL		_T("rcs.xroshot.com")
#define XROSHOT_URI			_T("catalogs/MAS/recommended/0")

#define	XSHOT_METHOD_STR_RES_AUTH					_T("res_auth")
#define	XSHOT_METHOD_STR_RES_REGIST					_T("res_regist")
#define	XSHOT_METHOD_STR_RES_UNREGIST				_T("res_unregist")
#define	XSHOT_METHOD_STR_RES_SEND_MESSAGE_ALL		_T("res_send_message_all")
#define	XSHOT_METHOD_STR_REQ_REPORT					_T("req_report")
#define	XSHOT_METHOD_STR_RES_STORAGE				_T("res_storage")
#define	XSHOT_METHOD_STR_RES_FINISH_UPLOAD			_T("res_finish_upload")
#define	XSHOT_METHOD_STR_RES_CONVERT				_T("res_convert")
#define	XSHOT_METHOD_STR_RES_PING					_T("res_ping")
#define	XSHOT_METHOD_STR_RES_SEND_MESSAGE			_T("res_send_message")
#define	XSHOT_METHOD_STR_REQ_UNREGIST				_T("req_unregist")

//#define XROSHOT_ELASPE_CHECK

enum {
	XSHOT_METHOD_RES_AUTH,
	XSHOT_METHOD_RES_REGIST,
	XSHOT_METHOD_RES_UNREGIST,
	XSHOT_METHOD_RES_SEND_MESSAGE_ALL,
	XSHOT_METHOD_REQ_REPORT,
	XSHOT_METHOD_RES_STORAGE,
	XSHOT_METHOD_RES_FINISH_UPLOAD,
	XSHOT_METHOD_RES_CONVERT,
	XSHOT_METHOD_RES_PING,
	XSHOT_METHOD_RES_SEND_MESSAGE,
	XSHOT_METHOD_REQ_UNREGIST
} XSHOT_METHOD_TYPE;

struct XSHOT_METHOD
{
	TCHAR *method;
	int no;
}g_xroshot_method[] = {
	{XSHOT_METHOD_STR_RES_AUTH,
		XSHOT_METHOD_RES_AUTH},
	{XSHOT_METHOD_STR_RES_REGIST,
		XSHOT_METHOD_RES_REGIST},
	{XSHOT_METHOD_STR_RES_UNREGIST,
		XSHOT_METHOD_RES_UNREGIST},
	{XSHOT_METHOD_STR_RES_SEND_MESSAGE_ALL,
		XSHOT_METHOD_RES_SEND_MESSAGE_ALL},
	{XSHOT_METHOD_STR_REQ_REPORT,
		XSHOT_METHOD_REQ_REPORT},
	{XSHOT_METHOD_STR_RES_STORAGE,
		XSHOT_METHOD_RES_STORAGE},
	{XSHOT_METHOD_STR_RES_FINISH_UPLOAD,
		XSHOT_METHOD_RES_FINISH_UPLOAD},
	{XSHOT_METHOD_STR_RES_CONVERT,
		XSHOT_METHOD_RES_CONVERT},
	{XSHOT_METHOD_STR_RES_PING,
		XSHOT_METHOD_RES_PING},
	{XSHOT_METHOD_STR_RES_SEND_MESSAGE,
		XSHOT_METHOD_RES_SEND_MESSAGE},
	{XSHOT_METHOD_STR_REQ_UNREGIST,
		XSHOT_METHOD_REQ_UNREGIST},
};

//#define FILE_CHUNK


CommKTxroShot::CommKTxroShot(CString id, CString pwd, CString certFile, CString serverUrl, CommManager *manager)
	: CommBase(manager),
_MSG_TYPE_SMS(_T("1")),
_MSG_TYPE_LMS(_T("4")),
_MSG_TYPE_FAX(_T("3")),
_MSG_TYPE_MMS(_T("4")),
_MSG_SUBTYPE_TEXT(_T("1")),
_MSG_SUBTYPE_IMAGE(_T("3"))
{

	_id = id;
	_password = pwd;
	_certFile.Format(_T(".\\certkey\\%s"), certFile);
	_center_url = serverUrl;

	_isConnnted	= FALSE;
	_isLogin			= FALSE;

	_bNonePingPacket = TRUE;

	memset(_receiveBuf, 0x00, sizeof(_receiveBuf));

	_receiveBufSize = 0;
}


CommKTxroShot::~CommKTxroShot(void)
{

}


BOOL CommKTxroShot::Init()
{
	// 환경얻기
	//_center_url = g_config.xroshot.center;
	//_id			= g_config.xroshot.id;
	//_password	= g_config.xroshot.password;
	//_certFile.Format(_T(".\\certkey\\%s"), g_config.xroshot.certkey);

	Logmsg(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::Init(), CommKTxroShot, id:%s, pwd:%s, key:%s"), _id, _password, _certFile);

	_mmsServerIP = g_config.kshot.mmsServerIP;
	_mmsServerPort= g_config.kshot.mmsServerPort;

	// 암호화 키 생성
	MakeShaKey();

	memset(_iv, 0x00, CryptoPP::AES::BLOCKSIZE);
	char* rawIv = "00000000000000000000000000";
	hex2byte(rawIv, strlen(rawIv), _iv);

	memset(_key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH);
	memcpy(_key, _secretKeySHA1, 16);  //원래 20 이엇음


	XMLError err = _xmldoc.LoadFile("./send_message.xml");

	if (err != XML_SUCCESS) {
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot, Init() XML LoadFile failed"));
		return FALSE;
	}

	_serverInfo = FindMCSServer(_center_url, XROSHOT_URI);

	if( _serverInfo.IsEmpty() ) {
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot, Init() _serverInfo empty"));
		return FALSE;
	}

	GetServerInfo(_serverInfo);

	return TRUE;
}

void CommKTxroShot::SetActivate(BOOL b) 
{
	_activate = b;
}

void CommKTxroShot::SetActivateDate(CString date) {
	_activateDate = date;
}


BOOL CommKTxroShot::Uninit()
{
	map<CString , XroshotSendInfo*>::iterator it;
	for (it=_sendList.begin();it!=_sendList.end();it++) {
		delete (XroshotSendInfo*)it->second;
	}
	_sendList.clear();

	Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot, 종료"));

	return CommBase::Uninit();
}

BOOL CommKTxroShot::Connect()
{
	if( _socket.Create() == 0 )
	{
		Log(CMdlLog::LEVEL_ERROR, _T("Xroshot 소켓 생성 실패"));
		return FALSE;
	}

	int port = _ttoi(_serverPort.GetBuffer());

	Logmsg(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::Connect(), CommKTxroShot 연결시도(%s,%d)"), _serverIP, port );

	if( _socket.Connect(_serverIP, port) == FALSE )
	{
		int err;
		if((err = GetLastError()) != WSAEWOULDBLOCK) 
		{
			CString msg;
			msg.Format(_T("서버 연결 실패 ErrorCode(%d)"), err);
			Log(CMdlLog::LEVEL_ERROR, msg);
			return FALSE;
		}
	}

	return TRUE;
}

void CommKTxroShot::MakeShaKey()
{
	SHA1* sha1 = new SHA1();

	sha1->addBytes( ONE_TIME_SECRETKEY, 12 );
	unsigned char* digest = sha1->getDigest(); //다 쓰고나서 free() 해줄꺼.

	memcpy(_secretKeySHA1,digest,20);

	free(digest);
	delete sha1;


	/////////////////////////////////////

	unsigned char digestxor[64] = {0};

	memcpy(digestxor,_secretKeySHA1,20);

	unsigned char tt[64];
	unsigned char result[64];

	memset(tt,0x36,sizeof(tt));


	for(int i = 0;i < 64; i++)
	{
		result[i] = tt[i] ^ digestxor[i];  
	}

	//이것을 16바이트 절사한다.
	SHA1* sha2 = new SHA1();

	sha2->addBytes( reinterpret_cast<char const *>(result), 64 );
	unsigned char* digest2 = sha2->getDigest();

	memcpy(_secretKeySHA1,digest2,16);

	free(digest2);
	delete sha2;

}

CString	CommKTxroShot::FindMCSServer(CString URL, CString URI)
{
	HINTERNET hInternet, hHttp;
	HINTERNET hReq;
	DWORD Size;
	DWORD dwRead;

	TCHAR buf[512] = {0};
	CString buffer;

	hInternet=InternetOpen(_T("HTTP"), INTERNET_OPEN_TYPE_PRECONFIG,NULL, NULL, 0);

	if (hInternet == NULL) {
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::FindMCSServer(),  InternetOpen Failed"));
		return "";
	}
	
	hHttp=InternetConnect(hInternet,URL,INTERNET_DEFAULT_HTTP_PORT,_T(""),_T(""),INTERNET_SERVICE_HTTP,0,0);
	if (hHttp==NULL)
	{
	   	Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot, FindMCSServer() InternetConnect Failed, URL(%s)"), URL);
		return "";
	}

	LPCTSTR accept[2]={ _T("*/*"), NULL };


	hReq = HttpOpenRequest(hHttp,_T("GET"),URI,NULL,NULL,accept, INTERNET_FLAG_DONT_CACHE,0);
	BOOL b = HttpSendRequest(hReq,NULL,0,NULL,0);

	do {
		InternetQueryDataAvailable(hReq,&Size,0,0);
		InternetReadFile(hReq,buf,Size,&dwRead);
		buf[dwRead]=0;
		//strcat(buf2,buf);
		buffer += buf;
	} while (dwRead != 0);

   
	InternetCloseHandle(hHttp);
	InternetCloseHandle(hInternet);
	hHttp=NULL;
	hInternet=NULL;

	return buffer;
}

BOOL CommKTxroShot::GetServerInfo(CString serverInfo)
{
	string temp;
	temp = (LPSTR)(LPCTSTR)_serverInfo.GetBuffer(0);

	_serverInfo.ReleaseBuffer();

	int startidx = temp.find("<?");

	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetServerInfo(), XML 파싱 실패"));
	    return FALSE;
	}

    temp = temp.substr(startidx,temp.length());


    tinyxml2::XMLDocument doc;
	//doc.Parse(pxmldata);
	doc.Parse(temp.c_str());

	XMLElement *pRoot = doc.FirstChildElement("RCP");
	if(!pRoot) return (false);

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return (false);

	char *pszResult = (char *)pElem->GetText();  //결과값

	if(pszResult == NULL || strcmp(pszResult,"0"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetServerInfo(), 아이피,포트 얻기 실패(%s)"), pszResult);
		return FALSE;
	}

	pElem = pRoot->FirstChildElement("Resource")->FirstChildElement("Address");
    if(!pElem) return FALSE;

    char *pszAddress = (char *)pElem->GetText();  //IP주소
    _serverIP = pszAddress;

	pElem = pRoot->FirstChildElement("Resource")->FirstChildElement("Port");
    if(!pElem) return FALSE;

	char *pszPort = (char *)pElem->GetText();  //포트번호
    _serverPort = pszPort;

	return TRUE;

}

void CommKTxroShot::RequestAuth()
{
	char xmlbuf[256] = {0};

	MakeAuth(xmlbuf);

	memcpy(_socket._sendBuffer + _socket._toSendBufSize,(LPCSTR)xmlbuf, strlen(xmlbuf) + 1);  //큐에 집어 넛는다.
    _socket._toSendBufSize += strlen(xmlbuf) + 1;
}

void CommKTxroShot::MakeAuth(TCHAR* xmlbuf)
{
	tinyxml2::XMLDocument doc;
	XMLDeclaration * pDecl = doc.NewDeclaration();
	doc.LinkEndChild(pDecl);

    XMLElement *pRoot = doc.NewElement("MAS");
	pRoot->SetAttribute("method","req_auth");
	doc.LinkEndChild(pRoot);

	XMLElement *pElem = doc.NewElement("ServiceProviderID");
	pElem->LinkEndChild(doc.NewText(_id));
	pRoot->LinkEndChild(pElem);

    XMLPrinter printer;
    //printer.SetStreamPrinting();
    doc.Accept( &printer ); 

    //Xmldata = (char *)printer.CStr(); 
	//위에 포인터는 여기서 소멸된다.....  
    sprintf(xmlbuf,"%s",(char *)printer.CStr());
}

int CommKTxroShot::AnalysisPacket(int* msgId, short int* msglen)
{
	int			id;
	short int	len;

	CString szBuf = _receiveBuf;
 
	int idx = -1;
	//if( (idx = szBuf.Find(_T("<?"))) == -1 )
	//	return -1;

	CString marker1 = _T("</FUP>");
	CString marker2 = _T("</MAS>");

	if( (idx = szBuf.Find(marker1)) != -1 )
		*msglen = idx + marker1.GetLength();
	else if( (idx = szBuf.Find(marker2)) != -1 ) {
		// 보통 팻킷 끝에 NULL까지 포함해서 전송된다. 일단 +1
		// 그러나 , NULL 포함되지 않은 경우도 있다.
		*msglen = idx + marker2.GetLength() + 1;
	}
	else
		return -1;
		
	int detachLen = (_receiveBufSize - *msglen);

	// NULL 포함이나 미포함으로 -1의 차이가 생기는 경우가 있다.
	if( detachLen < -1 ) {
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::AnalysisPacket(), Invalid Packet"));
		Log(CMdlLog::LEVEL_ERROR, _receiveBuf );
		
		//ASSERT( 0 );
		// 잘못된 버퍼를 비운다.
		memset(_receiveBuf, 0x00, sizeof(_receiveBuf));
		_receiveBufSize = 0;

		return -1;
	}

	*msgId = AnalysisReceive(_receiveBuf);

	if( *msgId == -1 ) {
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::AnalysisPacket(), Invalid msgId"));
		Log(CMdlLog::LEVEL_ERROR, _receiveBuf );

		//ASSERT( 0 );
		// 잘못된 버퍼를 비운다.
		memset(_receiveBuf, 0x00, sizeof(_receiveBuf));
		_receiveBufSize = 0;

		return -1;
	}
	
	return detachLen;
}

int CommKTxroShot::AnalysisReceive(TCHAR* buf)
{
	string temp = buf;

	int startidx = temp.find("<?");

	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::AnalysisReceive(), XML 파싱 실패"));
		return -1;
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());

	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(pRoot)
	{
		const char *str = pRoot->Attribute("method"); 

		for( int i = 0; i < sizeof(g_xroshot_method); i++ )
		{
			if( !strcmp(g_xroshot_method[i].method, str) )
				return g_xroshot_method[i].no;
		}
	}
	else {
		pRoot = doc.FirstChildElement("FUP");
		if(pRoot) 
			return -999;
	}

	return -1;
}

void CommKTxroShot::GetTime(TCHAR *buf)
{
	string temp = buf;

	int startidx = temp.find("<?");
	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetTime(), xml 파싱 실패"));
        return;
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return;

	char *pszResult = (char *)pElem->GetText();  //결과값

	if(pszResult == NULL || strcmp(pszResult,"0"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetTime(), 시간얻기 실패"),pszResult);
	    return;
	}

	pElem = pRoot->FirstChildElement("Time");
    if(!pElem) return;

	
	char *pszTime = (char *)pElem->GetText();  //서버시간
	_serverTime = pszTime;

	TRACE(_T("서버 시간:%s\n"), _serverTime);
}

void CommKTxroShot::RequestLogin()
{
	char xmlbuf[1024] = {0};

	MakeLogin(xmlbuf);

	_socket.SendChar(xmlbuf);
}

void CommKTxroShot::MakeLogin(TCHAR* xmlbuf)
{
	if( MakeAuthLogin() == FALSE ) {
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot, MakeAuthLogin() FALSE"));
		return;
	}

	TCHAR *szId = _id.GetBuffer(0);

	_id.ReleaseBuffer();

	tinyxml2::XMLDocument doc;
	XMLDeclaration * pDecl = doc.NewDeclaration();
	doc.LinkEndChild(pDecl);


	XMLElement *pRoot = doc.NewElement("MAS");
	pRoot->SetAttribute("method","req_regist");
	doc.LinkEndChild(pRoot);

	XMLElement *pElem = doc.NewElement("ServiceProviderID");
	pElem->LinkEndChild(doc.NewText(szId));
	pRoot->LinkEndChild(pElem);

	XMLElement *pElem2 = doc.NewElement("EndUserID");
	pElem2->LinkEndChild(doc.NewText(szId));
	pRoot->LinkEndChild(pElem2);

	XMLElement *pElem3 = doc.NewElement("AuthTicket");
	pElem3->LinkEndChild(doc.NewText(_encryptAuthTicket.c_str()));
	pRoot->LinkEndChild(pElem3);

	XMLElement *pElem4 = doc.NewElement("AuthKey");
	pElem4->LinkEndChild(doc.NewText("77"));
	pRoot->LinkEndChild(pElem4);

	XMLElement *pElem5 = doc.NewElement("Version");
	pElem5->LinkEndChild(doc.NewText("1.0"));
	pRoot->LinkEndChild(pElem5);


	XMLPrinter printer;
	//printer.SetStreamPrinting();
	doc.Accept( &printer ); 

	sprintf(xmlbuf,"%s",(char *)printer.CStr());
}

BOOL CommKTxroShot::MakeAuthLogin()
{
	////////////////////////////////////////////////////////
	// 이 메소드에서 메모리누스가 발생함. 10줄 정도
	////////////////////////////////////////////////////////

	unsigned char buf[128];

	FILE *fp = NULL;

	fp = fopen(_certFile,"rb");
	if(fp == NULL)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot, MakeAuthLogin() _certFile open failed"));
		return FALSE;
	}

	fseek(fp,77,SEEK_SET); 
	fread(buf,1,128,fp);

	fclose(fp);


	SHA1* sha1 = new SHA1();

	sha1->addBytes( reinterpret_cast<char const *>(buf), 128 );
	unsigned char* digest = sha1->getDigest(); //다 쓰고나서 free() 해줄꺼.

	delete sha1;

	unsigned char digestxor[64] = {0};

	memcpy(digestxor,digest,20);

	unsigned char tt[64];
	unsigned char result[64];

	memset(tt,0x36,sizeof(tt));

	for(int i = 0;i < 64; i++)
	{
		result[i] = tt[i] ^ digestxor[i];  
	}

	//이것을 16바이트 절사한다.
	SHA1* sha2 = new SHA1();

	sha2->addBytes( reinterpret_cast<char const *>(result), 64 );
	unsigned char* digest2 = sha2->getDigest();

	delete sha2;

	char AuthTicket[128];
	sprintf(AuthTicket,"%s|%s|%s|%s|kpmobileskey", _id, _password, _id, _serverTime);

	std::string plaintext = AuthTicket;
	std::string ciphertext;


	byte key[16];
	memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );

	memcpy(key,digest2,16);

	byte iv[CryptoPP::AES::BLOCKSIZE];
	memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE );
	char* rawIv="00000000000000000000000000";
	hex2byte(rawIv, strlen(rawIv), iv);


	unsigned int plainTextLength = plaintext.length();

	_encryptAuthTicket.erase();

	//AES암호화 수행..
	CryptoPP::AES::Encryption 
		aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
	CryptoPP::CBC_Mode_ExternalCipher::Encryption 
		cbcEncryption(aesEncryption, iv);

	CryptoPP::StreamTransformationFilter 
		stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext), CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING);
	stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plaintext.c_str()), plainTextLength + 1);  ///+1 을 뺄것인가...
	stfEncryptor.MessageEnd();


	CryptoPP::StringSource(ciphertext, true,
							new CryptoPP::Base64Encoder(
								new CryptoPP::StringSink(_encryptAuthTicket),false
							) // Base64Encoder
	); // StringSource


	free(digest);
	free(digest2);

	return TRUE;
}

void CommKTxroShot::hex2byte(const char *in, unsigned int len, byte *out)
{
	for(unsigned int i = 0; i < len; i+=2)
	{
		char c0 = in[i+0];
		char c1 = in[i+1];
		byte c = (
			((c0 & 0x40 ? (c0 & 0x20 ? c0 - 0x57 : c0 - 0x37) : c0 - 0x30) << 4) |
			((c1 & 0x40 ? (c1 & 0x20 ? c1 - 0x57 : c1 - 0x37) : c1 - 0x30))
		);
		out[i/2] = c;
	}
}

void CommKTxroShot::confirmLogin(TCHAR *buf)
{
	string temp = buf;

	int startidx = temp.find("<?");

	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::confirmLogin(), XML 파싱 실패"));
        return;
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return;

	char *pszResult = (char *)pElem->GetText();  //결과값

	if(pszResult == NULL || strcmp(pszResult,"0") == 0)
	{
		Logmsg(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot 로그인 성공(%s)"), pszResult);

		_isLogin = TRUE;
	}
	else 
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot 로그인 실패(%s)"), pszResult);

		_isLogin = FALSE;
	}	
}

void CommKTxroShot::OnClose(int nErrorCode)
{
	Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::OnClose(), 소켓 연결이 끊겼습니다.(%d)"), nErrorCode);

	_isConnnted = FALSE;
	_isLogin = FALSE;

	_socket.Close();

	Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::OnClose(), 연결 재시도."));

	CWnd* cwnd = AfxGetMainWnd();
	HWND hwnd = cwnd->GetSafeHwnd();
	PostMessage(hwnd, WM_RECONNECT_TELECOM, TELECOM_KT_XROSHOT, 0);
}

void CommKTxroShot::OnConnect(int nErrorCode)
{
	Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot Server Connected"));

	_isConnnted = TRUE;

	// 주기적인 핑전송을 위해 최상위 윈도우에게 Timer 요청
	//CWnd* cwnd = AfxGetMainWnd();
	//HWND hwnd = cwnd->GetSafeHwnd();
	//PostMessage(hwnd, WM_PING_TIMER, XROSHOT_TIMER_ID, 0);

	// 서버 연결후 Auth(time)요청
	RequestAuth();
}


void AsciiToUTF8(CString parm_ascii_string, CString &parm_utf8_string) 
{
     parm_utf8_string.Empty(); 
 
     // 아스키 코드를 UTF8형식의 코드로 변환해야 한다. 아스키 코드를 UTF8 코드로 변환할때는 
     // 아스키 코드를 유니코드로 먼저 변환하고 변환된 유니코드를 UTF8 코드로 변환해야 한다.
 
     // 아스키 코드로된 문자열을 유니코드화 시켰을 때의 길이를 구한다.
      int temp_length = MultiByteToWideChar(CP_ACP, 0, (char *)(const char *)parm_ascii_string, -1, NULL, 0);
     // 변환된 유니코드를 저장할 공간을 할당한다.
     BSTR unicode_str = SysAllocStringLen(NULL, temp_length + 1);
 
     // 아스키 코드로된 문자열을 유니 코드 형식의 문자열로 변경한다.
      MultiByteToWideChar(CP_ACP, 0, (char *)(const char *)parm_ascii_string, -1, unicode_str, temp_length);
 
     // 유니코드 형식의 문자열을 UTF8 형식으로 변경했을때 필요한 메모리 공간의 크기를 얻는다.
     temp_length = WideCharToMultiByte( CP_UTF8, 0, unicode_str, -1, NULL, 0, NULL, NULL );
 
     if(temp_length > 0){
        CString str;
        // UTF8 코드를 저장할 메모리 공간을 할당한다.
        char *p_utf8_string = new char[temp_length];
        memset(p_utf8_string, 0, temp_length);
        // 유니코드를 UTF8코드로 변환한다.
        WideCharToMultiByte(CP_UTF8, 0, unicode_str, -1, p_utf8_string, temp_length, NULL, NULL);
  
        // UTF8 형식으로 변경된 문자열을 각 문자의 코드값별로 웹 URL에 사용되는 형식으로 변환한다.
        for(int i = 0; i < temp_length - 1; i++){
            if(p_utf8_string[i] & 0x80){
                // 현재 코드가 한글인 경우..
                // UTF8 코드로 표현된 한글은 3바이트로 표시된다. "한글"  ->  %ED%95%9C%EA%B8%80
                for(int sub_i = 0; sub_i < 3; sub_i++){
                    str.Format("%%%X", p_utf8_string[i] & 0x00FF);
                    parm_utf8_string += str;
                    i++;
                } 
    
                i--;
            } else {
                // 현재 코드가 영문인 경우, 변경없이 그대로 사용한다.
               parm_utf8_string += p_utf8_string[i];
            }
        }                                                              
  
        delete[] p_utf8_string;
     }
 
      // 유니코드 형식의 문자열을 저장하기 위해 생성했던 메모리를 삭제한다.
      SysFreeString(unicode_str);
}

// 웹상의 파일을 다운로드
int CommKTxroShot::getFileFromHttp(char* pszUrl, int filesize, char* pszFileBuffer)
{
	const int READ_BUF_SIZE = 1024;

    HINTERNET    hInet, hUrl;
    DWORD        dwReadSize = 0;

	CString strUrl = pszUrl;
	CString strUrl_8;
	AsciiToUTF8(strUrl, strUrl_8);
	
    // WinINet함수 초기화
    if ((hInet = InternetOpen("MyWeb",            // user agent in the HTTP protocol
                    INTERNET_OPEN_TYPE_DIRECT,    // AccessType
                    NULL,                        // ProxyName
                    NULL,                        // ProxyBypass
                    0)) != NULL)                // Options
    {
        // 입력된 HTTP주소를 열기
        if ((hUrl = InternetOpenUrl(hInet,        // 인터넷 세션의 핸들
                    //pszUrl,                       // URL
					strUrl_8,
                    NULL,                        // HTTP server에 보내는 해더
                    0,                            // 해더 사이즈
                    0,                            // Flag
                    0)) != NULL)                // Context
        {
            //FILE    *fp;

            // 다운로드할 파일 만들기
            //if ((fp = fopen(pszFile, "wb")) != NULL)
            {
                TCHAR    szBuff[READ_BUF_SIZE];
                DWORD    dwSize;
                DWORD    dwDebug = 10;

                do {
                    // 웹상의 파일 읽기
                    InternetReadFile(hUrl, szBuff, READ_BUF_SIZE, &dwSize);

                    // 웹상의 파일을 만들어진 파일에 써넣기
                    //fwrite(szBuff, dwSize, 1, fp);
					if( dwSize > 0 ) 
						memcpy(pszFileBuffer+dwReadSize, szBuff, dwSize);

                    dwReadSize += dwSize;
                } while ((dwSize != 0) || (--dwDebug != 0));

               //fclose( fp );
            }

            // 인터넷 핸들 닫기
            InternetCloseHandle(hUrl);
        }
		else
			Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot, getFileFromHttp() InternetOpenUrl FALSE"));

        // 인터넷 핸들 닫기
        InternetCloseHandle(hInet);
    }
	else
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot, getFileFromHttp() InternetOpen FALSE"));

    return dwReadSize;
}

BOOL CommKTxroShot::SendMMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, const int size)
{
	//CString filepath = _T("HTTP://IP:PORT") + imageFilePath;

	CString webpath;
	webpath.Format(_T("http://%s:%s%s"), _mmsServerIP, _mmsServerPort, imageFilePath);

	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot::SendMMS(), wetpath:%s"), webpath);

	char *filebuffer = new char[size];
	int ret_size = getFileFromHttp(webpath.GetBuffer(0), size, filebuffer);
	webpath.ReleaseBuffer();

	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot::SendMMS(), FileSize assert Eqeual(%d == %d)"), size, ret_size);

	BOOL b = SendCoreMMS(sendId, callbackNO, receiveNO, subject, msg, filename, (byte*)filebuffer, size, _MSG_TYPE_MMS, _MSG_SUBTYPE_IMAGE);

	delete[] filebuffer;

	return b;
}

BOOL CommKTxroShot::SendFAX(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, const int size)
{
	//CString filepath = _T("HTTP://IP:PORT") + imageFilePath;

	CString webpath;
	webpath.Format(_T("http://%s:%s%s"), _mmsServerIP, _mmsServerPort, imageFilePath);

	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot::SendMMS(), wetpath:%s"), webpath);

	char *filebuffer = new char[size];
	int ret_size = getFileFromHttp(webpath.GetBuffer(0), size, filebuffer);
	webpath.ReleaseBuffer();

	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot::SendMMS(), FileSize assert Eqeual(%d == %d)"), size, ret_size);

	BOOL b = SendCoreMMS(sendId, callbackNO, receiveNO, subject, msg, filename, (byte*)filebuffer, size, _MSG_TYPE_FAX, _MSG_SUBTYPE_TEXT);

	delete[] filebuffer;

	return b;
}

BOOL CommKTxroShot::SendCoreMMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, byte* imageData, int size, CString messageType, CString messageSubType)
{   
	XroshotSendInfo *info = new XroshotSendInfo(sendId,
													callbackNO,
													receiveNO,
													subject,
													msg,
													filename,
													imageData,
													size,
													messageType,	messageSubType);

	_cs.Lock();

	_sendList[sendId] = info;

	_cs.Unlock();

	char xmlbuf[1024] = {0};

	MakeStorage(sendId, filename, xmlbuf);

	_socket.SendChar(xmlbuf);

	info->_bSent = TRUE;

	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS AuthKey와 파일 위치 요청(kshot_number:%s, filename:%s"), sendId, filename);

	return TRUE;
}

void CommKTxroShot::SendMMSByUploadDone(XroshotSendInfo *info)
{
	if (info->_msgtype == "3") {
		// 변환요청
		SendReqConvert(info);
	}
	else {
		//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS Encrpyt Data 생성"));

		MakeEncryptionData(info);

		//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS Packet 생성"));

		// 장문 버퍼
		char xmlbuf[4096 + 1024] = { 0 };
		MakeMMSData(info, xmlbuf);

		//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS 전송"));

		_socket.SendChar(xmlbuf);
	}
}

void CommKTxroShot::SendReqConvert(XroshotSendInfo *info)
{
	char xmlbuf[1024] = { 0 };

	tinyxml2::XMLDocument doc;
	XMLDeclaration * pDecl = doc.NewDeclaration();
	doc.LinkEndChild(pDecl);


	XMLElement *pRoot = doc.NewElement("MAS");
	pRoot->SetAttribute("method", "req_convert");
	doc.LinkEndChild(pRoot);

	XMLElement *pElem = doc.NewElement("ConvertType");
	pElem->LinkEndChild(doc.NewText("TTF_CONV"));
	pRoot->LinkEndChild(pElem);

	XMLElement *pElem2 = doc.NewElement("Path");
	pElem2->LinkEndChild(doc.NewText(info->_filePath));
	pRoot->LinkEndChild(pElem2);

	XMLElement *pElem3 = doc.NewElement("CustomMessageID");
	pElem3->LinkEndChild(doc.NewText(info->_sendId.GetBuffer()));
	pRoot->LinkEndChild(pElem3);

	XMLPrinter printer;
	doc.Accept(&printer);

	sprintf(xmlbuf, "%s", (char *)printer.CStr());

	// 원래 서버로 완료 요청을 보낸다.
	_socket.SendChar(xmlbuf);
}

void CommKTxroShot::GetResConvert(char *buf)
{
	string temp = buf;

	int startidx = temp.find("<?");

	if (startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetResConvert(), XML 파싱 실패"));
		return;
	}

	temp = temp.substr(startidx, temp.length());

	tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if (!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
	if (!pElem) return;
	char *pszResult = (char *)pElem->GetText();  //결과값

	if (pszResult == NULL || strcmp(pszResult, "0") != 0)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetResConvert(), Authkey Path 획득 실패"));
		return;
	}

	pElem = pRoot->FirstChildElement("Path");
	if (!pElem) return;
	CString path = (char *)pElem->GetText();

	pElem = pRoot->FirstChildElement("FileSize");
	if (!pElem) return;
	CString filesize = (char *)pElem->GetText();

	pElem = pRoot->FirstChildElement("PageCount");
	if (!pElem) return;
	CString pageCount = (char *)pElem->GetText();
	int count = _ttoi(pageCount);

	pElem = pRoot->FirstChildElement("CustomMessageID");
	if (!pElem) return;
	CString sendId = (char *)pElem->GetText();  //결과값

	XroshotSendInfo *info = FindMMSInfoById(sendId);
	if (info == NULL)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetResConvert(), Fax 에서 변환된 Info정보가 없음."));
		return ;
	}

	info->_filePath = path;
	info->_pageCount = count;
	
	SendFAXByDoneConvert(info);
}

void CommKTxroShot::SendFAXByDoneConvert(XroshotSendInfo *info)
{
	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS Encrpyt Data 생성"));

	MakeEncryptionData(info);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS Packet 생성"));

	// 장문 버퍼
	char xmlbuf[4096 + 1024] = { 0 };
	MakeMMSData(info, xmlbuf);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS 전송"));

	_socket.SendChar(xmlbuf);
}

BOOL CommKTxroShot::SendLMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg)
{
	XroshotSendInfo *info = AddSendInfo(sendId, callbackNO, receiveNO, subject, msg, _MSG_TYPE_LMS, _MSG_SUBTYPE_TEXT);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot LMS Encrpyt Data 생성"));

	MakeEncryptionData(info);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot LMS Packet 생성"));
	
	// 장문 버퍼
	char xmlbuf[4096+1024] = {0};
	MakeSMSDataFromFile(info, xmlbuf);

	_socket.SendChar(xmlbuf);

	info->_bSent = TRUE;

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot LMS 전송"));

	return TRUE;
}

XroshotSendInfo* CommKTxroShot::AddSendInfo(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString msgtype, CString msgsubtype)
{
	XroshotSendInfo *info = new XroshotSendInfo(sendId,
													callbackNO,
													receiveNO,
													subject,
													msg,
													msgtype, 
													msgsubtype);

	_cs.Lock();

	_sendList[sendId] = info;

	_cs.Unlock();

	return info;
}

BOOL CommKTxroShot::MakeEncryptionData(XroshotSendInfo* info)
{
	static char prev_callbackNo[128] = { 0 };
	static char prev_msg[4000] = { 0 };
	static char prev_receiveNo[128] = { 0 };
	static char prev_env_callbackNo[128] = { 0 };
	static char prev_env_msg[4000] = { 0 };
	static char prev_env_receiveNo[128] = { 0 };

	char enc_callbackNo[128];
	char enc_msg[4000];

	if (prev_callbackNo != NULL && strcmp(info->_callbackNO, prev_callbackNo) != 0 ) {
		MakeCryptData(info->_callbackNO, enc_callbackNo);
	}
	else {
		strcpy(enc_callbackNo, prev_env_callbackNo);
	}


	if (prev_msg != NULL && strcmp(info->_msg, prev_msg) != 0) {
		MakeCryptData(info->_msg, enc_msg);
	}
	else {
		strcpy(enc_msg, prev_env_msg);
	}

	info->_enc_callbackNO = enc_callbackNo;
	info->_enc_msg = enc_msg;

	//int size = info->_receiveNO.GetSize();
	//for( int i = 0; i < size; i++ ) 
	//{
		char sha1_receive[128];

		CString receive = info->_receiveNO;
		MakeCryptData(receive, sha1_receive);
		info->_enc_receiveNO = sha1_receive;

		//strcpy(prev_env_receiveNo, sha1_receive);
	//}

	strcpy(prev_callbackNo, info->_callbackNO);
	strcpy(prev_msg, info->_msg);
	//strcpy(prev_receiveNo, info->_receiveNO.GetAt(0));

	strcpy(prev_env_callbackNo, enc_callbackNo);
	strcpy(prev_env_msg, enc_msg);
	

	return TRUE;
}

BOOL CommKTxroShot::SendSMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg)
{
	XroshotSendInfo *info = AddSendInfo(sendId, callbackNO, receiveNO, subject, msg, _MSG_TYPE_SMS, _MSG_SUBTYPE_TEXT);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot SMS Encrpyt Data 생성"));

	MakeEncryptionData(info);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot SMS Packet 생성"));

	// 단문 버퍼
	char xmlbuf[2048] = {0};
	MakeSMSData(info, xmlbuf);

	_socket.SendChar(xmlbuf);

	info->_bSent = TRUE;

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot SMS 전송"));

	return TRUE;
}

void CommKTxroShot::MakeSMSData(XroshotSendInfo *info, char *pxmldata)
{
	//CTime    CurTime = CTime::GetCurrentTime();
	//CString  CD    =   CurTime.Format("%Y%m%d%H%M%S");

	tinyxml2::XMLDocument doc;
	XMLDeclaration * pDecl = doc.NewDeclaration();
	doc.LinkEndChild(pDecl);

	XMLElement *pRoot = doc.NewElement("MAS");
	pRoot->SetAttribute("method","req_send_message_2");
	doc.LinkEndChild(pRoot);

	XMLElement *pElem = doc.NewElement("MessageType");
	pElem->LinkEndChild(doc.NewText(info->_msgtype));
	pRoot->LinkEndChild(pElem);

	pElem = doc.NewElement("MessageSubType");
	pElem->LinkEndChild(doc.NewText(info->_msgsubtype));
	pRoot->LinkEndChild(pElem);

	pElem = doc.NewElement("CallbackNumber");
	pElem->LinkEndChild(doc.NewText(info->_enc_callbackNO));
	pRoot->LinkEndChild(pElem);

	pElem = doc.NewElement("CustomMessageID");
	pElem->LinkEndChild(doc.NewText(info->_sendId));
	pRoot->LinkEndChild(pElem);

	pElem = doc.NewElement("Message");
	pRoot->LinkEndChild(pElem);

	//int size = info->_enc_receiveNO.GetSize();
	//for(int i = 0; i < size; i++ )
	//{
		CString seqNo; 
		seqNo.Format(_T("%d"), 1);

		CString receiveNo = info->_enc_receiveNO;

		XMLElement *pSubElem = doc.NewElement("ReceiveNumber");
		pSubElem->SetAttribute("seqNo", seqNo);
		pSubElem->LinkEndChild(doc.NewText(receiveNo));
		pElem->LinkEndChild(pSubElem);
	//}


	//string strMulti = CW2A(L"유니코드를 멀티바이트로 변환");
	// 한글변환
	// 멀티바이트 => 유니코드 => UTF-8 로 변환
	wstring subject_uni = CA2W(info->_subject);
	string subject_u8 = CW2A(subject_uni.c_str(),CP_UTF8);	

	pSubElem = doc.NewElement("Subject");
	//pSubElem->LinkEndChild(doc.NewText(info->_subject));
	pSubElem->LinkEndChild(doc.NewText(subject_u8.c_str()));
	pElem->LinkEndChild(pSubElem);

	pSubElem = doc.NewElement("Content");
	pSubElem->LinkEndChild(doc.NewText(info->_enc_msg));
	pElem->LinkEndChild(pSubElem);

	XMLPrinter printer;
	doc.Accept( &printer ); 

	sprintf(pxmldata,"%s",(char *)printer.CStr());
}

void CommKTxroShot::MakeSMSDataFromFile(XroshotSendInfo *info, char *pxmldata)
{
	/*
	static char prev_type[5] = { 0 };
	static char prev_subtype[5] = { 0 };
	static char prev_callbackNo[128] = { 0 };
	static char prev_msg[4000] = { 0 };

	static BOOL firstSend = TRUE;

	BOOL notSame = TRUE;
	if (firstSend == FALSE) 
	{
		if (strcmp(prev_type, info->_msgtype) == 0 &&
			strcmp(prev_subtype, info->_msgsubtype) == 0 &&
			strcmp(prev_callbackNo, info->_callbackNO) == 0 &&
			strcmp(prev_msg, info->_msg) == 0
			)
			notSame = FALSE;
	}
	*/

	XMLElement *pRoot = _xmldoc.FirstChildElement("MAS");
	XMLElement *pElem;
	//if (firstSend || notSame) 
	//{
		pElem = pRoot->FirstChildElement("MessageType");
		pElem->SetText(info->_msgtype);

		pElem = pRoot->FirstChildElement("MessageSubType");
		pElem->SetText(info->_msgsubtype);

		pElem = pRoot->FirstChildElement("CallbackNumber");
		pElem->SetText(info->_enc_callbackNO);

		pElem = pRoot->FirstChildElement("CustomMessageID");
		pElem->SetText(info->_sendId);
	//}

	pElem = pRoot->FirstChildElement("Message");

	XMLElement *pSubElem = NULL;

	//int size = info->_enc_receiveNO.GetSize();
	//for (int i = 0; i < size; i++)
	//{
		CString seqNo;
		seqNo.Format(_T("%d"), 1);

		CString receiveNo = info->_enc_receiveNO;

		pSubElem = pElem->FirstChildElement("ReceiveNumber");
		pSubElem->SetAttribute("seqNo", seqNo);

		pSubElem->SetText(receiveNo);
	//}

	//if (firstSend || notSame)
	//{
		//string strMulti = CW2A(L"유니코드를 멀티바이트로 변환");
		// 한글변환
		// 멀티바이트 => 유니코드 => UTF-8 로 변환
		wstring subject_uni = CA2W(info->_subject);
		string subject_u8 = CW2A(subject_uni.c_str(), CP_UTF8);

		pSubElem = pElem->FirstChildElement("Subject");
		pSubElem->SetText(subject_u8.c_str());

		pSubElem = pElem->FirstChildElement("Content");
		pSubElem->SetText(info->_enc_msg);
	//}

	XMLPrinter printer;
	_xmldoc.Accept(&printer);

	sprintf(pxmldata, "%s", (char *)printer.CStr());

	/*
	strcpy(prev_type, info->_msgtype);
	strcpy(prev_subtype, info->_msgsubtype);
	strcpy(prev_callbackNo, info->_callbackNO);
	strcpy(prev_msg, info->_msg);

	firstSend = FALSE;
	*/
}

void CommKTxroShot::MakeCryptData(const char *OrgData,char *CryptData)  //원본데이터를 받아서 암호화해서 넘기준다.
{
#if( defined(DEBUG_ELAPSE_CHECK) && defined( XROSHOT_ELASPE_CHECK ))
	LARGE_INTEGER Frequency;
	LARGE_INTEGER BeginTime;
	LARGE_INTEGER Endtime;

	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&BeginTime);
#endif
	
	std::string plaintext;
	//plaintext.erase();
	plaintext = OrgData;

	std::string ciphertext;

	//ciphertext.erase();

	std::string base64encodedciphertdata;

	//base64encodedciphertdata.erase();

#if( defined(DEBUG_ELAPSE_CHECK) && defined( XROSHOT_ELASPE_CHECK ))
	ELAPSE_CHECK_END("MakeCryptData", 1)
#endif

	//byte iv[CryptoPP::AES::BLOCKSIZE];
	//memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE );
	//char* rawIv="00000000000000000000000000";
	//hex2byte(rawIv, strlen(rawIv), iv);

#if( defined(DEBUG_ELAPSE_CHECK) && defined( XROSHOT_ELASPE_CHECK ))
	ELAPSE_CHECK_END("MakeCryptData", 2)
#endif


	//byte key[16];
	//memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
	//memcpy(key,_secretKeySHA1,16);  //원래 20 이엇음

	unsigned int plainTextLength = plaintext.length();

	//AES암호화 수행..
	CryptoPP::AES::Encryption 
		aesEncryption(_key, CryptoPP::AES::DEFAULT_KEYLENGTH);

#if( defined(DEBUG_ELAPSE_CHECK) && defined( XROSHOT_ELASPE_CHECK ))
	ELAPSE_CHECK_END("MakeCryptData", 3)
#endif

	CryptoPP::CBC_Mode_ExternalCipher::Encryption 
		cbcEncryption(aesEncryption, _iv);

	CryptoPP::StreamTransformationFilter 
		stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext), CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING);

#if( defined(DEBUG_ELAPSE_CHECK) && defined( XROSHOT_ELASPE_CHECK ))
	ELAPSE_CHECK_END("MakeCryptData", 4)
#endif

	stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plaintext.c_str()), plainTextLength + 1);  ///+1 을 뺄것인가...
	stfEncryptor.MessageEnd();

#if( defined(DEBUG_ELAPSE_CHECK) && defined( XROSHOT_ELASPE_CHECK ))
	ELAPSE_CHECK_END("MakeCryptData", 5)
#endif 

	CryptoPP::StringSource(ciphertext, true,
		new CryptoPP::Base64Encoder(
		new CryptoPP::StringSink(base64encodedciphertdata),false
		) // Base64Encoder
		); // StringSource

#if( defined(DEBUG_ELAPSE_CHECK) && defined( XROSHOT_ELASPE_CHECK ))
	ELAPSE_CHECK_END("MakeCryptData", 6)
#endif

	sprintf(CryptData,"%s",(char *)base64encodedciphertdata.c_str());

#if( defined(DEBUG_ELAPSE_CHECK) && defined( XROSHOT_ELASPE_CHECK ))
	ELAPSE_CHECK_END("MakeCryptData", 7)
#endif
}

void CommKTxroShot::MakeStorage(CString sendId, CString filename, char *xmlData)
{

	tinyxml2::XMLDocument doc;
	XMLDeclaration * pDecl = doc.NewDeclaration();
	doc.LinkEndChild(pDecl);

	XMLElement *pRoot = doc.NewElement("MAS");
	pRoot->SetAttribute("method","req_storage");
	doc.LinkEndChild(pRoot);

	//string strMulti = CW2A(L"유니코드를 멀티바이트로 변환");
	// 한글변환
	// 멀티바이트 => 유니코드 => UTF-8 로 변환
	wstring filename_uni = CA2W(filename);
	string filename_u8 = CW2A(filename_uni.c_str(), CP_UTF8);	


	XMLElement *pElem = doc.NewElement("Filename");
	//pElem->LinkEndChild(doc.NewText(filename.GetBuffer()));
	pElem->LinkEndChild(doc.NewText(filename_u8.c_str()));
	pRoot->LinkEndChild(pElem);

	XMLElement *pElem2 = doc.NewElement("CustomMessageID");
	pElem2->LinkEndChild(doc.NewText(sendId.GetBuffer()));
	pRoot->LinkEndChild(pElem2);

	filename.ReleaseBuffer();
	sendId.ReleaseBuffer();

	XMLPrinter printer;
	doc.Accept( &printer ); 

	sprintf(xmlData,"%s",(char *)printer.CStr()); 	
}

BOOL CommKTxroShot::GetAuthKeyNPath(char *buf, XroshotSendInfo **info)
{
	string temp = buf;

	int startidx = temp.find("<?");

	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetAuthKeyNPath(), XML 파싱 실패"));
        return FALSE;
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return FALSE;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return FALSE;

	char *pszResult = (char *)pElem->GetText();  //결과값
	if(pszResult == NULL || strcmp(pszResult,"0") != 0)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetAuthKeyNPath(), Authkey Path 획득 실패"));
		return FALSE;
	}

	pElem = pRoot->FirstChildElement("AuthTicket");
    if(!pElem) return FALSE;

	CString fileAuthKey = (char *)pElem->GetText();  //결과값

	pElem = pRoot->FirstChildElement("Path");
    if(!pElem) return FALSE;

	CString filePath = (char *)pElem->GetText();  //결과값

	pElem = pRoot->FirstChildElement("CustomMessageID");
    if(!pElem) return FALSE;

	CString sendId = (char *)pElem->GetText();  //결과값

	*info = FindMMSInfoById(sendId);
	if( *info == NULL )
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetAuthKeyNPath(), MMS맵에 없는 MMS 발송"));
		return FALSE;
	}

	(*info)->_fileAuthKey	= fileAuthKey;
	(*info)->_filePath	= filePath;

	return TRUE;
}

void CommKTxroShot::GetUploadServerInfo(XroshotSendInfo *info)
{
	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS 서버 정보 얻기"));
	
	//CString uploadInfo = FindMCSServer(_T("rcs.xroshot.com"), _T("catalogs/FUS-TCSMSG/recommended/0"));
	CString uploadInfo = FindMCSServer(_center_url, _T("catalogs/FUS-TCSMSG/recommended/0"));

	// xml 로 분석

	string temp;
	temp = (LPSTR)(LPCTSTR)uploadInfo.GetBuffer();

	int startidx = temp.find("<?");

	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadServerInfo(), XML 파싱 실패"));
	    return;
	}

    temp = temp.substr(startidx,temp.length());


    tinyxml2::XMLDocument doc;
	//doc.Parse(pxmldata);
	doc.Parse(temp.c_str());

	XMLElement *pRoot = doc.FirstChildElement("RCP");
	if(!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return;

	char *pszResult = (char *)pElem->GetText();  //결과값

	if(pszResult == NULL || strcmp(pszResult,"0"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadServerInfo(), CommKTxroShot MMS 업로드 서버정보 얻기 실패(%s)"), pszResult);
	    return;
	}

	pElem = pRoot->FirstChildElement("Resource")->FirstChildElement("Address");
    if(!pElem) return;

    char *pszAddress = (char *)pElem->GetText();  //IP주소
    CString fileServerIP = pszAddress;

	pElem = pRoot->FirstChildElement("Resource")->FirstChildElement("Port");
    if(!pElem) return;

	char *pszPort = (char *)pElem->GetText();  //포트번호
    CString fileServerPort = pszPort;

	_fileServerIP = fileServerIP;
	_fileServerPort = fileServerPort;

	// MMS파일 업로드
	UpLoadFile(info);
}

void CommKTxroShot::UpLoadFile()
{
	_cs.Lock();

	map<CString, XroshotSendInfo*>::iterator it = _sendList.begin();
	while( it != _sendList.end() )
	{
		XroshotSendInfo* info = (XroshotSendInfo*)it->second;

		if( info->_fileAuthKey.IsEmpty() == FALSE &&
			info->_filePath.IsEmpty() == FALSE )
			MakeFileEncryptData(info);
		it++;
	}
	_cs.Unlock();
}

void CommKTxroShot::UpLoadFile(XroshotSendInfo* info)
{
	if( info->_fileAuthKey.IsEmpty() == FALSE &&
		info->_filePath.IsEmpty() == FALSE )
		MakeFileEncryptData(info);
}

void CommKTxroShot::MakeFileEncryptData(XroshotSendInfo* info)
{
	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS FILE Encrypt"));
	
  // 키 할당
    //
    //byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
    //memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
    //char* rawKey="f4150d4a1ac5708c29e437749045a39a";
    //hex2byte(rawKey, strlen(rawKey), key);

	byte key[16];
	memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
	memcpy(key,_secretKeySHA1,16);  //원래 20 이엇음

   // IV 할당
    byte iv[CryptoPP::AES::BLOCKSIZE];
    memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE );
    //char* rawIv="86afc43868fea6abd40fbf6d5ed50905";
	char* rawIv="00000000000000000000000000";
    hex2byte(rawIv, strlen(rawIv), iv);

	
	std::string base64encodedciphertext = info->_filePath;
	std::string decryptedtext;
	std::string base64decryptedciphertext;

    //
    // Base64 디코딩
    //
    CryptoPP::StringSource(base64encodedciphertext, true,
         new CryptoPP::Base64Decoder(
             new CryptoPP::StringSink( base64decryptedciphertext)
         ) // Base64Encoder
    ); // StringSource

	//memcpy(key, _FileAuthKey.GetBuffer(), strlen(_FileAuthKey.GetBuffer())+1);

    //
    // AES 복호화
    //
    CryptoPP::AES::Decryption aesDecryption(key, 
       CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption 
       cbcDecryption(aesDecryption, iv );

    CryptoPP::StreamTransformationFilter 
       stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
    stfDecryptor.Put( reinterpret_cast<const unsigned char*>
       (base64decryptedciphertext.c_str()), base64decryptedciphertext.size());
    stfDecryptor.MessageEnd();

	info->_decryptFilePath = decryptedtext.c_str();

	UploadFileToSocket(info, info->_decryptFilePath);
}


BOOL CommKTxroShot::UploadFileToSocket(XroshotSendInfo* info, CString URI)
{
	char XMLData[MMS_MAX_SIZE] = {0};

	CString method;
	CString host;
	CString ContentLength;
	CString xfus;
	CString space;
	
	CString http_header;

	method.Format(_T("POST %s HTTP/1.1\r\n"), URI);
	host.Format(_T("HOST: %s\r\n"), _fileServerIP);
	xfus.Format(_T("X-FUS-Authentication: %s\r\n"), info->_fileAuthKey);
	space.Format(_T("\r\n"));

#ifdef FILE_CHUNK
	CString boundary = _T("kpmbile251234xcvblklkertlqsflsfasf");

	CString ContentType;
	ContentType.Format(_T("Content-Type: multipart/form-data; boundary=%s\r\n"), boundary);

	CString begin_chunk, end_chunk;

	begin_chunk.Format(_T("--%s\r\nContent-Disposition: form-data; name=\"file1\"; filename=\"%s\"\r\nContent-Type: image/jpeg\r\n\r\n"), boundary, info->_filename);
	
	end_chunk.Format(_T("\r\n--%s--"), boundary);

	int begin_chunk_size= begin_chunk.GetLength();
	int end_chunk_size = end_chunk.GetLength();
#endif 

	int	content_size;

#ifdef FILE_CHUNK
	content_size = begin_chunk_size + info->_filesize + end_chunk_size;
#else
	content_size = info->_filesize;
#endif

	ContentLength.Format(_T("Content-Length: %d\r\n"), content_size);


	http_header += method;
	http_header += host;
	http_header += ContentLength;
	http_header += xfus;
#ifdef FILE_CHUNK
	http_header += ContentType;
#endif
	http_header += space;

	int header_size		= http_header.GetLength();

	int pos = 0;
	memcpy(XMLData+pos, http_header.GetBuffer(), header_size);
	pos += header_size;

#ifdef FILE_CHUNK
	memcpy(XMLData+pos, begin_chunk.GetBuffer(), begin_chunk_size);
	pos += begin_chunk_size;
#endif

	memcpy(XMLData+pos, info->_filebuf, info->_filesize);
	pos += info->_filesize;

#ifdef FILE_CHUNK
	memcpy(XMLData+pos, end_chunk.GetBuffer(), end_chunk_size);
#endif

	int packet_size = header_size + content_size;

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS FILE Upload"));

	SendMMSFileData(XMLData, packet_size);

	return TRUE;
}

void CommKTxroShot::SendMMSFileData(char *buf, int size)
{
	CSocket mmsfileSocket;
	if( mmsfileSocket.Create() == 0 )
	{
        Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::SendMMSFileData(), MMS socket 생성 실패"));
		return;
	}

	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS 서버(ip:%s, port:%s) 연결시도"), _fileServerIP, _fileServerPort);

	if( mmsfileSocket.Connect(_fileServerIP, _ttoi(_fileServerPort.GetBuffer()) ) == FALSE )
	{
        Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::SendMMSFileData(), MMS 서버 연결 실패"));
		return;
	}

	int retval = -1;
	if( (retval = mmsfileSocket.Send(buf, size)) == -1 )
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::SendMMSFileData(), MMS 서버로 파일 전송 실패"));
		return;
	}

	char recvBuf[1024] = {0};
	if( retval = mmsfileSocket.Receive(recvBuf, sizeof(recvBuf)) == -1)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::SendMMSFileData(), MMS 서버로 파일 전송후, 응답 수신 실패"));
		return;
	}

	GetUploadResult(recvBuf);
}


void CommKTxroShot::GetUploadResult(char *resbuf)
{
	string temp;
	temp = (LPSTR)(LPCTSTR)resbuf;

	int startidx = temp.find("<?");

	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadResult(), XML 파싱 실패"));
		return;
	}

	temp = temp.substr(startidx,temp.length());


	tinyxml2::XMLDocument doc;
	//doc.Parse(pxmldata);
	doc.Parse(temp.c_str());

	XMLElement *pRoot = doc.FirstChildElement("FUP");
	if(!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
	if(!pElem) return;

	char *pszResult = (char *)pElem->GetText();  //결과값

	if(pszResult == NULL || strcmp(pszResult,"0"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadResult(), CommKTxroShot 업로드 서버정보 얻기 실패(%s)"), pszResult);
		return;
	}

	pElem = pRoot->FirstChildElement("Path");
	if(!pElem) return;

	char *pszPath = (char *)pElem->GetText();

	pElem = pRoot->FirstChildElement("Message");
	if(!pElem) return;

	char *pszMessage = (char *)pElem->GetText();

	XroshotSendInfo* info = NULL;
	if( (info = FindMMSInfo(pszPath)) != NULL ) 
	{
		RequestUploadDone(info);
	}
}

XroshotSendInfo* CommKTxroShot::FindMMSInfo(CString descrytPath)
{
	_cs.Lock();

	map<CString , XroshotSendInfo*>::iterator it;
	for (it=_sendList.begin();it!=_sendList.end();it++) 
	{
		XroshotSendInfo* info = (XroshotSendInfo*)it->second;

		if( info->_decryptFilePath == descrytPath ) {
			_cs.Unlock();
			return info;
		}
	}

	_cs.Unlock();

	return NULL;
}

XroshotSendInfo* CommKTxroShot::FindMMSInfoById(CString sendId)
{
	_cs.Lock();

	map<CString, XroshotSendInfo*>::iterator it = _sendList.find(sendId);
	if (it == _sendList.end()) {
		_cs.Unlock();
		return NULL;
	}

	_cs.Unlock();

	return it->second;
}

void CommKTxroShot::RequestUploadDone(XroshotSendInfo* info)
{
	char xmlbuf[1024] = {0};

	tinyxml2::XMLDocument doc;
	XMLDeclaration * pDecl = doc.NewDeclaration();
	doc.LinkEndChild(pDecl);


	XMLElement *pRoot = doc.NewElement("MAS");
	pRoot->SetAttribute("method","req_finish_upload");
	doc.LinkEndChild(pRoot);

	XMLElement *pElem = doc.NewElement("Path");
	pElem->LinkEndChild(doc.NewText(info->_filePath.GetBuffer()));
	pRoot->LinkEndChild(pElem);

	char szfilesize[10];
	sprintf(szfilesize, "%d", info->_filesize);

	XMLElement *pElem2 = doc.NewElement("FileSize");
	pElem2->LinkEndChild(doc.NewText(szfilesize));
	pRoot->LinkEndChild(pElem2);

	XMLElement *pElem3 = doc.NewElement("CustomMessageID");
	pElem3->LinkEndChild(doc.NewText(info->_sendId.GetBuffer()));
	pRoot->LinkEndChild(pElem3);

	XMLPrinter printer;
	doc.Accept( &printer ); 

	sprintf(xmlbuf,"%s",(char *)printer.CStr()); 	

	// 원래 서버로 완료 요청을 보낸다.
	_socket.SendChar(xmlbuf);
}


void CommKTxroShot::GetUploadComplete(char *resbuf)
{
	// xml 로 분석

	string temp;
	temp = (LPSTR)(LPCTSTR)resbuf;

	int startidx = temp.find("<?");

	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadComplete(), XML 파싱 실패"));
	    return;
	}

    temp = temp.substr(startidx,temp.length());


    tinyxml2::XMLDocument doc;
	//doc.Parse(pxmldata);
	doc.Parse(temp.c_str());

	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return;

	char *pszResult = (char *)pElem->GetText();  //결과값

	if(pszResult == NULL || strcmp(pszResult,"0"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadComplete(), CommKTxroShot 업로드 서버정보 얻기 실패(%s)"), pszResult);
	    return;
	}

	pElem = pRoot->FirstChildElement("CustomMessageID");
    if(!pElem) return;

    CString sendId = (char *)pElem->GetText();  //IP주소

	_cs.Lock();

	map<CString, XroshotSendInfo*>::iterator it = _sendList.find(sendId);
	if( it != _sendList.end() ) 
	{
		SendMMSByUploadDone(it->second);	
	}
	_cs.Unlock();
}

void CommKTxroShot::MakeMMSData(XroshotSendInfo *info, char *pxmldata)
{

	CTime    CurTime = CTime::GetCurrentTime();
	CString  CD    =   CurTime.Format("%Y%m%d%H%M%S");

	tinyxml2::XMLDocument doc;
	XMLDeclaration * pDecl = doc.NewDeclaration();
	doc.LinkEndChild(pDecl);

	XMLElement *pRoot = doc.NewElement("MAS");
	pRoot->SetAttribute("method","req_send_message_2");
	doc.LinkEndChild(pRoot);

	XMLElement *pElem = doc.NewElement("MessageType");
	pElem->LinkEndChild(doc.NewText(info->_msgtype));
	pRoot->LinkEndChild(pElem);

	pElem = doc.NewElement("MessageSubType");
	pElem->LinkEndChild(doc.NewText(info->_msgsubtype));
	pRoot->LinkEndChild(pElem);

	pElem = doc.NewElement("CallbackNumber");
	pElem->LinkEndChild(doc.NewText(info->_enc_callbackNO));
	pRoot->LinkEndChild(pElem);

	pElem = doc.NewElement("CustomMessageID");
	pElem->LinkEndChild(doc.NewText(info->_sendId));
	pRoot->LinkEndChild(pElem);

	pElem = doc.NewElement("Message");
	pRoot->LinkEndChild(pElem);

	//int size = info->_enc_receiveNO.GetSize();
	//for(int i = 0; i < size; i++ )
	//{
		CString seqNo; 
		seqNo.Format(_T("%d"), 1);

		CString receiveNo = info->_enc_receiveNO;

		XMLElement *pSubElem = doc.NewElement("ReceiveNumber");
		pSubElem->SetAttribute("seqNo", seqNo);
		pSubElem->LinkEndChild(doc.NewText(receiveNo));
		pElem->LinkEndChild(pSubElem);
	//}

	//string strMulti = CW2A(L"유니코드를 멀티바이트로 변환");
	// 한글변환
	// 멀티바이트 => 유니코드 => UTF-8 로 변환
	wstring subject_uni = CA2W(info->_subject);
	string subject_u8 = CW2A(subject_uni.c_str(),CP_UTF8);	

	pSubElem = doc.NewElement("Subject");
	//pSubElem->LinkEndChild(doc.NewText(info->_subject));
	pSubElem->LinkEndChild(doc.NewText(subject_u8.c_str()));
	pElem->LinkEndChild(pSubElem);

	pSubElem = doc.NewElement("Content");
	pSubElem->LinkEndChild(doc.NewText(info->_enc_msg));
	pElem->LinkEndChild(pSubElem);

	pSubElem = doc.NewElement("Attachment");
	pSubElem->SetAttribute("attachID","1");
	pSubElem->LinkEndChild(doc.NewText(info->_filePath));
	pElem->LinkEndChild(pSubElem);

	char fileSize[10];
	sprintf(fileSize, _T("%d"), info->_filesize);

	pSubElem = doc.NewElement("FileSize");
	pSubElem->LinkEndChild(doc.NewText(fileSize));
	pElem->LinkEndChild(pSubElem);

	// 팩스
	if (info->_msgtype == _T("3")) {

		char pageCount[10];
		sprintf(pageCount, _T("%d"), info->_pageCount);

		pSubElem = doc.NewElement("PageCount");
		pSubElem->LinkEndChild(doc.NewText(pageCount));
		pElem->LinkEndChild(pSubElem);
	}

	XMLPrinter printer;
	//printer.SetStreamPrinting();
	doc.Accept( &printer ); 

	sprintf(pxmldata,"%s",(char *)printer.CStr());
}


void CommKTxroShot::Ping(int tid)
{
	if( _isLogin == FALSE )
		return;

	TRACE("CommKTxroShot Send Ping\n");

	if( _bNonePingPacket  )
	{
		memset(_xmlPingBuf,0x00,sizeof(_xmlPingBuf));

		MakePing(_xmlPingBuf);

		_bNonePingPacket = FALSE;
	}

	_socket.SendChar(_xmlPingBuf);
}

void CommKTxroShot::MakePing(TCHAR* xmlbuf)
{
	tinyxml2::XMLDocument doc;
	XMLDeclaration * pDecl = doc.NewDeclaration();
	doc.LinkEndChild(pDecl);

	XMLElement *pRoot = doc.NewElement("MAS");
	pRoot->SetAttribute("method","req_ping");
	doc.LinkEndChild(pRoot);

	XMLElement *pElem = doc.NewElement("Result");
	pElem->LinkEndChild(doc.NewText("0"));
	pRoot->LinkEndChild(pElem);

	XMLPrinter printer;
	doc.Accept( &printer ); 

	sprintf(xmlbuf,"%s",(char *)printer.CStr());                    // char* 를 반환한다.
}

void CommKTxroShot::OnReceive(TCHAR *buf, int size)
{
	//DWORD start = GetTickCount();


	AttachToReceiveBuf((BYTE*)buf, size);

	ProcessReceive(_receiveBuf);


	//DWORD end = GetTickCount();
	//DWORD duration = end - start;
	//TRACE(_T("경과시간%d\n"), duration);
}


void CommKTxroShot::AttachToReceiveBuf(BYTE* buf, int len)
{
	ASSERT( _receiveBufSize+len < XROSHOT_RECEIVE_MAX_SIZE );

	memcpy(_receiveBuf+_receiveBufSize, buf, len);

	_receiveBufSize += len;
}

void CommKTxroShot::DetachFromReceiveBuf(int detachLen, int restLen)
{
	ASSERT( _receiveBufSize == (detachLen+restLen) );

	//int restbufsize = _receiveBufSize - detachLen;

	char *tempbuf = _receiveBuf+detachLen;

	memcpy(_receiveBuf, tempbuf, restLen);

	memset(_receiveBuf+restLen, 0x00, XROSHOT_RECEIVE_MAX_SIZE-restLen);

	_receiveBufSize = restLen;
}

void CommKTxroShot::ProcessReceive(char *buf)
{
	//int method = AnalysisReceive(buf);

	int msgId;
	short int msglen;

	int restlen = 0;
	while( (restlen = AnalysisPacket(&msgId, &msglen)) >= 0 )
	{
		//if (restlen < 0)
		//{
		//	TRACE("패킷량이 부족하다. 다시 받아라\n");
		//	return;
		//}

		if (msgId == -999)
			GetUploadResult(buf);
		else
		{
			switch (msgId)
			{
			case XSHOT_METHOD_RES_AUTH:
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_auth"));

				// 서버 시간 얻고
				GetTime(buf);

				// 로그인 요청
				if (_isLogin == FALSE)
					RequestLogin();
				break;

			case XSHOT_METHOD_RES_REGIST:
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_regist"));

				// 로그인(인증)응답
				if (_isLogin == FALSE)
					confirmLogin(buf);
				break;
			case XSHOT_METHOD_RES_SEND_MESSAGE:
				// 개별건에 대한 응답
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_send_message"));

				GetResponse(buf);

				break;
			case XSHOT_METHOD_RES_SEND_MESSAGE_ALL:
				// 대표건에 대한 응답. 
				// 반드시 위의 XSHOT_METHOD_RES_SEND_MESSAGE 를 받고 
				// XSHOT_METHOD_RES_SEND_MESSAGE_ALL를 받는다.
				
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_send_message_all"));

				break;
			case XSHOT_METHOD_REQ_REPORT:
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), req_report"));
				
				GetResult(buf);

				break;
			case XSHOT_METHOD_RES_STORAGE:
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_storage"));

				{
					XroshotSendInfo *info = NULL;

					if (GetAuthKeyNPath(buf, &info))
						GetUploadServerInfo(info);
				}

				break;
			case XSHOT_METHOD_RES_FINISH_UPLOAD:
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_finish_upload"));
				GetUploadComplete(buf);
				break;
			case XSHOT_METHOD_RES_CONVERT:
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_convert"));
				GetResConvert(buf);
				break;
			case XSHOT_METHOD_RES_PING:
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_ping"));
				break;
			case XSHOT_METHOD_REQ_UNREGIST:
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), req_unregist"));
				break;
			case XSHOT_METHOD_RES_UNREGIST:
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_unregist"));
				break;
			default:
				break;
			};
		}

		//처리된 패킷을 지운다.
		if (restlen == 0)
		{
			memset(_receiveBuf, 0x00, sizeof(_receiveBuf));
			_receiveBufSize = 0;
			break;
		}
		else if (restlen > 0)
		{
			//패킷량이 초과이다. 처리된 패킷을 지운다.
			// 메세지량, 초과된량
			DetachFromReceiveBuf(msglen, restlen);

			// 패킷 최소단위
			//if (_receiveBufSize > 16)
			//	ProcessReceive(_receiveBuf);

			if (_receiveBufSize <= 16) {
				TRACE(_T("남아있는 버퍼량가 작아서 다시 받는다."));
				break;
			}
		}
	}

	return;
}

void CommKTxroShot::OnSend(int nErrorCode)
{

}

void CommKTxroShot::GetResponse(char *buf)
{
	char            result[8];
	char            time[32];
	char            sendId[30];
	char            telcoInfo[8];
	char            jobId[16];

	string temp = buf;

	int startidx = temp.find("<?");
	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetResponse(), XML 파싱 실패"));
        return;
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return;

	wsprintf(result,"%s",(char *)pElem->GetText());  //결과값

	pElem = pRoot->FirstChildElement("Time");
    if(!pElem) return;
	
	wsprintf(time,"%s",(char *)pElem->GetText());  //서버시간

    pElem = pRoot->FirstChildElement("CustomMessageID");
    if(!pElem) return;
	
	wsprintf(sendId,"%s",(char *)pElem->GetText());  //

	pElem = pRoot->FirstChildElement("JobID");
    if(!pElem) return;
	
	wsprintf(jobId,"%s",(char *)pElem->GetText());

	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MSG요청에 대한 응답(result:%s, kshot_number:%s, jobid:%s)"), result, sendId, jobId);

	g_sendManager->ResponseFromTelecom(sendId, _ttoi(result), time);
}

void CommKTxroShot::GetResult(char *buf)
{
	char  jobId[16];

	if( ParsingResult(buf, jobId) )
	{
		// 결과 응답
		char xmlbuf[256] = {0};

		MakeXMLResReport(jobId, xmlbuf);

		//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MSG요청 결과 전송"));

		_socket.SendChar(xmlbuf);
	}
}

bool CommKTxroShot::ParsingResult(char *xmlbuf, char* jobId)
{
	///*
	char            result[8];
	char            time[32];
	char            sendId[30];
	char            telcoInfo[8];
	char            seqNo[16];

	string temp = xmlbuf;

	int startidx = temp.find("<?");
	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::ParsingResult(), XML 파싱 실패"));
        return (false);
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return (false);

	// 전송결과
	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return (false);

	wsprintf(result,"%s",(char *)pElem->GetText());

	// 메세지가 전달된 시간
	pElem = pRoot->FirstChildElement("Time");
    if(!pElem) return (false);
	
	wsprintf(time,"%s",(char *)pElem->GetText());

	// sendId
	pElem = pRoot->FirstChildElement("CustomMessageID");
    if(!pElem) return (false);
	
	wsprintf(sendId,"%s",(char *)pElem->GetText());  

	// MCS에서 부여한 jobId
	pElem = pRoot->FirstChildElement("JobID");
    if(!pElem) return (false);
	
	wsprintf(jobId,"%s",(char *)pElem->GetText());

	// 통신사
	pElem = pRoot->FirstChildElement("TelcoInfo");
    if(!pElem) return (false);
	
	wsprintf(telcoInfo,"%s",(char *)pElem->GetText());

	// SequenceNumber 
	pElem = pRoot->FirstChildElement("SequenceNumber");
    if(!pElem) return (false);
	
	wsprintf(seqNo,"%s",(char *)pElem->GetText());

	// 전송결과
	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MSG요청 결과(%s, %s) 전송"), sendId, result);

	g_sendManager->PushResult(sendId, _ttoi(result), time, _ttoi(telcoInfo));


	_cs.Lock();

	// 결과 처리 루틴에서 결과값 반영
	// 하나의 전송에는 여러 수신처(동보전송)이 있기 때문에 여러번 결과(req_report)를 받는다.
	map<CString, XroshotSendInfo*>::iterator it = _sendList.find(sendId);
	if( it != _sendList.end() ) 
	{
		XroshotSendInfo *info = it->second;

		// 아직 전송전이면 넘어간다.
		if (info->_bSent == FALSE) {
			_cs.Unlock();
			return true;
		}


		// seqNo 가 1부터 저장됨.
		int no = atoi(seqNo)-1;
		ASSERT( no >= 0 );

		//info->_result.SetAt(no, result);
		info->_result = result;
	
		// 결과가 모두 온것에 대해서는 리포트 처리를 하고, 삭제한다.
		bool alldone = true;

		//int size = info->_result.GetSize();
		//for(int i = 0; i < size; i++)
		//{
		//	CString result = info->_result.GetAt(i);
		//	if( result.IsEmpty() ) {
		//		alldone = false;
		//		break;
		//	}
		//}

		//// 모든 결과 받음.
		//if( alldone )
		//{
			delete info;
			_sendList.erase(it);
		//}
	}

	_cs.Unlock();

	return true; 
}

void CommKTxroShot::MakeXMLResReport(char *jobID,char *pxmldata)
{
	tinyxml2::XMLDocument doc;
	XMLDeclaration * pDecl = doc.NewDeclaration();
	doc.LinkEndChild(pDecl);


	XMLElement *pRoot = doc.NewElement("MAS");
	pRoot->SetAttribute("method","res_report");
	doc.LinkEndChild(pRoot);

	XMLElement *pElem = doc.NewElement("Result");
	pElem->LinkEndChild(doc.NewText("0"));
	pRoot->LinkEndChild(pElem);

	pElem = doc.NewElement("JobID");
	pElem->LinkEndChild(doc.NewText(jobID));
	pRoot->LinkEndChild(pElem);


	XMLPrinter printer;
	//printer.SetStreamPrinting();
	doc.Accept( &printer );

	wsprintf(pxmldata,"%s",(char *)printer.CStr());                    // char* 를 반환한다.
}