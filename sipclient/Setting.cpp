#include "StdAfx.h"
#include <Shlwapi.h>
#include "Setting.h"

CSetting::CSetting(void)
{
	TCHAR szPath[MAX_PATH];  
	if(GetModuleFileName(NULL, szPath, MAX_PATH))  
	{  
		PathRemoveFileSpec(szPath);  
	} 

	m_szIniPath = szPath;
	m_szIniPath += "\\config.ini";

	LoadConfig();
}

CSetting::~CSetting(void)
{
}

CSetting& CSetting::Instance()
{
	static CSetting agent;
	return agent;
}

void CSetting::LoadConfig()
{
	char buff[256];
	int len = 256;

	GetPrivateProfileString("setup","localUser", "1002", buff, len, m_szIniPath);
	m_szUser = buff;
	
	GetPrivateProfileString("setup","pw", "123456", buff, len, m_szIniPath);
	m_szPW = buff;

	GetPrivateProfileString("setup","server",   "115.28.170.118", buff, len, m_szIniPath);
	m_szServer = buff;

	GetPrivateProfileString("setup","destUser", "1001", buff, len, m_szIniPath);
	m_szDestUser = buff;

	GetPrivateProfileString("setup","localPort", "5060", buff, len, m_szIniPath);
	m_szPort = buff;

	GetPrivateProfileString("setup","localVidioPort", "10000", buff, len, m_szIniPath);
	m_szPort2 = buff;

	GetPrivateProfileString("setup","appType", "0", buff, len, m_szIniPath);
	m_szAppType = buff;

	GetPrivateProfileString("rtsp","url1", "rtsp://admin:12345@192.168.3.191/h264/ch01/main/av_stream", buff, len, m_szIniPath);
	m_rtspUrl[0] = buff;

	GetPrivateProfileString("rtsp","url2", "rtsp://admin:12345@192.168.3.191/h264/ch01/main/av_stream", buff, len, m_szIniPath);
	m_rtspUrl[1] = buff;

	GetPrivateProfileString("rtsp","url3", "rtsp://admin:12345@192.168.3.191/h264/ch01/main/av_stream", buff, len, m_szIniPath);
	m_rtspUrl[2] = buff;

	GetPrivateProfileString("rtsp","url4", "rtsp://admin:12345@192.168.3.191/h264/ch01/main/av_stream", buff, len, m_szIniPath);
	m_rtspUrl[3] = buff;
}