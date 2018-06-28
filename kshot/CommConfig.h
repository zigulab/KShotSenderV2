#pragma once
class CommConfig
{
public:
	CommConfig(void);
	virtual ~CommConfig(void);


///////////////////////////////////////////////////////////////////////////////// MEMBER DATA
	 TCHAR		_currentPath[_MAX_PATH];
	 TCHAR		_iniPath[_MAX_PATH]; 

	 TCHAR		_enable_intelligent[_MAX_PATH];
	 BOOL		_enable_intelli;

	 class CFG_XROSHOT
	 {
	 public:
		 TCHAR		center[_MAX_PATH]; 
		 TCHAR		id[_MAX_PATH]; 
		 TCHAR		password[_MAX_PATH]; 
		 TCHAR		certkey[_MAX_PATH]; 
		 TCHAR		limitlms[_MAX_PATH]; 
	 };
	 CFG_XROSHOT	xroshot;

	 class CFG_INTELLI
	 {
		 public:
		 TCHAR		server[_MAX_PATH]; 
		 TCHAR		port[_MAX_PATH]; 
		 TCHAR		id[_MAX_PATH]; 
		 TCHAR		password[_MAX_PATH]; 
	 };
	 CFG_INTELLI	intelli;

	 class CFG_DB
	 {
		 public:
		 TCHAR		server[_MAX_PATH]; 
		 TCHAR		id[_MAX_PATH]; 
		 TCHAR		password[_MAX_PATH]; 
		 TCHAR		dbname[_MAX_PATH]; 
		 TCHAR		dbport[_MAX_PATH]; 
	 };
	 CFG_DB	db;

	 class CFG_KSHOT
	 {
		 public:
		 TCHAR		port[_MAX_PATH]; 
		 TCHAR		bindServerIP[_MAX_PATH]; 
		 TCHAR		bindServerPort[_MAX_PATH]; 
		 TCHAR		mmsServerIP[_MAX_PATH]; 
		 TCHAR		mmsServerPort[_MAX_PATH]; 
	 };
	 CFG_KSHOT	kshot;

	 class CFG_CONFIG
	 {
		public:
			 TCHAR debug_trace[10];
			 TCHAR close_Idleclient[2];
			 TCHAR expire_time[4];
	 };
	 CFG_CONFIG config;

	 class CFG_WISECANAGENT
	 {
	 public:
		 TCHAR useAgent[10];
		 TCHAR dbName[20];
	 };
	 CFG_WISECANAGENT wisecanAgent;

	 class CFG_XPEDITEAGENT
	 {
	 public:
		 TCHAR useAgent[10];
		 TCHAR dbName[20];
		 TCHAR userId[20];
		 TCHAR pwd[20];
		 TCHAR faxFilePath[200];
	 };
	 CFG_XPEDITEAGENT xpediteAgent;

	 class CFG_HAOMUN
	 {
	 public:
		 TCHAR		useAgent[10];
		 TCHAR		server[_MAX_PATH];
		 TCHAR		id[_MAX_PATH];
		 TCHAR		password[_MAX_PATH];
		 TCHAR		dbname[_MAX_PATH];
		 TCHAR		dbport[_MAX_PATH];
	 };
	 CFG_HAOMUN	haomunAgent;

	 class CFG_LGUPLUSAGENT
	 {
	 public:
		 TCHAR		useAgent[10];
		 TCHAR		dbname[_MAX_PATH];
	 };
	 CFG_LGUPLUSAGENT lguplusAgent;

	 class CFG_KSHOTAGENT
	 {
	 public:
		 TCHAR version[10];

	 };
	 CFG_KSHOTAGENT kshotAgent;

///////////////////////////////////////////////////////////////////////////////// METHOD
	void		Init();

	void		Exit();
};

