
#include "stdafx.h"


#ifndef _UNICODE
#pragma comment(lib, "./lib/Mdllog/MdlLog.lib")
#else
#pragma comment(lib, "./lib/Mdllog/MdlLogu.lib")
#endif

///////////////////////////////////////////////////////////////////////////// 전역객체

// 로그객체
CMdlLog			g_log;

// 환경설정(config.ini) 얻기
CommConfig		g_config;

// 발송관리 객체 전역화
SendManager*	g_sendManager;

// DB 엑세스 동기화 객체
CCriticalSection	g_cs;

// 이전 경과된 시간
DWORD			g_prevElapseTime = GetTickCount();

// LOG에서 TRACE 출력 유무
BOOL	g_bDebugTrace;

//////////////////////// 동기화 객체들
// 발송요청에대한  DB저장 이벤트
HANDLE g_hDBEvent;
HANDLE g_hDBEvent2;

// 발송 이벤트
HANDLE g_hSendEvent;

// kshot 발송번호
unsigned int	g_kshot_send_number = 0;

// 메인 스레드 id
DWORD g_mainTid;

// 지능망 사용유무
BOOL g_enable_intelli;

///////////////////////////////////////////////////////////////////////////// 전역함수
char * ANSIToUTF8(char * pszCode)
{
    int     nLength, nLength2;
    BSTR    bstrCode; 
    char    *pszUTFCode = NULL;

    nLength = MultiByteToWideChar(CP_ACP, 0, pszCode, lstrlen(pszCode), NULL, NULL); 
    bstrCode = SysAllocStringLen(NULL, nLength); 
    MultiByteToWideChar(CP_ACP, 0, pszCode, lstrlen(pszCode), bstrCode, nLength);

    nLength2 = WideCharToMultiByte(CP_UTF8, 0, bstrCode, -1, pszUTFCode, 0, NULL, NULL); 
    pszUTFCode = (char*)malloc(nLength2+1); 
    WideCharToMultiByte(CP_UTF8, 0, bstrCode, -1, pszUTFCode, nLength2, NULL, NULL); 

    return pszUTFCode;
}

void Logmsg(CMdlLog::Level loglevel, const char* fmt, ...)
{
	char temp[4096];
	va_list Marker;

	va_start(Marker, fmt);
	vsprintf(temp, fmt, Marker);
	va_end(Marker);

	Log(loglevel, temp);
}

void Log(CMdlLog::Level loglevel, CString msg)
{
	// config 구성에서 debug_trace가 설정되면
	if (g_bDebugTrace) {
		TRACE(msg);
		TRACE(_T("\n"));
	}

	g_log.Write(loglevel, msg);
}

/*
void TRACE(const char* fmt, ...)
{
	// config 구성에서 debug_trace가 설정되면

	DWORD ticktime = GetTickCount();

	if (!g_bDebugTrace)
		return;

	char temp[512];
	va_list Marker;

	va_start(Marker, fmt);
	vsprintf(temp, fmt, Marker);
	va_end(Marker);

	TRACE(_T("%d, "), ticktime);
	TRACE(temp);
	TRACE(_T("\n"));
}
*/