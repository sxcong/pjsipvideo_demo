#pragma once

#include <vector>
using namespace std;
#define MAX_RTSP 64

#include "RTSPStream.h"


class rtspAPI
{
public:
	rtspAPI(void);
	~rtspAPI(void);
	static rtspAPI& Instance();

	int Init();
	void UnInit();

	int openStream(char const* progName, char const* rtspURL, int debugLevel);
	int getStreamStatus(int nStream);
	int closeStream(int nStream);
public:
	//vector<fytRTSP*> m_rtspList;
	CRTSPSession* m_rtsp[64];
};

int IPNC_OpenStream(char const* progName, char const* rtspURL, int debugLevel);
int IPNC_GetStreamStatus(int nStream);
int IPNC_CloseStream(int nStream);
