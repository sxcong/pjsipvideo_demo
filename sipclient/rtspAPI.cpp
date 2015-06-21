#include "StdAfx.h"
#include "rtspAPI.h"

//#include "RTSPSession.h"
//#include "API.h"

rtspAPI::rtspAPI(void)
{
	Init();
}

rtspAPI::~rtspAPI(void)
{
	UnInit();
}

rtspAPI& rtspAPI::Instance()
{
	static rtspAPI _agent;
	return _agent;
}

int rtspAPI::Init()
{
	for (int i = 0; i < MAX_RTSP; i++)
	{
		m_rtsp[i] = NULL;
	}
	return 0;
}

void rtspAPI::UnInit()
{
	for (int i = 0; i < MAX_RTSP; i++)
	{
		if (m_rtsp[i] != NULL)
		{
			m_rtsp[i]->stopRTSPClient();
			delete m_rtsp[i];
			m_rtsp[i] = NULL;
		}
	}
}

int rtspAPI::openStream(char const* progName, char const* rtspURL, int debugLevel)
{
	int nIndex = -1;
	for (int i = 0; i < MAX_RTSP; i++)
	{
		if (m_rtsp[i] == NULL)
		{
			nIndex = i;
			break;
		}
	}
	if (nIndex < 0)
		return -1; //busy

	CRTSPSession* pRtsp = new CRTSPSession;
	if (pRtsp->startRTSPClient(progName, rtspURL, debugLevel))
	{
		delete pRtsp;
		pRtsp = NULL;
		return -2;
	}
	pRtsp->m_nID = nIndex;
	m_rtsp[nIndex] = pRtsp;
	return nIndex;
}

int rtspAPI::getStreamStatus(int nStream)
{
	CRTSPSession* pRtsp = m_rtsp[nStream];
	if (pRtsp)
	{
		return pRtsp->CheckStatus();
	}
	return -1;
}

int rtspAPI::closeStream(int nStream)
{
	CRTSPSession* pRtsp = m_rtsp[nStream];
	if (pRtsp)
	{
		pRtsp->stopRTSPClient();
		while(1)
		{
			if (pRtsp->CheckStatus() == 2)
			{
				delete pRtsp;
				pRtsp = NULL;
				m_rtsp[nStream] = NULL;
				break;
			}
            //sleep(1);
		}
	}
	return 0;
}


//****************************************
//API
int IPNC_OpenStream(char const* progName, char const* rtspURL, int debugLevel)
{
	return rtspAPI::Instance().openStream(progName, rtspURL, debugLevel);
}

int IPNC_GetStreamStatus(int nStream)
{
	return rtspAPI::Instance().getStreamStatus(nStream);
}

int IPNC_CloseStream(int nStream)
{
	return rtspAPI::Instance().closeStream(nStream);
}

