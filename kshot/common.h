
#include <WinInet.h>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <queue>

#include ".\lib\Mdllog\MdlLog.h"

#include "CommConfig.h"

using namespace std;


///////////////////////////////////////////////////////////////////////////// 전역 상수
// KShotWatcher와 프로세스간 통신( 살아있네?! )
#define WM_ALIVE_MSG			WM_USER+0x1001

// 통신사 서버 목록
typedef enum tagTELECOM_SERVER_NAME
{
	TELECOM_KT_XROSHOT,
	TELECOM_KT_INTELLI
}TELECOM_SERVER_NAME;

// 연결 메세지
#define WM_CONNECT_TELECOM			WM_USER+0x1003
#define WM_RECONNECT_TELECOM			WM_USER+0x1004

// 클라이언트 목록 갱신
#define WM_ADD_CLIENT_LIST				WM_USER+0x1005
#define WM_DEL_CLIENT_LIST				WM_USER+0x1006
#define WM_UPDATE_CLIENT_LIST			WM_USER+0x1007
#define WM_UPDATE_DOING_COUNT		WM_USER+0x1008
#define WM_STATISTIC_RECAL				WM_USER+0x1009


// 클라이언트 응답없음으로 연결 종료
//#define WM_DISCONNECT_NOTREPLY  WM_USER+0x1005
#define WM_CLOSE_FOR_NOREPLY  WM_USER+0x1005

// 새로운 클라이언트 생성
#define WM_NEW_CONNECTION  WM_USER+0x1006

//////////////////////////////////////////////////////////// Timer
//////// 시스템 초기화
#define	KSHOT_START_TIMER_ID				901				
#define	KSHOT_AUTO_START_TIMER_ID		902
#define  KSHOT_AUTO_START_WAIT_TIMER_ID 903
#define  KSHOT_AUTO_START_WAIT			20000			// 20 sec		

//////// 크로샷,지능망 Timer를 위한 메세지(*** 현재 사용하지 않음 ) 
#define WM_PING_TIMER			WM_USER+0x1005

// 크로샷,지능망 timer id
#define XROSHOT_TIMER_ID		1
#define INTELLI_TIMER_ID		2

// timer interval( xroshot:50초에 한번, intelli: 7초에 한번 )
#define XROSHOT_TIMER_INTERVAL	50000
#define INTELLI_TIMER_INTERVAL	7000

///////// 클라이언트 Timer의 interval(초단위)
// 링크체크 주기
#define	LINKCHK_INTERVAL_MSEC	20000

// 링크체크 응답없이, 제한시간(180초[3분])이상 경과되면, 클라이언트 연결종료
#define LINKCHK_LIMIT_SEC		180

// 링크체크 응답하지만, 문자전송 없이 제한시간(1800초[30분])이상 경과되면, 클라이언트 연결종료
#define	LINKCHK_LIMIT_IDLE_SEC	1800

// Agent로 결과전송 지연 제한 시간(300초:5분)
#define RESULT_DELAY_LIMITTIME 300

//////////////////////////////////////////////////////////// 소켓버퍼 설정
#define	SOCKET_RECEIVE_BUF_SIZE (1024*1000)		// 1000K, 1M
#define	SOCKET_SEND_BUF_SIZE	(1024*1000)		// 1000K, 1M

//#define SOCKET_MSG_BASE_SIZE	4527
#define SOCKET_MSG_BASE_SIZE	2527

///////////////////////////////////////////////////////////////////////////// 전역객체

// 로그객체
extern CMdlLog		g_log;

//  환경설정(config.ini)
extern CommConfig	g_config;

// 발송관리 객체 전역화
class SendManager;
extern SendManager* g_sendManager;

// DB 엑세스 동기화 객체
extern CCriticalSection g_cs;

// 이전 경과된 시간
extern DWORD	g_prevElapseTime;

// LOG에서 TRACE 출력 유무
extern BOOL	g_bDebugTrace;

// 동기화 객체들
extern HANDLE g_hDBEvent;
extern HANDLE g_hDBEvent2;

extern HANDLE g_hSendEvent;

// kshot 발송번호
extern unsigned int	g_kshot_send_number;

// 메인 스레드 id
extern DWORD g_mainTid;

// 지능망 사용유무
extern BOOL g_enable_intelli;

///////////////////////////////////////////////////////////////////////////// 전역함수

// 한글코드 변환
char*	ANSIToUTF8(char * pszCode);

// 로그 출력
void Logmsg(CMdlLog::Level loglevel, const char* fmt, ...);
void Log(CMdlLog::Level loglevel, CString msg);

//void TRACE(const char* fmt, ...);

///////////////////////////////////////////////////////////////////////////// DEBUGGING

// 라인단위 속도 체크

#define DEBUG_ELAPSE_CHECK
#define ELAPSE_CHECK_END(fn, n) 	QueryPerformanceCounter(&Endtime); \
{LONGLONG elapsed = Endtime.QuadPart - BeginTime.QuadPart; \
double duringtime = (double)elapsed / (double)Frequency.QuadPart; \
TRACE(_T("%s-%d).EncrpytTime: %f\n"), fn, n, duringtime); \
BeginTime = Endtime;}
