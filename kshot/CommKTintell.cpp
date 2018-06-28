#include "StdAfx.h"
#include "CommKTintell.h"

#include "CommManager.h"
#include "Intelli_def.h"

#include "sendManager.h"

CommKTintell::CommKTintell(CommManager *manager)
	: CommBase(manager),
	_grant_socket(this)
{
	_run_state = STATE_READY;

	memset(_receiveBuf, 0x00, sizeof(_receiveBuf));

	_receiveBufSize = 0;

	_isBind = FALSE;
}


CommKTintell::~CommKTintell(void)
{
}


BOOL CommKTintell::Init()
{
	// 환경얻기

	_grantIP	= g_config.intelli.server;
	_grantPort	= g_config.intelli.port;
	_id			= g_config.intelli.id;
	_password	= g_config.intelli.password;


	return TRUE;
}

BOOL CommKTintell::Uninit()
{
	_grant_socket.Close();

	Log(CMdlLog::LEVEL_EVENT, _T("CommKTintell, 종료"));

	return CommBase::Uninit();
}

BOOL CommKTintell::Connect()
{
	return ConnectGrantServer();
}

BOOL CommKTintell::SendSMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg)
{
	char *callbackNo;
	char *receiveNo;
	char *message;

	int sid = _ttoi(sendId.GetBuffer(0));
	
	CString r = receiveNO;

	callbackNo	= callbackNO.GetBuffer(0);
	receiveNo	= r.GetBuffer(0);
	message		= msg.GetBuffer(0);

    memset(&_pkt_deliver,0x00,sizeof(_pkt_deliver));

    _pkt_deliver.Frame_Start0 = 0xfe; ///
    _pkt_deliver.Frame_Start1 = 0xfe;
    _pkt_deliver.MSGLEN = htons(210);
	_pkt_deliver.MSGID = htonl(SUBMIT_REQ);
    _pkt_deliver.Request_ID = htonl(0x00);
    _pkt_deliver.Version_MSB = 0x01;
    _pkt_deliver.Version_LSB = 0x00;
    _pkt_deliver.Filler = htons(0x00);   //reserved 

	_pkt_deliver.PID = htonl(0);  
	_pkt_deliver.SERIAL_NUM = htonl(sid);  

	if( strlen(callbackNo) <= sizeof(_pkt_deliver.mCALLBACK) )
		strncpy(_pkt_deliver.mCALLBACK,callbackNo,strlen(callbackNo));
	else
		strncpy(_pkt_deliver.mCALLBACK,callbackNo,sizeof(_pkt_deliver.mCALLBACK));

	if( strlen(receiveNo) <= sizeof(_pkt_deliver.DESTADDR) )
		strncpy(_pkt_deliver.DESTADDR,receiveNo,strlen(receiveNo));
	else
		strncpy(_pkt_deliver.DESTADDR,receiveNo,sizeof(_pkt_deliver.DESTADDR));

	// 연동규격의 버퍼(120byte)를 다 채우면 결과가 않오는 버그 때문에 10byte 보정
	if( strlen(message) <= (sizeof(_pkt_deliver.MESSAGE)-10) )
		strncpy(_pkt_deliver.MESSAGE,message,strlen(message));
	else
		strncpy(_pkt_deliver.MESSAGE,message,(sizeof(_pkt_deliver.MESSAGE)-10));

	_pkt_deliver.Frame_End0 = 0xfd;
    _pkt_deliver.Frame_End1 = 0xfd;

	_socket.SendBin((byte*)&_pkt_deliver, sizeof(_pkt_deliver));

	return TRUE;
}

BOOL CommKTintell::SendLMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg)
{
	ASSERT(0);
	
	return TRUE;
}

BOOL CommKTintell::SendMMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size)
{
	ASSERT(0);

	return true;
}

BOOL CommKTintell::SendFAX(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size)
{
	ASSERT(0);

	return true;
}

void CommKTintell::Ping(int tid)
{
	if( _isBind == FALSE )
		return;

	CTime curTime = CTime::GetCurrentTime();

	CTimeSpan TimeDiff;

	TimeDiff = curTime  - _lastLinkCheckTime;

	// 7초(7000/1000) 이상이 경과 되었으면, 링크체크 시도
	if(TimeDiff.GetTotalSeconds() > (INTELLI_TIMER_INTERVAL/1000) )
	{
		//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTintell::Ping(), Send LinkReq()"));

		LinkReq();
	}
}

void CommKTintell::OnClose(int nErrorCode)
{
	// 재연결 시도 
	// 자동 연결에 대한 로직 필요
	if( _run_state == STATE_BINDED )
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTintell::OnClose(), 소켓 연결이 끊김(ErrorCode=%d)"), nErrorCode);

		_socket.Close();

		_run_state = STATE_READY;

		Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTintell::OnClose(), CommKTintelli 연결 재시도."));

		// 재연결 시도
		//ConnectGrantServer();
		CWnd* cwnd = AfxGetMainWnd();
		HWND hwnd = cwnd->GetSafeHwnd();
		PostMessage(hwnd, WM_RECONNECT_TELECOM, TELECOM_KT_INTELLI, 0);
	}
}

void CommKTintell::OnConnect(int nErrorCode)
{
	if( _run_state == STATE_GRANTING )
	{
		Log(CMdlLog::LEVEL_EVENT, _T("CommKTintell GrantServer Connected"));
		
		GrantReq();
	}
	else
	{
		Log(CMdlLog::LEVEL_EVENT, _T("CommKTintell BindServer Connected"));

		BindReq();
	}
}

void CommKTintell::OnReceive(TCHAR *buf, int size)
{
	AttachToReceiveBuf((BYTE*)buf, size);

	ProcessReceive((BYTE*)_receiveBuf);
}

void CommKTintell::AttachToReceiveBuf(BYTE* buf, int len)
{
	ASSERT( _receiveBufSize+len < INTELLI_RECEIVE_MAX_SIZE );

	memcpy(_receiveBuf+_receiveBufSize, buf, len);

	_receiveBufSize += len;
}

void CommKTintell::DetachFromReceiveBuf(int detachLen, int restLen)
{
	ASSERT( _receiveBufSize == (detachLen+restLen) );

	//int restbufsize = _receiveBufSize - detachLen;

	char *tempbuf = _receiveBuf+detachLen;

	memcpy(_receiveBuf, tempbuf, restLen);

	memset(_receiveBuf+restLen, 0x00, INTELLI_RECEIVE_MAX_SIZE-restLen);

	_receiveBufSize = restLen;
}

bool CommKTintell::ProcessReceive(BYTE* buf)
{
	//int msgid;
	//int len = ResponseAnalysis(buf, &msgid);

	int msgId;
	short int msglen;

	int retval = 0;
	while ((retval = AnalysisPacket(&msgId, &msglen)) >= 0)
	{
		//if (retval < 0)
		//{
		//	TRACE("패킷량이 부족하다. 다시 받아라\n");
		//	return false;
		//}

		if (msgId == GRANT_ACK) // 시간응답
		{
			memcpy(&_pkt_grantack, buf, msglen);

			_bindServerIP = _pkt_grantack.SERVER_IP;
			_bindServerPort = ntohl(_pkt_grantack.PORT);
			_bindId = _pkt_grantack.BIND_ID;

			//Logmsg(CMdlLog::LEVEL_EVENT, _T("CommKTintell::ProcessReceive(), grant_ack(ip:%s, port:%d)"),	_bindServerIP, _bindServerPort);

			// grant 연결 해제
			_grant_socket.Close();

			// bind 연결 시도
			ConnectBindServer();
		}
		else if (msgId == BIND_ACK)
		{
			memcpy(&_pkt_bindack, buf, msglen);

			int result = ntohl(_pkt_bindack.RESULT);
			int tr_max_mps = ntohl(_pkt_bindack.TR_MAX_MPS);

			if (result == 0)
			{
				Logmsg(CMdlLog::LEVEL_EVENT, _T("CommKTintell::ProcessReceive(), CommKTintell 로그인 성공(%d)"), result);

				_isBind = TRUE;

				_run_state = STATE_BINDED;

				// 주기적인 핑 전송을 위해 최상위 윈도우에게 Timer 요청
				//CWnd* cwnd = AfxGetMainWnd();
				//HWND hwnd = cwnd->GetSafeHwnd();
				//PostMessage(hwnd, WM_PING_TIMER, INTELLI_TIMER_ID, 0);
			}
			else
			{
				Logmsg(CMdlLog::LEVEL_ERROR, _T("CommKTintell::ProcessReceive(), CommKTintell 로그인 실패(%d)"), result);
			}
		}
		else if (msgId == SUBMIT_ACK)
		{
			_lastLinkCheckTime = CTime::GetCurrentTime();

			memcpy(&_pkt_deliverack, buf, msglen);

			int result = ntohl(_pkt_deliverack.RESULT);
			int serial_num = ntohl(_pkt_deliverack.SERIAL_NUM);

			//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTintell::ProcessReceive(), submit_ack(result:%d, kshot_number:%d)"), result, serial_num);

			char kshot_number[20];
			sprintf(kshot_number, _T("%d"), serial_num);

			g_sendManager->ResponseFromTelecom(kshot_number, result, NULL);
		}
		else if (msgId == REPORT_REQ)
		{
			memcpy(&_pkt_report, buf, msglen);

			int request_id = ntohl(_pkt_report.Request_ID);

			int result = ntohl(_pkt_report.RESULT);
			int serial_num = ntohl(_pkt_report.SERIAL_NUM);

			CString destAddr = _pkt_report.DESTADDR;
			CString deliverTime = _pkt_report.DELIVERY_TIME;

			int telecom = ntohl(_pkt_report.TELECOM_ID);

			//Logmsg(CMdlLog::LEVEL_DEBUG, _T("CommKTintell::ProcessReceive(), report_req(result:%d, kshot_number:%d)"), result, serial_num);

			char kshot_number[20];
			sprintf(kshot_number, _T("%d"), serial_num);


			// 이통사 코드 통합하기
			/*
			[크로샷]
			1: SKT, 2:KT, 3:LG, 4:AHNN, 5:DACOM

			[지능망]
			0:Unkwon, 1 : KT, 2 : DACOM, 3 : SK Broadband, 4 : SKT, 5 : KTF, 6 : LGT"

			1: SKT, 2 : KT, 3 : LG, 4 : AHNN, 5 : DACOM,
			6 : SK Broadband,
			7 : KTF,
			8 : LGT"
			0 : Unkwon

			*/

			if (telecom == 1)
				telecom = 2;
			else if (telecom == 2)
				telecom = 5;
			else if (telecom == 3)
				telecom = 6;
			else if (telecom == 4)
				telecom = 1;
			else if (telecom == 5)
				telecom = 7;
			else if (telecom == 6)
				telecom = 8;
			else
				telecom = 0;

			g_sendManager->PushResult(kshot_number, result, deliverTime.GetBuffer(), telecom);

			// report ack
			ReportAck(request_id);
		}
		else if (msgId == LINKCHK_REQ)
		{
			//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTintell::ProcessReceive(), linkchk_req"));

			memcpy(&_pkt_linesend, buf, msglen);

			int request_id = ntohl(_pkt_linesend.Request_ID);

			LinkAck(request_id);
		}
		else if (msgId == LINKCHK_ACK)
		{
			//Log(CMdlLog::LEVEL_DEBUG, _T("CommKTintell::ProcessReceive(), linkchk_ack"));

			_lastLinkCheckTime = CTime::GetCurrentTime();
		}

		//처리된 패킷을 지운다.
		if (retval == 0)
		{
			memset(_receiveBuf, 0x00, sizeof(_receiveBuf));
			_receiveBufSize = 0;
			break;
		}
		else if (retval > 0)
		{
			//패킷량이 초과이다. 처리된 패킷을 지운다.
			// 메세지량, 초과된량
			DetachFromReceiveBuf(msglen, retval);

			// 패킷 최소단위
			//if (_receiveBufSize > 16)
			//	ProcessReceive((BYTE*)_receiveBuf);

			if (_receiveBufSize <= 16) {
				TRACE(_T("남아있는 버퍼량가 작아서 다시 받는다."));
				break;
			}
		}
	}

	return true;
}

void CommKTintell::OnSend(int nErrorCode)
{

}

BOOL CommKTintell::ConnectGrantServer()
{
	if(_grant_socket.Create() == 0)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTintell GrantServer 소켓 생성 실패"));
		return FALSE;
	}

	_run_state = STATE_GRANTING;

	Logmsg(CMdlLog::LEVEL_EVENT, _T("CommKTintell::ConnectGrantServer(), CommKTintell GrantServer 연결시도(%s,%d)"), _grantIP, _ttoi(_grantPort.GetBuffer(0)) );

	if( _grant_socket.Connect(_grantIP, _ttoi(_grantPort.GetBuffer(0)) ) ==  FALSE)
	{
		int err;
		if((err = GetLastError()) != WSAEWOULDBLOCK) 
		{
			CString msg;
			msg.Format(_T("CommKTintell::ConnectGrantServer(), 서버 연결 실패 ErrorCode(%d)"), err);
			Log(CMdlLog::LEVEL_ERROR, msg);
			return FALSE;
		}
	}

	_grantPort.ReleaseBuffer();

	return TRUE;
}

BOOL CommKTintell::ConnectBindServer()
{
	if(_socket.Create() == 0)
	{
		Log(CMdlLog::LEVEL_ERROR, _T("CommKTintell 소켓 생성 실패"));
		return FALSE;
	}

	_run_state = STATE_BINDING;

	if( _socket.Connect(_T(_bindServerIP), _bindServerPort) == FALSE )
	{
		int err;
		if((err = GetLastError()) != WSAEWOULDBLOCK) 
		{
			CString msg;
			msg.Format(_T("CommKTintell::ConnectBindServer(), 서버 연결 실패 ErrorCode(%d)"), err);
			Log(CMdlLog::LEVEL_ERROR, msg);

			return FALSE;
		}
	}

	return TRUE;
}

void CommKTintell::GrantReq()
{
    memset(&_pkt_grant,0x00,sizeof(_pkt_grant));

    _pkt_grant.Frame_Start0 = 0xfe; ///
    _pkt_grant.Frame_Start1 = 0xfe;
    _pkt_grant.MSGLEN = htons(34);
	_pkt_grant.MSGID = htonl(GRANT_REQ);
    _pkt_grant.Request_ID = htonl(0x00);
    _pkt_grant.Version_MSB = 0x01;
    _pkt_grant.Version_LSB = 0x00;
    _pkt_grant.Filler = htons(0x00);   //reserved 

	strncpy(_pkt_grant.LOGIN_ID,_id.GetBuffer(),strlen(_id.GetBuffer()));
    
	_pkt_grant.Frame_End0 = 0xfd;
    _pkt_grant.Frame_End1 = 0xfd;

	//memcpy(_grant_socket->_sendBuffer + _grant_socket->_sendTotalByte,(LPCSTR)&_pkt_grant, sizeof(_pkt_grant));  //큐에 집어 넛는다.
 //   _grant_socket->_sendTotalByte += sizeof(_pkt_grant);

	//SendBufferData(_grant_socket);

	_grant_socket.SendBin((byte*)&_pkt_grant, sizeof(_pkt_grant));
}

void CommKTintell::BindReq()
{
    memset(&_pkt_bind,0x00,sizeof(_pkt_bind));

    _pkt_bind.Frame_Start0 = 0xfe; ///
    _pkt_bind.Frame_Start1 = 0xfe;
    _pkt_bind.MSGLEN = htons(68);
	_pkt_bind.MSGID = htonl(BIND_REQ);
    _pkt_bind.Request_ID = htonl(0x00);
    _pkt_bind.Version_MSB = 0x01;
    _pkt_bind.Version_LSB = 0x00;
    _pkt_bind.Filler = htons(0x00);   //reserved 

	strncpy(_pkt_bind.LOGIN_ID,_id.GetBuffer(),strlen(_id.GetBuffer()));
	strncpy(_pkt_bind.LOGIN_PWD,_password.GetBuffer(),strlen(_password.GetBuffer()));
	strncpy(_pkt_bind.BIND_ID,_bindId.GetBuffer(),strlen(_bindId.GetBuffer()));

	_pkt_bind.RV_MO_FLAG = 0x00;
	_pkt_bind.RV_REPORT_FLAG = 0x01;  //////////////
    
	_pkt_bind.Frame_End0 = 0xfd;
    _pkt_bind.Frame_End1 = 0xfd;

	//memcpy(_bind_socket->_sendBuffer + _bind_socket->_sendTotalByte,(LPCSTR)&_pkt_bind, sizeof(_pkt_bind));  //큐에 집어 넛는다.
	//_bind_socket->_sendTotalByte += sizeof(_pkt_bind);

	//SendBufferData(_bind_socket);

	_socket.SendBin((byte*)&_pkt_bind, sizeof(_pkt_bind));
}

int	CommKTintell::ResponseAnalysis(char * resbuf, int *msgid)
{
	byte *buf = (byte*)resbuf;

	short int	len;
	int			id;

	memcpy((short int *)&len, buf+2, 2);
    memcpy((int *)&id, buf+4, 4);

	*msgid = ntohl(id);

	return ntohs(len);
}

int CommKTintell::AnalysisPacket(int* msgId, short int* msglen)
{
	int			id;
	short int	len;

	memcpy((short int *)&len, _receiveBuf+2, 2);
    memcpy((int *)&id, _receiveBuf+4, 4);

	*msgId = ntohl(id);
	*msglen = ntohs(len);

	if( *msglen > _receiveBufSize )
		return -1;
	else if( *msglen < _receiveBufSize )
		return (_receiveBufSize - *msglen);
	else
		return 0;
}

void CommKTintell::LinkReq()
{
    memset(&_pkt_linesend,0x00,sizeof(_pkt_linesend));

    _pkt_linesend.Frame_Start0 = 0xfe; ///
    _pkt_linesend.Frame_Start1 = 0xfe;
    _pkt_linesend.MSGLEN = htons(18);
	_pkt_linesend.MSGID = htonl(LINKCHK_REQ);
    _pkt_linesend.Request_ID = htonl(0x00);
    _pkt_linesend.Version_MSB = 0x01;
    _pkt_linesend.Version_LSB = 0x00;
    _pkt_linesend.Filler = htons(0x00);   //reserved 
  
	_pkt_linesend.Frame_End0 = 0xfd;
    _pkt_linesend.Frame_End1 = 0xfd;

	_socket.SendBin((byte*)&_pkt_linesend, sizeof(_pkt_linesend));
}

void CommKTintell::LinkAck(int request_id)
{
    memset(&_pkt_linesendack,0x00,sizeof(_pkt_linesendack));

    _pkt_linesendack.Frame_Start0 = 0xfe; ///
    _pkt_linesendack.Frame_Start1 = 0xfe;
    _pkt_linesendack.MSGLEN = htons(18);
	_pkt_linesendack.MSGID = htonl(LINKCHK_ACK);
    _pkt_linesendack.Request_ID = htonl(request_id);
    _pkt_linesendack.Version_MSB = 0x01;
    _pkt_linesendack.Version_LSB = 0x00;
    _pkt_linesendack.Filler = htons(0x00);   //reserved 
  
	_pkt_linesendack.Frame_End0 = 0xfd;
    _pkt_linesendack.Frame_End1 = 0xfd;

	_socket.SendBin((byte*)&_pkt_linesendack, sizeof(_pkt_linesendack));
}


void CommKTintell::ReportAck(int request_id)
{
    memset(&_pkt_reportack,0x00,sizeof(_pkt_reportack));

    _pkt_reportack.Frame_Start0 = 0xfe; ///
    _pkt_reportack.Frame_Start1 = 0xfe;
    _pkt_reportack.MSGLEN = htons(22);
	_pkt_reportack.MSGID = htonl(REPORT_ACK);
    
	_pkt_reportack.Request_ID = htonl(request_id);

    _pkt_reportack.Version_MSB = 0x01;
    _pkt_reportack.Version_LSB = 0x00;
    _pkt_reportack.Filler = htons(0x00);   //reserved 

	_pkt_reportack.RESULT = htonl(0);

	_pkt_reportack.Frame_End0 = 0xfd;
    _pkt_reportack.Frame_End1 = 0xfd;

	_socket.SendBin((byte*)&_pkt_reportack, sizeof(_pkt_reportack));
}
