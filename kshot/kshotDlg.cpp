
// kshotDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "kshot.h"
#include "kshotDlg.h"
#include "afxdialogex.h"

#include "SetAgentVersionDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

vector<KSHOTCLIENT> _vtKshotClients;

char		CkshotDlg::_msb = 0x01;
char		CkshotDlg::_lsb = 0x00;

BOOL g_bApplyRejectList = FALSE;

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CkshotDlg 대화 상자




CkshotDlg::CkshotDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CkshotDlg::IDD, pParent)
	, _bReconn_Xroshot(FALSE)
	, _bReconn_Intelli(FALSE)
	, _agentVersion(_T(""))
	, _bApplyRejectList(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	_bReconn_Xroshot = TRUE;
	_bReconn_Intelli = TRUE;

	_bRunning = FALSE;

	_waitTime = KSHOT_AUTO_START_WAIT;

	_waitTime = _waitTime / 1000;

	_agentVersion = "1.0";
}

void CkshotDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CLIENT_LIST, _ctrl_client_list);
	DDX_Control(pDX, IDC_DOING_SEND_COUNT, _ctrl_doing_send_count);
	DDX_Text(pDX, IDC_AGENT_VERSION, _agentVersion);
	DDX_Control(pDX, IDC_XROSHOT_LIST, _ctrlXroShotList);
	DDX_Check(pDX, IDC_APPLY_REJECT_SPAM, _bApplyRejectList);
}

BEGIN_MESSAGE_MAP(CkshotDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_PING_TIMER, OnTimerMessage)
	ON_MESSAGE(WM_CONNECT_TELECOM, OnConnect)
	ON_MESSAGE(WM_RECONNECT_TELECOM, OnReconnect)
	ON_MESSAGE(WM_ALIVE_MSG, OnAliveMessage)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_KSHOT_START, &CkshotDlg::OnBnClickedKshotStart)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_SET_KSHOT_NUMBER, &CkshotDlg::OnBnClickedSetKshotNumber)
	ON_BN_CLICKED(IDC_ENABLE_INTELLIGENT, &CkshotDlg::OnBnClickedEnableIntelligent)
	ON_BN_CLICKED(IDC_SET_VERSION, &CkshotDlg::OnBnClickedSetVersion)
	ON_BN_CLICKED(IDC_ADD_ACCOUNT, &CkshotDlg::OnBnClickedAddAccount)
	ON_BN_CLICKED(IDC_DEL_ACCOUNT, &CkshotDlg::OnBnClickedDelAccount)
	ON_BN_CLICKED(IDC_ACCOUNT_ACTIVATE, &CkshotDlg::OnBnClickedAccountActivate)
	ON_BN_CLICKED(IDC_APPLY_REJECT_SPAM, &CkshotDlg::OnBnClickedApplyRejectSpam)
	ON_BN_CLICKED(IDC_RESULT_DONE, &CkshotDlg::OnBnClickedResultDone)
END_MESSAGE_MAP()


// CkshotDlg 메시지 처리기

BOOL CkshotDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	if (prevInit())
	{
		// kshot_number표시
		CEdit* edt = (CEdit*)GetDlgItem(IDC_KSHOT_NUMBER);

		CString szKshotNumber;
		szKshotNumber.Format(_T("%d"), g_kshot_send_number);
		edt->SetWindowText(szKshotNumber);

		// enable_intelligent표시
		// 최초 config.ini 에 따라 표시하며 최종적으로 g_enable_intelli 값에 따라 처리된다.
		CButton* enable_box = (CButton*)GetDlgItem(IDC_ENABLE_INTELLIGENT);
		if (g_config._enable_intelli) {
			enable_box->SetCheck(BST_CHECKED);
			g_enable_intelli = TRUE;
		}
		else
		{
			enable_box->SetCheck(BST_UNCHECKED);
			g_enable_intelli = FALSE;
		}


		SetTimer(KSHOT_AUTO_START_WAIT_TIMER_ID, 1000, NULL);

		// 10초후 자동 시작	// #define  KSHOT_AUTO_START_WAIT			10000			// 10 SEC
		SetTimer(KSHOT_AUTO_START_TIMER_ID, KSHOT_AUTO_START_WAIT, NULL);

		return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
	}
	else {
		AfxMessageBox(_T("시스템 prevInit()작업에 문제가 있습니다. DB연결과 시스템환경을 확인해보기 바랍니다."));
		return FALSE;  
	}
}

void CkshotDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CkshotDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CkshotDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CkshotDlg::OnTimerMessage(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
		case XROSHOT_TIMER_ID :
			SetTimer(XROSHOT_TIMER_ID, XROSHOT_TIMER_INTERVAL, NULL);
			break;
		case INTELLI_TIMER_ID :
			SetTimer(INTELLI_TIMER_ID, INTELLI_TIMER_INTERVAL, NULL);
			break;
		default:
			break;
	}

	return 1;
}

LRESULT CkshotDlg::OnConnect(WPARAM wParam, LPARAM lParam)
{
	if (!_sendManager.Connect()) {
		Log(CMdlLog::LEVEL_ERROR, _T("통신사에 연결할 수 없습니다. 시스템 점검해 주세요"));
	}
	return 1;
}

LRESULT CkshotDlg::OnReconnect(WPARAM wParam, LPARAM lParam)
{
	if( !_sendManager.Reconnect((TELECOM_SERVER_NAME)wParam)) {
		Log(CMdlLog::LEVEL_ERROR, _T("통신사에 재 연결할 수 없습니다. 시스템 점검해 주세요"));
	}
	return 1;
}

LRESULT CkshotDlg::OnAliveMessage(WPARAM wParam, LPARAM lParam)
{
	// 살아있는지 확인

	///////////////////////////////////////
	/* Debugging Code 
	static int count = 0;
	count++;
	if( count > 3 ) {
		//ASSERT(0);
		return 0;
	}
	*/
	///////////////////////////////////////


    CWnd *wnd = FindWindow(NULL, TEXT("KShotWatcher"));
	HWND hwnd = wnd->GetSafeHwnd();
 
    if( hwnd ) {
		//CWnd *wndbtn = FindWindowEx(hwnd, NULL, NULL, TEXT("버튼 클릭"));
        ::PostMessage(hwnd, WM_ALIVE_MSG, 0, 0);
	}
    else
        Log(CMdlLog::LEVEL_ERROR, _T("KShotWatcher가 실행되어 있지 않았습니다."));
 
	return 1;
}

void CkshotDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent)
	{
	case KSHOT_AUTO_START_WAIT_TIMER_ID:
		ShowElapseTime();
		break;
	case 	KSHOT_START_TIMER_ID:
		StopTimerNInit();
		break;

	// 몇초후 자동 재시작
	case 	KSHOT_AUTO_START_TIMER_ID:
		StopTimerNInit();
		break;

	// Ping 처리
	case XROSHOT_TIMER_ID:
		_sendManager._commManager.Ping(XROSHOT_TIMER_ID);
		break;
	case INTELLI_TIMER_ID:
		_sendManager._commManager.Ping(INTELLI_TIMER_ID);
		break;
	default:
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

BOOL CkshotDlg::prevInit()
{
	g_mainTid = GetCurrentThreadId();

	InitClientListControl();
	InitXroShotListControl();

	g_log.SetInit(FALSE, 512, CMdlLog::LEVEL_DEBUG);

	//화면출력 유무
	g_log.ShowConsole(TRUE);

	Log(CMdlLog::LEVEL_EVENT, _T("KSHOTSender Startup"));

	g_config.Init();

	_agentVersion = g_config.kshotAgent.version;
	UpdateData(FALSE);
	SetVersion();

	g_sendManager = &_sendManager;

	return _sendManager.prevInit();
}

void CkshotDlg::SetVersion()
{
	int pos = _agentVersion.Find(".");
	if (pos != -1) {
		CString szMsb = _agentVersion.Left(1);
		CString szLsb = _agentVersion.Mid(pos + 1);

		_msb = _ttoi(szMsb);
		_lsb = _ttoi(szLsb);
	}
}

void CkshotDlg::InitClientListControl()
{
	_ctrl_client_list.DeleteAllItems();
	_ctrl_client_list.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	_ctrl_client_list.InsertColumn(0, _T("uid"), LVCFMT_LEFT, 140, -1);
	_ctrl_client_list.InsertColumn(1, _T("연결Instance수"), LVCFMT_CENTER, 100, -1);
	_ctrl_client_list.InsertColumn(2, _T("최근연결(종료)시간"), LVCFMT_CENTER, 140, -1);
	_ctrl_client_list.InsertColumn(3, _T("최근요청시간"), LVCFMT_CENTER, 140, -1);
	_ctrl_client_list.InsertColumn(4, _T("최근요청건수"), LVCFMT_CENTER, 100, -1);
}

void CkshotDlg::Init()
{
	if( _sendManager.Init() == FALSE) {
		Log(CMdlLog::LEVEL_ERROR, _T("초기화에 성공하지 못했습니다. 시스템 점검후 다시 구동해 주십시오."));
		return;
	}

	// 크로샷목록 갱신
	UpdateXroShotList();

	_bDoneTimerProc = FALSE;
	_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	AfxBeginThread(TimerProc, this);

	_bRunning = TRUE;

	PostMessage(WM_CONNECT_TELECOM, 0, 0);

	Log(CMdlLog::LEVEL_EVENT, _T("KSHOTSender Running..."));
}

void CkshotDlg::Uninit()
{
	_bRunning = FALSE;

	_sendManager.UnInit();

	// 처리중인 내용을 중단한다.
	if (_hExitEvent)
	{
		::SetEvent(_hExitEvent);
		while( !_bDoneTimerProc )
			::Sleep(10);

		::CloseHandle(_hExitEvent);
	}	
}

void CkshotDlg::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
}


BOOL CkshotDlg::DestroyWindow()
{
	if( _bRunning )
		Uninit();

	return CDialogEx::DestroyWindow();
}

HANDLE CkshotDlg::GetExitEvent()
{
	return _hExitEvent;
}

void CkshotDlg::SetDoneThread()
{
	_bDoneTimerProc = TRUE;
}

UINT CkshotDlg::TimerProc(LPVOID lpParameter)
{
	CkshotDlg *dlg = (CkshotDlg*)lpParameter;

	CTime prevIntelli = CTime::GetCurrentTime();
	CTime prevXroshot = CTime::GetCurrentTime();

	while( ::WaitForSingleObject(dlg->GetExitEvent(), 1000) != WAIT_OBJECT_0 )
	{
		CTime curTime = CTime::GetCurrentTime();

		CTimeSpan TimeDiffIntelli, TimeDiffXroshot;

		TimeDiffIntelli = curTime  - prevIntelli;
		TimeDiffXroshot = curTime  - prevXroshot;

		// 7초(7000/1000) 이상이 경과 되었으면, 링크체크 시도
		if(TimeDiffIntelli.GetTotalSeconds() > (INTELLI_TIMER_INTERVAL/1000) ) 
		{
			g_sendManager->_commManager.Ping(INTELLI_TIMER_ID);	
			
			prevIntelli = CTime::GetCurrentTime();
		}

		// 50초(50000/1000) 이상이 경과 되었으면, 링크체크 시도
		if(TimeDiffXroshot.GetTotalSeconds() > (XROSHOT_TIMER_INTERVAL/1000) ) 
		{
			g_sendManager->_commManager.Ping(XROSHOT_TIMER_ID);

			prevXroshot = CTime::GetCurrentTime();
		}

		Sleep(1000);
	}

	dlg->SetDoneThread();

	return 1;
}


void CkshotDlg::OnBnClickedKshotStart()
{
	SetTimer(KSHOT_START_TIMER_ID, 1, NULL);
}


void CkshotDlg::OnClose()
{
	int retval = 0;
	if ((retval = AfxMessageBox(_T("발송 중일때는 데이타가 유실될 수 있으며,\n결과값의 오류가 발생할 수 있습니다!!\n\n그래도 서버를 중지할까요?"), MB_YESNO)) == IDYES) {
		CDialogEx::OnClose();
	}
}


void CkshotDlg::OnBnClickedSetKshotNumber()
{
	CEdit* edt = (CEdit*)GetDlgItem(IDC_KSHOT_NUMBER);

	CString szKshotNumber;
	edt->GetWindowText(szKshotNumber);
	edt->EnableWindow(FALSE);

	((CButton*)GetDlgItem(IDC_SET_KSHOT_NUMBER))->EnableWindow(FALSE);

	g_kshot_send_number = _ttoi(szKshotNumber.GetBuffer());
}

void CkshotDlg::SetFreezingValue()
{
	((CEdit*)GetDlgItem(IDC_KSHOT_NUMBER))->EnableWindow(FALSE);

	((CEdit*)GetDlgItem(IDC_SET_KSHOT_NUMBER))->EnableWindow(FALSE);

	((CButton*)GetDlgItem(IDC_KSHOT_START))->EnableWindow(FALSE);
}

void CkshotDlg::ShowElapseTime()
{
	_waitTime--;

	CString szTime;
	szTime.Format(_T("%d"), _waitTime);

	((CStatic*)GetDlgItem(IDC_TIME))->SetWindowText(szTime);
	((CStatic*)GetDlgItem(IDC_TIME))->UpdateWindow();
}

void CkshotDlg::StopTimerNInit()
{
	// 타이머 중지
	KillTimer(KSHOT_AUTO_START_WAIT_TIMER_ID);
	KillTimer(KSHOT_START_TIMER_ID);
	KillTimer(KSHOT_AUTO_START_TIMER_ID);

	// 설정값 고정
	SetFreezingValue();

	((CStatic*)GetDlgItem(IDC_TIME_MSG))->ShowWindow(SW_HIDE);
	((CStatic*)GetDlgItem(IDC_TIME))->ShowWindow(SW_HIDE);

	// 초기화 시작
	Init();
}

void CkshotDlg::AddClient(CString uid)
{
	CTime    CurTime = CTime::GetCurrentTime();
	CString  szTime = CurTime.Format("%Y-%m-%d %H:%M:%S");

	BOOL notFind = TRUE;

	vector<KSHOTCLIENT>::iterator it = _vtKshotClients.begin();
	for (; it != _vtKshotClients.end(); it++)
	{
		KSHOTCLIENT& client = *it;
		if (client.uid == uid) {
			client.instance_count += 1;
			client.conn_close_time = szTime;
			notFind = FALSE;
			break;
		}
	}

	if (notFind) {
		KSHOTCLIENT client;
		client.uid = uid;
		client.instance_count = 1;
		client.conn_close_time = szTime;
		client.list_request_date = "";
		client.list_request_count = "";

		_vtKshotClients.push_back(client);
	}

	vector<KSHOTCLIENT> *vtClient = new vector<KSHOTCLIENT>();

	vtClient->insert(vtClient->begin(), _vtKshotClients.begin(), _vtKshotClients.end());

	PostThreadMessage(g_mainTid, WM_UPDATE_CLIENT_LIST, (WPARAM)vtClient, 0);
}

void CkshotDlg::DelClient(CString uid)
{
	CTime    CurTime = CTime::GetCurrentTime();
	CString  szTime = CurTime.Format("%Y-%m-%d %H:%M:%S");

	vector<KSHOTCLIENT>::iterator it = _vtKshotClients.begin();
	for (; it != _vtKshotClients.end(); it++)
	{
		KSHOTCLIENT& client = *it;
		if (client.uid == uid) 
		{
			//_vtKshotClients.erase(it);

			client.instance_count -= 1;
			client.conn_close_time = szTime;
			break;
		}
	}

	vector<KSHOTCLIENT> *vtClient = new vector<KSHOTCLIENT>();

	vtClient->insert(vtClient->begin(), _vtKshotClients.begin(), _vtKshotClients.end());

	PostThreadMessage(g_mainTid, WM_UPDATE_CLIENT_LIST, (WPARAM)vtClient, 0);
}

void CkshotDlg::UpdateClientList(CString uid, CString lastRequestDate, int lastRequestCount)
{
	CTime    CurTime = CTime::GetCurrentTime();
	CString  szTime = CurTime.Format("%Y-%m-%d %H:%M:%S");

	BOOL notFind = TRUE;

	vector<KSHOTCLIENT>::iterator it = _vtKshotClients.begin();
	for (; it != _vtKshotClients.end(); it++)
	{
		KSHOTCLIENT& client = *it;
		if (client.uid == uid) {
			//client.list_request_date	= lastRequestDate;
			client.list_request_date = szTime;
			client.list_request_count.Format(_T("%d"), lastRequestCount);
			notFind = FALSE;
			break;
		}
	}
	
	if (notFind) 
		ASSERT(0);
	
	vector<KSHOTCLIENT> *vtClient = new vector<KSHOTCLIENT>();

	vtClient->insert(vtClient->begin(), _vtKshotClients.begin(), _vtKshotClients.end());

	PostThreadMessage(g_mainTid, WM_UPDATE_CLIENT_LIST, (WPARAM)vtClient, 0);
}

void CkshotDlg::UpdateClientListControl(vector<KSHOTCLIENT> *pvtClient)
{
	_ctrl_client_list.DeleteAllItems();

	//int count = _vtKshotClients.size();
	//vector<KSHOTCLIENT>::iterator it = pvtClient->begin();
	//for (int i = 0; it < pvtClient->end(); it++, i++)
	//KSHOTCLIENT client = *it;

	int count = pvtClient->size();
	for (int i = 0; i < count; i++)
	{
		//KSHOTCLIENT client = _vtKshotClients[i];

		KSHOTCLIENT client = pvtClient->at(i);

		_ctrl_client_list.InsertItem(i, client.uid);

		CString strCount;
		strCount.Format(_T("%d"), client.instance_count);
		
		_ctrl_client_list.SetItem(i, 1, LVIF_TEXT, strCount, 0, 0, 0, NULL);

		_ctrl_client_list.SetItem(i, 2, LVIF_TEXT, client.conn_close_time, 0, 0, 0, NULL);
		_ctrl_client_list.SetItem(i, 3, LVIF_TEXT, client.list_request_date, 0, 0, 0, NULL);
		_ctrl_client_list.SetItem(i, 4, LVIF_TEXT, client.list_request_count, 0, 0, 0, NULL);
	}

	delete pvtClient;

	_ctrl_client_list.UpdateWindow();
}

void CkshotDlg::SetDoingSendCount(int count)
{
	PostThreadMessage(g_mainTid, WM_UPDATE_DOING_COUNT, (WPARAM)count, 0);
}

void CkshotDlg::UpdateDoingSendCount(int count)
{
	CString szCount;
	szCount.Format(_T("%d"), count);
	_ctrl_doing_send_count.SetWindowText(szCount);
}

void CkshotDlg::OnBnClickedEnableIntelligent()
{
	CButton* enable_box = (CButton*)GetDlgItem(IDC_ENABLE_INTELLIGENT);

	if( enable_box->GetCheck() == BST_CHECKED )
		g_enable_intelli = TRUE;
	else
		g_enable_intelli = FALSE;
}


void CkshotDlg::OnBnClickedSetVersion()
{
	CSetAgentVersionDlg dlg;

	dlg.SetCurVersion(_msb, _lsb);
	if (dlg.DoModal() == IDOK) {
		_agentVersion.Format(_T("%d.%d"), dlg._cur_msb, dlg._cur_lsb);
		SetVersion();
		UpdateData(FALSE);
	}
}


void CkshotDlg::OnBnClickedAddAccount()
{
	CString id, pwd, certFile, serverUrl;

	CButton* btnId = (CButton*)GetDlgItem(IDC_XST_NAME);
	btnId->GetWindowText(id);
	CButton* btnPwd = (CButton*)GetDlgItem(IDC_XST_PWD);
	btnPwd->GetWindowText(pwd);
	CButton* btnFile = (CButton*)GetDlgItem(IDC_XST_CERTFILE);
	btnFile->GetWindowText(certFile);
	CButton* btnUrl = (CButton*)GetDlgItem(IDC_XST_SERVER);
	btnUrl->GetWindowText(serverUrl);

	// 빈칸 정보 체크
	if (id.IsEmpty() ||
		pwd.IsEmpty() ||
		certFile.IsEmpty() ||
		serverUrl.IsEmpty()) {
		AfxMessageBox(_T("빈 항목이 있습니다."));
		return;
	}

	// 실행파일 경로(실행파일까지 들어간 Full Path임.)
	char programpath[_MAX_PATH];
	GetModuleFileName(NULL, programpath, _MAX_PATH);

	// 현재 폴더 경로
	GetCurrentDirectory(_MAX_PATH, programpath);

	CString realCertPath;
	realCertPath.Format(_T("%s\\certkey\\%s"), programpath, certFile);

	// 파일존재 여부 체크
	CFileFind pFind;
	BOOL bret = pFind.FindFile(realCertPath);
	if (!bret) {
		AfxMessageBox(_T("CertFile이 존재하지 않습니다."));
		return;
	}

	// 중복등록 체크
	
	int size = _ctrlXroShotList.GetItemCount(); 
	int i = 0;

	while (i < size)
	{
		CString account = _ctrlXroShotList.GetItemText(i, 0);

		if (account == id) {
			AfxMessageBox(_T("이미 등록돼 있는 계정입니다."));
			return;
		}

		i++;
	}
	
	if (IDNO == AfxMessageBox(_T("[계정추가]\n계정-비번-CertFile-ServerUrl\n모든 항목이 옳바르게 설정되었나요? 계속 진행하시겠습니까?"), MB_YESNO))
		return;

	CommKT *commkt = (CommKT*)_sendManager._commManager._messenger;
	CommKTxroShot* xroShot = commkt->AddXroShot(id, pwd, certFile, serverUrl);

	xroShot->Init();

	commkt->ActivateXroShot(id);

	commkt->ConnectToXroShot();

	//btnId->SetWindowText("");
	//btnPwd->SetWindowText("");
	//btnFile->SetWindowText("");
	//btnUrl->SetWindowText("");
	
	UpdateXroShotList();
}


void CkshotDlg::OnBnClickedDelAccount()
{
	CString id;

	POSITION pos = _ctrlXroShotList.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		TRACE(_T("No items were selected!\n"));
		return;
	}
	else
	{
		//while (pos)
		{
			int nItem = _ctrlXroShotList.GetNextSelectedItem(pos);

			id = _ctrlXroShotList.GetItemText(nItem, 0);

			CString activate = _ctrlXroShotList.GetItemText(nItem, 1);

			activate.Trim();
			if (activate == _T("활성")) {
				AfxMessageBox(_T("현재 사용중인 \"활성\" 상태의 계정은 삭제할 수 없습니다."));
				return;
			}
			
			//TRACE(_T("Item %d was selected!\n"), nItem);
			// you could do your own processing on nItem here
		}
	}

	CommKT *commkt = (CommKT*)_sendManager._commManager._messenger;
	commkt->DelXroShot(id);

	UpdateXroShotList();
}


void CkshotDlg::OnBnClickedAccountActivate()
{
	CString id;

	POSITION pos = _ctrlXroShotList.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		TRACE(_T("No items were selected!\n"));
		return;
	}
	else
	{
		//while (pos)
		{
			int nItem = _ctrlXroShotList.GetNextSelectedItem(pos);

			id = _ctrlXroShotList.GetItemText(nItem, 0);

			CString activate = _ctrlXroShotList.GetItemText(nItem, 1);
			activate.Trim();

			// 이미 활성화된 상태는 의미 없음.
			if (activate == _T("활성")) {
				return;
			}
		}
	}

	CommKT *commkt = (CommKT*)_sendManager._commManager._messenger;
	commkt->ActivateXroShot(id);

	UpdateXroShotList();
}

void CkshotDlg::InitXroShotListControl()
{
	_ctrlXroShotList.DeleteAllItems();
	_ctrlXroShotList.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	_ctrlXroShotList.InsertColumn(0, _T("크로샷계정"), LVCFMT_LEFT, 140, -1);
	_ctrlXroShotList.InsertColumn(1, _T("상태"), LVCFMT_CENTER, 100, -1);
	_ctrlXroShotList.InsertColumn(2, _T("상태변경 시간"), LVCFMT_CENTER, 140, -1);
	_ctrlXroShotList.InsertColumn(3, _T(""), LVCFMT_CENTER, 140, -1);
}

void CkshotDlg::UpdateXroShotList() 
{
	_ctrlXroShotList.DeleteAllItems();

	map<CString, CommKTxroShot*> mapList;

	CommKT *commkt = (CommKT*)_sendManager._commManager._messenger;
	commkt->GetXroShotList(mapList);

	CTime   CurTime = CTime::GetCurrentTime();
	CString szTime = CurTime.Format("%Y-%m-%d %H:%M:%S");

	CString strRecentTime = "";

	map<CString, CommKTxroShot*>::iterator it = mapList.begin();
	for (int i = 0; it != mapList.end(); it++, i++)
	{
		CString account = it->first;
		CommKTxroShot *xroshot = it->second;

		_ctrlXroShotList.InsertItem(i, account);

		CString strActivate;

		if (xroshot->_activate == TRUE)
			strActivate = _T("활성");
		else
			strActivate = _T("비활성");

		_ctrlXroShotList.SetItem(i, 1, LVIF_TEXT, strActivate, 0, 0, 0, NULL);

		_ctrlXroShotList.SetItem(i, 2, LVIF_TEXT, szTime, 0, 0, 0, NULL);
		_ctrlXroShotList.SetItem(i, 3, LVIF_TEXT, strRecentTime, 0, 0, 0, NULL);
	}

	_ctrlXroShotList.UpdateWindow();
}

void CkshotDlg::OnBnClickedApplyRejectSpam()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	UpdateData(TRUE);

	g_bApplyRejectList = _bApplyRejectList;
}


void CkshotDlg::OnBnClickedResultDone()
{
	_sendManager.SetResultDoneEvent();
}
