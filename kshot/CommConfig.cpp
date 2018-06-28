#include "StdAfx.h"
#include "CommConfig.h"


// KShot
#define		CFG__FILENAME				_T("config.ini")

#define		CFG_CONNECT_CATEGORY		_T("CONNECT")
#define		CFG_ENABLE_INTELLIGENT		_T("ENABLE_INTELLIGENT")

#define		CFG_XROSTHOT_CATEGORY		_T("XROSHOT")
#define		CFG_XROSTHOT_CENTER			_T("CENTER")
#define		CFG_XROSTHOT_ID				_T("ID")
#define		CFG_XROSTHOT_PASSWORD		_T("pwd")
#define		CFG_XROSTHOT_CERTKEY		_T("CERTKEY")
#define		CFG_XROSTHOT_LIMITLMS		_T("LIMITLMS")

// Intelligent
#define		CFG_INTELLI_CATEGORY		_T("Intelligent")
#define		CFG_INTELLI_SERVER			_T("server")
#define		CFG_INTELLI_PORT			_T("port")
#define		CFG_INTELLI_ID				_T("id")
#define		CFG_INTELLI_PASSWORD		_T("pwd")

// DB
#define		CFG_DB_CATEGORY				_T("DB")
#define		CFG_DB_SERVER				_T("server")
#define		CFG_DB_ID					_T("id")
#define		CFG_DB_PASSWORD				_T("pwd")
#define		CFG_DB_DBNAME				_T("dbname")
#define		CFG_DB_DBPORT				_T("dbport")

// SERVER
#define		CFG_KSHOT_CATEGORY			_T("KSHOT")
#define		CFG_KSHOT_PORT				_T("port")
#define		CFG_KSHOT_BIND_SERVERIP		_T("bindServerIP")
#define		CFG_KSHOT_BIND_SERVERPORT	_T("bindServerPort")
#define		CFG_KSHOT_MMS_SERVERIP		_T("mmsWebServerIP")
#define		CFG_KSHOT_MMS_SERVERPORT	_T("mmsWebServerPort")

// CONFIG
#define		CFG_CONFIG_CATEGORY		_T("CONFIG")
#define		CFG_CONFIG_DEBUG_TRACE	_T("debug_trace")
#define		CFG_CONFIG_CLOSE_IDLECLIENT _T("close_Idleclient")
#define		CFG_CONFIG_EXPIRE_TIME		_T("expire_time")

//OUTAGENT
#define		CFG_WISECAN_CATEGORY			_T("WISECAN")
#define		CFG_WISECAN_USEAGENT			_T("useAgent")
#define		CFG_WISECAN_DBNAME				_T("dbName")

//FAXAGENT
#define		CFG_XPEDITE_CATEGORY			_T("XPEDITE")
#define		CFG_XPEDITE_USEAGENT			_T("useAgent")
#define		CFG_XPEDITE_DBNAME				_T("dbName")
#define		CFG_XPEDITE_USERID				_T("userId")
#define		CFG_XPEDITE_PWD					_T("pwd")
#define		CFG_XPEDITE_FAXFILE_PATH		_T("faxFile_Path")

// HAODB
#define		CFG_HAOMUN_CATEGORY			_T("HAOMUN")
#define		CFG_HAOMUN_USEAGENT			_T("useAgent")
#define		CFG_HAOMUN_SERVER				_T("server")
#define		CFG_HAOMUN_ID						_T("id")
#define		CFG_HAOMUN_PASSWORD			_T("pwd")
#define		CFG_HAOMUN_DBNAME				_T("dbname")
#define		CFG_HAOMUN_DBPORT				_T("dbport")

// LGUplusAgent
#define		CFG_LGUPLUS_CATEGORY			_T("LGUPLUS")
#define		CFG_LGUPLUS_USEAGENT			_T("useAgent")
#define		CFG_LGUPLUS_DBNAME				_T("dbname")

//KSHOTAGENT
#define		CFG_KSHOTAGENT_CATEGORY	_T("KSHOTAGENT")
#define		CFG_KSHOTAGENT_VERSION		_T("version")


CommConfig::CommConfig(void)
{
}


CommConfig::~CommConfig(void)
{
}


void CommConfig::Init()
{
	//_getcwd(_currentPath, _MAX_PATH);
	//GetModuleFileName(NULL, _currentPath, _MAX_PATH);

	GetCurrentDirectory( _MAX_PATH, _currentPath);

	wsprintf(_iniPath,"%s\\%s",_currentPath,CFG__FILENAME);
	
	//config.ini 를 읽어온다.

	GetPrivateProfileString(CFG_CONNECT_CATEGORY, CFG_ENABLE_INTELLIGENT,	"",	_enable_intelligent, sizeof(_enable_intelligent),_iniPath);

	if( lstrcmp(_enable_intelligent, "on") == 0 )
		_enable_intelli = TRUE;
	else
		_enable_intelli = FALSE;
		

    GetPrivateProfileString(CFG_XROSTHOT_CATEGORY, CFG_XROSTHOT_ID,			"",	xroshot.id,			sizeof(xroshot.id),			_iniPath);
	GetPrivateProfileString(CFG_XROSTHOT_CATEGORY, CFG_XROSTHOT_CENTER,		"",	xroshot.center,		sizeof(xroshot.center),		_iniPath);
	GetPrivateProfileString(CFG_XROSTHOT_CATEGORY, CFG_XROSTHOT_PASSWORD,	"",	xroshot.password,	sizeof(xroshot.password),	_iniPath);
	GetPrivateProfileString(CFG_XROSTHOT_CATEGORY, CFG_XROSTHOT_CERTKEY,	"",	xroshot.certkey,	sizeof(xroshot.certkey),	_iniPath);
	GetPrivateProfileString(CFG_XROSTHOT_CATEGORY, CFG_XROSTHOT_LIMITLMS,	"",	xroshot.limitlms,	sizeof(xroshot.limitlms),	_iniPath);

    GetPrivateProfileString(CFG_INTELLI_CATEGORY, CFG_INTELLI_SERVER,		"",	intelli.server,		sizeof(intelli.server),		_iniPath);
	GetPrivateProfileString(CFG_INTELLI_CATEGORY, CFG_INTELLI_PORT,			"",	intelli.port,		sizeof(intelli.port),		_iniPath);
	GetPrivateProfileString(CFG_INTELLI_CATEGORY, CFG_INTELLI_ID,			"",	intelli.id,			sizeof(intelli.id),			_iniPath);
	GetPrivateProfileString(CFG_INTELLI_CATEGORY, CFG_INTELLI_PASSWORD,		"",	intelli.password,	sizeof(intelli.password),	_iniPath);

	GetPrivateProfileString(CFG_DB_CATEGORY, CFG_DB_SERVER,		"",	db.server,		sizeof(db.server),		_iniPath);
	GetPrivateProfileString(CFG_DB_CATEGORY, CFG_DB_ID,			"",	db.id,			sizeof(db.id),			_iniPath);
	GetPrivateProfileString(CFG_DB_CATEGORY, CFG_DB_PASSWORD,		"",	db.password,	sizeof(db.password),	_iniPath);
	GetPrivateProfileString(CFG_DB_CATEGORY, CFG_DB_DBNAME,			"",	db.dbname,		sizeof(db.dbname),		_iniPath);
	GetPrivateProfileString(CFG_DB_CATEGORY, CFG_DB_DBPORT,			"",	db.dbport,		sizeof(db.dbport),		_iniPath);

	GetPrivateProfileString(CFG_KSHOT_CATEGORY, CFG_KSHOT_PORT,		"",	kshot.port,		sizeof(kshot.port),		_iniPath);
	GetPrivateProfileString(CFG_KSHOT_CATEGORY, CFG_KSHOT_BIND_SERVERIP,		"",	kshot.bindServerIP,		sizeof(kshot.bindServerIP),		_iniPath);
	GetPrivateProfileString(CFG_KSHOT_CATEGORY, CFG_KSHOT_BIND_SERVERPORT,		"",	kshot.bindServerPort,		sizeof(kshot.bindServerPort),		_iniPath);
	GetPrivateProfileString(CFG_KSHOT_CATEGORY, CFG_KSHOT_MMS_SERVERIP,		"",	kshot.mmsServerIP,		sizeof(kshot.mmsServerIP),		_iniPath);
	GetPrivateProfileString(CFG_KSHOT_CATEGORY, CFG_KSHOT_MMS_SERVERPORT,		"",	kshot.mmsServerPort,		sizeof(kshot.mmsServerPort),		_iniPath);

	GetPrivateProfileString(CFG_CONFIG_CATEGORY, CFG_CONFIG_DEBUG_TRACE, "", config.debug_trace, sizeof(config.debug_trace), _iniPath);
	GetPrivateProfileString(CFG_CONFIG_CATEGORY, CFG_CONFIG_CLOSE_IDLECLIENT, "", config.close_Idleclient, sizeof(config.close_Idleclient), _iniPath);
	GetPrivateProfileString(CFG_CONFIG_CATEGORY, CFG_CONFIG_EXPIRE_TIME, "", config.expire_time, sizeof(config.expire_time), _iniPath);

	GetPrivateProfileString(CFG_WISECAN_CATEGORY, CFG_WISECAN_USEAGENT, "", wisecanAgent.useAgent, sizeof(wisecanAgent.useAgent), _iniPath);
	GetPrivateProfileString(CFG_WISECAN_CATEGORY, CFG_WISECAN_DBNAME, "", wisecanAgent.dbName, sizeof(wisecanAgent.dbName), _iniPath);

	GetPrivateProfileString(CFG_XPEDITE_CATEGORY, CFG_XPEDITE_USEAGENT, "", xpediteAgent.useAgent, sizeof(xpediteAgent.useAgent), _iniPath);
	GetPrivateProfileString(CFG_XPEDITE_CATEGORY, CFG_XPEDITE_DBNAME, "", xpediteAgent.dbName, sizeof(xpediteAgent.dbName), _iniPath);
	GetPrivateProfileString(CFG_XPEDITE_CATEGORY, CFG_XPEDITE_USERID, "", xpediteAgent.userId, sizeof(xpediteAgent.userId), _iniPath);
	GetPrivateProfileString(CFG_XPEDITE_CATEGORY, CFG_XPEDITE_PWD, "", xpediteAgent.pwd, sizeof(xpediteAgent.pwd), _iniPath);
	GetPrivateProfileString(CFG_XPEDITE_CATEGORY, CFG_XPEDITE_FAXFILE_PATH, "", xpediteAgent.faxFilePath, sizeof(xpediteAgent.faxFilePath), _iniPath);

	GetPrivateProfileString(CFG_HAOMUN_CATEGORY, CFG_HAOMUN_USEAGENT, "", haomunAgent.useAgent, sizeof(haomunAgent.useAgent), _iniPath);
	GetPrivateProfileString(CFG_HAOMUN_CATEGORY, CFG_HAOMUN_SERVER, "",		haomunAgent.server, sizeof(haomunAgent.server), _iniPath);
	GetPrivateProfileString(CFG_HAOMUN_CATEGORY, CFG_HAOMUN_ID, "",				haomunAgent.id, sizeof(haomunAgent.id), _iniPath);
	GetPrivateProfileString(CFG_HAOMUN_CATEGORY, CFG_HAOMUN_PASSWORD, "",	haomunAgent.password, sizeof(haomunAgent.password), _iniPath);
	GetPrivateProfileString(CFG_HAOMUN_CATEGORY, CFG_HAOMUN_DBNAME, "",		haomunAgent.dbname, sizeof(haomunAgent.dbname), _iniPath);
	GetPrivateProfileString(CFG_HAOMUN_CATEGORY, CFG_HAOMUN_DBPORT, "",		haomunAgent.dbport, sizeof(haomunAgent.dbport), _iniPath);

	GetPrivateProfileString(CFG_LGUPLUS_CATEGORY, CFG_LGUPLUS_USEAGENT, "", lguplusAgent.useAgent, sizeof(lguplusAgent.useAgent), _iniPath);
	GetPrivateProfileString(CFG_LGUPLUS_CATEGORY, CFG_LGUPLUS_DBNAME, "", lguplusAgent.dbname, sizeof(lguplusAgent.dbname), _iniPath);

	GetPrivateProfileString(CFG_KSHOTAGENT_CATEGORY, CFG_KSHOTAGENT_VERSION, "", kshotAgent.version, sizeof(kshotAgent.version), _iniPath);

	Log(CMdlLog::LEVEL_EVENT, _T("Configuration Loaded"));
}

void CommConfig::Exit()
{

}