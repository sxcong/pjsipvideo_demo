// sipclientDlg.h : 头文件
//

#pragma once


#include "VideoWnd.h"
#include "afxcmn.h"
// CsipclientDlg 对话框
class CsipclientDlg : public CDialog
{
// 构造
public:
	CsipclientDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CsipclientDlg();

	int m_nOp;//0 idle 1 call out 2 call in 3 获取列表
	

// 对话框数据
	enum { IDD = IDD_SIPCLIENT_DIALOG };

	void UpdateListStatus(int accid, bool status);

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


typedef enum
{
    MSG_IDLE = 1,
    MSG_WAIT_ACK = 2,
};


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonLogin();
	afx_msg void OnBnClickedButtonLogout();

	void UpdateStatus(int nEvent, int callid, char* local, char* remote);

	void SendRtp(char* buff, int len);

	CString path;
	CString m_szUser;
	CString m_szPW;
	CString m_szServer;
	CString m_szDestUser;
	CString m_szPort;
	
	CVideoWnd* m_pLocalWnd;
	CVideoWnd* m_pRemoteWnd;


	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonCall();
	afx_msg void OnBnClickedButtonHungup();
	afx_msg void OnBnClickedButtonSend();
	afx_msg void OnBnClickedButtonAnswer();
	afx_msg void OnBnClickedButtonHungup2();
	afx_msg void OnBnClickedButtonRtspStart();
	afx_msg void OnBnClickedButtonRtspStop();
	afx_msg void OnBnClickedButtonSendRtp();
	afx_msg void OnBnClickedButtonSetup();
	afx_msg void OnBnClickedButtonInit();
	afx_msg void OnBnClickedRadio1();
	afx_msg void OnBnClickedRadio2();
	afx_msg void OnBnClickedButtonGetlist();
	CTreeCtrl m_Tree;
	HTREEITEM m_hRoot;
	afx_msg void OnNMDblclkTree1(NMHDR *pNMHDR, LRESULT *pResult);


	void GetSchoolList();
	void OnRecvMsg(char* remote, char* msg, int len);
	int  m_MsgState;

	CString m_szScoolName;
	int acc_id;

	void StartRtsp(int index);
	bool m_bSendRtp;
};
