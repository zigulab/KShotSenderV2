#pragma once


// CSetAgentVersionDlg ��ȭ �����Դϴ�.

class CSetAgentVersionDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSetAgentVersionDlg)

public:
	CSetAgentVersionDlg(CWnd* pParent = NULL);   // ǥ�� �������Դϴ�.
	virtual ~CSetAgentVersionDlg();

// ��ȭ ���� �������Դϴ�.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

	DECLARE_MESSAGE_MAP()
public:
	int _cur_msb;
	int _cur_lsb;
	CString _newVersion;
	afx_msg void OnBnClickedOk();

	void SetCurVersion(int msb, int lsb);
	virtual BOOL OnInitDialog();
};
