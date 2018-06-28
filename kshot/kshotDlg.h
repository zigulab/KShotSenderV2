
// kshotDlg.h : ��� ����
//

#pragma once

#include "SendManager.h"
#include "afxcmn.h"
#include "afxwin.h"


typedef struct _KSHOTCLIENT {
	CString	uid;
	int			instance_count;
	CString  conn_close_time;				// ����, ���� �ð�
	CString	list_request_date;
	CString	list_request_count;
}KSHOTCLIENT;



// CkshotDlg ��ȭ ����
class CkshotDlg : public CDialogEx
{
// �����Դϴ�.
public:
	CkshotDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_KSHOT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.

public:
	// �߼� ����
	SendManager	_sendManager;			

	HANDLE			_hExitEvent;
	BOOL				_bDoneTimerProc;

	BOOL				_bReconn_Xroshot;
	BOOL				_bReconn_Intelli;

	BOOL				_bRunning;

	int					_waitTime;

	CString			_agentVersion;

	static char		_msb;
	static char		_lsb;

// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	afx_msg LRESULT OnTimerMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnConnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReconnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAliveMessage(WPARAM wParam, LPARAM lParam);

public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	BOOL	prevInit();
	void	Init();
	void	Uninit();

	virtual void PostNcDestroy();
	virtual BOOL DestroyWindow();

	static UINT TimerProc(LPVOID lpParameter);
	HANDLE GetExitEvent();
	void SetDoneThread();
	void SetFreezingValue();

	void ShowElapseTime();
	void StopTimerNInit();

	void InitClientListControl();

	void AddClient(CString uid);
	void AddClientControl();

	void DelClient(CString uid);
	void DelClientControl();

	void UpdateClientList(CString uid, CString lastRequestDate, int lastRequestCount);
	void UpdateClientListControl(vector<KSHOTCLIENT> *pvtClient);

	afx_msg void OnBnClickedKshotStart();
	afx_msg void OnClose();
	afx_msg void OnBnClickedSetKshotNumber();

	CListCtrl _ctrl_client_list;
	int _doing_send_count;
	CStatic _ctrl_doing_send_count;

	void SetDoingSendCount(int count);
	void UpdateDoingSendCount(int count);
	afx_msg void OnBnClickedEnableIntelligent();
	afx_msg void OnBnClickedSetVersion();

	void SetVersion();
	afx_msg void OnBnClickedAddAccount();
	afx_msg void OnBnClickedDelAccount();
	afx_msg void OnBnClickedAccountActivate();

	void UpdateXroShotList();

	void InitXroShotListControl();

	CListCtrl _ctrlXroShotList;
	BOOL _bApplyRejectList;
	afx_msg void OnBnClickedApplyRejectSpam();
	afx_msg void OnBnClickedResultDone();
};
