// CommManager.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "kshot.h"
#include "CommManager.h"

// CommManager

CommManager::CommManager()
{
	// ȯ�漳���� ���� �ٲ�� ��.
	_tele_line = LINE_KT;

	//_sms_messenger = NULL;
	//_lms_messenger = NULL;
	//_mms_messenger = NULL;

	_messenger = NULL;
}

CommManager::~CommManager()
{
	if( _messenger != NULL ) {
		delete _messenger;
		_messenger = NULL;
	}
}


// CommManager ��� �Լ�

BOOL CommManager::Init()
{
	Log(CMdlLog::LEVEL_EVENT, _T("CommManager �ʱ�ȭ"));

	switch(_tele_line)
	{
	case LINE_KT:
		_messenger = new CommKT(this);
		break;
	case LINE_SKT:
		break;
	case LINE_LG:
		break;
	default:
		break;
	}

	if( _messenger->Init() == FALSE ) {
		Log(CMdlLog::LEVEL_ERROR, _T("��Ż� ä�� �ʱ�ȭ ����"));
		return FALSE;
	}

	return TRUE;
}

BOOL	CommManager::ConnectToTelecom()
{
	Log(CMdlLog::LEVEL_EVENT, _T("��Ż� ���� ���� �õ�"));

	return _messenger->Connect();
}

BOOL	CommManager::ConnectToXroShot()
{
	Log(CMdlLog::LEVEL_EVENT, _T("Xroshot ���� ���� �õ�"));

	return ((CommKT*)_messenger)->ConnectToXroShot();
}

BOOL	CommManager::ConnectToIntelli()
{
	Log(CMdlLog::LEVEL_EVENT, _T("Intelli ���� ���� �õ�"));

	return ((CommKT*)_messenger)->ConnectToIntelli();
}

BOOL CommManager::Uninit()
{
	Log(CMdlLog::LEVEL_EVENT, _T("CommManager, �������"));

	return _messenger->Uninit();
}

void CommManager::Ping(int tid)
{
	_messenger->Ping(tid);

}


BOOL CommManager::SendSMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg)
{
	return _messenger->SendSMS(sendId, callbackNO, receiveNO, subject, msg);
}

BOOL CommManager::SendLMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg)
{
	return _messenger->SendLMS(sendId, callbackNO, receiveNO, subject, msg);
}

BOOL CommManager::SendMMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size)
{
	return _messenger->SendMMS(sendId, callbackNO, receiveNO, subject, msg, filename, imageFilePath, size);
}

BOOL CommManager::SendFAX(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size)
{
	return _messenger->SendFAX(sendId, callbackNO, receiveNO, subject, msg, filename, imageFilePath, size);
}


BOOL CommManager::Receive(byte *msg, int len)
{
	return TRUE;
}