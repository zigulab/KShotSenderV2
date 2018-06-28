// ClientManager.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "ClientManager.h"

#include "Client.h"
#include "sendManager.h"

#include "ClientAcceptor.h"

#include "kshotDlg.h"

//#define CLIENTMANAGER_ELASPE_CHECK

// ClientManager

ClientManager::ClientManager()
	: _pool(this)
{
	//_port = 3010;

	_bDoneThreadProc[0] = FALSE;
	_bDoneThreadProc[1] = FALSE;

	_SEND_RESULT_FILE = _T("send_result.dat");
}

ClientManager::~ClientManager()
{
}


// ClientManager 멤버 함수

bool ClientManager::Init()
{
	Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::Init(), ClietManager 초기화"));

	_bDoneThreadProc[0] = FALSE;
	_bDoneThreadProc[1] = FALSE;

	_port = _ttoi(g_config.kshot.port);

	if( _ttoi(g_config.config.close_Idleclient) )
		_close_idleclient = TRUE;
	else
		_close_idleclient = FALSE;

	_expire_time = _ttoi(g_config.config.expire_time);

	// 이전 저장된 결과 load
	_prevSendResultInfo.LoadInfo(_SEND_RESULT_FILE);

	if( !_pool.Init() )
		return false;

	//if( !_socket.Create(_port) ) {
	//	Log(CMdlLog::LEVEL_ERROR, _T("KSHOTSender Lintening Socket 생성 실패"));
	//	return false;
	//}

	//Log(CMdlLog::LEVEL_EVENT, _T("ClietManager Linten Socket, Listening..."));

	//_socket.Listen();

	_acceptor = (ClientAcceptor*)AfxBeginThread(RUNTIME_CLASS(ClientAcceptor), 0, 0, CREATE_SUSPENDED);
	_acceptor->Init(this, _port);
	_acceptor->ResumeThread();


	_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//::_beginthread(ThreadLogSingle, 0, this);

	AfxBeginThread(ResultProc, this);
	AfxBeginThread(LinkCheckProc, this);

	Log(CMdlLog::LEVEL_EVENT, _T("ClietManager::ResultProc() Running..."));
	Log(CMdlLog::LEVEL_EVENT, _T("ClietManager::LinkCheckProc() Running..."));

	return true;
}

bool ClientManager::UnInit()
{
	Log(CMdlLog::LEVEL_EVENT, _T("ClientManager, 종료시작"));

	// 먼저, 소켓을 닫고
	//_socket.ShutDown();
	//_socket.Close();
	_acceptor->Uninit();

	// 처리중인 내용을 중단한다.
	if (_hExitEvent)
	{
		::SetEvent(_hExitEvent);
		while( !_bDoneThreadProc[0] || !_bDoneThreadProc[1] )
			::Sleep(10);

		::CloseHandle(_hExitEvent);
	}	

	// 클라이언트 객체를 삭제한다.
	/*
	vector<SEND_RESULT_INFO*>::iterator it = _vtSendResult.begin();
	for(; it != _vtSendResult.end(); it++ )
	{
		SEND_RESULT_INFO *result = *it;
		delete result;
	}
	_vtSendResult.clear();
	*/

	// 결과 목록을 파일로 저장한다.

	//CString uid;
	//CString pid;
	//CString send_number;
	//unsigned int kshot_number;
	//int msgType;
	//CString receiveNo;
	//CString done_time;
	//int result;
	//int telecom;
	//float price;
	//CTime reg_time;
	//DWORD result_time;
	//BOOL doneflag;
	//BOOL completeflag;

	CSendResultInfo srInfo;
	map<CString, SEND_RESULT_INFO*>::iterator pos = _mapSendResult.begin();
	for (; pos != _mapSendResult.end(); pos++)
	{
		SEND_RESULT_INFO *info = pos->second;

		//uid = 
		//pid = 
		//send_number = 
		//kshot_number = 
		//msgType = 
		//receiveNo = 
		//done_time = 
		//result = 
		//telecom = 
		//price = 
		//reg_time = 
		//result_time = 
		//doneflag = 
		//completeflag = 

		//uid = info->client->_id;
		//pid = info->pid;
		//send_number = info->send_number;
		//kshot_number = info->kshot_number;
		//msgType = info->msgType;
		//receiveNo = info->receiveNo;
		//done_time = info->done_time;
		//result = info->result;
		//telecom = info->telecom;
		//price = info->price;
		//reg_time = info->reg_time;
		//result_time = info->result_time;
		//doneflag = info->doneflag;
		//completeflag = info->completeflag;

		srInfo.AddResult(info->client->_id, info->pid, info->send_number, info->kshot_number, info->msgType, info->receiveNo,
								info->done_time, info->result, info->telecom, info->price, info->reg_time, info->result_time, info->doneflag, info->completeflag);
	}
	srInfo.SaveInfo(_SEND_RESULT_FILE);


	// 결과 목록을 삭제 한다.
	map<CString, SEND_RESULT_INFO*>::iterator it2 = _mapSendResult.begin();
	for (; it2 != _mapSendResult.end(); it2++)
	{
		SEND_RESULT_INFO *info = it2->second;
		delete info;
	}
	_mapSendResult.clear();


	// 쓰레드 풀을 정리한다.
	_pool.UnInit();


	Log(CMdlLog::LEVEL_EVENT, _T("ClientManager, 자료구조 삭제"));

	return true;
}

void ClientManager::OnAccept()
{
	Client* client = _pool.GetClient();

	ASSERT(client);

	Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::OnAccept(), new 클라이언트 연결"));

	CAsyncSocket socket;
	_acceptor->Accept(socket);

	SOCKET hsocket = socket.Detach();
	
	client->PostThreadMessage(WM_NEW_CONNECTION, (WPARAM)hsocket, 0);

	client->_socket.OnConnectByAccept();
}

int ClientManager::GetClientCount()
{
	return _mapClients.size();
}

CString ClientManager::GetClientId(int index)
{
	map<CString, Client*>::iterator it = _mapClients.begin();
	for (int idx = 0; it != _mapClients.end(); it++) {
		if (idx == index) {
			return it->first;
		}
		idx++;
	}

	return "";
}

void ClientManager::AddClient(CString uid, Client* client)
{
	_cs.Lock();

	_mapClients[uid] = client;

	// CLIENT LIST UI 갱신 호출
	CkshotDlg* dlg = (CkshotDlg*)AfxGetMainWnd();
	dlg->AddClient(uid);
	//

	RetryResultNotify(client, uid);

	_cs.Unlock();
}

void ClientManager::DelClient(CString uid, Client* client)
{
	_cs.Lock();

	map<CString, Client*>::iterator it = _mapClients.find(uid);
	if (it != _mapClients.end()) {
		//Client* client = it->second;
		CString uid = it->first;
		_mapClients.erase(it);
	}

	// CLIENT LIST UI 갱신 호출
	CkshotDlg* dlg = (CkshotDlg*)AfxGetMainWnd();
	dlg->DelClient(uid);
	//

	_cs.Unlock();
}

Client* ClientManager::GetClient(CString uid)
{
	_cs.Lock();

	Client* pClient = NULL;

	map<CString, Client*>::iterator it = _mapClients.find(uid);
	if (it != _mapClients.end()) {
		pClient = it->second;
	}

	_cs.Unlock();

	return pClient;
}

void ClientManager::UpdateClientList(CString uid, CString lastRequestDate, int lastRequestCount)
{
	_cs.Lock();

	CkshotDlg* dlg = (CkshotDlg*)AfxGetMainWnd();
	dlg->UpdateClientList(uid, lastRequestDate, lastRequestCount);

	_cs.Unlock();
}

void ClientManager::RetryResultNotify(Client* client, CString uid)
{
	//_cs.Lock();

	// 새로운 클라이언트가 연결되었을 때 기존 결과를 처리한다.

	/*//////////////////////////////////////////////////////////////////////////
	// 메모리에 저장된 결과를 복원한다.(Agent가 로그아웃되고 로그인일때 적용)
	////////////////////////////////////////////////////////////////////////////*/
	map<CString, SEND_RESULT_INFO*>::iterator pos = _mapSendResult.begin();
	for (; pos != _mapSendResult.end(); pos++)
	{
		SEND_RESULT_INFO* info = pos->second;

		if (info->client->_id == uid) {
			info->client = client;
		}
	}

	/*//////////////////////////////////////////////////////////////////////////
	// 파일에서 읽어들인 이전 결과를 복원한다.( kshot서버 재시작 일때 적용됨 )
	////////////////////////////////////////////////////////////////////////////*/
	vector<CSendResultInfo::SEND_RESULT_INFO>::iterator it = _prevSendResultInfo.vtSendResultInfo.begin();
	for (; it != _prevSendResultInfo.vtSendResultInfo.end(); ) {

		CSendResultInfo::SEND_RESULT_INFO prev_info = *it;

		if (prev_info.uid == uid) {

			SEND_RESULT_INFO *info = new SEND_RESULT_INFO();

			info->client = client;
			info->send_number = prev_info.send_number;
			info->receiveNo = prev_info.receiveNo;
			info->msgType = prev_info.msgType;
			info->price = prev_info.price;
			info->doneflag = prev_info.doneflag;
			info->completeflag = prev_info.completeflag;
			info->result = prev_info.result;
			info->telecom = prev_info.telecom;
			info->kshot_number = prev_info.kshot_number;
			info->pid = prev_info.pid;
			info->reg_time = prev_info.reg_time;

			info->done_time = prev_info.done_time;
			info->result_time = prev_info.result_time;

			CString uidnumber;
			uidnumber.Format(_T("%s-%s"), client->_id, prev_info.send_number);

			_cs.Lock();

			_mapSendResult[uidnumber] = info;

			_mapKShotKey_UidSendNumber[info->kshot_number] = uidnumber;

			_cs.Unlock();

			it = _prevSendResultInfo.vtSendResultInfo.erase(it);
		}
		else
			it++;
	}


	/*
	////////////////////////// 결과 재 전송 ////////////////////////////////
	// done 결과 리스트
	CString notifyList;

	vector<SEND_RESULT_INFO*> vtList;
	g_sendManager->_dbManager.LoadNonResultNotify(uid, vtList);

	if (vtList.size() > 0) 
	{
		CString send_queue_id;

		vector<SEND_RESULT_INFO*>::iterator it = vtList.begin();
		for (; it != vtList.end(); it++)
		{
			SEND_RESULT_INFO* info = *it;

			client->Send_Report(info->result, _ttoi(info->send_number), info->receiveNo.GetBuffer(), info->done_time.GetBuffer(), info->telecom);

			send_queue_id.Format(_T("%d,"), info->kshot_number);
			notifyList += send_queue_id;
		}

		// 최종 완료된것은 msgresult update
		if (notifyList.IsEmpty() == FALSE) {
			notifyList.Delete(notifyList.GetLength() - 1);
			g_sendManager->_dbManager.UpdateResultNotify(notifyList);
		}

		// clear
		it = vtList.begin();
		for (; it != vtList.end(); it++)
		{
			SEND_RESULT_INFO* info = *it;
			delete info;
		}
		vtList.clear();
	}
	*/

	//_cs.Unlock();
}

unsigned int ClientManager::GetKShotSendNumber()
{
	int number; 

	//_cs2.Lock();

	//g_kshot_send_number++;

	//number = g_kshot_send_number;

	//_cs2.Unlock();

	if (theApp.m_Mutex.Lock(1000))
	{
		int curVal = _ttoi(theApp.m_pSharedMemory);
		if (curVal != 0)
			g_kshot_send_number = curVal;

		g_kshot_send_number++;

		number = g_kshot_send_number;
		wsprintf(theApp.m_pSharedMemory, _T("%d"), g_kshot_send_number);

		theApp.m_Mutex.Unlock();
	}
	else
	{
		g_kshot_send_number++;
		number = g_kshot_send_number;
		Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::GetKShotSendNumber(), theApp.m_Mutex.Lock() Failed"));
	}


	return number;
}

void ClientManager::AddResult(Client *client, unsigned int kshot_number, CString send_number, CString receiveNo, int msgType, float priceValue, CString pid)
{
	_cs.Lock();

	SEND_RESULT_INFO *info = new SEND_RESULT_INFO();
	info->client = client;
	info->send_number = send_number;
	info->receiveNo = receiveNo;
	info->msgType = msgType;
	info->price = priceValue;
	info->doneflag = FALSE;
	info->completeflag = FALSE;
	info->result = -1;
	info->telecom = -1;
	info->kshot_number = kshot_number;
	info->pid = pid;
	info->reg_time = CTime::GetCurrentTime();

	info->done_time = "";
	info->result_time = 0;

	CString uidnumber;
	uidnumber.Format(_T("%s-%s"), client->_id, send_number);

	_mapSendResult[uidnumber] = info;

	_mapKShotKey_UidSendNumber[kshot_number] = uidnumber;

	_cs.Unlock();
}

void ClientManager::GetId_Price_MsgType(CString uid_sendno, unsigned int kshot_number, CString& uid, CString& pid, float* price, int* msgType)
{
	_cs.Lock();

	map<CString, SEND_RESULT_INFO*>::iterator it = _mapSendResult.find(uid_sendno);
	if (it != _mapSendResult.end()) 
	{
		SEND_RESULT_INFO *info = it->second;
		if (info->kshot_number == kshot_number)
		{
			CString uidnumber = it->first;

			int idx = uidnumber.Find('-', 0);

			uid = uidnumber.Left(idx);
			
			pid = info->pid;

			*price = info->price;
			*msgType = info->msgType;
		}
	}

	_cs.Unlock();
}

void ClientManager::SetResultKShotNumber(CString uid, CString send_number, unsigned int kshot_number)
{
	CString uidnumber;
	uidnumber.Format(_T("%s-%s"), uid, send_number);

	_cs.Lock();

	map<CString, SEND_RESULT_INFO*>::iterator it = _mapSendResult.find(uidnumber);
	if (it != _mapSendResult.end()) {
		SEND_RESULT_INFO *info = it->second;
		info->kshot_number = kshot_number;

		//_mapKShotKey_UidSendNumber[kshot_number] = uidnumber;
	}

	_cs.Unlock();
}

CString ClientManager::GetUidSendNumberByKShotKey(unsigned int kshot_number)
{
	_cs.Lock();

	CString uid_sendno;
	
	map<unsigned int, CString>::iterator pos = _mapKShotKey_UidSendNumber.find(kshot_number);
	if (pos != _mapKShotKey_UidSendNumber.end()) {
		uid_sendno = pos->second;
	}

	_cs.Unlock();

	return uid_sendno;
}

void ClientManager::SetResult(CString uid_sendno, unsigned int kshot_number, int result, CString done_time, int telecom)
{
	_cs.Lock();

	map<CString, SEND_RESULT_INFO*>::iterator it = _mapSendResult.find(uid_sendno);
	if (it != _mapSendResult.end()) 
	{
		SEND_RESULT_INFO *info = it->second;
		if (info->kshot_number == kshot_number)
		{
			info->result = result;
			info->done_time = done_time;
			info->telecom = telecom;
			info->doneflag = TRUE;
			info->result_time = GetTickCount();
		}
	}

	/*
	int n = 0;
	map<CString, SEND_RESULT_INFO*>::iterator it = _mapSendResult.begin();
	for (; it != _mapSendResult.end(); it++)
	{
		SEND_RESULT_INFO *info = it->second;
		if (info->kshot_number == kshot_number)
		{
			info->result = result;
			info->done_time = done_time;
			info->telecom = telecom;
			info->doneflag = TRUE;
			info->result_time = GetTickCount();
			break;
		}

		n++;
		if (n % 1000 == 0)	Sleep(0);
	}
	*/

	_cs.Unlock();
}

void ClientManager::DelResult(SEND_RESULT_INFO *info)
{
	/*
	vector<SEND_RESULT_INFO*>::iterator it = _vtSendResult.begin();
	for(; it != _vtSendResult.end(); )
	{
		SEND_RESULT_INFO *result = *it;
		if( result == info ) {
			delete result;
			_vtSendResult.erase(it);
			break;
		}
	}
	*/
}

void ClientManager::FlushResult()
{
	_cs.Lock();

	// done 결과 리스트
	CString notifyList;

	DWORD now = GetTickCount();

	int loopcnt = 0;

	map<CString, SEND_RESULT_INFO*>::iterator it = _mapSendResult.begin();
	for (; it != _mapSendResult.end(); )
	{
		SEND_RESULT_INFO *info = it->second;
		
		CString send_queue_id;

		BOOL del_flag = FALSE;

		// Agent로 결과송신이 완료된 것은 삭제 ( doneflag => completeflag )
		if ( info->completeflag)
		{
			del_flag = TRUE;
			
			send_queue_id.Format(_T("%d,"), info->kshot_number);
			notifyList += send_queue_id;
		} 
		// 연결이 끊어진 것도 삭제( 재접속시 DB를 읽어 재전송 )
		/*
		else if (info->client->_bUse == false || info->client->_bBind == false) {
			del_flag = TRUE;
		}
		*/

		// 결과를 받은것중에 
		//if( info->doneflag ) {
		//	if (now - info->result_time > RESULT_DELAY_LIMITTIME)   // 결과송신이 5분이상 지연된 것은 삭제	
		//		del_flag = TRUE;
		//}


		if (del_flag)
		{
			delete info;
			info = NULL;
			it = _mapSendResult.erase(it);
		}
		else
			it++;

		loopcnt++;

		if (loopcnt % 1000 == 0)	Sleep(10);
	}

	// 최종 완료된것은 msgresult.notify_result update
	if (notifyList.IsEmpty() == FALSE) {
		notifyList.Delete(notifyList.GetLength() - 1);
		g_sendManager->_dbManager.UpdateResultNotify(notifyList);
	}

	_cs.Unlock();
}

void ClientManager::FlushResult(map<CString, SEND_RESULT_INFO*>& doneResult)
{
	_cs.Lock();

	// done 결과 리스트
	CString notifyList;

	int n = 0;
	CString uidnumber;
	map<CString, SEND_RESULT_INFO*>::iterator it = doneResult.begin();
	for (; it != doneResult.end(); it++)
	{
		uidnumber = it->first;
		SEND_RESULT_INFO *info = it->second;

		map<CString, SEND_RESULT_INFO*>::iterator pos = _mapSendResult.find(uidnumber);
		if (pos != _mapSendResult.end()) {
			_mapSendResult.erase(pos);
		}

		map<unsigned int, CString>::iterator pos2 = _mapKShotKey_UidSendNumber.find(info->kshot_number);
		if (pos2 != _mapKShotKey_UidSendNumber.end()) {
			_mapKShotKey_UidSendNumber.erase(pos2);
		}

		CString send_queue_id;
		send_queue_id.Format(_T("%d,"), info->kshot_number);

		notifyList += send_queue_id;

		delete info;
		info = NULL;

		n++;
		if (n % 1000 == 0)	Sleep(0);
	}
	
	_cs.Unlock();

	// 최종 완료된것은 msgresult.notify_result update
	if (notifyList.IsEmpty() == FALSE) {
		notifyList.Delete(notifyList.GetLength() - 1);
		g_sendManager->_dbManager.UpdateResultNotify(notifyList);
	}

	doneResult.clear();
}


int ClientManager::GetResult(map<CString, SEND_RESULT_INFO*>& result)
{
	_cs.Lock();

	//ULONGLONG b = GetTickCount64();

	result = _mapSendResult;

	//ULONGLONG e = GetTickCount64();

	//TRACE(_T("GetResult() Elapse Time:%d\n"), e - b);

	_cs.Unlock();

	return result.size();
}

int ClientManager::GetDoneResult(map<CString, SEND_RESULT_INFO*>& result)
{
#if( defined(DEBUG_ELAPSE_CHECK) && defined( CLIENTMANAGER_ELASPE_CHECK ))
	LARGE_INTEGER Frequency;
	LARGE_INTEGER BeginTime;
	LARGE_INTEGER Endtime;

	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&BeginTime);
#endif

	_cs.Lock();

	//ULONGLONG b = GetTickCount64();
	
	int n = 0;
	map<CString, SEND_RESULT_INFO*>::iterator it = _mapSendResult.begin();
	for (; it != _mapSendResult.end(); it++)
	{
		SEND_RESULT_INFO *info = it->second;
		if( info->doneflag )
			result[it->first] = it->second;

		n++;
		if (n % 1000 == 0)	Sleep(0);
	}
	
	//ULONGLONG e = GetTickCount64();
	//TRACE(_T("GetDoneResult() Elapse Time:%d\n"), e - b);

	_cs.Unlock();

#if( defined(DEBUG_ELAPSE_CHECK) && defined( CLIENTMANAGER_ELASPE_CHECK ))
	ELAPSE_CHECK_END("GetDoneResult", 1)
#endif

	return result.size();
}


HANDLE ClientManager::GetExitEvent()
{
	return _hExitEvent;
}

void ClientManager::SetDoneThread(int index)
{
	_bDoneThreadProc[index] = TRUE;
}

UINT ClientManager::ResultProc(LPVOID lpParameter)
{
	ClientManager *manager = (ClientManager*)lpParameter;

	// 이전 날
	int prev_day = CTime::GetCurrentTime().GetDay();

	// 하루에 한번 처리
	BOOL doneTaskOneForDay = FALSE;

	while( ::WaitForSingleObject(manager->GetExitEvent(), 1500) != WAIT_OBJECT_0 )
	{
		////////////// 미수신 만료 처리
		// 현재 시스템으로부터 날짜 및 시간을 얻어 온다.
		CTime cTime = CTime::GetCurrentTime();

		int now_day = cTime.GetDay();
		int now_hour = CTime::GetCurrentTime().GetHour();

		// 하루가 바뀌면 
		if (now_day != prev_day)
			doneTaskOneForDay = FALSE;

		if (doneTaskOneForDay == FALSE)
		{
			// 하루 한번 미수신 처리
			if (now_hour == manager->_expire_time)
			{
				CTime now_time = CTime::GetCurrentTime();

				// 하루에 한번 완료
				doneTaskOneForDay = TRUE;

				// 하루에 한번 처리
				prev_day = now_day;

				// 일괄 실패 및 환불처리
				//g_sendManager->_dbManager.FailForExpireBatch();

				// 건건이 처리
				// 미수신 만료에 따라 실패로 처리

				CString expiredKnumbers;
				int expiredCount = 0;

				map<CString, SEND_RESULT_INFO*> resultList;
				if (manager->GetResult(resultList) > 0)
				{
					map<CString, SEND_RESULT_INFO*>::iterator it = resultList.begin();
					for (; it != resultList.end(); it++)
					{
						SEND_RESULT_INFO *info = it->second;

						CTimeSpan elapsedTime = now_time - info->reg_time;
						
						int elapse_sec = elapsedTime.GetTotalSeconds();

						// 2일 (172800초) 경과된 것은 실패로 처리
						if (elapse_sec > 172800)
						{
							expiredCount++;

							if( info->msgType == 0 )
								info->result = 999;
							else
								info->result = 9999;

							info->doneflag = TRUE;
							info->telecom = 0;

							CString strKNumber;
							strKNumber.Format(_T("%d,"), info->kshot_number);
							expiredKnumbers += strKNumber;

							// 실패 및 환불 처리
							g_sendManager->_dbManager.FailForExpire(info->kshot_number, info->client->_id, info->pid, info->msgType, info->price);
						}
					}
				}

				if ( expiredCount > 0 )
				{
					PostThreadMessage(g_mainTid, WM_STATISTIC_RECAL, 0, 0);

					// Logmsg 에서 너무 큰 데이타는 처리하지 못하는 문제가 발생함. 그래서 /data 폴더의 rawData에 기록함.
					//Logmsg(CMdlLog::LEVEL_EVENT, _T("[expired] 미수신 만료로 실패/환불(count:%d)처리, 처리된 kshot_number목록(%s)"), expiredCount, expiredKnumbers);

					CString formatMsg;
					formatMsg.Format(_T("[expired] 미수신 만료로 실패/환불(count:%d)처리, 처리된 kshot_number목록(%s)"), expiredCount, expiredKnumbers);
					Client::WriteSendRawData(formatMsg);

					Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::ResultProc(), [expired] 미수신 만료로 실패/환불처리(count:%d)"), expiredCount);
				}

			}
		}


		////////////// 결과 처리
		int loopcnt = 0;
		int notify_cnt = 0;

		map<CString, SEND_RESULT_INFO*> result;
		
		//Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::ResultProc(), [msg] manager->GetDoneResult()"));

		if( manager->GetDoneResult(result) > 0 )
		{
			map<CString, SEND_RESULT_INFO*>::iterator it = result.begin();
			for (; it != result.end(); it++)
			{
				SEND_RESULT_INFO *info = it->second;

				ASSERT(info);
				Client *client = info->client;

				if (client == NULL) continue;

				// 사용중에 연결이 끊긴 것은 배제한다.
				if (client->_bUse == false || client->_bBind == false) {
					loopcnt++;
					continue;
				}

				// 결과 전송하기
				//if( info->doneflag && ! info->completeflag )
				//if (info->doneflag)
				//{
					int retval = info->result;
					int telecom = info->telecom;
					char *time = info->done_time.GetBuffer(0);
					char *sendnumber = info->send_number.GetBuffer(0);
					char *receiveNo = info->receiveNo.GetBuffer(0);

					// 클라이언트에게 결과 ACK
					client->Send_Report(retval, _ttoi(sendnumber), receiveNo, time, telecom);

					info->completeflag = TRUE;

					info->done_time.ReleaseBuffer();
					info->send_number.ReleaseBuffer();
					info->receiveNo.ReleaseBuffer();

					notify_cnt++;
				//}

				loopcnt++;

				if (loopcnt % 500 == 0) {
					Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::ResultProc(), [msg] 새로운 수신 결과 Agent로 송신(count:%d)"), notify_cnt);
					Sleep(5);
				}
			}			

			if (notify_cnt > 0)
			{
				Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::ResultProc(), [msg] 새로운 수신 결과 Agent로 송신(count:%d)"), notify_cnt);

				//Log(CMdlLog::LEVEL_DEBUG, _T("수신 결과 비우기"));
				manager->FlushResult(result);
			}

			// 처리건수 갱신 호출
			CkshotDlg* dlg = (CkshotDlg*)AfxGetMainWnd();
			dlg->SetDoingSendCount(manager->_mapSendResult.size());
			//

		}

	}

	manager->SetDoneThread(0);

	Log(CMdlLog::LEVEL_DEBUG, _T("ClientManager::ResultProc(), 종료됨."));

	return 1;
}


UINT ClientManager::LinkCheckProc(LPVOID lpParameter)
{
	ClientManager *manager = (ClientManager*)lpParameter;

	while( ::WaitForSingleObject(manager->GetExitEvent(), LINKCHK_INTERVAL_MSEC) != WAIT_OBJECT_0 )
	{

		list<vector<Client*>*>	vectorList = manager->_pool._vectorList;

		list<vector<Client*>*>::iterator pos = vectorList.begin();
	
		while( pos != vectorList.end() )
		{	
			vector<Client*>* vtClient = *pos;

			vector<Client*>::iterator it = vtClient->begin();
			for( ; it != vtClient->end(); it++ )
			{
				Client* client  = *it;

				BOOL validClient = TRUE;

				if( client->_bUse && client->_bBind )
				{
					TRACE(_T("LinkCheckProc(), Client LinkCheck\n"));

					// 현재시간
					CTime curTime = CTime::GetCurrentTime();

					// 마지막 링크체크 이후 경과시간 = 현재시간 - 마지막링크체크시간
					CTimeSpan LinkDiff = curTime  - client->_lastLinkCheckTime;
					
					// 마지막 링크체크 이후 경과시간(초)
					LONGLONG elapsedLink = LinkDiff.GetTotalSeconds();

					// 마지막 발송 이후 경과시간 = 현재시간 - 마지막발송시간
					CTimeSpan SandDiff = curTime  - client->_lastSendTime;

					// 마지막 발송 이후 경과시간(초)
					LONGLONG elapsedSend = SandDiff.GetTotalSeconds();

					TRACE(_T("client->_id:%s, Interval Time Sec: %d\n"), client->_id, elapsedLink);

					// 링크체크 검사
					// 마지막 링크체크 이후 경과시간(초) > LINKCHK_LIMIT_SEC

					//if(elapsedLink >= 10)   // TEST CODE
					if(elapsedLink >= LINKCHK_LIMIT_SEC)
					{
						//연결종료

						//client->_socket.Close();

						//client->PostThreadMessage(WM_DISCONNECT_NOTREPLY, 0, 0);

						//CWnd* cwnd = AfxGetMainWnd();
						//HWND hwnd = cwnd->GetSafeHwnd();
						//DWORD main_id = ::AfxGetThread()->m_nThreadID;
						//cwnd->PostMessage(WM_DISCONNECT_NOTREPLY, client->_socket, 0);

						//::PostThreadMessage((main_id, WM_DISCONNECT_NOTREPLY, client->_socket, 0);

						//연결종료
						//::AfxGetApp()->PostThreadMessage(WM_DISCONNECT_NOTREPLY, (WPARAM)client, 0);

						Log(CMdlLog::LEVEL_ERROR, _T("링크체크 응답없이 제한시간(180초[3분])경과, WM_CLOSE_FOR_NOREPLY 처리"));

						//client->PostThreadMessage(WM_CLOSE_FOR_NOREPLY, 0, 0);
						//validClient = FALSE;
					}
					else if( elapsedSend >= LINKCHK_LIMIT_IDLE_SEC )
					{
						if (manager->_close_idleclient) 
						{
							Log(CMdlLog::LEVEL_ERROR, _T("문자 전송요청 없이 제한시간(1800초[30분]) 경과, WM_CLOSE_FOR_NOREPLY 처리"));

							client->PostThreadMessage(WM_CLOSE_FOR_NOREPLY, 0, 0);

							validClient = FALSE;
						}
					}

					if( validClient ) 
					{
						TRACE(_T("LinkCheckProc(), Send LinkCheck(client_id:%s)"), client->_id);

						// 링크 체크 전송
						client->Send_LinkChk();
					}

				}
			}

			pos++;
		}
	}

	manager->SetDoneThread(1);

	Log(CMdlLog::LEVEL_DEBUG, _T("LinkCheckProc(), 종료됨."));

	return 1;
}

