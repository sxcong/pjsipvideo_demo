#pragma once

class CSetting
{
public:
	CSetting(void);
	~CSetting(void);
	static CSetting& Instance();

	void LoadConfig();

	CString m_szIniPath;
	CString m_szUser;
	CString m_szPW;
	CString m_szServer;
	CString m_szDestUser;
	CString m_szPort;
	CString m_szPort2;
	CString m_szAppType;

	CString m_rtspUrl[4];

};
