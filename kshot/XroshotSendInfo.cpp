#include "StdAfx.h"
#include "XroshotSendInfo.h"


XroshotSendInfo::XroshotSendInfo(	CString			sendId,
									CString			callbackNO,
									CString			receiveNO,
									CString			subject,
									CString			msg,
									CString			msgtype,
									CString			msgsubtype
									) 
{
	_sendId		= sendId;
	_callbackNO = callbackNO;
	_subject	= subject;
	_msg		= msg;
	_msgtype	= msgtype;
	_msgsubtype	= msgsubtype;

	_receiveNO = receiveNO;

	_filebuf = NULL;

	_bSent = FALSE;

	//_result.SetSize(receiveNO.GetSize());

	_pageCount = 0;
};

XroshotSendInfo::XroshotSendInfo(	CString			sendId,
									CString			callbackNO,
									CString			receiveNO,
									CString			subject,
									CString			msg,
									CString			filename,
									byte*			filebuf,
									int				filesize,
									CString			msgtype,
									CString			msgsubtype 
									) 
{
	_sendId		= sendId;
	_callbackNO = callbackNO;
	_subject	= subject;
	_msg		= msg;
	_filename   = filename;
	_filesize	= filesize;
	_msgtype	= msgtype;
	_msgsubtype	= msgsubtype;

	_receiveNO = receiveNO;

	_filebuf = new byte[filesize];
	memcpy(_filebuf, filebuf, filesize);

	//_result.SetSize(receiveNO.GetSize());

	_bSent = FALSE;

	_pageCount = 0;
};

XroshotSendInfo::~XroshotSendInfo() 
{
	if( _filebuf != NULL )
		delete _filebuf;
};
