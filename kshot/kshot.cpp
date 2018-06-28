
// kshot.cpp : 응용 프로그램에 대한 클래스 동작을 정의합니다.
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


// CkshotApp 생성

CkshotApp::CkshotApp()
	:m_Mutex(FALSE, TEXT("KSHOT_SHARED_MUTEX"))
{
	// 다시 시작 관리자 지원
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	m_hMap = NULL;
	m_pSharedMemory = NULL;

	// TODO: 여기에 생성 코드를 추가합니다.
	// InitInstance에 모든 중요한 초기화 작업을 배치합니다.
}


// 유일한 CkshotApp 개체입니다.

CkshotApp theApp;


// CkshotApp 초기화

BOOL CkshotApp::InitInstance()
{
	// 응용 프로그램 매니페스트가 ComCtl32.dll 버전 6 이상을 사용하여 비주얼 스타일을
	// 사용하도록 지정하는 경우, Windows XP 상에서 반드시 InitCommonControlsEx()가 필요합니다.
	// InitCommonControlsEx()를 사용하지 않으면 창을 만들 수 없습니다.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 응용 프로그램에서 사용할 모든 공용 컨트롤 클래스를 포함하도록
	// 이 항목을 설정하십시오.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if( IsAlreadyExists() ) 
	{
		if( IDNO == AfxMessageBox(_T("이미 실행중인 KShot이 있습니다. 그래도 실행하시겠습니까?"), MB_YESNO) )
			return FALSE;
	}

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// 공유메모리 초기화
	if (!InitSharedMemory())
		return FALSE;

	AfxEnableControlContainer();

	// 대화 상자에 셸 트리 뷰 또는
	// 셸 목록 뷰 컨트롤이 포함되어 있는 경우 셸 관리자를 만듭니다.
	CShellManager *pShellManager = new CShellManager;

	// 표준 초기화
	// 이들 기능을 사용하지 않고 최종 실행 파일의 크기를 줄이려면
	// 아래에서 필요 없는 특정 초기화
	// 루틴을 제거해야 합니다.
	// 해당 설정이 저장된 레지스트리 키를 변경하십시오.
	// TODO: 이 문자열을 회사 또는 조직의 이름과 같은
	// 적절한 내용으로 수정해야 합니다.
	SetRegistryKey(_T("로컬 응용 프로그램 마법사에서 생성된 응용 프로그램"));

	CkshotDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: 여기에 [확인]을 클릭하여 대화 상자가 없어질 때 처리할
		//  코드를 배치합니다.
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 여기에 [취소]를 클릭하여 대화 상자가 없어질 때 처리할
		//  코드를 배치합니다.
	}

	// 위에서 만든 셸 관리자를 삭제합니다.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// 대화 상자가 닫혔으므로 응용 프로그램의 메시지 펌프를 시작하지 않고  응용 프로그램을 끝낼 수 있도록 FALSE를
	// 반환합니다.
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
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
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
		TRACE("KShotAssister가 실행되지 않았습니다.\n");
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
		AfxMessageBox(_T("맵을생성할수없음"));
		return FALSE;
	}
	m_pSharedMemory = (TCHAR*)::MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS,
		0, 0, sizeof(TCHAR) * 128);
	if (m_pSharedMemory == NULL)
	{
		AfxMessageBox(_T("파일핸들로부터읽어오지못함"));
		return FALSE;
	}

	return TRUE;
}
