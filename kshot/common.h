
#include <WinInet.h>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <queue>

#include ".\lib\Mdllog\MdlLog.h"

#include "CommConfig.h"

using namespace std;


///////////////////////////////////////////////////////////////////////////// ���� ���
// KShotWatcher�� ���μ����� ���( ����ֳ�?! )
#define WM_ALIVE_MSG			WM_USER+0x1001

// ��Ż� ���� ���
typedef enum tagTELECOM_SERVER_NAME
{
	TELECOM_KT_XROSHOT,
	TELECOM_KT_INTELLI
}TELECOM_SERVER_NAME;

// ���� �޼���
#define WM_CONNECT_TELECOM			WM_USER+0x1003
#define WM_RECONNECT_TELECOM			WM_USER+0x1004

// Ŭ���̾�Ʈ ��� ����
#define WM_ADD_CLIENT_LIST				WM_USER+0x1005
#define WM_DEL_CLIENT_LIST				WM_USER+0x1006
#define WM_UPDATE_CLIENT_LIST			WM_USER+0x1007
#define WM_UPDATE_DOING_COUNT		WM_USER+0x1008
#define WM_STATISTIC_RECAL				WM_USER+0x1009


// Ŭ���̾�Ʈ ����������� ���� ����
//#define WM_DISCONNECT_NOTREPLY  WM_USER+0x1005
#define WM_CLOSE_FOR_NOREPLY  WM_USER+0x1005

// ���ο� Ŭ���̾�Ʈ ����
#define WM_NEW_CONNECTION  WM_USER+0x1006

//////////////////////////////////////////////////////////// Timer
//////// �ý��� �ʱ�ȭ
#define	KSHOT_START_TIMER_ID				901				
#define	KSHOT_AUTO_START_TIMER_ID		902
#define  KSHOT_AUTO_START_WAIT_TIMER_ID 903
#define  KSHOT_AUTO_START_WAIT			20000			// 20 sec		

//////// ũ�μ�,���ɸ� Timer�� ���� �޼���(*** ���� ������� ���� ) 
#define WM_PING_TIMER			WM_USER+0x1005

// ũ�μ�,���ɸ� timer id
#define XROSHOT_TIMER_ID		1
#define INTELLI_TIMER_ID		2

// timer interval( xroshot:50�ʿ� �ѹ�, intelli: 7�ʿ� �ѹ� )
#define XROSHOT_TIMER_INTERVAL	50000
#define INTELLI_TIMER_INTERVAL	7000

///////// Ŭ���̾�Ʈ Timer�� interval(�ʴ���)
// ��ũüũ �ֱ�
#define	LINKCHK_INTERVAL_MSEC	20000

// ��ũüũ �������, ���ѽð�(180��[3��])�̻� ����Ǹ�, Ŭ���̾�Ʈ ��������
#define LINKCHK_LIMIT_SEC		180

// ��ũüũ ����������, �������� ���� ���ѽð�(1800��[30��])�̻� ����Ǹ�, Ŭ���̾�Ʈ ��������
#define	LINKCHK_LIMIT_IDLE_SEC	1800

// Agent�� ������� ���� ���� �ð�(300��:5��)
#define RESULT_DELAY_LIMITTIME 300

//////////////////////////////////////////////////////////// ���Ϲ��� ����
#define	SOCKET_RECEIVE_BUF_SIZE (1024*1000)		// 1000K, 1M
#define	SOCKET_SEND_BUF_SIZE	(1024*1000)		// 1000K, 1M

//#define SOCKET_MSG_BASE_SIZE	4527
#define SOCKET_MSG_BASE_SIZE	2527

///////////////////////////////////////////////////////////////////////////// ������ü

// �αװ�ü
extern CMdlLog		g_log;

//  ȯ�漳��(config.ini)
extern CommConfig	g_config;

// �߼۰��� ��ü ����ȭ
class SendManager;
extern SendManager* g_sendManager;

// DB ������ ����ȭ ��ü
extern CCriticalSection g_cs;

// ���� ����� �ð�
extern DWORD	g_prevElapseTime;

// LOG���� TRACE ��� ����
extern BOOL	g_bDebugTrace;

// ����ȭ ��ü��
extern HANDLE g_hDBEvent;
extern HANDLE g_hDBEvent2;

extern HANDLE g_hSendEvent;

// kshot �߼۹�ȣ
extern unsigned int	g_kshot_send_number;

// ���� ������ id
extern DWORD g_mainTid;

// ���ɸ� �������
extern BOOL g_enable_intelli;

///////////////////////////////////////////////////////////////////////////// �����Լ�

// �ѱ��ڵ� ��ȯ
char*	ANSIToUTF8(char * pszCode);

// �α� ���
void Logmsg(CMdlLog::Level loglevel, const char* fmt, ...);
void Log(CMdlLog::Level loglevel, CString msg);

//void TRACE(const char* fmt, ...);

///////////////////////////////////////////////////////////////////////////// DEBUGGING

// ���δ��� �ӵ� üũ

#define DEBUG_ELAPSE_CHECK
#define ELAPSE_CHECK_END(fn, n) 	QueryPerformanceCounter(&Endtime); \
{LONGLONG elapsed = Endtime.QuadPart - BeginTime.QuadPart; \
double duringtime = (double)elapsed / (double)Frequency.QuadPart; \
TRACE(_T("%s-%d).EncrpytTime: %f\n"), fn, n, duringtime); \
BeginTime = Endtime;}
