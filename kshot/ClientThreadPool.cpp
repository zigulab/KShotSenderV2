#include "StdAfx.h"
#include "ClientThreadPool.h"

#include "Client.h"
#include "ClientManager.h"


/////////////////////////////////////////////////////////////////////////////////
/* 참고 소스
	map<CString, XroshotSendInfo*>::iterator it = _sendList.find(sendId);
	if( it != _sendList.end() ) 
	{
		XroshotSendInfo *info = it->second;

		// 모든 결과 받음.
		if( alldone )
		{
			delete info;
			_sendList.erase(it);
		}


     map<CString , XroshotSendInfo*>::iterator it;
     for (it=_sendList.begin();it!=_sendList.end();it++) {
          delete (XroshotSendInfo*)it->second;
     }

	 _sendList.clear();


	 _sendList[sendId] = info;

*/
////////////////////////////////////////////////////////////////////////////////


ClientThreadPool::ClientThreadPool(ClientManager *manager)
{
	_manager = manager;

	_currentClientCount = 0;
}


ClientThreadPool::~ClientThreadPool(void)
{
}

bool ClientThreadPool::Init()
{
	vector<Client*>* vt = new vector<Client*>;

	int i = 0;
	for(; i < INIT_THREAD_NUM; i++ )
	{
		Client* client  = (Client*)AfxBeginThread(RUNTIME_CLASS(Client), 0, 0, CREATE_SUSPENDED);

		//BOOL flag = client->m_bAutoDelete;
	
		client->SetNo(i);

		client->SetParent(_manager);
		
		vt->push_back(client);

		client->ResumeThread();
	}

	_currentClientCount = i;

	_vectorList.push_back(vt);


	Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientThreadPool::Init(), ClientThreadPool, 초기 생성(%d)"), _currentClientCount);


	return true;
}

bool ClientThreadPool::UnInit()
{
	list<vector<Client*>*>::iterator pos = _vectorList.begin();
	
	while( pos != _vectorList.end() )
	{	
		vector<Client*>* vtClient = *pos;

		int sz = vtClient->size();

		HANDLE *Handles = new HANDLE[sz];

		int i = 0;
		vector<Client*>::iterator it = vtClient->begin();
		for( ; it != vtClient->end(); i++ )
		{
			Client* client  = *it;

			//client->_socket.Close();
			
			Handles[i] = client->m_hThread;

			client->PostThreadMessage(WM_QUIT, 0, 0);

			it++;
		}

		DWORD ret = ::WaitForMultipleObjects(sz, Handles, TRUE, INFINITE);

		delete [] Handles; 
	
		pos++;
	}

	pos = _vectorList.begin();
	while( pos != _vectorList.end() )
	{	
		vector<Client*>* vtClient = *pos;
		delete vtClient;
		pos++;
	}

	_vectorList.clear();

	Log(CMdlLog::LEVEL_EVENT, _T("ClientThreadPool, 종료"));

	return true;
}

Client* ClientThreadPool::GetClient()
{
	Client* client = NULL;

	// 전체 그룹에서 사용하지 않는 쓰레드를 할당한다.
	// 행단위 열단위 검사
	list<vector<Client*>*>::iterator pos = _vectorList.begin();
	int i = 0;
	for(; pos != _vectorList.end(); i++ )
	{	
		vector<Client*>* vtClient = *pos;
		vector<Client*>::iterator it = vtClient->begin();
		while(it != vtClient->end())
		{
			client  = *it;
			if( client->isUse() == false ) {
				client->SetUse(true);

				//Logmsg(CMdlLog::LEVEL_DEBUG, _T("Idle Client(%d) Return"), client->_no);

				return client;
			}

			it++;
		}
		
		pos++;
	}

	
	// 빈곳이 없으면

	// 일단 새로운 쓰레드 생성하고
	client = (Client*)AfxBeginThread(RUNTIME_CLASS(Client), 0, 0, CREATE_SUSPENDED);
	
	client->SetNo(_currentClientCount);
	client->SetParent(_manager);

	_currentClientCount++;

	client->SetUse(true);

	// 새로운 곳에 배정한다.
	vector<Client*>* vtClient = _vectorList.back();

	// 현재크기가 그룹크기(GROUP_THREAD_NUM)보다 작으면
	// 그 그룹의 뒤쪽에 배정한다.
	if( vtClient->size() < GROUP_THREAD_NUM )
	{
		
		vtClient->push_back(client);
	}
	else	// 아니면 새로운 그룹을 만들어 배정한다.
	{
		vector<Client*>* vt = new vector<Client*>;
		vt->push_back(client);
		_vectorList.push_back(vt);
	}

	Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientThreadPool::GetClient(), New Client(%d) 생성하고 Return"), client->_no);

	client->ResumeThread();

	return client;
}