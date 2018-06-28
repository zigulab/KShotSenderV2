// ClientManager.cpp : ���� �����Դϴ�.
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


// ClientManager ��� �Լ�

bool ClientManager::Init()
{
	Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::Init(), ClietManager �ʱ�ȭ"));

	_bDoneThreadProc[0] = FALSE;
	_bDoneThreadProc[1] = FALSE;

	_port = _ttoi(g_config.kshot.port);

	if( _ttoi(g_config.config.close_Idleclient) )
		_close_idleclient = TRUE;
	else
		_close_idleclient = FALSE;

	_expire_time = _ttoi(g_config.config.expire_time);

	// ���� ����� ��� load
	_prevSendResultInfo.LoadInfo(_SEND_RESULT_FILE);

	if( !_pool.Init() )
		return false;

	//if( !_socket.Create(_port) ) {
	//	Log(CMdlLog::LEVEL_ERROR, _T("KSHOTSender Lintening Socket ���� ����"));
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
	Log(CMdlLog::LEVEL_EVENT, _T("ClientManager, �������"));

	// ����, ������ �ݰ�
	//_socket.ShutDown();
	//_socket.Close();
	_acceptor->Uninit();

	// ó������ ������ �ߴ��Ѵ�.
	if (_hExitEvent)
	{
		::SetEvent(_hExitEvent);
		while( !_bDoneThreadProc[0] || !_bDoneThreadProc[1] )
			::Sleep(10);

		::CloseHandle(_hExitEvent);
	}	

	// Ŭ���̾�Ʈ ��ü�� �����Ѵ�.
	/*
	vector<SEND_RESULT_INFO*>::iterator it = _vtSendResult.begin();
	for(; it != _vtSendResult.end(); it++ )
	{
		SEND_RESULT_INFO *result = *it;
		delete result;
	}
	_vtSendResult.clear();
	*/

	// ��� ����� ���Ϸ� �����Ѵ�.

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


	// ��� ����� ���� �Ѵ�.
	map<CString, SEND_RESULT_INFO*>::iterator it2 = _mapSendResult.begin();
	for (; it2 != _mapSendResult.end(); it2++)
	{
		SEND_RESULT_INFO *info = it2->second;
		delete info;
	}
	_mapSendResult.clear();


	// ������ Ǯ�� �����Ѵ�.
	_pool.UnInit();


	Log(CMdlLog::LEVEL_EVENT, _T("ClientManager, �ڷᱸ�� ����"));

	return true;
}

void ClientManager::OnAccept()
{
	Client* client = _pool.GetClient();

	ASSERT(client);

	Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::OnAccept(), new Ŭ���̾�Ʈ ����"));

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

	// CLIENT LIST UI ���� ȣ��
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

	// CLIENT LIST UI ���� ȣ��
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

	// ���ο� Ŭ���̾�Ʈ�� ����Ǿ��� �� ���� ����� ó���Ѵ�.

	/*//////////////////////////////////////////////////////////////////////////
	// �޸𸮿� ����� ����� �����Ѵ�.(Agent�� �α׾ƿ��ǰ� �α����϶� ����)
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
	// ���Ͽ��� �о���� ���� ����� �����Ѵ�.( kshot���� ����� �϶� ����� )
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
	////////////////////////// ��� �� ���� ////////////////////////////////
	// done ��� ����Ʈ
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

		// ���� �Ϸ�Ȱ��� msgresult update
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

	// done ��� ����Ʈ
	CString notifyList;

	DWORD now = GetTickCount();

	int loopcnt = 0;

	map<CString, SEND_RESULT_INFO*>::iterator it = _mapSendResult.begin();
	for (; it != _mapSendResult.end(); )
	{
		SEND_RESULT_INFO *info = it->second;
		
		CString send_queue_id;

		BOOL del_flag = FALSE;

		// Agent�� ����۽��� �Ϸ�� ���� ���� ( doneflag => completeflag )
		if ( info->completeflag)
		{
			del_flag = TRUE;
			
			send_queue_id.Format(_T("%d,"), info->kshot_number);
			notifyList += send_queue_id;
		} 
		// ������ ������ �͵� ����( �����ӽ� DB�� �о� ������ )
		/*
		else if (info->client->_bUse == false || info->client->_bBind == false) {
			del_flag = TRUE;
		}
		*/

		// ����� �������߿� 
		//if( info->doneflag ) {
		//	if (now - info->result_time > RESULT_DELAY_LIMITTIME)   // ����۽��� 5���̻� ������ ���� ����	
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

	// ���� �Ϸ�Ȱ��� msgresult.notify_result update
	if (notifyList.IsEmpty() == FALSE) {
		notifyList.Delete(notifyList.GetLength() - 1);
		g_sendManager->_dbManager.UpdateResultNotify(notifyList);
	}

	_cs.Unlock();
}

void ClientManager::FlushResult(map<CString, SEND_RESULT_INFO*>& doneResult)
{
	_cs.Lock();

	// done ��� ����Ʈ
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

	// ���� �Ϸ�Ȱ��� msgresult.notify_result update
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

	// ���� ��
	int prev_day = CTime::GetCurrentTime().GetDay();

	// �Ϸ翡 �ѹ� ó��
	BOOL doneTaskOneForDay = FALSE;

	while( ::WaitForSingleObject(manager->GetExitEvent(), 1500) != WAIT_OBJECT_0 )
	{
		////////////// �̼��� ���� ó��
		// ���� �ý������κ��� ��¥ �� �ð��� ��� �´�.
		CTime cTime = CTime::GetCurrentTime();

		int now_day = cTime.GetDay();
		int now_hour = CTime::GetCurrentTime().GetHour();

		// �Ϸ簡 �ٲ�� 
		if (now_day != prev_day)
			doneTaskOneForDay = FALSE;

		if (doneTaskOneForDay == FALSE)
		{
			// �Ϸ� �ѹ� �̼��� ó��
			if (now_hour == manager->_expire_time)
			{
				CTime now_time = CTime::GetCurrentTime();

				// �Ϸ翡 �ѹ� �Ϸ�
				doneTaskOneForDay = TRUE;

				// �Ϸ翡 �ѹ� ó��
				prev_day = now_day;

				// �ϰ� ���� �� ȯ��ó��
				//g_sendManager->_dbManager.FailForExpireBatch();

				// �ǰ��� ó��
				// �̼��� ���ῡ ���� ���з� ó��

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

						// 2�� (172800��) ����� ���� ���з� ó��
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

							// ���� �� ȯ�� ó��
							g_sendManager->_dbManager.FailForExpire(info->kshot_number, info->client->_id, info->pid, info->msgType, info->price);
						}
					}
				}

				if ( expiredCount > 0 )
				{
					PostThreadMessage(g_mainTid, WM_STATISTIC_RECAL, 0, 0);

					// Logmsg ���� �ʹ� ū ����Ÿ�� ó������ ���ϴ� ������ �߻���. �׷��� /data ������ rawData�� �����.
					//Logmsg(CMdlLog::LEVEL_EVENT, _T("[expired] �̼��� ����� ����/ȯ��(count:%d)ó��, ó���� kshot_number���(%s)"), expiredCount, expiredKnumbers);

					CString formatMsg;
					formatMsg.Format(_T("[expired] �̼��� ����� ����/ȯ��(count:%d)ó��, ó���� kshot_number���(%s)"), expiredCount, expiredKnumbers);
					Client::WriteSendRawData(formatMsg);

					Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::ResultProc(), [expired] �̼��� ����� ����/ȯ��ó��(count:%d)"), expiredCount);
				}

			}
		}


		////////////// ��� ó��
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

				// ����߿� ������ ���� ���� �����Ѵ�.
				if (client->_bUse == false || client->_bBind == false) {
					loopcnt++;
					continue;
				}

				// ��� �����ϱ�
				//if( info->doneflag && ! info->completeflag )
				//if (info->doneflag)
				//{
					int retval = info->result;
					int telecom = info->telecom;
					char *time = info->done_time.GetBuffer(0);
					char *sendnumber = info->send_number.GetBuffer(0);
					char *receiveNo = info->receiveNo.GetBuffer(0);

					// Ŭ���̾�Ʈ���� ��� ACK
					client->Send_Report(retval, _ttoi(sendnumber), receiveNo, time, telecom);

					info->completeflag = TRUE;

					info->done_time.ReleaseBuffer();
					info->send_number.ReleaseBuffer();
					info->receiveNo.ReleaseBuffer();

					notify_cnt++;
				//}

				loopcnt++;

				if (loopcnt % 500 == 0) {
					Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::ResultProc(), [msg] ���ο� ���� ��� Agent�� �۽�(count:%d)"), notify_cnt);
					Sleep(5);
				}
			}			

			if (notify_cnt > 0)
			{
				Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientManager::ResultProc(), [msg] ���ο� ���� ��� Agent�� �۽�(count:%d)"), notify_cnt);

				//Log(CMdlLog::LEVEL_DEBUG, _T("���� ��� ����"));
				manager->FlushResult(result);
			}

			// ó���Ǽ� ���� ȣ��
			CkshotDlg* dlg = (CkshotDlg*)AfxGetMainWnd();
			dlg->SetDoingSendCount(manager->_mapSendResult.size());
			//

		}

	}

	manager->SetDoneThread(0);

	Log(CMdlLog::LEVEL_DEBUG, _T("ClientManager::ResultProc(), �����."));

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

					// ����ð�
					CTime curTime = CTime::GetCurrentTime();

					// ������ ��ũüũ ���� ����ð� = ����ð� - ��������ũüũ�ð�
					CTimeSpan LinkDiff = curTime  - client->_lastLinkCheckTime;
					
					// ������ ��ũüũ ���� ����ð�(��)
					LONGLONG elapsedLink = LinkDiff.GetTotalSeconds();

					// ������ �߼� ���� ����ð� = ����ð� - �������߼۽ð�
					CTimeSpan SandDiff = curTime  - client->_lastSendTime;

					// ������ �߼� ���� ����ð�(��)
					LONGLONG elapsedSend = SandDiff.GetTotalSeconds();

					TRACE(_T("client->_id:%s, Interval Time Sec: %d\n"), client->_id, elapsedLink);

					// ��ũüũ �˻�
					// ������ ��ũüũ ���� ����ð�(��) > LINKCHK_LIMIT_SEC

					//if(elapsedLink >= 10)   // TEST CODE
					if(elapsedLink >= LINKCHK_LIMIT_SEC)
					{
						//��������

						//client->_socket.Close();

						//client->PostThreadMessage(WM_DISCONNECT_NOTREPLY, 0, 0);

						//CWnd* cwnd = AfxGetMainWnd();
						//HWND hwnd = cwnd->GetSafeHwnd();
						//DWORD main_id = ::AfxGetThread()->m_nThreadID;
						//cwnd->PostMessage(WM_DISCONNECT_NOTREPLY, client->_socket, 0);

						//::PostThreadMessage((main_id, WM_DISCONNECT_NOTREPLY, client->_socket, 0);

						//��������
						//::AfxGetApp()->PostThreadMessage(WM_DISCONNECT_NOTREPLY, (WPARAM)client, 0);

						Log(CMdlLog::LEVEL_ERROR, _T("��ũüũ ������� ���ѽð�(180��[3��])���, WM_CLOSE_FOR_NOREPLY ó��"));

						//client->PostThreadMessage(WM_CLOSE_FOR_NOREPLY, 0, 0);
						//validClient = FALSE;
					}
					else if( elapsedSend >= LINKCHK_LIMIT_IDLE_SEC )
					{
						if (manager->_close_idleclient) 
						{
							Log(CMdlLog::LEVEL_ERROR, _T("���� ���ۿ�û ���� ���ѽð�(1800��[30��]) ���, WM_CLOSE_FOR_NOREPLY ó��"));

							client->PostThreadMessage(WM_CLOSE_FOR_NOREPLY, 0, 0);

							validClient = FALSE;
						}
					}

					if( validClient ) 
					{
						TRACE(_T("LinkCheckProc(), Send LinkCheck(client_id:%s)"), client->_id);

						// ��ũ üũ ����
						client->Send_LinkChk();
					}

				}
			}

			pos++;
		}
	}

	manager->SetDoneThread(1);

	Log(CMdlLog::LEVEL_DEBUG, _T("LinkCheckProc(), �����."));

	return 1;
}

