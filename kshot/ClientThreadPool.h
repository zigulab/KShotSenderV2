#pragma once

// ������ �׷� ũ��
#define GROUP_THREAD_NUM		64

// �ʱⰪ ( ù��° �׷��� �ʱ�ũ��)
#define INIT_THREAD_NUM			10


class Client;
class ClientManager;

class ClientThreadPool
{
public:
	ClientThreadPool(ClientManager	*manager);
	virtual ~ClientThreadPool(void);

/////////////////////////////////////////////////////////////////////////// DATA

	// 2���� �迭 ����, ǥ ������ ������(Ŭ���̾�Ʈ)���
	// �ϳ��� ���� ������ �׷��� �ȴ�.
	// ������ ������ �׷�ũ��(GROUP_THREAD_NUM)�� ������ ���ο� �׷�(��)�� ����� �����Ѵ�.
	list<vector<Client*>*>	_vectorList;

	int						_currentClientCount;

	ClientManager			*_manager;

/////////////////////////////////////////////////////////////////////////// METHOD
	// �ʱ�ȭ
	virtual	bool	Init();
	virtual bool	UnInit();
	//
	virtual	Client*	GetClient();
};

