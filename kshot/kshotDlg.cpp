
// kshotDlg.cpp : ���� ����
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

// ���� ���α׷� ������ ���Ǵ� CAboutDlg ��ȭ �����Դϴ�.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

// �����Դϴ�.
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


// CkshotDlg ��ȭ ����




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


// CkshotDlg �޽��� ó����

BOOL CkshotDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �ý��� �޴��� "����..." �޴� �׸��� �߰��մϴ�.

	// IDM_ABOUTBOX�� �ý��� ��� ������ �־�� �մϴ�.
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

	// �� ��ȭ ������ �������� �����մϴ�. ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	if (prevInit())
	{
		// kshot_numberǥ��
		CEdit* edt = (CEdit*)GetDlgItem(IDC_KSHOT_NUMBER);

		CString szKshotNumber;
		szKshotNumber.Format(_T("%d"), g_kshot_send_number);
		edt->SetWindowText(szKshotNumber);

		// enable_intelligentǥ��
		// ���� config.ini �� ���� ǥ���ϸ� ���������� g_enable_intelli ���� ���� ó���ȴ�.
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

		// 10���� �ڵ� ����	// #define  KSHOT_AUTO_START_WAIT			10000			// 10 SEC
		SetTimer(KSHOT_AUTO_START_TIMER_ID, KSHOT_AUTO_START_WAIT, NULL);

		return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
	}
	else {
		AfxMessageBox(_T("�ý��� prevInit()�۾��� ������ �ֽ��ϴ�. DB����� �ý���ȯ���� Ȯ���غ��� �ٶ��ϴ�."));
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

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�. ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CkshotDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ�Դϴ�.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
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
		Log(CMdlLog::LEVEL_ERROR, _T("��Ż翡 ������ �� �����ϴ�. �ý��� ������ �ּ���"));
	}
	return 1;
}

LRESULT CkshotDlg::OnReconnect(WPARAM wParam, LPARAM lParam)
{
	if( !_sendManager.Reconnect((TELECOM_SERVER_NAME)wParam)) {
		Log(CMdlLog::LEVEL_ERROR, _T("��Ż翡 �� ������ �� �����ϴ�. �ý��� ������ �ּ���"));
	}
	return 1;
}

LRESULT CkshotDlg::OnAliveMessage(WPARAM wParam, LPARAM lParam)
{
	// ����ִ��� Ȯ��

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
		//CWnd *wndbtn = FindWindowEx(hwnd, NULL, NULL, TEXT("��ư Ŭ��"));
        ::PostMessage(hwnd, WM_ALIVE_MSG, 0, 0);
	}
    else
        Log(CMdlLog::LEVEL_ERROR, _T("KShotWatcher�� ����Ǿ� ���� �ʾҽ��ϴ�."));
 
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

	// ������ �ڵ� �����
	case 	KSHOT_AUTO_START_TIMER_ID:
		StopTimerNInit();
		break;

	// Ping ó��
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

	//ȭ����� ����
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
	_ctrl_client_list.InsertColumn(1, _T("����Instance��"), LVCFMT_CENTER, 100, -1);
	_ctrl_client_list.InsertColumn(2, _T("�ֱٿ���(����)�ð�"), LVCFMT_CENTER, 140, -1);
	_ctrl_client_list.InsertColumn(3, _T("�ֱٿ�û�ð�"), LVCFMT_CENTER, 140, -1);
	_ctrl_client_list.InsertColumn(4, _T("�ֱٿ�û�Ǽ�"), LVCFMT_CENTER, 100, -1);
}

void CkshotDlg::Init()
{
	if( _sendManager.Init() == FALSE) {
		Log(CMdlLog::LEVEL_ERROR, _T("�ʱ�ȭ�� �������� ���߽��ϴ�. �ý��� ������ �ٽ� ������ �ֽʽÿ�."));
		return;
	}

	// ũ�μ���� ����
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

	// ó������ ������ �ߴ��Ѵ�.
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

		// 7��(7000/1000) �̻��� ��� �Ǿ�����, ��ũüũ �õ�
		if(TimeDiffIntelli.GetTotalSeconds() > (INTELLI_TIMER_INTERVAL/1000) ) 
		{
			g_sendManager->_commManager.Ping(INTELLI_TIMER_ID);	
			
			prevIntelli = CTime::GetCurrentTime();
		}

		// 50��(50000/1000) �̻��� ��� �Ǿ�����, ��ũüũ �õ�
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
	if ((retval = AfxMessageBox(_T("�߼� ���϶��� ����Ÿ�� ���ǵ� �� ������,\n������� ������ �߻��� �� �ֽ��ϴ�!!\n\n�׷��� ������ �����ұ��?"), MB_YESNO)) == IDYES) {
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
	// Ÿ�̸� ����
	KillTimer(KSHOT_AUTO_START_WAIT_TIMER_ID);
	KillTimer(KSHOT_START_TIMER_ID);
	KillTimer(KSHOT_AUTO_START_TIMER_ID);

	// ������ ����
	SetFreezingValue();

	((CStatic*)GetDlgItem(IDC_TIME_MSG))->ShowWindow(SW_HIDE);
	((CStatic*)GetDlgItem(IDC_TIME))->ShowWindow(SW_HIDE);

	// �ʱ�ȭ ����
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

	// ��ĭ ���� üũ
	if (id.IsEmpty() ||
		pwd.IsEmpty() ||
		certFile.IsEmpty() ||
		serverUrl.IsEmpty()) {
		AfxMessageBox(_T("�� �׸��� �ֽ��ϴ�."));
		return;
	}

	// �������� ���(�������ϱ��� �� Full Path��.)
	char programpath[_MAX_PATH];
	GetModuleFileName(NULL, programpath, _MAX_PATH);

	// ���� ���� ���
	GetCurrentDirectory(_MAX_PATH, programpath);

	CString realCertPath;
	realCertPath.Format(_T("%s\\certkey\\%s"), programpath, certFile);

	// �������� ���� üũ
	CFileFind pFind;
	BOOL bret = pFind.FindFile(realCertPath);
	if (!bret) {
		AfxMessageBox(_T("CertFile�� �������� �ʽ��ϴ�."));
		return;
	}

	// �ߺ���� üũ
	
	int size = _ctrlXroShotList.GetItemCount(); 
	int i = 0;

	while (i < size)
	{
		CString account = _ctrlXroShotList.GetItemText(i, 0);

		if (account == id) {
			AfxMessageBox(_T("�̹� ��ϵ� �ִ� �����Դϴ�."));
			return;
		}

		i++;
	}
	
	if (IDNO == AfxMessageBox(_T("[�����߰�]\n����-���-CertFile-ServerUrl\n��� �׸��� �ǹٸ��� �����Ǿ�����? ��� �����Ͻðڽ��ϱ�?"), MB_YESNO))
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
			if (activate == _T("Ȱ��")) {
				AfxMessageBox(_T("���� ������� \"Ȱ��\" ������ ������ ������ �� �����ϴ�."));
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

			// �̹� Ȱ��ȭ�� ���´� �ǹ� ����.
			if (activate == _T("Ȱ��")) {
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

	_ctrlXroShotList.InsertColumn(0, _T("ũ�μ�����"), LVCFMT_LEFT, 140, -1);
	_ctrlXroShotList.InsertColumn(1, _T("����"), LVCFMT_CENTER, 100, -1);
	_ctrlXroShotList.InsertColumn(2, _T("���º��� �ð�"), LVCFMT_CENTER, 140, -1);
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
			strActivate = _T("Ȱ��");
		else
			strActivate = _T("��Ȱ��");

		_ctrlXroShotList.SetItem(i, 1, LVIF_TEXT, strActivate, 0, 0, 0, NULL);

		_ctrlXroShotList.SetItem(i, 2, LVIF_TEXT, szTime, 0, 0, 0, NULL);
		_ctrlXroShotList.SetItem(i, 3, LVIF_TEXT, strRecentTime, 0, 0, 0, NULL);
	}

	_ctrlXroShotList.UpdateWindow();
}

void CkshotDlg::OnBnClickedApplyRejectSpam()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.

	UpdateData(TRUE);

	g_bApplyRejectList = _bApplyRejectList;
}


void CkshotDlg::OnBnClickedResultDone()
{
	_sendManager.SetResultDoneEvent();
}
