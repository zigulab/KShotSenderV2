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
	// ȯ����
	//_center_url = g_config.xroshot.center;
	//_id			= g_config.xroshot.id;
	//_password	= g_config.xroshot.password;
	//_certFile.Format(_T(".\\certkey\\%s"), g_config.xroshot.certkey);

	Logmsg(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::Init(), CommKTxroShot, id:%s, pwd:%s, key:%s"), _id, _password, _certFile);

	_mmsServerIP = g_config.kshot.mmsServerIP;
	_mmsServerPort= g_config.kshot.mmsServerPort;

	// ��ȣȭ Ű ����
	MakeShaKey();

	memset(_iv, 0x00, CryptoPP::AES::BLOCKSIZE);
	char* rawIv = "00000000000000000000000000";
	hex2byte(rawIv, strlen(rawIv), _iv);

	memset(_key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH);
	memcpy(_key, _secretKeySHA1, 16);  //���� 20 �̾���


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

	Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot, ����"));

	return CommBase::Uninit();
}

BOOL CommKTxroShot::Connect()
{
	if( _socket.Create() == 0 )
	{
		Log(CMdlLog::LEVEL_ERROR, _T("Xroshot ���� ���� ����"));
		return FALSE;
	}

	int port = _ttoi(_serverPort.GetBuffer());

	Logmsg(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::Connect(), CommKTxroShot ����õ�(%s,%d)"), _serverIP, port );

	if( _socket.Connect(_serverIP, port) == FALSE )
	{
		int err;
		if((err = GetLastError()) != WSAEWOULDBLOCK) 
		{
			CString msg;
			msg.Format(_T("���� ���� ���� ErrorCode(%d)"), err);
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
	unsigned char* digest = sha1->getDigest(); //�� ������ free() ���ٲ�.

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

	//�̰��� 16����Ʈ �����Ѵ�.
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
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetServerInfo(), XML �Ľ� ����"));
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

	char *pszResult = (char *)pElem->GetText();  //�����

	if(pszResult == NULL || strcmp(pszResult,"0"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetServerInfo(), ������,��Ʈ ��� ����(%s)"), pszResult);
		return FALSE;
	}

	pElem = pRoot->FirstChildElement("Resource")->FirstChildElement("Address");
    if(!pElem) return FALSE;

    char *pszAddress = (char *)pElem->GetText();  //IP�ּ�
    _serverIP = pszAddress;

	pElem = pRoot->FirstChildElement("Resource")->FirstChildElement("Port");
    if(!pElem) return FALSE;

	char *pszPort = (char *)pElem->GetText();  //��Ʈ��ȣ
    _serverPort = pszPort;

	return TRUE;

}

void CommKTxroShot::RequestAuth()
{
	char xmlbuf[256] = {0};

	MakeAuth(xmlbuf);

	memcpy(_socket._sendBuffer + _socket._toSendBufSize,(LPCSTR)xmlbuf, strlen(xmlbuf) + 1);  //ť�� ���� �Ӵ´�.
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
	//���� �����ʹ� ���⼭ �Ҹ�ȴ�.....  
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
		// ���� ��Ŷ ���� NULL���� �����ؼ� ���۵ȴ�. �ϴ� +1
		// �׷��� , NULL ���Ե��� ���� ��쵵 �ִ�.
		*msglen = idx + marker2.GetLength() + 1;
	}
	else
		return -1;
		
	int detachLen = (_receiveBufSize - *msglen);

	// NULL �����̳� ���������� -1�� ���̰� ����� ��찡 �ִ�.
	if( detachLen < -1 ) {
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::AnalysisPacket(), Invalid Packet"));
		Log(CMdlLog::LEVEL_ERROR, _receiveBuf );
		
		//ASSERT( 0 );
		// �߸��� ���۸� ����.
		memset(_receiveBuf, 0x00, sizeof(_receiveBuf));
		_receiveBufSize = 0;

		return -1;
	}

	*msgId = AnalysisReceive(_receiveBuf);

	if( *msgId == -1 ) {
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::AnalysisPacket(), Invalid msgId"));
		Log(CMdlLog::LEVEL_ERROR, _receiveBuf );

		//ASSERT( 0 );
		// �߸��� ���۸� ����.
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
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::AnalysisReceive(), XML �Ľ� ����"));
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
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetTime(), xml �Ľ� ����"));
        return;
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return;

	char *pszResult = (char *)pElem->GetText();  //�����

	if(pszResult == NULL || strcmp(pszResult,"0"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetTime(), �ð���� ����"),pszResult);
	    return;
	}

	pElem = pRoot->FirstChildElement("Time");
    if(!pElem) return;

	
	char *pszTime = (char *)pElem->GetText();  //�����ð�
	_serverTime = pszTime;

	TRACE(_T("���� �ð�:%s\n"), _serverTime);
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
	// �� �޼ҵ忡�� �޸𸮴����� �߻���. 10�� ����
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
	unsigned char* digest = sha1->getDigest(); //�� ������ free() ���ٲ�.

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

	//�̰��� 16����Ʈ �����Ѵ�.
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

	//AES��ȣȭ ����..
	CryptoPP::AES::Encryption 
		aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
	CryptoPP::CBC_Mode_ExternalCipher::Encryption 
		cbcEncryption(aesEncryption, iv);

	CryptoPP::StreamTransformationFilter 
		stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext), CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING);
	stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plaintext.c_str()), plainTextLength + 1);  ///+1 �� �����ΰ�...
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
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::confirmLogin(), XML �Ľ� ����"));
        return;
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return;

	char *pszResult = (char *)pElem->GetText();  //�����

	if(pszResult == NULL || strcmp(pszResult,"0") == 0)
	{
		Logmsg(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot �α��� ����(%s)"), pszResult);

		_isLogin = TRUE;
	}
	else 
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot �α��� ����(%s)"), pszResult);

		_isLogin = FALSE;
	}	
}

void CommKTxroShot::OnClose(int nErrorCode)
{
	Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::OnClose(), ���� ������ ������ϴ�.(%d)"), nErrorCode);

	_isConnnted = FALSE;
	_isLogin = FALSE;

	_socket.Close();

	Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::OnClose(), ���� ��õ�."));

	CWnd* cwnd = AfxGetMainWnd();
	HWND hwnd = cwnd->GetSafeHwnd();
	PostMessage(hwnd, WM_RECONNECT_TELECOM, TELECOM_KT_XROSHOT, 0);
}

void CommKTxroShot::OnConnect(int nErrorCode)
{
	Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot Server Connected"));

	_isConnnted = TRUE;

	// �ֱ����� �������� ���� �ֻ��� �����쿡�� Timer ��û
	//CWnd* cwnd = AfxGetMainWnd();
	//HWND hwnd = cwnd->GetSafeHwnd();
	//PostMessage(hwnd, WM_PING_TIMER, XROSHOT_TIMER_ID, 0);

	// ���� ������ Auth(time)��û
	RequestAuth();
}


void AsciiToUTF8(CString parm_ascii_string, CString &parm_utf8_string) 
{
     parm_utf8_string.Empty(); 
 
     // �ƽ�Ű �ڵ带 UTF8������ �ڵ�� ��ȯ�ؾ� �Ѵ�. �ƽ�Ű �ڵ带 UTF8 �ڵ�� ��ȯ�Ҷ��� 
     // �ƽ�Ű �ڵ带 �����ڵ�� ���� ��ȯ�ϰ� ��ȯ�� �����ڵ带 UTF8 �ڵ�� ��ȯ�ؾ� �Ѵ�.
 
     // �ƽ�Ű �ڵ�ε� ���ڿ��� �����ڵ�ȭ ������ ���� ���̸� ���Ѵ�.
      int temp_length = MultiByteToWideChar(CP_ACP, 0, (char *)(const char *)parm_ascii_string, -1, NULL, 0);
     // ��ȯ�� �����ڵ带 ������ ������ �Ҵ��Ѵ�.
     BSTR unicode_str = SysAllocStringLen(NULL, temp_length + 1);
 
     // �ƽ�Ű �ڵ�ε� ���ڿ��� ���� �ڵ� ������ ���ڿ��� �����Ѵ�.
      MultiByteToWideChar(CP_ACP, 0, (char *)(const char *)parm_ascii_string, -1, unicode_str, temp_length);
 
     // �����ڵ� ������ ���ڿ��� UTF8 �������� ���������� �ʿ��� �޸� ������ ũ�⸦ ��´�.
     temp_length = WideCharToMultiByte( CP_UTF8, 0, unicode_str, -1, NULL, 0, NULL, NULL );
 
     if(temp_length > 0){
        CString str;
        // UTF8 �ڵ带 ������ �޸� ������ �Ҵ��Ѵ�.
        char *p_utf8_string = new char[temp_length];
        memset(p_utf8_string, 0, temp_length);
        // �����ڵ带 UTF8�ڵ�� ��ȯ�Ѵ�.
        WideCharToMultiByte(CP_UTF8, 0, unicode_str, -1, p_utf8_string, temp_length, NULL, NULL);
  
        // UTF8 �������� ����� ���ڿ��� �� ������ �ڵ尪���� �� URL�� ���Ǵ� �������� ��ȯ�Ѵ�.
        for(int i = 0; i < temp_length - 1; i++){
            if(p_utf8_string[i] & 0x80){
                // ���� �ڵ尡 �ѱ��� ���..
                // UTF8 �ڵ�� ǥ���� �ѱ��� 3����Ʈ�� ǥ�õȴ�. "�ѱ�"  ->  %ED%95%9C%EA%B8%80
                for(int sub_i = 0; sub_i < 3; sub_i++){
                    str.Format("%%%X", p_utf8_string[i] & 0x00FF);
                    parm_utf8_string += str;
                    i++;
                } 
    
                i--;
            } else {
                // ���� �ڵ尡 ������ ���, ������� �״�� ����Ѵ�.
               parm_utf8_string += p_utf8_string[i];
            }
        }                                                              
  
        delete[] p_utf8_string;
     }
 
      // �����ڵ� ������ ���ڿ��� �����ϱ� ���� �����ߴ� �޸𸮸� �����Ѵ�.
      SysFreeString(unicode_str);
}

// ������ ������ �ٿ�ε�
int CommKTxroShot::getFileFromHttp(char* pszUrl, int filesize, char* pszFileBuffer)
{
	const int READ_BUF_SIZE = 1024;

    HINTERNET    hInet, hUrl;
    DWORD        dwReadSize = 0;

	CString strUrl = pszUrl;
	CString strUrl_8;
	AsciiToUTF8(strUrl, strUrl_8);
	
    // WinINet�Լ� �ʱ�ȭ
    if ((hInet = InternetOpen("MyWeb",            // user agent in the HTTP protocol
                    INTERNET_OPEN_TYPE_DIRECT,    // AccessType
                    NULL,                        // ProxyName
                    NULL,                        // ProxyBypass
                    0)) != NULL)                // Options
    {
        // �Էµ� HTTP�ּҸ� ����
        if ((hUrl = InternetOpenUrl(hInet,        // ���ͳ� ������ �ڵ�
                    //pszUrl,                       // URL
					strUrl_8,
                    NULL,                        // HTTP server�� ������ �ش�
                    0,                            // �ش� ������
                    0,                            // Flag
                    0)) != NULL)                // Context
        {
            //FILE    *fp;

            // �ٿ�ε��� ���� �����
            //if ((fp = fopen(pszFile, "wb")) != NULL)
            {
                TCHAR    szBuff[READ_BUF_SIZE];
                DWORD    dwSize;
                DWORD    dwDebug = 10;

                do {
                    // ������ ���� �б�
                    InternetReadFile(hUrl, szBuff, READ_BUF_SIZE, &dwSize);

                    // ������ ������ ������� ���Ͽ� ��ֱ�
                    //fwrite(szBuff, dwSize, 1, fp);
					if( dwSize > 0 ) 
						memcpy(pszFileBuffer+dwReadSize, szBuff, dwSize);

                    dwReadSize += dwSize;
                } while ((dwSize != 0) || (--dwDebug != 0));

               //fclose( fp );
            }

            // ���ͳ� �ڵ� �ݱ�
            InternetCloseHandle(hUrl);
        }
		else
			Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot, getFileFromHttp() InternetOpenUrl FALSE"));

        // ���ͳ� �ڵ� �ݱ�
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

	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS AuthKey�� ���� ��ġ ��û(kshot_number:%s, filename:%s"), sendId, filename);

	return TRUE;
}

void CommKTxroShot::SendMMSByUploadDone(XroshotSendInfo *info)
{
	if (info->_msgtype == "3") {
		// ��ȯ��û
		SendReqConvert(info);
	}
	else {
		//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS Encrpyt Data ����"));

		MakeEncryptionData(info);

		//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS Packet ����"));

		// �幮 ����
		char xmlbuf[4096 + 1024] = { 0 };
		MakeMMSData(info, xmlbuf);

		//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS ����"));

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

	// ���� ������ �Ϸ� ��û�� ������.
	_socket.SendChar(xmlbuf);
}

void CommKTxroShot::GetResConvert(char *buf)
{
	string temp = buf;

	int startidx = temp.find("<?");

	if (startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetResConvert(), XML �Ľ� ����"));
		return;
	}

	temp = temp.substr(startidx, temp.length());

	tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if (!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
	if (!pElem) return;
	char *pszResult = (char *)pElem->GetText();  //�����

	if (pszResult == NULL || strcmp(pszResult, "0") != 0)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetResConvert(), Authkey Path ȹ�� ����"));
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
	CString sendId = (char *)pElem->GetText();  //�����

	XroshotSendInfo *info = FindMMSInfoById(sendId);
	if (info == NULL)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetResConvert(), Fax ���� ��ȯ�� Info������ ����."));
		return ;
	}

	info->_filePath = path;
	info->_pageCount = count;
	
	SendFAXByDoneConvert(info);
}

void CommKTxroShot::SendFAXByDoneConvert(XroshotSendInfo *info)
{
	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS Encrpyt Data ����"));

	MakeEncryptionData(info);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS Packet ����"));

	// �幮 ����
	char xmlbuf[4096 + 1024] = { 0 };
	MakeMMSData(info, xmlbuf);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS ����"));

	_socket.SendChar(xmlbuf);
}

BOOL CommKTxroShot::SendLMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg)
{
	XroshotSendInfo *info = AddSendInfo(sendId, callbackNO, receiveNO, subject, msg, _MSG_TYPE_LMS, _MSG_SUBTYPE_TEXT);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot LMS Encrpyt Data ����"));

	MakeEncryptionData(info);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot LMS Packet ����"));
	
	// �幮 ����
	char xmlbuf[4096+1024] = {0};
	MakeSMSDataFromFile(info, xmlbuf);

	_socket.SendChar(xmlbuf);

	info->_bSent = TRUE;

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot LMS ����"));

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

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot SMS Encrpyt Data ����"));

	MakeEncryptionData(info);

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot SMS Packet ����"));

	// �ܹ� ����
	char xmlbuf[2048] = {0};
	MakeSMSData(info, xmlbuf);

	_socket.SendChar(xmlbuf);

	info->_bSent = TRUE;

	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot SMS ����"));

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


	//string strMulti = CW2A(L"�����ڵ带 ��Ƽ����Ʈ�� ��ȯ");
	// �ѱۺ�ȯ
	// ��Ƽ����Ʈ => �����ڵ� => UTF-8 �� ��ȯ
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
		//string strMulti = CW2A(L"�����ڵ带 ��Ƽ����Ʈ�� ��ȯ");
		// �ѱۺ�ȯ
		// ��Ƽ����Ʈ => �����ڵ� => UTF-8 �� ��ȯ
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

void CommKTxroShot::MakeCryptData(const char *OrgData,char *CryptData)  //���������͸� �޾Ƽ� ��ȣȭ�ؼ� �ѱ��ش�.
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
	//memcpy(key,_secretKeySHA1,16);  //���� 20 �̾���

	unsigned int plainTextLength = plaintext.length();

	//AES��ȣȭ ����..
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

	stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plaintext.c_str()), plainTextLength + 1);  ///+1 �� �����ΰ�...
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

	//string strMulti = CW2A(L"�����ڵ带 ��Ƽ����Ʈ�� ��ȯ");
	// �ѱۺ�ȯ
	// ��Ƽ����Ʈ => �����ڵ� => UTF-8 �� ��ȯ
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
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetAuthKeyNPath(), XML �Ľ� ����"));
        return FALSE;
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return FALSE;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return FALSE;

	char *pszResult = (char *)pElem->GetText();  //�����
	if(pszResult == NULL || strcmp(pszResult,"0") != 0)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetAuthKeyNPath(), Authkey Path ȹ�� ����"));
		return FALSE;
	}

	pElem = pRoot->FirstChildElement("AuthTicket");
    if(!pElem) return FALSE;

	CString fileAuthKey = (char *)pElem->GetText();  //�����

	pElem = pRoot->FirstChildElement("Path");
    if(!pElem) return FALSE;

	CString filePath = (char *)pElem->GetText();  //�����

	pElem = pRoot->FirstChildElement("CustomMessageID");
    if(!pElem) return FALSE;

	CString sendId = (char *)pElem->GetText();  //�����

	*info = FindMMSInfoById(sendId);
	if( *info == NULL )
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetAuthKeyNPath(), MMS�ʿ� ���� MMS �߼�"));
		return FALSE;
	}

	(*info)->_fileAuthKey	= fileAuthKey;
	(*info)->_filePath	= filePath;

	return TRUE;
}

void CommKTxroShot::GetUploadServerInfo(XroshotSendInfo *info)
{
	//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS ���� ���� ���"));
	
	//CString uploadInfo = FindMCSServer(_T("rcs.xroshot.com"), _T("catalogs/FUS-TCSMSG/recommended/0"));
	CString uploadInfo = FindMCSServer(_center_url, _T("catalogs/FUS-TCSMSG/recommended/0"));

	// xml �� �м�

	string temp;
	temp = (LPSTR)(LPCTSTR)uploadInfo.GetBuffer();

	int startidx = temp.find("<?");

	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadServerInfo(), XML �Ľ� ����"));
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

	char *pszResult = (char *)pElem->GetText();  //�����

	if(pszResult == NULL || strcmp(pszResult,"0"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadServerInfo(), CommKTxroShot MMS ���ε� �������� ��� ����(%s)"), pszResult);
	    return;
	}

	pElem = pRoot->FirstChildElement("Resource")->FirstChildElement("Address");
    if(!pElem) return;

    char *pszAddress = (char *)pElem->GetText();  //IP�ּ�
    CString fileServerIP = pszAddress;

	pElem = pRoot->FirstChildElement("Resource")->FirstChildElement("Port");
    if(!pElem) return;

	char *pszPort = (char *)pElem->GetText();  //��Ʈ��ȣ
    CString fileServerPort = pszPort;

	_fileServerIP = fileServerIP;
	_fileServerPort = fileServerPort;

	// MMS���� ���ε�
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
	
  // Ű �Ҵ�
    //
    //byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
    //memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
    //char* rawKey="f4150d4a1ac5708c29e437749045a39a";
    //hex2byte(rawKey, strlen(rawKey), key);

	byte key[16];
	memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
	memcpy(key,_secretKeySHA1,16);  //���� 20 �̾���

   // IV �Ҵ�
    byte iv[CryptoPP::AES::BLOCKSIZE];
    memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE );
    //char* rawIv="86afc43868fea6abd40fbf6d5ed50905";
	char* rawIv="00000000000000000000000000";
    hex2byte(rawIv, strlen(rawIv), iv);

	
	std::string base64encodedciphertext = info->_filePath;
	std::string decryptedtext;
	std::string base64decryptedciphertext;

    //
    // Base64 ���ڵ�
    //
    CryptoPP::StringSource(base64encodedciphertext, true,
         new CryptoPP::Base64Decoder(
             new CryptoPP::StringSink( base64decryptedciphertext)
         ) // Base64Encoder
    ); // StringSource

	//memcpy(key, _FileAuthKey.GetBuffer(), strlen(_FileAuthKey.GetBuffer())+1);

    //
    // AES ��ȣȭ
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
        Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::SendMMSFileData(), MMS socket ���� ����"));
		return;
	}

	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MMS ����(ip:%s, port:%s) ����õ�"), _fileServerIP, _fileServerPort);

	if( mmsfileSocket.Connect(_fileServerIP, _ttoi(_fileServerPort.GetBuffer()) ) == FALSE )
	{
        Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::SendMMSFileData(), MMS ���� ���� ����"));
		return;
	}

	int retval = -1;
	if( (retval = mmsfileSocket.Send(buf, size)) == -1 )
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::SendMMSFileData(), MMS ������ ���� ���� ����"));
		return;
	}

	char recvBuf[1024] = {0};
	if( retval = mmsfileSocket.Receive(recvBuf, sizeof(recvBuf)) == -1)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::SendMMSFileData(), MMS ������ ���� ������, ���� ���� ����"));
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
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadResult(), XML �Ľ� ����"));
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

	char *pszResult = (char *)pElem->GetText();  //�����

	if(pszResult == NULL || strcmp(pszResult,"0"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadResult(), CommKTxroShot ���ε� �������� ��� ����(%s)"), pszResult);
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

	// ���� ������ �Ϸ� ��û�� ������.
	_socket.SendChar(xmlbuf);
}


void CommKTxroShot::GetUploadComplete(char *resbuf)
{
	// xml �� �м�

	string temp;
	temp = (LPSTR)(LPCTSTR)resbuf;

	int startidx = temp.find("<?");

	if(startidx == string::npos)  //not found
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadComplete(), XML �Ľ� ����"));
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

	char *pszResult = (char *)pElem->GetText();  //�����

	if(pszResult == NULL || strcmp(pszResult,"0"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetUploadComplete(), CommKTxroShot ���ε� �������� ��� ����(%s)"), pszResult);
	    return;
	}

	pElem = pRoot->FirstChildElement("CustomMessageID");
    if(!pElem) return;

    CString sendId = (char *)pElem->GetText();  //IP�ּ�

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

	//string strMulti = CW2A(L"�����ڵ带 ��Ƽ����Ʈ�� ��ȯ");
	// �ѱۺ�ȯ
	// ��Ƽ����Ʈ => �����ڵ� => UTF-8 �� ��ȯ
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

	// �ѽ�
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

	sprintf(xmlbuf,"%s",(char *)printer.CStr());                    // char* �� ��ȯ�Ѵ�.
}

void CommKTxroShot::OnReceive(TCHAR *buf, int size)
{
	//DWORD start = GetTickCount();


	AttachToReceiveBuf((BYTE*)buf, size);

	ProcessReceive(_receiveBuf);


	//DWORD end = GetTickCount();
	//DWORD duration = end - start;
	//TRACE(_T("����ð�%d\n"), duration);
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
		//	TRACE("��Ŷ���� �����ϴ�. �ٽ� �޾ƶ�\n");
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

				// ���� �ð� ���
				GetTime(buf);

				// �α��� ��û
				if (_isLogin == FALSE)
					RequestLogin();
				break;

			case XSHOT_METHOD_RES_REGIST:
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_regist"));

				// �α���(����)����
				if (_isLogin == FALSE)
					confirmLogin(buf);
				break;
			case XSHOT_METHOD_RES_SEND_MESSAGE:
				// �����ǿ� ���� ����
				//Log(CMdlLog::LEVEL_EVENT, _T("CommKTxroShot::ProcessReceive(), res_send_message"));

				GetResponse(buf);

				break;
			case XSHOT_METHOD_RES_SEND_MESSAGE_ALL:
				// ��ǥ�ǿ� ���� ����. 
				// �ݵ�� ���� XSHOT_METHOD_RES_SEND_MESSAGE �� �ް� 
				// XSHOT_METHOD_RES_SEND_MESSAGE_ALL�� �޴´�.
				
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

		//ó���� ��Ŷ�� �����.
		if (restlen == 0)
		{
			memset(_receiveBuf, 0x00, sizeof(_receiveBuf));
			_receiveBufSize = 0;
			break;
		}
		else if (restlen > 0)
		{
			//��Ŷ���� �ʰ��̴�. ó���� ��Ŷ�� �����.
			// �޼�����, �ʰ��ȷ�
			DetachFromReceiveBuf(msglen, restlen);

			// ��Ŷ �ּҴ���
			//if (_receiveBufSize > 16)
			//	ProcessReceive(_receiveBuf);

			if (_receiveBufSize <= 16) {
				TRACE(_T("�����ִ� ���۷��� �۾Ƽ� �ٽ� �޴´�."));
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
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::GetResponse(), XML �Ľ� ����"));
        return;
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return;

	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return;

	wsprintf(result,"%s",(char *)pElem->GetText());  //�����

	pElem = pRoot->FirstChildElement("Time");
    if(!pElem) return;
	
	wsprintf(time,"%s",(char *)pElem->GetText());  //�����ð�

    pElem = pRoot->FirstChildElement("CustomMessageID");
    if(!pElem) return;
	
	wsprintf(sendId,"%s",(char *)pElem->GetText());  //

	pElem = pRoot->FirstChildElement("JobID");
    if(!pElem) return;
	
	wsprintf(jobId,"%s",(char *)pElem->GetText());

	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MSG��û�� ���� ����(result:%s, kshot_number:%s, jobid:%s)"), result, sendId, jobId);

	g_sendManager->ResponseFromTelecom(sendId, _ttoi(result), time);
}

void CommKTxroShot::GetResult(char *buf)
{
	char  jobId[16];

	if( ParsingResult(buf, jobId) )
	{
		// ��� ����
		char xmlbuf[256] = {0};

		MakeXMLResReport(jobId, xmlbuf);

		//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MSG��û ��� ����"));

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
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTxroShot::ParsingResult(), XML �Ľ� ����"));
        return (false);
	}

    temp = temp.substr(startidx,temp.length());

    tinyxml2::XMLDocument doc;
	doc.Parse(temp.c_str());


	XMLElement *pRoot = doc.FirstChildElement("MAS");
	if(!pRoot) return (false);

	// ���۰��
	XMLElement *pElem = pRoot->FirstChildElement("Result");
    if(!pElem) return (false);

	wsprintf(result,"%s",(char *)pElem->GetText());

	// �޼����� ���޵� �ð�
	pElem = pRoot->FirstChildElement("Time");
    if(!pElem) return (false);
	
	wsprintf(time,"%s",(char *)pElem->GetText());

	// sendId
	pElem = pRoot->FirstChildElement("CustomMessageID");
    if(!pElem) return (false);
	
	wsprintf(sendId,"%s",(char *)pElem->GetText());  

	// MCS���� �ο��� jobId
	pElem = pRoot->FirstChildElement("JobID");
    if(!pElem) return (false);
	
	wsprintf(jobId,"%s",(char *)pElem->GetText());

	// ��Ż�
	pElem = pRoot->FirstChildElement("TelcoInfo");
    if(!pElem) return (false);
	
	wsprintf(telcoInfo,"%s",(char *)pElem->GetText());

	// SequenceNumber 
	pElem = pRoot->FirstChildElement("SequenceNumber");
    if(!pElem) return (false);
	
	wsprintf(seqNo,"%s",(char *)pElem->GetText());

	// ���۰��
	//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTxroShot MSG��û ���(%s, %s) ����"), sendId, result);

	g_sendManager->PushResult(sendId, _ttoi(result), time, _ttoi(telcoInfo));


	_cs.Lock();

	// ��� ó�� ��ƾ���� ����� �ݿ�
	// �ϳ��� ���ۿ��� ���� ����ó(��������)�� �ֱ� ������ ������ ���(req_report)�� �޴´�.
	map<CString, XroshotSendInfo*>::iterator it = _sendList.find(sendId);
	if( it != _sendList.end() ) 
	{
		XroshotSendInfo *info = it->second;

		// ���� �������̸� �Ѿ��.
		if (info->_bSent == FALSE) {
			_cs.Unlock();
			return true;
		}


		// seqNo �� 1���� �����.
		int no = atoi(seqNo)-1;
		ASSERT( no >= 0 );

		//info->_result.SetAt(no, result);
		info->_result = result;
	
		// ����� ��� �°Ϳ� ���ؼ��� ����Ʈ ó���� �ϰ�, �����Ѵ�.
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

		//// ��� ��� ����.
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

	wsprintf(pxmldata,"%s",(char *)printer.CStr());                    // char* �� ��ȯ�Ѵ�.
}