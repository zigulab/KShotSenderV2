#pragma once


// CSetAgentVersionDlg 대화 상자입니다.

class CSetAgentVersionDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSetAgentVersionDlg)

public:
	CSetAgentVersionDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CSetAgentVersionDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	int _cur_msb;
	int _cur_lsb;
	CString _newVersion;
	afx_msg void OnBnClickedOk();

	void SetCurVersion(int msb, int lsb);
	virtual BOOL OnInitDialog();
};
