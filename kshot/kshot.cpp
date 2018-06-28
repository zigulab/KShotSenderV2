
// kshot.cpp : ���� ���α׷��� ���� Ŭ���� ������ �����մϴ�.
//

#include "stdafx.h"
#include "kshot.h"
#include "kshotDlg.h"

#include "Client.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CkshotApp

BEGIN_MESSAGE_MAP(CkshotApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
	ON_THREAD_MESSAGE(WM_UPDATE_CLIENT_LIST, OnUpdateClientList)
	ON_THREAD_MESSAGE(WM_UPDATE_DOING_COUNT, OnUpdateDoingCount)
	ON_THREAD_MESSAGE(WM_STATISTIC_RECAL, OnUpdateStatistic)
END_MESSAGE_MAP()


// CkshotApp ����

CkshotApp::CkshotApp()
	:m_Mutex(FALSE, TEXT("KSHOT_SHARED_MUTEX"))
{
	// �ٽ� ���� ������ ����
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	m_hMap = NULL;
	m_pSharedMemory = NULL;

	// TODO: ���⿡ ���� �ڵ带 �߰��մϴ�.
	// InitInstance�� ��� �߿��� �ʱ�ȭ �۾��� ��ġ�մϴ�.
}


// ������ CkshotApp ��ü�Դϴ�.

CkshotApp theApp;


// CkshotApp �ʱ�ȭ

BOOL CkshotApp::InitInstance()
{
	// ���� ���α׷� �Ŵ��佺Ʈ�� ComCtl32.dll ���� 6 �̻��� ����Ͽ� ���־� ��Ÿ����
	// ����ϵ��� �����ϴ� ���, Windows XP �󿡼� �ݵ�� InitCommonControlsEx()�� �ʿ��մϴ�.
	// InitCommonControlsEx()�� ������� ������ â�� ���� �� �����ϴ�.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ���� ���α׷����� ����� ��� ���� ��Ʈ�� Ŭ������ �����ϵ���
	// �� �׸��� �����Ͻʽÿ�.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if( IsAlreadyExists() ) 
	{
		if( IDNO == AfxMessageBox(_T("�̹� �������� KShot�� �ֽ��ϴ�. �׷��� �����Ͻðڽ��ϱ�?"), MB_YESNO) )
			return FALSE;
	}

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// �����޸� �ʱ�ȭ
	if (!InitSharedMemory())
		return FALSE;

	AfxEnableControlContainer();

	// ��ȭ ���ڿ� �� Ʈ�� �� �Ǵ�
	// �� ��� �� ��Ʈ���� ���ԵǾ� �ִ� ��� �� �����ڸ� ����ϴ�.
	CShellManager *pShellManager = new CShellManager;

	// ǥ�� �ʱ�ȭ
	// �̵� ����� ������� �ʰ� ���� ���� ������ ũ�⸦ ���̷���
	// �Ʒ����� �ʿ� ���� Ư�� �ʱ�ȭ
	// ��ƾ�� �����ؾ� �մϴ�.
	// �ش� ������ ����� ������Ʈ�� Ű�� �����Ͻʽÿ�.
	// TODO: �� ���ڿ��� ȸ�� �Ǵ� ������ �̸��� ����
	// ������ �������� �����ؾ� �մϴ�.
	SetRegistryKey(_T("���� ���� ���α׷� �����翡�� ������ ���� ���α׷�"));

	CkshotDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: ���⿡ [Ȯ��]�� Ŭ���Ͽ� ��ȭ ���ڰ� ������ �� ó����
		//  �ڵ带 ��ġ�մϴ�.
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: ���⿡ [���]�� Ŭ���Ͽ� ��ȭ ���ڰ� ������ �� ó����
		//  �ڵ带 ��ġ�մϴ�.
	}

	// ������ ���� �� �����ڸ� �����մϴ�.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// ��ȭ ���ڰ� �������Ƿ� ���� ���α׷��� �޽��� ������ �������� �ʰ�  ���� ���α׷��� ���� �� �ֵ��� FALSE��
	// ��ȯ�մϴ�.
	return FALSE;
}


BOOL CkshotApp::IsAlreadyExists()
{
 	HANDLE hMutex = CreateMutex(NULL, TRUE, _T("kshot"));
 	if(GetLastError() == ERROR_ALREADY_EXISTS)
 	{
 		ReleaseMutex(hMutex);

		CWnd *pWndPrev, *pWndChild;
		pWndPrev = CWnd::FindWindow(NULL, _T("kshot"));
		if(pWndPrev)
		{
			pWndChild = pWndPrev->GetLastActivePopup();

			if(pWndChild->IsIconic())
				pWndPrev->ShowWindow(SW_RESTORE);

			pWndChild->SetForegroundWindow();
		}
		return TRUE;
	}
	ReleaseMutex(hMutex);

	return FALSE;
}


int CkshotApp::ExitInstance()
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	if (m_pSharedMemory != NULL)
		::UnmapViewOfFile(m_pSharedMemory);
	if (m_hMap != NULL)
		::CloseHandle(m_hMap);

	return CWinApp::ExitInstance();
}

void CkshotApp::OnUpdateClientList(WPARAM wParam, LPARAM lParam)
{
	vector<KSHOTCLIENT> *vtClient = (vector<KSHOTCLIENT>*)wParam;
	((CkshotDlg*)m_pMainWnd)->UpdateClientListControl(vtClient);
}

void CkshotApp::OnUpdateDoingCount(WPARAM wParam, LPARAM lParam)
{
	((CkshotDlg*)m_pMainWnd)->UpdateDoingSendCount((int)wParam);
}

void CkshotApp::OnUpdateStatistic(WPARAM wParam, LPARAM lParam)
{
	HWND hwnd = FindWindow(NULL, TEXT("KShotAssister"));
	//HWND hwnd = wnd->GetSafeHwnd();

	if (hwnd) {
		::PostMessage(hwnd, WM_STATISTIC_RECAL, 0, 0);
	}
	else
		TRACE("KShotAssister�� ������� �ʾҽ��ϴ�.\n");
}

BOOL CkshotApp::InitSharedMemory(void)
{
	m_hMap = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
		0, sizeof(TCHAR) * 128, _T("IPC_TEST_SHARED_MEMORY"));
	if (::GetLastError() == ERROR_ALREADY_EXISTS)
	{
		m_hMap = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _T("IPC_TEST_SHARED_MEMORY"));
	}

	if (m_hMap == NULL)
	{
		AfxMessageBox(_T("���������Ҽ�����"));
		return FALSE;
	}
	m_pSharedMemory = (TCHAR*)::MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS,
		0, 0, sizeof(TCHAR) * 128);
	if (m_pSharedMemory == NULL)
	{
		AfxMessageBox(_T("�����ڵ�κ����о��������"));
		return FALSE;
	}

	return TRUE;
}
