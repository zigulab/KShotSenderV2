#pragma once

#include "commbase.h"

#include "CommKTxroShot.h"
#include "CommKTintell.h"


class CommKT :	public CommBase
{
public:
	CommKT(CommManager *manager);
	virtual ~CommKT(void);

///////////////////////////////////////////////////////////////////////////////// MEMBER DATA

	CommManager *				_manager;

	map<CString, CommKTxroShot*>	_mapXroShot;

	// 현재 활성화된 크로샷 객체
	CommKTxroShot*				_curXroShot;
	CommKTintell*					_intelli;

	CCriticalSection				_cs;

///////////////////////////////////////////////////////////////////////////////// METHOD
// INTERFACE
	virtual BOOL	Init();
	virtual BOOL	Uninit();

	virtual BOOL	Connect();

	virtual BOOL ConnectToXroShot();
	virtual BOOL ConnectToIntelli();

	virtual void	Ping(int tid);

	virtual BOOL	SendSMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg);
	virtual BOOL	SendLMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg);
	virtual BOOL	SendMMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size);
	virtual BOOL	SendFAX(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size);

	virtual void	OnClose(int nErrorCode)		{}
	virtual void	OnConnect(int nErrorCode)	{}
	virtual void	OnReceive(TCHAR *buf, int size) {}
	virtual void	OnSend(int nErrorCode) {}

// GENERAL

	CommKTxroShot* AddXroShot(CString id, CString pwd, CString certFile, CString serverUrl);
	void DelXroShot(CString id);
	void ActivateXroShot(CString id);

	void GetXroShotList(map<CString, CommKTxroShot*> &mapList);

	void SetCurXroShot(CommKTxroShot* xshot);

};

