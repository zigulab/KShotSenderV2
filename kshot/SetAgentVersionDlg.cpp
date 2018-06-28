// SetAgentVersionDlg.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "kshot.h"
#include "SetAgentVersionDlg.h"
#include "afxdialogex.h"


// CSetAgentVersionDlg ��ȭ �����Դϴ�.

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


// CSetAgentVersionDlg �޽��� ó�����Դϴ�.


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
			AfxMessageBox(_T("���� �������� �����������θ� ������ �� �ֽ��ϴ�."));
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
				  // ����: OCX �Ӽ� �������� FALSE�� ��ȯ�ؾ� �մϴ�.
}
