#pragma once
#include "afxwin.h"


// CSetupDlg 对话框

class CSetupDlg : public CDialog
{
	DECLARE_DYNAMIC(CSetupDlg)

public:
	CSetupDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSetupDlg();

// 对话框数据
	enum { IDD = IDD_SETUP_DLG };

	CString path;
	CString m_szUser;
	CString m_szPW;
	CString m_szServer;
	CString m_szDestUser;
	CString m_szPort;
	CString m_szPort2;
	CString m_szAppType;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CComboBox m_comboType;
};
