// VideoWnd.cpp : 实现文件
//

#include "stdafx.h"
#include "sipclient.h"
#include "VideoWnd.h"
#include "rtspAPI.h"
#include "src/pjsua_app.h"


// CVideoWnd 对话框

BITMAPINFOHEADER m_bih;
BITMAPINFO* m_pInfo = NULL;
uint8_t* m_pData = NULL;

IMPLEMENT_DYNAMIC(CVideoWnd, CDialog)

CVideoWnd::CVideoWnd(CWnd* pParent /*=NULL*/)
	: CDialog(CVideoWnd::IDD, pParent)
{
	m_pDecode = NULL;
	m_bIsRunning = false;
	m_pInfo = NULL;
	m_pData = NULL;
	m_nRtspID = -1;
	CMyAPI::BufQueueInit(&m_VBufQue,VIDEO_STREAM_BUFQUE);
	CMyAPI::SetBufQueueSize(&m_VBufQue,0);
	m_bEventLoopStarted = FALSE;
	
}

CVideoWnd::~CVideoWnd()
{
	if (m_pDecode)
	{
		delete m_pDecode;
		m_pDecode = NULL;
	}

	SAFE_DELETE(m_pInfo);
	SAFE_DELETE(m_pData);

	CMyAPI::BufQueueFree(&m_VBufQue);
}
//FILE* pf =NULL;
//FILE* pf1 =NULL;

void CVideoWnd::initVideo()
{
	m_pDecode = new CFfmpegDecode;
	if (m_pDecode->initFFMPEG() < 0)
	{
		exit(0);
	}
	m_pDecode->openDecoder(0, 0, this);

	
	// pf =fopen("c:\\rtp.264", "wb");
	
	//rtspAPI::Instance().m_rtsp[m_nRtspID]->m_pCB = this;
}

void CVideoWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CVideoWnd, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_CREATE()
END_MESSAGE_MAP()


// CVideoWnd 消息处理程序
int  CVideoWnd::startVideo(string& url)
{
	m_Url = url;
	if (url.substr(0, 7) == "rtsp://")
	{

		if (m_nRtspID >= 0)
		{
			IPNC_CloseStream(m_nRtspID);
		}

		if (m_pDecode)
		{
			delete m_pDecode;
			m_pDecode = NULL;
		}


		m_pDecode = new CFfmpegDecode;
		if (m_pDecode->initFFMPEG() < 0)
		{
			exit(0);
		}
		m_pDecode->openDecoder(0, 0, this);
		
		//pf1 = fopen("c:\\rtsp.264", "wb");

		m_nRtspID = IPNC_OpenStream("zehin", url.c_str(), 0);
		rtspAPI::Instance().m_rtsp[m_nRtspID]->m_pDecode = m_pDecode;
		rtspAPI::Instance().m_rtsp[m_nRtspID]->m_pCB = this;
		return m_nRtspID;
	}
	return -1;
}

void  CVideoWnd::stopVideo()
{
	if (m_nRtspID >= 0)
		IPNC_CloseStream(m_nRtspID);

	//fclose(pf1);
}

void CVideoWnd::videoCB(int width, int height, uint8_t* buff, int len)
{
	m_bih.biSize = sizeof(BITMAPINFOHEADER);
	m_bih.biWidth = width;
	m_bih.biHeight = -height;
	m_bih.biBitCount = 24;
	m_bih.biSizeImage = 0;
	m_bih.biClrImportant = 0;
	m_bih.biClrUsed = 0;
	m_bih.biXPelsPerMeter = 0;
	m_bih.biYPelsPerMeter = 0;
	m_bih.biPlanes = 1;

	int nH = abs(m_bih.biHeight);
	int nW = m_bih.biWidth;

	if(nH <= 0 || nW <= 0)
		return;

	nW = 4*(nW * m_bih.biBitCount + 31)/32;
	
	int m_nVideoSize = m_bih.biSizeImage;
	if(m_nVideoSize  == 0)
	{
		m_nVideoSize = nW * nH;
	}

	if (m_pInfo != NULL)
		delete m_pInfo;

	//if (m_pInfo == NULL)
	{
		m_pInfo = (BITMAPINFO*)new BYTE[sizeof(BITMAPINFOHEADER)+256* sizeof(RGBQUAD)];
	}

	memcpy(m_pInfo, (BITMAPINFO*)&m_bih, sizeof(BITMAPINFO));	

	if (m_pData != NULL)
	{
		delete m_pData;
	}
	//if (m_pData == NULL)
	{
		m_pData = new uint8_t[len];
	}
	memcpy(m_pData, buff, len);

	//OnDrawImage(m_pInfo, m_pData, len);
	Invalidate();
	
}

static void PR_RTP_DataCB(const char *data, int len, int ip, int port, void* pParam)
{
	CVideoWnd* pThis = (CVideoWnd*)pParam;
	pThis->rtpDataCB((uint8_t*)data, len);
}

void CVideoWnd::SetDataCB()
{
	set_data_callback(PR_RTP_DataCB, this);

	//int r = pthread_create (&tid, NULL, EventLoop, this);
	//if (r)
	//{
		//perror ("pthread_create()");
	//	return ;
	//}
	//m_bEventLoopStarted =  true;

}


void DrawDib(LPBITMAPINFO pInfo,LPBYTE pData,HDC hDC,LPRECT pDestRect,LPRECT pSrcRect)
{
//	SetStretchBltMode(hDC,HALFTONE);
//	SetBrushOrgEx(hDC,0,0,NULL);
	LONG	srcX = 0,srcY = 0,srcW = pInfo->bmiHeader.biWidth,srcH = abs(pInfo->bmiHeader.biHeight);
	/*if(pSrcRect)
	{
		srcX = pSrcRect->left;
		srcY = pSrcRect->top;
		srcW = pSrcRect->right-pSrcRect->left;
		srcH = pSrcRect->bottom-pSrcRect->top;
	}*/

	long nWidth=abs(pInfo->bmiHeader.biWidth);
	long nHeight = abs(pInfo->bmiHeader.biHeight);
	long nWndWidth = pDestRect->right-pDestRect->left;
	long nWndHeight = pDestRect->bottom - pDestRect->top;

	SetStretchBltMode(hDC,COLORONCOLOR);
	::StretchDIBits(hDC,
		  2, 2, nWndWidth-4,nWndHeight-4,
		  0,0,srcW,srcH,
		  pData,pInfo,DIB_RGB_COLORS,SRCCOPY);

	return;

}

int send_index = 0;
int recv_index = 0;

void CVideoWnd::rtpDataCB(uint8_t* buff, int len)
{
	//fwrite(buff, 1, len, pf);
	//CMyAPI::BufQueueInput(&m_VBufQue, buff, len);
	//pjcall_thread_register();
	if (m_pDecode)
	{
		int ret = m_pDecode->decode_rtsp_frame(buff, len, false);
		CString str;
		str.Format("decode = %d  seq = %d, len = %d \r\n", ret, recv_index++, len);
		//OutputDebugString(str);
	}
}

void CVideoWnd::rtspDataCB(uint8_t* buff, int len)
{
	if (m_nID == 0)
	{
		CString str;
		str.Format("send_rtp seq = %d, len = %d \r\n", send_index++, len);
		//OutputDebugString(str);

		//fwrite(buff, 1, len, pf1);
		send_rtp((char*)buff, len);
		return;
	}
	
	/*int outlen = 1920*1080*3;
	char* out = new char[outlen];

	pjcall_thread_register();

	int ret = decode((char*)buff, len, out, outlen);
	CString str;
	str.Format("decode = %d \r\n", ret);
	OutputDebugString(str);

	delete []out;
	*/
}

BOOL CVideoWnd::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_pInfo && m_pData)
		return TRUE;

	CRect Rect;
	GetClientRect(Rect);
	CBrush brush(RGB(0,0,0));
	CBrush* pOldBrush = pDC->SelectObject(&brush);
	pDC->FillRect(Rect, &brush);

	//pDC->SetTextColor(RGB(0,255,0)); //设置字体颜色
	//pDC->SetBkMode(TRANSPARENT);
	//pDC->TextOut(0, 0, m_strID, m_strID.GetLength());
	

	pDC->SelectObject(pOldBrush);
	brush.DeleteObject();
	

	return TRUE;
	

	return __super::OnEraseBkgnd(pDC);
}

void CVideoWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 __super::OnPaint()
	if (m_pInfo && m_pData)
	{
		OnDrawImage(m_pInfo,m_pData, 0);
	}
	/*else
	{
		CRect Rect;
		GetClientRect(Rect);
		CBrush brush(RGB(0,0,0));
		CBrush* pOldBrush = dc.SelectObject(&brush);
		dc.FillRect(Rect, &brush);

		dc.SelectObject(pOldBrush);
		brush.DeleteObject();
	}
	*/
	
}


void CVideoWnd::OnDrawImage(LPBITMAPINFO pInfo, LPBYTE pData, int nSize)
{
	if(pInfo && pData)
	{
		CRect rt;
		GetClientRect(rt);
		CDC* pDC = GetDC();
		DrawDib(pInfo,pData,pDC->m_hDC,&rt,NULL);
		ReleaseDC(pDC);
	}
}

void*   CVideoWnd::EventLoop(void* lpParameter)
{
	CVideoWnd* pThis = (CVideoWnd*)lpParameter;
	pThis->ProcessEvent();
	return NULL;
}

void  CVideoWnd::ProcessEvent()
{
	BYTE* pBuffer = new BYTE[MAX_LEN_BUFFER];
	while (m_bEventLoopStarted)
	{
		memset(pBuffer, '\0', MAX_LEN_BUFFER);
		int iSize = CMyAPI::BufQueueOutput(&m_VBufQue, pBuffer,MAX_LEN_BUFFER);
		int iSizeAudio = 0;

		if( iSize<=0 )
		{
			Sleep(40);
			continue;
		}

		pjcall_thread_register();
		if (m_pDecode)
		{
			int ret = m_pDecode->decode_rtsp_frame(pBuffer, iSize, false);
			CString str;
			str.Format("decode = %d  seq = %d, len = %d \r\n", ret, recv_index++, iSize);
			OutputDebugString(str);
		}
	}

	delete []pBuffer;
}

void CVideoWnd::StopRTPRecv()
{
	//m_bEventLoopStarted = false;
	//pthread_join(tid, NULL);
	//fclose(pf);
}
int CVideoWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码

	return 0;
}

BOOL CVideoWnd::OnInitDialog()
{
	__super::OnInitDialog();

	// TODO:  在此添加额外的初始化

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}
