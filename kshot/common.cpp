
#include "stdafx.h"


#ifndef _UNICODE
#pragma comment(lib, "./lib/Mdllog/MdlLog.lib")
#else
#pragma comment(lib, "./lib/Mdllog/MdlLogu.lib")
#endif

///////////////////////////////////////////////////////////////////////////// ������ü

// �αװ�ü
CMdlLog			g_log;

// ȯ�漳��(config.ini) ���
CommConfig		g_config;

// �߼۰��� ��ü ����ȭ
SendManager*	g_sendManager;

// DB ������ ����ȭ ��ü
CCriticalSection	g_cs;

// ���� ����� �ð�
DWORD			g_prevElapseTime = GetTickCount();

// LOG���� TRACE ��� ����
BOOL	g_bDebugTrace;

//////////////////////// ����ȭ ��ü��
// �߼ۿ�û������  DB���� �̺�Ʈ
HANDLE g_hDBEvent;
HANDLE g_hDBEvent2;

// �߼� �̺�Ʈ
HANDLE g_hSendEvent;

// kshot �߼۹�ȣ
unsigned int	g_kshot_send_number = 0;

// ���� ������ id
DWORD g_mainTid;

// ���ɸ� �������
BOOL g_enable_intelli;

///////////////////////////////////////////////////////////////////////////// �����Լ�
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
	// config �������� debug_trace�� �����Ǹ�
	if (g_bDebugTrace) {
		TRACE(msg);
		TRACE(_T("\n"));
	}

	g_log.Write(loglevel, msg);
}

/*
void TRACE(const char* fmt, ...)
{
	// config �������� debug_trace�� �����Ǹ�

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