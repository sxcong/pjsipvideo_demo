#pragma once


// CVideoWnd 对话框
#include <string>
using namespace std;

#include "cdecodemgr.h"
#include "myapi.h"
#include "pthread.h"

class CVideoWnd : public CDialog, public CDecodeCB
{
	DECLARE_DYNAMIC(CVideoWnd)

public:
	CVideoWnd(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CVideoWnd();
	void OnDrawImage(LPBITMAPINFO pInfo, LPBYTE pData, int nSize);

	void SetDataCB();
	void StopRTPRecv();

public:

	void rtpDataCB(uint8_t* buff, int len);

	void initVideo();
	int startVideo(string& url);
    void stopVideo();
	int m_nID;
	int m_nRtspID;
	string m_Url;

	 virtual void videoCB(int width, int height, uint8_t* buff, int len);
	 virtual void rtspDataCB(uint8_t* buff, int len);

	 CFfmpegDecode* m_pDecode;

	 StreamBufQueue_t m_VBufQue;

	 static void*  EventLoop(void* lpParameter);
	 void ProcessEvent();
	 bool m_bEventLoopStarted;
	 pthread_t    tid;

	 //BITMAPINFO* m_pInfo;
	 //char* m_pData;

	 bool m_bIsRunning;

// 对话框数据
	enum { IDD = IDD_DLG_VIDEOWND };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL OnInitDialog();
};
