#pragma once

#include "ServerSocket.h"

class CommManager;

class CommBase
{
public:
	CommBase(CommManager *manager);
	virtual ~CommBase(void);

///////////////////////////////////////////////////////////////////////////////// MEMBER DATA

	CommManager*	_manager;

	ServerSocket	_socket;

	CString			_serverIP;
	CString			_serverPort;

	CString			_id;
	CString			_pwd;


///////////////////////////////////////////////////////////////////////////////// METHOD
// INTERFACE
	virtual BOOL	Init() { return TRUE; };
	virtual BOOL	Uninit();

	virtual BOOL	Connect() { return TRUE; };

	virtual void	Ping(int tid) { };

	virtual BOOL	SendSMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg) { return TRUE;};
	virtual BOOL	SendLMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg) { return TRUE;};
	virtual BOOL	SendMMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size) { return TRUE;};
	virtual BOOL	SendFAX(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size) { return TRUE; };

	virtual void	OnClose(int nErrorCode)		{}
	virtual void	OnConnect(int nErrorCode)	{}
	virtual void	OnReceive(TCHAR *buf, int size) {}
	virtual void	OnSend(int nErrorCode) {}

// GENERAL
};

