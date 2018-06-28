// Client.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "Client.h"

#include "kshotDlg.h"
#include "sendManager.h"

// Client

CString	Client::_curDate = "";
CString	Client::_curDateAck = "";

CStdioFile	Client::_sendLogFile;

CStdioFile	Client::_reportAckFile;

IMPLEMENT_DYNCREATE(Client, CWinThread)

Client::Client()
: _socket(this)
{
	_no		= -1;
	_bUse	= false;
	_bBind	= false;


	_receiveBuf = (char*)malloc(CLIENT_RECEIVE_MAX_SIZE);

	//memset(_receiveBuf, 0x00, sizeof(_receiveBuf));
	memset(_receiveBuf, 0x00, CLIENT_RECEIVE_MAX_SIZE);

	_assigned_size = CLIENT_RECEIVE_MAX_SIZE;

	_receiveBufSize = 0;

	//_curDate = "";
}

Client::~Client()
{
	Logmsg(CMdlLog::LEVEL_DEBUG, _T("Client::~Client(), Client(%d) 객체 소멸"), _no);
}

BOOL Client::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	//CWnd* cwnd = AfxGetMainWnd();
	//HWND hwnd = cwnd->GetSafeHwnd();

	// 다른 쓰레드에서 아래와 같이 쓰면 엉뚱한 포인트를 전달함.
	//CkshotDlg* dlg = (CkshotDlg*)AfxGetMainWnd();

	CkshotDlg* dlg = (CkshotDlg*) AfxGetApp()->GetMainWnd();

	_sendManager = &(dlg->_sendManager);

	return TRUE;
}

int Client::ExitInstance()
{
	_socket.Close();

	free(_receiveBuf);

	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(Client, CWinThread)
	ON_THREAD_MESSAGE(WM_NEW_CONNECTION, OnNewConnection)
	ON_THREAD_MESSAGE(WM_CLOSE_FOR_NOREPLY, OnCloseForNoReply)
END_MESSAGE_MAP()


// Client 메시지 처리기입니다.

bool Client::isUse()
{
	return _bUse;
}

void Client::SetUse(bool b)
{
	memset(_receiveBuf, 0x00, sizeof(_assigned_size));
	_receiveBufSize = 0;

	_bUse = b;
}

void Client::SetNo(int no)
{
	_no = no;
}

void Client::SetParent(ClientManager *manager)
{
	_manager = manager;
}

void Client::OnClose(int nErrorCode)
{
	_bUse	= false;
	_bBind	= false;

	_socket.Close();

	_manager->DelClient(_id, this);

	Logmsg(CMdlLog::LEVEL_EVENT, _T("Client::OnClose(), 클라이언트(uid:%s) 연결 종료, errorCode:%d"), _id, nErrorCode);
}

void Client::OnReceive(byte* buf, int len)
{
	AttachToReceiveBuf(buf, len);

	ProcessReceive((BYTE*)_receiveBuf);
}

void Client::AttachToReceiveBuf(BYTE* buf, int len)
{
	//ASSERT( _receiveBufSize+len < CLIENT_RECEIVE_MAX_SIZE );

	//int assigned_size = _msize(_receiveBuf);
	
	if (_receiveBufSize + len > _assigned_size) 
	{
		int new_size = (_receiveBufSize + len);

		_receiveBuf = (char*)realloc(_receiveBuf, new_size);

		_assigned_size = new_size;

		CString msg;
		msg.Format(_T("Client::AttachToReceiveBuf(), [msg] 전송요청 수신, _assigned_size: %d \n"), _assigned_size);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}

	memcpy(_receiveBuf+_receiveBufSize, buf, len);

	_receiveBufSize += len;
}

void Client::DetachFromReceiveBuf(int detachLen, int restLen)
{
	ASSERT( _receiveBufSize == (detachLen+restLen) );

	char *tempbuf = _receiveBuf+detachLen;

	memcpy(_receiveBuf, tempbuf, restLen);

	memset(_receiveBuf+restLen, 0x00, _assigned_size -restLen);

	_receiveBufSize = restLen;
}

bool Client::ProcessReceive(BYTE* buf)
{
	int msgId;
	int msglen;

	int retval = AnalysisPacket(&msgId, &msglen);
	if( retval < 0 ) 
	{
		TRACE(_T("패킷량이 부족하다. 다음 패킷을 기다림\n"));
		return false;
	}

	switch(msgId)
	{
	case CPT_PKT_LINKCHK:
		Send_LinkChkAck();
		break;
	case CPT_PKT_LINKCHK_ACK:
		Get_LinkChkAck();
		break;
	case CPT_PKT_GRANT:
		//Log(CMdlLog::LEVEL_EVENT, _T("Client::ProcessReceive(), grant"));
		ParseGrant(buf, msglen);
		break;
	case CPT_PKT_GRANT_ACK:
		//Log(CMdlLog::LEVEL_ERROR, _T("Client::ProcessReceive(), grant_ack"));
		break;
	case CPT_PKT_BIND:
		//Log(CMdlLog::LEVEL_EVENT, _T("Client::ProcessReceive(), bind"));
		Parse_Bind(buf, msglen);
		break;
	case CPT_PKT_BIND_ACK:
		//Log(CMdlLog::LEVEL_ERROR, _T("Client::ProcessReceive(), bind_ack"));
		break;
	case CPT_PKT_MESSAGE:
		//Log(CMdlLog::LEVEL_EVENT, _T("Client::ProcessReceive(), message"));

		Logmsg(CMdlLog::LEVEL_EVENT, _T("Client::ProcessReceive(), [msg] 전송요청 수신(uid:%s)"), _id);
		
		ParseMessage(buf, msglen);
		break;
	case CPT_PKT_MESSAGE_ACK:	
		//Log(CMdlLog::LEVEL_ERROR, _T("Client::ProcessReceive(), message_ack"));
		break;
	case CPT_PKT_REPORT:
		//Log(CMdlLog::LEVEL_ERROR, _T("Client::ProcessReceive(), report_ack"));
		break;
	case CPT_PKT_REPORT_ACK:
		//Log(CMdlLog::LEVEL_EVENT, _T("Client::ProcessReceive(), report_ack(%d)"));
		ParseReportAck(buf, msglen);
		break;
	default:
		Log(CMdlLog::LEVEL_ERROR, _T("Client::ProcessReceive(), unkown packet!!"));
		// 강제로 뒤의 패킷을 모두 지운다.
		retval = 0;
		break;
	}

	//처리된 패킷을 지운다.
	if( retval == 0 )
	{
		memset(_receiveBuf, 0x00, sizeof(_receiveBuf));
		_receiveBufSize = 0;

		//TRACE(_T("처리된것:%d, 남은것:%d, bufsize:%d\n"), msglen, retval, _receiveBufSize);
	}
	else if( retval > 0 )
	{
		//패킷량이 초과이다. 처리된 패킷을 지운다.
		// 메세지량, 초과된량

		//TRACE(_T("before처리된것:%d, 남은것:%d, bufsize:%d, buf[0]:%d, buf[1]:%d \n"), msglen, retval, _receiveBufSize, _receiveBuf[0], _receiveBuf[1]);

		DetachFromReceiveBuf(msglen, retval);

		//TRACE(_T("after처리된것:%d, 남은것:%d, bufsize:%d, buf[0]:%d, buf[1]:%d \n"), msglen, retval, _receiveBufSize, _receiveBuf[0], _receiveBuf[1]);

		// 패킷 최소단위
		if( _receiveBufSize > 16 ) {
			ProcessReceive((BYTE*)_receiveBuf);
		}
	
	}

	return true;
}

int Client::AnalysisPacket(int* msgId, int* msglen)
{
	int	id;
	int	len;

	memcpy((int *)&len, _receiveBuf+2, 4);
    memcpy((int *)&id, _receiveBuf+6, 4);

	*msgId = ntohl(id);
	*msglen = ntohl(len);

	if( *msglen > _receiveBufSize )
		return -1;
	else if( *msglen < _receiveBufSize ) 
		return (_receiveBufSize - *msglen);
	else
		return 0;
}

void Client::Send_LinkChk()
{
    memset(&_pkt_linkchk,0x00,sizeof(_pkt_linkchk));

	_pkt_linkchk.PACKET_BEGIN0	= 0xfe;
	_pkt_linkchk.PACKET_BEGIN1	= 0xfe;
	_pkt_linkchk.MSGLEN			= htonl(20);
	_pkt_linkchk.MSGID			= htonl(CPT_PKT_LINKCHK);
	_pkt_linkchk.REQUESTID		= htonl(0x00);
	_pkt_linkchk.VERSION_MSB	= CkshotDlg::_msb;
	_pkt_linkchk.VERSION_LSB	= CkshotDlg::_lsb;
	_pkt_linkchk.RESERVED		= htons(0x00);
	_pkt_linkchk.PACKET_END0	= 0xfd;
	_pkt_linkchk.PACKET_END1	= 0xfd;

	_socket.SendBuffer((byte*)&_pkt_linkchk, sizeof(_pkt_linkchk));
}

void Client::Send_LinkChkAck()
{
    memset(&_pkt_linkchk_ack,0x00,sizeof(_pkt_linkchk_ack));

	_pkt_linkchk_ack.PACKET_BEGIN0	= 0xfe;
	_pkt_linkchk_ack.PACKET_BEGIN1	= 0xfe;
	_pkt_linkchk_ack.MSGLEN			= htonl(20);
	_pkt_linkchk_ack.MSGID			= htonl(CPT_PKT_LINKCHK_ACK);
	_pkt_linkchk_ack.REQUESTID		= htonl(0x00);
	_pkt_linkchk_ack.VERSION_MSB	= CkshotDlg::_msb;
	_pkt_linkchk_ack.VERSION_LSB	= CkshotDlg::_lsb;
	_pkt_linkchk_ack.RESERVED		= htons(0x00);
	_pkt_linkchk_ack.PACKET_END0	= 0xfd;
	_pkt_linkchk_ack.PACKET_END1	= 0xfd;

	_socket.SendBuffer((byte*)&_pkt_linkchk_ack, sizeof(_pkt_linkchk_ack));	
}

void Client::Get_LinkChkAck()
{
	// 마지막 체크 갱신
	_lastLinkCheckTime = CTime::GetCurrentTime();
}

void Client::ParseGrant(BYTE* buf, int len)
{
	memcpy(&_pkt_grant, buf, len);

	//int serial_num	= ntohl(_pkt_report.SERIAL_NUM);

	char login_id[16];
	memcpy(login_id, _pkt_grant.LOGIN_ID, sizeof(_pkt_grant.LOGIN_ID));
	
	//if( 등록된 login_id가 맞으면 )
	//{
		// 결과값 0
		// Bind_id 생성
		// 서버 IP와 포트 생성
		
		Send_GrantAck(0, "xzawessfsd", g_config.kshot.bindServerIP, _ttoi(g_config.kshot.bindServerPort));

	//}
}

void Client::Send_GrantAck(int result, char* bind_id, char* serverIP, int port)
{
    memset(&_pkt_grant_ack,0x00,sizeof(_pkt_grant_ack));

	_pkt_grant_ack.PACKET_BEGIN0	= 0xfe;
	_pkt_grant_ack.PACKET_BEGIN1	= 0xfe;
	_pkt_grant_ack.MSGLEN			= htonl(58);
	_pkt_grant_ack.MSGID			= htonl(CPT_PKT_GRANT_ACK);
	_pkt_grant_ack.REQUESTID		= htonl(0x00);
	_pkt_grant_ack.VERSION_MSB		= CkshotDlg::_msb;
	_pkt_grant_ack.VERSION_LSB		= CkshotDlg::_lsb;
	_pkt_grant_ack.RESERVED			= htons(0x00);
	
	_pkt_grant_ack.RESULT  			= htonl(result);
	strncpy(_pkt_grant_ack.BIND_ID, bind_id, strlen(bind_id));
	strncpy(_pkt_grant_ack.SERVER_IP, serverIP, strlen(serverIP));
	_pkt_grant_ack.PORT				= htons(port);

	_pkt_grant_ack.PACKET_END0		= 0xfd;
	_pkt_grant_ack.PACKET_END1		= 0xfd;

	_socket.SendBuffer((byte*)&_pkt_grant_ack, sizeof(_pkt_grant_ack));	
}

void Client::Parse_Bind(BYTE* buf, int len)
{
	memcpy(&_pkt_bind, buf, len);

	//int serial_num	= ntohl(_pkt_report.SERIAL_NUM);

	char login_id[16];
	char pwd[16];
	char bind_id[16];

	memcpy(login_id, _pkt_bind.LOGIN_ID, sizeof(_pkt_bind.LOGIN_ID));
	memcpy(pwd, _pkt_bind.PWD, sizeof(_pkt_bind.PWD));
	memcpy(bind_id, _pkt_bind.BIND_ID, sizeof(_pkt_bind.BIND_ID));

	Logmsg(CMdlLog::LEVEL_EVENT, _T("Client::Parse_Bind(), 로그인 시도(id:%s)"), login_id);

	// bind 요청을 분석해서, 성공하면
	if( g_sendManager->_dbManager.Login(login_id, pwd) )
	{
		_id = login_id;

		Logmsg(CMdlLog::LEVEL_EVENT, _T("Client::Parse_Bind(), 이전 데이타 Loading(id:%s)"), login_id);

		// 재연결할 때 결과알림 재시도
		_manager->AddClient(_id, this);

		_bBind = true;

		Logmsg(CMdlLog::LEVEL_EVENT, _T("Client::Parse_Bind(), Bind 성공(id:%s)"), login_id);

		// 결과값과 초당 전송수를 준다.
		Send_BindAck(0, 100);
	}
	else
	{
		Logmsg(CMdlLog::LEVEL_EVENT, _T("Client::Parse_Bind(), Bind 실패(id:%s, pwd:%s)"), login_id, pwd);

		Send_BindAck(99, 0);

		//연결을 종료한다.
		_socket.Close();

		Sleep(1);

		_bUse	= false;
		_bBind	= false;
	}
}

void Client::Send_BindAck(int result, int maxofsec)
{
	// bind 가 성공하면
    memset(&_pkt_bind_ack,0x00,sizeof(_pkt_bind_ack));

	_pkt_bind_ack.PACKET_BEGIN0	= 0xfe;
	_pkt_bind_ack.PACKET_BEGIN1	= 0xfe;
	_pkt_bind_ack.MSGLEN		= htonl(28);
	_pkt_bind_ack.MSGID			= htonl(CPT_PKT_BIND_ACK);
	_pkt_bind_ack.REQUESTID		= htonl(0x00);
	_pkt_bind_ack.VERSION_MSB	= CkshotDlg::_msb;
	_pkt_bind_ack.VERSION_LSB	= CkshotDlg::_lsb;
	_pkt_bind_ack.RESERVED		= htons(0x00);
	
	_pkt_bind_ack.RESULT  		= htonl(result);
	_pkt_bind_ack.MAX_OFSEC		= htonl(maxofsec);

	_pkt_bind_ack.PACKET_END0	= 0xfd;
	_pkt_bind_ack.PACKET_END1	= 0xfd;

	_socket.SendBuffer((byte*)&_pkt_bind_ack, sizeof(_pkt_bind_ack));	

	// 시간 초기화의 의미
	_lastLinkCheckTime = CTime::GetCurrentTime();
	_lastSendTime = _lastLinkCheckTime;
}

void Client::ParseMessage(BYTE* buf, int len)
{
	_lastSendTime = CTime::GetCurrentTime();

	// SOCKET_MSG_BASE_SIZE + TARGET_SIZE
	ASSERT(len > SOCKET_MSG_BASE_SIZE+4);

	memcpy(&_pkt_message, buf, SOCKET_MSG_BASE_SIZE+4);

	char callback[32];
	char target[32];
	char subject[100];
	char msg[2000];
	char filename[100];
	char uploaded_path[300];
	char isReserved;		// y, n
	char reservedTime[30];
	
	int msgtype = ntohl(_pkt_message.MSGTYPE);

	int send_number	= ntohl(_pkt_message.SEND_NUMBER);

	memcpy(callback, _pkt_message.CALLBACK_NO, sizeof(_pkt_message.CALLBACK_NO));
	memcpy(target, _pkt_message.TARGET_NO, sizeof(_pkt_message.TARGET_NO));
	memcpy(subject, _pkt_message.SUBJECT, sizeof(_pkt_message.SUBJECT));
	memcpy(msg, _pkt_message.MESSAGE, sizeof(_pkt_message.MESSAGE));
	memcpy(filename, _pkt_message.FILENAME, sizeof(_pkt_message.FILENAME));
	memcpy(uploaded_path, _pkt_message.UPLOADED_PATH, sizeof(_pkt_message.UPLOADED_PATH));

	int	 file_size = ntohl(_pkt_message.FILE_SIZE);

	isReserved = _pkt_message.IS_RESERVED;
	memcpy(reservedTime, _pkt_message.RESERVED_TIME, sizeof(_pkt_message.RESERVED_TIME));

	int target_size = ntohl(_pkt_message.TARGET_SIZE);

	char *target_buf = new char[target_size+1];
	
	memset(target_buf, 0, target_size+1);

	memcpy(target_buf, buf + SOCKET_MSG_BASE_SIZE + 4, target_size);
	
	CString receiveList = target_buf;

	// 일관된 등록시간을 설정 하기 위해서 등록시간 지정
	CTime    CurTime= CTime::GetCurrentTime();
	CString  strRegTime	= CurTime.Format("%Y%m%d%H%M%S");
	char* szRegTime = strRegTime.GetBuffer();


	int errorCode = 0;

	////////////////////////////////////// 유효한 메시지
	BOOL validMessage = TRUE;

	////////////////////////////////////// 메시지 길이 검사
	int msglen = _tcslen(msg);

	if (msgtype == 0) {
		if (msglen > 90) {
			errorCode = 999;
			validMessage = FALSE;
		}
	}
	else {
		if (msglen > 2000) {
			errorCode = 9999;
			validMessage = FALSE;
		}
	}
	if (validMessage == FALSE)
		Logmsg(CMdlLog::LEVEL_DEBUG, _T("Client::ParseMessage(), 메세지 길이 초과로 실패 처리(_id, msgtype, len): %s, %d, %d"), _id, msgtype, msglen);
	////////////////////////////////////// 메시지 길이 검사

	////////////////////////////////////// 첨부파일 크기 검사
	if (validMessage)
	{
		if (msgtype == 2 || msgtype == 3)
		{
			// 5M ( 1024* 1000 * 5)
			int limitsize = 1024 * 1000 * 5;
			if (file_size > limitsize)
			{
				validMessage = FALSE;
				errorCode = 9999;
				Logmsg(CMdlLog::LEVEL_DEBUG, _T("Client::ParseMessage(), 첨부파일 크기(5M) 초과로 실패 처리(_id, msgtype, len): %s, %d, %d"), _id, msgtype, msglen);
			}
		}
	}
	////////////////////////////////////// 첨부파일 크기 검사

	////////////////////////////////////// 예약발송의 유효성 검사
	if (validMessage)
	{
		COleDateTime oleTime;
		if (toupper(isReserved) == 'Y')
		{
			if (reservedTime == NULL)
				validMessage = FALSE;
			else {
				// 유효한 포맷검사
				if (CheckInvalidDateFormat(reservedTime, &oleTime) == false)
					validMessage = FALSE;
			}
			if (validMessage == FALSE) {
				errorCode = 999;
				Logmsg(CMdlLog::LEVEL_DEBUG, _T("Client::ParseMessage(), 잘못된 예약일자로 실패 처리(_id, msgtype, reservedTime): %s, %d, %s"), _id, msgtype, reservedTime);
			}
		}
	}
	////////////////////////////////////// 예약발송의 유효성 검사

	////////////////////////////////////// 잔액 처리

	// receiveNo 갯수 구하기 
	CString receiveList_clone = receiveList;
	int count = receiveList_clone.Replace(';', '/');
	
	// 잔액 조사
	float price = 0.0;
	double balance = 0.0;
	CString pid;
	CString line;

	if (validMessage) 
	{
		if (g_sendManager->_dbManager.CheckBalance(_id, msgtype, count, &price, &balance, pid, line))
		{
			// 잔액이 충분하면,  전송기록과 발송전 삭감 처리 
			if (g_sendManager->_dbManager.AddEventLog_WithdrawBalance(_id, pid, msgtype, price, count, balance, szRegTime)) {

				// 윈시데이타 기록
				// 메세지의 개행문자 삭제
				CString message = msg;
				message.Remove('\r');
				message.Remove('\n');

				CString formatedMsg;
				formatedMsg.Format(_T("%s, %d, %s, %s, %s, %s, %s, %c, %s, %s"),
												_id, msgtype, callback, subject, message,
												filename, uploaded_path, isReserved, reservedTime, receiveList);

				WriteSendRawData(formatedMsg);
			}
			else {
				validMessage = FALSE;
				errorCode = 999;
			}
		}
		else {
			// 잔액 부족 또는 use_state = 1(사용않함) 일때
			validMessage = FALSE;

			Logmsg(CMdlLog::LEVEL_DEBUG, _T("Client::ParseMessage(), 잔액부족(또는사용않함)으로 실패로 처리(_id, msgtype, count, price): %s, %d, %d, %f"), _id, msgtype, count, price);

			if (msgtype == 0)
				errorCode = 888;
			else
				errorCode = 267;
		}
	}
	////////////////////////////////////// 잔액 처리


	// CLIENT LIST UI 갱신 호출
	//CkshotDlg* dlg = (CkshotDlg*)AfxGetMainWnd();
	//dlg->UpdateClientList(_id, szRegTime, count);
	_manager->UpdateClientList(_id, szRegTime, count);
	//



	// 길이제한 체크/수정
	CString callbackNo = callback;
	if (callbackNo.GetLength() > 15)
		callbackNo = callbackNo.Left(15);

	// '-' 문자 제거
	callbackNo.Remove('-');

	Logmsg(CMdlLog::LEVEL_EVENT, _T("Client::ParseMessage(), [msg] 요청버퍼에 기록 시작(uid:%s, count:%d)"), _id, count);


	vector<SEND_QUEUE_INFO*> vtRequest;

	count = 0;

	int pos = 0;
	CString token = receiveList.Tokenize(_T(";"), pos);
	while( token != _T("") )
	{
		int idx = token.Find(':', 0);

		CString receiveNo = token.Left(idx);

		// 길이제한 체크/수정
		if (receiveNo.GetLength() > 15)
			receiveNo = receiveNo.Left(15);


		CString send_No = token.Mid(idx+1);
		
		send_number = _ttoi(send_No.GetBuffer());

		//Logmsg(CMdlLog::LEVEL_DEBUG, _T("Client::ParseMessage(), SendManager::SendProcess(%s, %d, %d, %s, %s, %s, %s, %c, %s)"),
		//									_id, msgtype, send_number, strCallbackNo, target, filename, uploaded_path, isReserved, reservedTime);

		if (validMessage)
		{
			//_sendManager->SendProcess(_id, msgtype, send_No.GetBuffer(), callbackNo.GetBuffer(), receiveNo.GetBuffer(), subject, msg,
			//	filename, uploaded_path, file_size, isReserved, reservedTime, szRegTime);

			unsigned int kshot_number = _manager->GetKShotSendNumber();

			//_sendManager->_dbManager.AddSendData(_id, kshot_number, send_No.GetBuffer(), msgtype, callbackNo.GetBuffer(), receiveNo.GetBuffer(), subject, msg,
			//																							filename, uploaded_path, file_size, isReserved, reservedTime, szRegTime);

			SEND_QUEUE_INFO* info = new SEND_QUEUE_INFO();
			
			/*
			memset(info, 0x00, sizeof(info));
			strcpy(info->uid, _id.GetBuffer());
			info->msgtype = msgtype;
			strcpy(info->send_number, send_No.GetBuffer());
			strcpy(info->callback, callbackNo.GetBuffer());
			strcpy(info->receive, receiveNo.GetBuffer());
			strcpy(info->subject, subject);
			strcpy(info->msg, msg);
			strcpy(info->filename, filename);
			strcpy(info->filepath, uploaded_path);
			info->filesize = file_size;
			info->kshot_number = kshot_number;
			info->isReserved = isReserved;
			strcpy(info->reservedTime, reservedTime);
			strcpy(info->registTime, szRegTime);
			strcpy(info->line, line);
			*/

			info->uid = _id;
			info->msgtype = msgtype;
			info->send_number = send_No;
			info->callback = callbackNo;
			info->receive = receiveNo;
			info->subject = subject;
			info->msg = msg;
			info->filename = filename;
			info->filepath = uploaded_path;
			info->filesize = file_size;
			info->kshot_number = kshot_number;
			info->isReserved = isReserved;
			info->reservedTime = reservedTime;
			info->registTime = szRegTime;
			info->line = line;

			vtRequest.push_back(info);

			_manager->AddResult(this, kshot_number, send_No.GetBuffer(), receiveNo.GetBuffer(), msgtype, price, pid);

			//TRACE(_T("send_message_ack:%d\n"), send_number);

			Send_MessageAck(0, send_number);
		}
		else
		{
			// 잘못된(무효한메세지, 잔액부족, 기타..) 요청이면 바로 실패 응답
			Send_MessageAck(errorCode, send_number);

			// 결과 실패 전송
			Send_Report(errorCode, send_number, receiveNo.GetBuffer(), szRegTime, 0);
		}

		token = receiveList.Tokenize(_T(",; \r\n"), pos);

		count++;

		if (count % 1000 == 0) 
		{
			_sendManager->_dbManager.AddSendData(_id, vtRequest);
			vtRequest.clear();

			//SetEvent(g_hDBEvent);
			SetEvent(g_hSendEvent);
			Sleep(10);
		}
	}

	Logmsg(CMdlLog::LEVEL_EVENT, _T("Client::ParseMessage(), [msg] 요청버퍼에 기록 완료(uid:%s, count:%d)"), _id, count);

	if (vtRequest.size() > 0 )
	{
		_sendManager->_dbManager.AddSendData(_id, vtRequest);
		vtRequest.clear();

		//SetEvent(g_hDBEvent);
		SetEvent(g_hSendEvent);
		Sleep(10);
	}


	delete[] target_buf;
}

void Client::Send_MessageAck(int result, int send_number)
{
    memset(&_pkt_message_ack,0x00,sizeof(_pkt_message_ack));

	_pkt_message_ack.PACKET_BEGIN0	= 0xfe;
	_pkt_message_ack.PACKET_BEGIN1	= 0xfe;
	_pkt_message_ack.MSGLEN			= htonl(28);
	_pkt_message_ack.MSGID			= htonl(CPT_PKT_MESSAGE_ACK);
	_pkt_message_ack.REQUESTID		= htonl(0x00);
	_pkt_message_ack.VERSION_MSB	= CkshotDlg::_msb;
	_pkt_message_ack.VERSION_LSB	= CkshotDlg::_lsb;
	_pkt_message_ack.RESERVED		= htons(0x00);

	_pkt_message_ack.RESULT  		= htonl(result);
	_pkt_message_ack.SEND_NUMBER	= htonl(send_number);

	_pkt_message_ack.PACKET_END0	= 0xfd;
	_pkt_message_ack.PACKET_END1	= 0xfd;

	_socket.SendBuffer((byte*)&_pkt_message_ack, sizeof(_pkt_message_ack));	
}

void Client::ParseReportAck(BYTE* buf, int len)
{
	// SOCKET_MSG_BASE_SIZE + TARGET_SIZE
	ASSERT(len > 20 + 4);

	memcpy(&_pkt_report_ack, buf, 20 + 4);

	int list_size = ntohl(_pkt_report_ack.LIST_SIZE);

	char *list_buf = new char[list_size + 1];

	memset(list_buf, 0, list_size + 1);

	memcpy(list_buf, buf + 20 + 4, list_size);

	if (g_bDebugTrace)
	{
		CString strListData = list_buf;

		static int totcnt = 0;
		int cnt = strListData.Remove(';');
		cnt++;
		totcnt += cnt;

		TRACE(_T("\nLIST_COUNT: %d, TOTAL_CNT: %d\n"), cnt, totcnt);
		//TRACE(_T("LIST_SIZE: %d\n"), list_size);
	}

	WriteResultAck(_id, list_buf);

	delete[] list_buf;
}

void Client::Send_Report(int result, int send_number, char* target, char* done_time, int telecom)
{
	// 통신서버로부터 결과를 받고 이곳을 호출해야 한다.

    memset(&_pkt_report,0x00,sizeof(_pkt_report));

	_pkt_report.PACKET_BEGIN0	= 0xfe;
	_pkt_report.PACKET_BEGIN1	= 0xfe;
	_pkt_report.MSGLEN			= htonl(80);
	_pkt_report.MSGID			= htonl(CPT_PKT_REPORT);
	_pkt_report.REQUESTID		= htonl(0x00);
	_pkt_report.VERSION_MSB		= CkshotDlg::_msb;
	_pkt_report.VERSION_LSB		= CkshotDlg::_lsb;
	_pkt_report.RESERVED		= htons(0x00);
	
	_pkt_report.RESULT  		= htonl(result);
	_pkt_report.SEND_NUMBER		= htonl(send_number);
	strncpy(_pkt_report.TARGET_NO, target, strlen(target));
	strncpy(_pkt_report.DONE_TIME, done_time, strlen(done_time));
	_pkt_report.TELECOM_ID		= htonl(telecom);

	_pkt_report.PACKET_END0		= 0xfd;
	_pkt_report.PACKET_END1		= 0xfd;

	_socket.SendBuffer((byte*)&_pkt_report, sizeof(_pkt_report));
}

void Client::OnNewConnection(WPARAM wParam, LPARAM lParam)
{
	//ListenSocket *socket = (ListenSocket *)wParam;

	SOCKET hSocket = (SOCKET)wParam;

	_socket.Attach(hSocket);

	int szSendBuf;
	int oplen = sizeof(szSendBuf);
	if (_socket.GetSockOpt(SO_SNDBUF, &szSendBuf, &oplen) == FALSE) {
		DWORD errcode = GetLastError();
		if (errcode == WSAENOPROTOOPT)
			TRACE(_T("OPT ERROR"));
	}

	TRACE(_T("no ERROR"));

	// 65536
	szSendBuf = 1024 * 1024;
	if( _socket.SetSockOpt(SO_SNDBUF, &szSendBuf, oplen) == FALSE) {
		DWORD errcode = GetLastError();
		if (errcode == WSAENOPROTOOPT)
			TRACE(_T("OPT ERROR"));
	}

}

void Client::OnCloseForNoReply(WPARAM wParam, LPARAM lParam)
{
	Log(CMdlLog::LEVEL_EVENT, _T("Client::OnCloseForNoReply(), 에서 OnClose(-1)호출"));

	// 소켓 종료
	OnClose(-1);
}

void Client::WriteSendRawData(CString formatedMsg)
{

	// 발송 raw 데이타를 파일로 저장한다.
	// id 와 일자별로 파일을 만들고, 발송데이타를 추가한다.
	// 파일명 : uid_일자.dat

	CTime cTime = CTime::GetCurrentTime(); // 현재 시스템으로부터 날짜 및 시간을 얻어 온다.

	CString strDate;
	strDate.Format("%04d-%02d-%02d", cTime.GetYear(), cTime.GetMonth(), cTime.GetDay());

	CString strTime;
	strTime.Format("%02d:%02d:%02d", cTime.GetHour(), cTime.GetMinute(), cTime.GetSecond());

	if (_curDate != strDate)
	{
		// 이전 날짜 파일을 닫는다.
		if (_curDate.IsEmpty() == FALSE)
			_sendLogFile.Close();

		// 새로운 파일 만들고 
		_curDate = strDate;

		CString strFilename;
		strFilename.Format(_T(".\\data\\kshot_%s.log"), strDate);
		
		//setlocale(LC_ALL, "korean");

		CFileException ex;
		if (!_sendLogFile.Open(strFilename, CFile::modeNoTruncate | CFile::modeCreate | CFile::modeWrite | CFile::typeText | CFile::shareDenyNone, &ex))
		{
			TCHAR szError[1024] = { 0, };
			ex.GetErrorMessage(szError, 1024);

			CString errorMsg;
			errorMsg.Format(_T("파일 열기에 실패했습니다.(error=%s)"), szError);
			Logmsg(CMdlLog::LEVEL_ERROR, _T("Client::WriteSendRawData(), %msg, date-time:%s-%s"), errorMsg, _curDate, strTime);

			return;
		}

		_sendLogFile.WriteString(_T("날짜:시간, Id, 문자타입, 회신번호, 제목, 메세지, 파일명, 업로드경로, 예약유무, 예약일자, 수신목록\n\n"));

	}

	// 파일 끝
	_sendLogFile.SeekToEnd();

	CString rawData;
	rawData.Format(_T("%s:%s,\t%s\n"), _curDate, strTime, formatedMsg);

	_sendLogFile.WriteString(rawData);

	_sendLogFile.Flush();
}

void Client::WriteResultAck(CString id, CString ack_list)
{

	// 발송 raw 데이타를 파일로 저장한다.
	// id 와 일자별로 파일을 만들고, 발송데이타를 추가한다.
	// 파일명 : uid_일자.dat

	CTime cTime = CTime::GetCurrentTime(); // 현재 시스템으로부터 날짜 및 시간을 얻어 온다.

	CString strDate;
	strDate.Format("%04d-%02d-%02d", cTime.GetYear(), cTime.GetMonth(), cTime.GetDay());

	CString strTime;
	strTime.Format("%02d:%02d:%02d", cTime.GetHour(), cTime.GetMinute(), cTime.GetSecond());

	if (_curDateAck != strDate)
	{
		// 이전 날짜 파일을 닫는다.
		if (_curDateAck.IsEmpty() == FALSE)
			_reportAckFile.Close();

		// 새로운 파일 만들고 
		_curDateAck = strDate;

		CString strFilename;
		strFilename.Format(_T(".\\result_ack\\kshot_%s.log"), strDate);

		//setlocale(LC_ALL, "korean");

		CFileException ex;
		if (!_reportAckFile.Open(strFilename, CFile::modeNoTruncate | CFile::modeCreate | CFile::modeWrite | CFile::typeText | CFile::shareDenyNone, &ex))
		{
			TCHAR szError[1024] = { 0, };
			ex.GetErrorMessage(szError, 1024);

			CString errorMsg;
			errorMsg.Format(_T("reportAck 파일 열기에 실패했습니다.(error=%s)"), szError);
			Logmsg(CMdlLog::LEVEL_ERROR, _T("Client::WriteResultAck(),  errorMsg:%s, date-time:%s-%s"), errorMsg, _curDateAck, strTime);

			return;
		}

		_reportAckFile.WriteString(_T("날짜:시간, Id, reportAc목록\n\n"));

	}

	// 파일 끝
	_reportAckFile.SeekToEnd();

	CString rawData;
	rawData.Format(_T("%s:%s,\t%s,\t%s\n"), _curDateAck, strTime, id, ack_list);

	_reportAckFile.WriteString(rawData);

	_reportAckFile.Flush();
}

bool Client::CheckInvalidDateFormat(char* dateFormat, COleDateTime *oleTime)
{
	//유효성 검증

	CString strDate = dateFormat;

	oleTime->SetDateTime(atoi(strDate.Left(4)), atoi(strDate.Mid(5, 2)), atoi(strDate.Mid(8, 2)),
		atoi(strDate.Mid(11, 2)), atoi(strDate.Mid(14, 2)), atoi(strDate.Mid(17, 2)));

	// 잘못된 포맷이면 실패처리 
	if (oleTime->GetStatus() != COleDateTime::valid)
		return false;

	COleDateTime nowTime(COleDateTime::GetCurrentTime());

	COleDateTimeSpan span = *oleTime - nowTime;

	double timeStmp = span.GetTotalSeconds();

	// 지난 일자이면 실패처리
	if (timeStmp < 0L)
	{
		return false;
	}

	return true;
}