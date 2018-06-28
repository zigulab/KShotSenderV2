
// kshot.h : PROJECT_NAME 응용 프로그램에 대한 주 헤더 파일입니다.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH에 대해 이 파일을 포함하기 전에 'stdafx.h'를 포함합니다."
#endif

#include "resource.h"		// 주 기호입니다.


// CkshotApp:
// 이 클래스의 구현에 대해서는 kshot.cpp을 참조하십시오.
//

class CkshotApp : public CWinApp
{
public:
	CkshotApp();

	// 공유 메모리
	HANDLE			m_hMap;
	TCHAR*			m_pSharedMemory;
	CMutex			m_Mutex;


// 재정의입니다.
public:
	virtual BOOL InitInstance();

// 구현입니다.

	DECLARE_MESSAGE_MAP()

	BOOL IsAlreadyExists();

	virtual int ExitInstance();

	afx_msg void OnUpdateClientList(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateDoingCount(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateStatistic(WPARAM wParam, LPARAM lParam);

	BOOL InitSharedMemory(void);
};

extern CkshotApp theApp;