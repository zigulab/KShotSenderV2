#pragma once
class XroshotSendInfo
{
public:
	XroshotSendInfo(	CString			sendId,
						CString			callbackNO,
						CString			receiveNO,
						CString			subject,
						CString			msg,
						CString			msgtype,
						CString			msgsubtype
						);
	XroshotSendInfo(	CString			sendId,
						CString			callbackNO,
						CString			receiveNO,
						CString			subject,
						CString			msg,
						CString			filename,
						byte*			filebuf,
						int				filesize,
						CString			msgtype,
						CString			msgsubtype 
						);

	virtual ~XroshotSendInfo(void);

public:

	CString				_sendId;
	CString				_callbackNO;
	CString				_receiveNO;
	CString				_subject;
	CString				_msg;
	CString				_msgtype;
	CString				_msgsubtype;

	CString				_enc_callbackNO;
	CString				_enc_msg;
	CString				_enc_receiveNO;

	CString				_result;

	// 아래는 MMS에만 해당
	CString				_filename;
	byte*				_filebuf;
	int					_filesize;
	CString				_fileAuthKey;
	CString				_filePath;
	CString				_decryptFilePath;

	// 전송구분
	BOOL					_bSent;

	int						_pageCount;
};

