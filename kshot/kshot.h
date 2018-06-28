
// kshot.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CkshotApp:
// �� Ŭ������ ������ ���ؼ��� kshot.cpp�� �����Ͻʽÿ�.
//

class CkshotApp : public CWinApp
{
public:
	CkshotApp();

	// ���� �޸�
	HANDLE			m_hMap;
	TCHAR*			m_pSharedMemory;
	CMutex			m_Mutex;


// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()

	BOOL IsAlreadyExists();

	virtual int ExitInstance();

	afx_msg void OnUpdateClientList(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateDoingCount(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateStatistic(WPARAM wParam, LPARAM lParam);

	BOOL InitSharedMemory(void);
};

extern CkshotApp theApp;