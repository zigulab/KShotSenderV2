#pragma once

// 스레드 그룹 크기
#define GROUP_THREAD_NUM		64

// 초기값 ( 첫번째 그룹의 초기크기)
#define INIT_THREAD_NUM			10


class Client;
class ClientManager;

class ClientThreadPool
{
public:
	ClientThreadPool(ClientManager	*manager);
	virtual ~ClientThreadPool(void);

/////////////////////////////////////////////////////////////////////////// DATA

	// 2차원 배열 형태, 표 형태의 쓰레드(클라이언트)목록
	// 하나의 행이 스레드 그룹이 된다.
	// 쓰레드 갯수가 그룹크기(GROUP_THREAD_NUM)를 넘으면 새로운 그룹(행)를 만들어 누적한다.
	list<vector<Client*>*>	_vectorList;

	int						_currentClientCount;

	ClientManager			*_manager;

/////////////////////////////////////////////////////////////////////////// METHOD
	// 초기화
	virtual	bool	Init();
	virtual bool	UnInit();
	//
	virtual	Client*	GetClient();
};

