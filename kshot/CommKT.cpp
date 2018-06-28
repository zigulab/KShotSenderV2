#include "StdAfx.h"
#include "CommKT.h"

#include "CommManager.h"

CommKT::CommKT(CommManager *manager)
: CommBase(manager)
//_xroShot(manager),
//_intelli(manager)
{
	_manager = manager;
}


CommKT::~CommKT(void)
{
}

BOOL CommKT::Init()
{
	CString serverUrl  = g_config.xroshot.center;
	CString id			= g_config.xroshot.id;
	CString pwd		= g_config.xroshot.password;
	CString certFile	= g_config.xroshot.certkey;

	CommKTxroShot* xroShot = AddXroShot(id, pwd, certFile, serverUrl);

	ActivateXroShot(id);

	if( !_curXroShot->Init() ) return FALSE;

	//if( g_config._enable_intelli ) 
	//{

		_intelli = new CommKTintell(_manager);
		if( !_intelli->Init() ) return FALSE;
	//}

	return TRUE;
}

BOOL CommKT::Connect()
{
	if( !ConnectToXroShot() ) return FALSE;

	//if( g_config._enable_intelli ) 
	//{
		if( ! ConnectToIntelli()) return FALSE;
	//}

	return TRUE;
}

BOOL CommKT::ConnectToXroShot()
{
	BOOL retval;
	_cs.Lock();

	retval = _curXroShot->Connect();

	_cs.Unlock();
	return retval;
}

BOOL CommKT::ConnectToIntelli()
{
	return _intelli->Connect();
}

BOOL CommKT::Uninit()
{
	BOOL retval1, retval2; 

	retval1 = retval2 = TRUE;

	retval1 = _intelli->Uninit();

	delete _intelli;
	_intelli = NULL;

	map<CString, CommKTxroShot*>::iterator it = _mapXroShot.begin();
	for (; it != _mapXroShot.end(); it++)
	{
		CommKTxroShot *xroshot = it->second;
		if( xroshot->Uninit() ==  FALSE) 
			retval2 = FALSE;

		delete xroshot;
	}

	_mapXroShot.clear();
	
	return retval1 && retval2;
}


void CommKT::Ping(int tid)
{
	if (tid == XROSHOT_TIMER_ID) {
		//_xroShot->Ping(tid);

		map<CString, CommKTxroShot*>::iterator it = _mapXroShot.begin();
		for (; it != _mapXroShot.end(); it++)
		{
			CommKTxroShot *xroshot = it->second;
			xroshot->Ping(tid);
		}
	}
	else {
		if( _intelli != NULL )
			_intelli->Ping(tid);
	}
}

BOOL CommKT::SendSMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg)
{
	BOOL retval;
	_cs.Lock();

	if (g_enable_intelli)
		retval = _intelli->SendSMS(sendId, callbackNO, receiveNO, subject, msg);
	else
		retval = _curXroShot->SendSMS(sendId, callbackNO, receiveNO, subject, msg);

	_cs.Unlock();
	return retval;
}

BOOL CommKT::SendLMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg)
{
	BOOL retval;
	_cs.Lock();

	retval = _curXroShot->SendLMS(sendId, callbackNO, receiveNO, subject, msg);

	_cs.Unlock();
	return retval;
}

BOOL CommKT::SendMMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size)
{ 
	BOOL retval;
	_cs.Lock();
	
	retval = _curXroShot->SendMMS(sendId, callbackNO, receiveNO, subject, msg, filename, imageFilePath, size);

	_cs.Unlock();
	return retval;
}

BOOL CommKT::SendFAX(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size)
{
	BOOL retval;
	_cs.Lock();
	
	retval = _curXroShot->SendFAX(sendId, callbackNO, receiveNO, subject, msg, filename, imageFilePath, size);

	_cs.Unlock();
	return retval;
}

void CommKT::SetCurXroShot(CommKTxroShot* xshot)
{
	_cs.Lock();
	
	_curXroShot = xshot;

	_cs.Unlock();
}

CommKTxroShot* CommKT::AddXroShot(CString id, CString pwd, CString certFile, CString serverUrl)
{
	CommKTxroShot* xroshot = new CommKTxroShot(id, pwd, certFile, serverUrl, _manager);

	_mapXroShot[id] = xroshot;
	
	return xroshot;
}

void CommKT::DelXroShot(CString id)
{
	map<CString, CommKTxroShot*>::iterator it = _mapXroShot.begin();
	for (; it != _mapXroShot.end(); it++)
	{
		CString account = it->first;
		if (account == id) 
		{
			// 활성 상태의 계정은 삭제 불가
			if (((CommKTxroShot*)it->second)->_activate == TRUE)
				break;

			delete it->second;
			_mapXroShot.erase(it);
			break;
		}
	}
}

void CommKT::ActivateXroShot(CString id)
{
	CTime   CurTime = CTime::GetCurrentTime();
	CString szTime = CurTime.Format("%Y-%m-%d %H:%M:%S");

	map<CString, CommKTxroShot*>::iterator it = _mapXroShot.begin();
	for (; it != _mapXroShot.end(); it++)
	{
		CString account = it->first;
		CommKTxroShot *xroshot = it->second;

		if (account == id) {
			xroshot->SetActivate(TRUE);
			SetCurXroShot(xroshot);
		}
		else 
			xroshot->SetActivate(FALSE);

		xroshot->SetActivateDate(szTime);
	}
}

void CommKT::GetXroShotList(map<CString, CommKTxroShot*> &mapList)
{
	mapList = _mapXroShot;
}
