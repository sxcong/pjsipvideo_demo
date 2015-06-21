// sipclientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "sipclient.h"
#include "sipclientDlg.h"
#include "src/pjsua_app.h"
#include <Shlwapi.h>
#include "Setting.h"
#include "SetupDlg.h"
#include "evbuffer.h"
//#include "rtspAPI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CsipclientDlg 对话框

int call_id = -1;
//int in_call_id = -1;

#define CLIENT_RTP_PORT 6060
#define PROXY_RTP_PORT  6070

int RTSP_Index = 0;


static void RecvMsgCB(int accid, char* remote, char* msg, int len, void* pParam)
{
	CsipclientDlg* pThis = (CsipclientDlg*)pParam;
	pThis->OnRecvMsg(remote, msg, len);
}

static void RegisterStatusCB(int accid, bool bRet, void* pParam)
{
	CsipclientDlg* pThis = (CsipclientDlg*)pParam;
	pThis->UpdateListStatus(accid, bRet);
}

static void StatusCB(int status, pjsua_call_info *data, void* pParam)
{
	CsipclientDlg* pThis = (CsipclientDlg*)pParam;
	pThis->UpdateStatus(status, data->id, data->local_info.ptr, data->remote_info.ptr);
}

void CsipclientDlg::UpdateStatus(int nEvent, int callid, char* local, char* remote)
{
	CString log;
	switch (nEvent)
	{
	case PJSUA_STATUS_CALLING:
		{
			GetDlgItem(IDC_BUTTON_ANSWER)->EnableWindow(FALSE);
			GetDlgItem(IDC_BUTTON_HUNGUP2)->EnableWindow(FALSE);
			log.Format("PJSIP_INV_STATE_CALLING callid=%d, local = %s ,remote = %s \r\n", callid, local, remote);
		}
		break;
	case PJSUA_STATUS_DISCONNECTED:
		{
			log.Format("PJSUA_STATUS_DISCONNECTED callid=%d, local = %s ,remote = %s \r\n", callid, local, remote);
			m_nOp = 0;
			call_id = -1;

			log = "连接断开!\r\n";
			CString allLog;
			//allLog += "\r\n";
			GetDlgItem(IDC_EDIT_LOG)->GetWindowText(allLog);
			allLog += log;
			GetDlgItem(IDC_EDIT_LOG)->SetWindowText(allLog);
			}
		break;

	case PJSUA_STATUS_CONFIRMED:
		{
			call_id = callid;
			if (m_nOp == 1)
			{
				//呼出，得到对方应答，开始创建RTP SOCKET
				init_local_rtp();
			}
			else if (m_nOp == 2)
			{
				//收到呼入，启动接收RTP
				init_local_rtp();
			}
			log = "连接建立成功!\r\n";
			CString allLog;
			//allLog += "\r\n";
			GetDlgItem(IDC_EDIT_LOG)->GetWindowText(allLog);
			allLog += log;
			GetDlgItem(IDC_EDIT_LOG)->SetWindowText(allLog);
		}
		 break;
	case PJSUA_STATUS_INCOMING_CALL:
		{
			log.Format("PJSUA_STATUS_INCOMING_CALL callid=%d, local = %s ,remote = %s \r\n", callid, local, remote);
			call_id = callid;
			GetDlgItem(IDC_BUTTON_ANSWER)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_HUNGUP2)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_USER3)->SetWindowTextA(remote);

			//CString log;
			log.Format("收到呼叫: callid=%d, local = %s ,remote = %s \r\n", callid, local, remote);
			
			CString allLog;
			//allLog += "\r\n";
			GetDlgItem(IDC_EDIT_LOG)->GetWindowText(allLog);
			allLog += log;
			GetDlgItem(IDC_EDIT_LOG)->SetWindowText(allLog);

			//自动应答,并连接摄像机
			OnBnClickedButtonAnswer();
			//OnBnClickedButtonRtspStart();
		}
		break;
	case PJSUA_STATUS_RECVMSG:
		break;
	}
}

static void LogCB(int level, const char *data, int len, void* pParam)
{
	if (data == NULL)
		return;
	OutputDebugString(data);
	OutputDebugString("\r\n");
}


void CsipclientDlg::UpdateListStatus(int accid, bool status)
{
	CString log;
	if (status)
	{
		acc_id = accid;
		log.Format("%s 登录:成功, id=%d \r\n", m_szUser, accid);
	}
	else
	{
		log.Format("%s 登录:失败 \r\n");
	}

	CString allLog;
	allLog += "\r\n";
	GetDlgItem(IDC_EDIT_LOG)->GetWindowText(allLog);
	allLog += log;
	GetDlgItem(IDC_EDIT_LOG)->SetWindowText(allLog);
}

void CsipclientDlg::SendRtp(char* buff, int len)
{
}

CsipclientDlg::CsipclientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CsipclientDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pLocalWnd = NULL;
	m_pRemoteWnd = NULL;

	m_nOp = 0;
	m_MsgState = 0;
	acc_id = -1;
	m_bSendRtp = false;
}

CsipclientDlg::~CsipclientDlg()
{
	SAFE_DELETE(m_pLocalWnd);
	SAFE_DELETE(m_pRemoteWnd);

	//deinit_video_module();
}

void CsipclientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE1, m_Tree);
}

BEGIN_MESSAGE_MAP(CsipclientDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CsipclientDlg::OnBnClickedButtonLogin)
	ON_BN_CLICKED(IDC_BUTTON_LOGOUT, &CsipclientDlg::OnBnClickedButtonLogout)
	ON_BN_CLICKED(IDOK, &CsipclientDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_CALL, &CsipclientDlg::OnBnClickedButtonCall)
	ON_BN_CLICKED(IDC_BUTTON_HUNGUP, &CsipclientDlg::OnBnClickedButtonHungup)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CsipclientDlg::OnBnClickedButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_ANSWER, &CsipclientDlg::OnBnClickedButtonAnswer)
	ON_BN_CLICKED(IDC_BUTTON_HUNGUP2, &CsipclientDlg::OnBnClickedButtonHungup2)
	ON_BN_CLICKED(IDC_BUTTON_RTSP_START, &CsipclientDlg::OnBnClickedButtonRtspStart)
	ON_BN_CLICKED(IDC_BUTTON_RTSP_STOP, &CsipclientDlg::OnBnClickedButtonRtspStop)
	ON_BN_CLICKED(IDC_BUTTON_SEND_RTP, &CsipclientDlg::OnBnClickedButtonSendRtp)
	ON_BN_CLICKED(IDC_BUTTON_SETUP, &CsipclientDlg::OnBnClickedButtonSetup)
	ON_BN_CLICKED(IDC_BUTTON_INIT, &CsipclientDlg::OnBnClickedButtonInit)
	ON_BN_CLICKED(IDC_RADIO1, &CsipclientDlg::OnBnClickedRadio1)
	ON_BN_CLICKED(IDC_RADIO2, &CsipclientDlg::OnBnClickedRadio2)
	ON_BN_CLICKED(IDC_BUTTON_GETLIST, &CsipclientDlg::OnBnClickedButtonGetlist)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE1, &CsipclientDlg::OnNMDblclkTree1)
END_MESSAGE_MAP()


// CsipclientDlg 消息处理程序



BOOL CsipclientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	set_register_callback(RegisterStatusCB, this);
	set_log_callback(LogCB, this);
	set_callback(StatusCB, this);
	set_msg_callback(RecvMsgCB,this);


	
	m_szUser = CSetting::Instance().m_szUser;
	m_szPW = CSetting::Instance().m_szPW;
	m_szServer = CSetting::Instance().m_szServer;

	//GetPrivateProfileString("setup","destUser", "1001", buff, len, path);
	//m_szDestUser = buff;
	m_szPort = CSetting::Instance().m_szPort;
	//m_szPort2 = CSetting::Instance().m_szPort2;
	//m_szAppType = CSetting::Instance().m_szAppType;



	set_udp_port(atoi(m_szPort));

	GetDlgItem(IDC_EDIT_USER2)->SetWindowTextA(CSetting::Instance().m_szDestUser);

	//init_video_module();

	m_pLocalWnd= new CVideoWnd();
	m_pLocalWnd->Create(IDD_DLG_VIDEOWND, CWnd::FromHandle(m_hWnd));
	m_pLocalWnd->ShowWindow(SW_HIDE);
	m_pLocalWnd->MoveWindow(500, 40, 420, 310);
	m_pLocalWnd->m_nID = 0;

	m_pRemoteWnd= new CVideoWnd();
	m_pRemoteWnd->Create(IDD_DLG_VIDEOWND, CWnd::FromHandle(m_hWnd));
	m_pRemoteWnd->ShowWindow(SW_HIDE);
	m_pRemoteWnd->MoveWindow(25, 40, 420, 310); 
	m_pRemoteWnd->m_nID = 1;

	m_pRemoteWnd->SetDataCB();
	m_pRemoteWnd->initVideo();

	m_hRoot = m_Tree.InsertItem("杭州");


	if (CSetting::Instance().m_szAppType == "0")
	{
		((CButton*)GetDlgItem(IDC_RADIO1))->SetCheck(BST_CHECKED);
		OnBnClickedRadio1();
	}
	else
	{
		((CButton*)GetDlgItem(IDC_RADIO2))->SetCheck(BST_CHECKED);
		OnBnClickedRadio2();
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CsipclientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CsipclientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CsipclientDlg::OnBnClickedButtonLogin()
{
	// TODO: 在此添加控件通知处理程序代码

	m_szUser = CSetting::Instance().m_szUser;
	m_szPW = CSetting::Instance().m_szPW;
	m_szServer = CSetting::Instance().m_szServer;
	m_szPort = CSetting::Instance().m_szPort;
	
	pj_status_t status =sip_register(CStringA(m_szUser).GetBuffer(0), CStringA(m_szPW).GetBuffer(0), CStringA(m_szServer).GetBuffer(0), acc_id);
	if (status != PJ_SUCCESS)
	{
		AfxMessageBox(_T("user reg fail!"));
		return;
	}
}


void CsipclientDlg::OnBnClickedButtonCall()
{
	// TODO: 在此添加控件通知处理程序代码

	m_pRemoteWnd->ShowWindow(SW_SHOW);

	set_local_rtpport(CLIENT_RTP_PORT);

//	init_media(6000, "127.0.0.1", 7000);

#if 0
	//make_call("1001", "1002", "115.28.170.118");
	make_call("1001", "1002", "192.168.3.206");
	return;
#endif
	m_nOp = 1;
	GetDlgItem(IDC_EDIT_USER2)->GetWindowTextA(m_szDestUser);
	
	WritePrivateProfileString("setup", "destUser", m_szDestUser, path);
	CSetting::Instance().m_szDestUser = m_szDestUser;

	sip_call(acc_id, CStringA(m_szDestUser).GetBuffer(0), CStringA(m_szServer).GetBuffer(0), NULL);
}

void CsipclientDlg::OnBnClickedButtonHungup()
{
	// TODO: 在此添加控件通知处理程序代码
	m_nOp = 0;
	sip_hangup(call_id);
}

void CsipclientDlg::OnBnClickedButtonSend()
{
	// TODO: 在此添加控件通知处理程序代码

	string content = "LIST sip:hzjy.zjjy.cnjy@202.101.100.1 KSLP/1.0\r\n";

	GetDlgItem(IDC_EDIT_USER2)->GetWindowTextA(m_szDestUser);

	send_message(call_id, acc_id, m_szDestUser.GetBuffer(256),m_szServer.GetBuffer(256),  content.c_str());
}

void CsipclientDlg::OnBnClickedButtonAnswer()
{
	// TODO: 在此添加控件通知处理程序代码
	//init_media(7000, "127.0.0.1", 6000);

	set_local_rtpport(PROXY_RTP_PORT);
	m_nOp = 2;
	
	sip_answer(call_id, NULL);
	m_bSendRtp = true;
}

void CsipclientDlg::OnBnClickedButtonHungup2()
{
	// TODO: 在此添加控件通知处理程序代码
	m_nOp = 0;
	sip_hangup(call_id);
	m_bSendRtp = false;
	call_id = -1;
	GetDlgItem(IDC_BUTTON_ANSWER)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_HUNGUP2)->EnableWindow(FALSE);
}

void CsipclientDlg::OnBnClickedButtonRtspStart()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_pLocalWnd)
	{
		m_pLocalWnd->ShowWindow(SW_SHOW);

		if (RTSP_Index < 0 || RTSP_Index > 3)
			return;

		CString rtspUrl = CSetting::Instance().m_rtspUrl[RTSP_Index];
		//m_pLocalWnd->startVideo(string("rtsp://admin:12345@192.168.3.191/h264/ch01/main/av_stream"));
		//m_pLocalWnd->startVideo(string("rtsp://192.168.3.206/2.264"));
		m_pLocalWnd->startVideo(string(rtspUrl.GetBuffer(0)));
	}

	if (m_pRemoteWnd)
	{
		m_pRemoteWnd->ShowWindow(SW_HIDE);
	}
	
}

void CsipclientDlg::OnBnClickedButtonRtspStop()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_pLocalWnd)
	{
		m_pLocalWnd->stopVideo();
		m_pLocalWnd->Invalidate();
		m_pLocalWnd->ShowWindow(SW_HIDE);
	}
}

void CsipclientDlg::OnBnClickedButtonSendRtp()
{
	// TODO: 在此添加控件通知处理程序代码
	send_rtp("test\0", 5);
}

void CsipclientDlg::OnBnClickedButtonSetup()
{
	// TODO: 在此添加控件通知处理程序代码
	CSetupDlg dlg;
	dlg.DoModal();
}

void CsipclientDlg::OnBnClickedButtonInit()
{
	// TODO: 在此添加控件通知处理程序代码
	app_init();
	OnBnClickedButtonLogin();
}

void CsipclientDlg::OnBnClickedButtonLogout()
{
	// TODO: 在此添加控件通知处理程序代码
	m_nOp = 0;
	sip_upregister(acc_id);
}


void CsipclientDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	m_nOp = 0;
	OnBnClickedButtonLogout();
	if (m_pRemoteWnd)
	{
		m_pRemoteWnd->StopRTPRecv();
	}

	if (m_pLocalWnd)
	{
		m_pLocalWnd->stopVideo();
	}

	app_destroy();
	OnOK();
}

void CsipclientDlg::OnBnClickedRadio1()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = ((CButton*)GetDlgItem(IDC_RADIO1))->GetCheck();
	ret = ((CButton*)GetDlgItem(IDC_RADIO2))->GetCheck();
	if (m_pLocalWnd)
	{
		m_pLocalWnd->ShowWindow(SW_HIDE);
	}
	if (m_pRemoteWnd)
	{
		m_pRemoteWnd->ShowWindow(SW_SHOW );
	}
	GetDlgItem(IDC_BUTTON_RTSP_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_RTSP_STOP)->EnableWindow(FALSE);
	

	m_Tree.EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_CALL)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_HUNGUP)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_SEND)->EnableWindow(TRUE);
	SetWindowText("SIP 客户端");
}

void CsipclientDlg::OnBnClickedRadio2()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = ((CButton*)GetDlgItem(IDC_RADIO1))->GetCheck();
	ret = ((CButton*)GetDlgItem(IDC_RADIO2))->GetCheck();

	if (m_pLocalWnd)
	{
		m_pLocalWnd->ShowWindow(SW_SHOW);
	}
	if (m_pRemoteWnd)
	{
		m_pRemoteWnd->ShowWindow(SW_HIDE);
	}

	GetDlgItem(IDC_BUTTON_RTSP_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_RTSP_STOP)->EnableWindow(TRUE);

	GetDlgItem(IDC_BUTTON_CALL)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_HUNGUP)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_SEND)->EnableWindow(FALSE);
	m_Tree.EnableWindow(FALSE);
	SetWindowText("SIP 代理");

}

string ltrim(string& s)
{
    const string drop = " ";
    return s.erase(0,s.find_first_not_of(drop));
}

string rtrim(string& s)
{
    const string drop = " ";
    return    s.erase(s.find_last_not_of(drop)+1);
}

void CsipclientDlg::StartRtsp(int index)
{
	if (m_pLocalWnd)
	{
		CString rtspUrl = CSetting::Instance().m_rtspUrl[RTSP_Index];
		//m_pLocalWnd->startVideo(string("rtsp://admin:12345@192.168.3.191/h264/ch01/main/av_stream"));
		//m_pLocalWnd->startVideo(string("rtsp://192.168.3.206/2.264"));
		m_pLocalWnd->startVideo(string(rtspUrl.GetBuffer(0)));
	}
}
 

void  CsipclientDlg::OnRecvMsg(char* remote, char* msg, int len)
{
	string szMsg = msg;
	string temp = szMsg.substr(0,4);
	switch(m_MsgState)
	{
	case 0://收到新消息,需要回复
		{
			if (szMsg.substr(0, 5) == "PLAY0")
			{
				StartRtsp(0);
				return;
			}

			if (szMsg.substr(0, 5) == "PLAY1")
			{
				StartRtsp(1);
				return;
			}

			if (szMsg.substr(0, 5) == "PLAY2")
			{
				StartRtsp(2);
				return;
			}

			if (szMsg.substr(0, 5) == "PLAY3")
			{
				StartRtsp(3);
				return;
			}
			
			if (szMsg.substr(0, 4) == "LIST")
			{
				if (szMsg.find("sip:hzjy.zjjy.cnjy@202.101.100.1") != string::npos)
				{
					//返回学校列表
					string content = "KSLP/1.0 200 OK\r\n \
									  \r\n \
									  sip:yz.hzjy.zjjy.cnjy@202.101.101.1 一中DO ON \r\n ";

					send_message(call_id, acc_id, remote, m_szServer.GetBuffer(256), content.c_str());

					return;
				}

				if (szMsg.find("sip:yz.hzjy.zjjy.cnjy@202.101.101.1") != string::npos)
				{
					//返回一中摄像机列表
					string content = "KSLP/1.0 200 OK\r\n \
									  \r\n \
									  sip:yu00.yz.hzjy.zjjy.cnjy@202.101.102.1一考场EU ON|READY|AREC|VLOSS\r\n \
									  sip:yu01.yz.hzjy.zjjy.cnjy@202.101.102.1二考场EU ON|READY|AREC\r\n \
									  sip:yu02.yz.hzjy.zjjy.cnjy@202.101.102.1三考场EU ON|READY|AREC\r\n \
									  sip:yu03.yz.hzjy.zjjy.cnjy@202.101.102.1四考场EU ON|READY|AREC\r\n ";
					send_message(call_id, acc_id, remote, m_szServer.GetBuffer(256), content.c_str());

					return;
				}
			}
		}
		break;

	case 1://收到学校列表回复消息,不用再回了
		{
			if (szMsg.find("KSLP/1.0 200 OK") != string::npos)
			{
				m_Tree.DeleteAllItems();
				HTREEITEM hRoot = m_Tree.InsertItem("杭州");
				
				evbuffer* pTemp = evbuffer_new();
				evbuffer_add(pTemp, msg, len);
				while(1)
				{
					char* line = evbuffer_readline(pTemp);
					if (line == NULL)
						 break;
					string szLine = line;

					int nPos = szLine.find("sip:");
					if (nPos != string::npos)
					{
						szLine = szLine.substr(nPos);
						m_szScoolName = szLine.c_str();
						m_Tree.InsertItem(m_szScoolName, hRoot);
					}
				}

				evbuffer_free(pTemp);

				m_MsgState = 0;

				m_Tree.Expand(hRoot, TVE_EXPAND);
				return;
			}
		}
		break;

	case 2:
		{
			if (szMsg.find("KSLP/1.0 200 OK") != string::npos)
			{
				m_Tree.DeleteAllItems();
				HTREEITEM hRoot = m_Tree.InsertItem("杭州");
				HTREEITEM hSubRoot = m_Tree.InsertItem(m_szScoolName, hRoot);
				evbuffer* pTemp = evbuffer_new();
				evbuffer_add(pTemp, msg, len);

				int index = 0;
				while(1)
				{
					char* line = evbuffer_readline(pTemp);
					if (line == NULL)
						 break;
					string szLine = line;

					int nPos = szLine.find("sip:");
					if (nPos != string::npos)
					{
						szLine = szLine.substr(nPos);
						m_szScoolName = szLine.c_str();
						HTREEITEM hItem = m_Tree.InsertItem(m_szScoolName, hSubRoot);
						m_Tree.SetItemData(hItem, index++);
					}
				}

				evbuffer_free(pTemp);
				m_MsgState = 0;

				m_Tree.Expand(hRoot, TVE_EXPAND);
				m_Tree.Expand(hSubRoot, TVE_EXPAND);
				return;
			}
		}
		break;
	}
}


void CsipclientDlg::OnBnClickedButtonGetlist()
{
	// TODO: 在此添加控件通知处理程序代码

	m_nOp = 3;
	GetSchoolList();
}


void CsipclientDlg::GetSchoolList()
{
	GetDlgItem(IDC_EDIT_USER2)->GetWindowTextA(m_szDestUser);

	string content = "LIST sip:hzjy.zjjy.cnjy@202.101.100.1 KSLP/1.0\r\n";

	send_message(call_id, acc_id, m_szDestUser.GetBuffer(256),m_szServer.GetBuffer(256), content.c_str());
	
	m_MsgState = 1;
}

void CsipclientDlg::OnNMDblclkTree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	HTREEITEM item = m_Tree.GetSelectedItem();
	if(item == NULL)
	{
		return ;
	}

	HTREEITEM hP = m_Tree.GetParentItem(item);
	if (hP == NULL)//取城市下学校列表
	{
		GetSchoolList();
		return;
	}
	
	HTREEITEM hP1 = m_Tree.GetParentItem(hP);
	if (hP1 == NULL)//取学校的摄像机列表
	{
		CString str = m_Tree.GetItemText(item);
		GetDlgItem(IDC_EDIT_USER2)->GetWindowTextA(m_szDestUser);
		str = "LIST " + str;
		string content = str.GetBuffer(256);
		send_message(call_id, acc_id, m_szDestUser.GetBuffer(256),m_szServer.GetBuffer(256), content.c_str());

		m_MsgState = 2;
		return;
	}
	else
	{
		//CString str = m_Tree.GetItemText(item);

		int index = m_Tree.GetItemData(item);
		CString str ;
		str.Format("PLAY%d", index);
		GetDlgItem(IDC_EDIT_USER2)->GetWindowTextA(m_szDestUser);
		
		string content = str.GetBuffer(256);
		send_message(call_id, acc_id, m_szDestUser.GetBuffer(256),m_szServer.GetBuffer(256), content.c_str());
		m_MsgState = 0;
	}

	//play

	*pResult = 0;
}
