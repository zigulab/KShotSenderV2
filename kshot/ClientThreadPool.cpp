#include "StdAfx.h"
#include "ClientThreadPool.h"

#include "Client.h"
#include "ClientManager.h"


/////////////////////////////////////////////////////////////////////////////////
/* ���� �ҽ�
	map<CString, XroshotSendInfo*>::iterator it = _sendList.find(sendId);
	if( it != _sendList.end() ) 
	{
		XroshotSendInfo *info = it->second;

		// ��� ��� ����.
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


	Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientThreadPool::Init(), ClientThreadPool, �ʱ� ����(%d)"), _currentClientCount);


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

	Log(CMdlLog::LEVEL_EVENT, _T("ClientThreadPool, ����"));

	return true;
}

Client* ClientThreadPool::GetClient()
{
	Client* client = NULL;

	// ��ü �׷쿡�� ������� �ʴ� �����带 �Ҵ��Ѵ�.
	// ����� ������ �˻�
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

	
	// ����� ������

	// �ϴ� ���ο� ������ �����ϰ�
	client = (Client*)AfxBeginThread(RUNTIME_CLASS(Client), 0, 0, CREATE_SUSPENDED);
	
	client->SetNo(_currentClientCount);
	client->SetParent(_manager);

	_currentClientCount++;

	client->SetUse(true);

	// ���ο� ���� �����Ѵ�.
	vector<Client*>* vtClient = _vectorList.back();

	// ����ũ�Ⱑ �׷�ũ��(GROUP_THREAD_NUM)���� ������
	// �� �׷��� ���ʿ� �����Ѵ�.
	if( vtClient->size() < GROUP_THREAD_NUM )
	{
		
		vtClient->push_back(client);
	}
	else	// �ƴϸ� ���ο� �׷��� ����� �����Ѵ�.
	{
		vector<Client*>* vt = new vector<Client*>;
		vt->push_back(client);
		_vectorList.push_back(vt);
	}

	Logmsg(CMdlLog::LEVEL_EVENT, _T("ClientThreadPool::GetClient(), New Client(%d) �����ϰ� Return"), client->_no);

	client->ResumeThread();

	return client;
}