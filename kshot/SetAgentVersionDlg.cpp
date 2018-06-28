// SetAgentVersionDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "SetAgentVersionDlg.h"
#include "afxdialogex.h"


// CSetAgentVersionDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CSetAgentVersionDlg, CDialogEx)

CSetAgentVersionDlg::CSetAgentVersionDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DIALOG1, pParent)
	, _newVersion(_T(""))
{

}

CSetAgentVersionDlg::~CSetAgentVersionDlg()
{
}

void CSetAgentVersionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NEW_VERSION, _newVersion);
}


BEGIN_MESSAGE_MAP(CSetAgentVersionDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSetAgentVersionDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSetAgentVersionDlg 메시지 처리기입니다.


void CSetAgentVersionDlg::OnBnClickedOk()
{
	UpdateData(TRUE);

	int pos = _newVersion.Find(".");
	if (pos != -1) {
		CString szMsb = _newVersion.Left(1);
		CString szLsb = _newVersion.Mid(pos + 1);

		int _msb = _ttoi(szMsb);
		int _lsb = _ttoi(szLsb);

		if (_msb <= _cur_msb && _lsb <= _cur_lsb) {
			AfxMessageBox(_T("현재 버전보다 상위버전으로만 설정할 수 있습니다."));
			return;
		}
		else {
			_cur_msb = _msb;
			_cur_lsb = _lsb;
		}
	}
	else
		return;

	CDialogEx::OnOK();
}

void CSetAgentVersionDlg::SetCurVersion(int msb, int lsb)
{
	_cur_msb = msb;
	_cur_lsb = lsb;

	_newVersion.Format(_T("%d.%d"), _cur_msb, _cur_lsb);
}

BOOL CSetAgentVersionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();


	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}
