// SendResultInfo.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "kshot.h"
#include "SendResultInfo.h"


// CSendResultInfo

IMPLEMENT_SERIAL(CSendResultInfo, CObject, 1)

CSendResultInfo::CSendResultInfo()
{
	result_count = 0;
}

CSendResultInfo::~CSendResultInfo()
{
}


// CSendResultInfo ��� �Լ�

void CSendResultInfo::AddResult(CString uid, CString pid, CString send_number, unsigned int kshot_number, int msgType, CString receiveNo, CString done_time,
																			int result, int telecom, float price, CTime reg_time, DWORD result_time, BOOL doneflag, BOOL completeflag)
{
	SEND_RESULT_INFO info;

	//info.uid;
	//info.pid;
	//info.send_number;
	//info.kshot_number;
	//info.msgType;
	//info.receiveNo;
	//info.done_time;
	//info.result;
	//info.telecom;
	//info.price;
	//info.reg_time;
	//info.result_time;
	//info.doneflag;
	//info.completeflag;

	info.uid = uid;
	info.pid = pid;
	info.send_number = send_number;
	info.kshot_number = kshot_number;
	info.msgType = msgType;
	info.receiveNo = receiveNo;
	info.done_time = done_time;
	info.result = result;
	info.telecom = telecom;
	info.price = price;
	info.reg_time = reg_time;
	info.result_time = result_time;
	info.doneflag = doneflag;
	info.completeflag = completeflag;

	vtSendResultInfo.push_back(info);
}

void CSendResultInfo::Serialize(CArchive& archive)
{
	CObject::Serialize(archive);

	if (archive.IsStoring())
	{
		//archive << _userid << _pwd << _callbackNo << _receiveNo << _subject << _message;

		result_count = vtSendResultInfo.size();

		archive << result_count;

		vector<SEND_RESULT_INFO>::iterator it = vtSendResultInfo.begin();
		for (; it != vtSendResultInfo.end(); it++) {

			SEND_RESULT_INFO info = *it;

			//archive << info;

			archive << info.uid;
			archive << info.pid;
			archive << info.send_number;
			archive << info.kshot_number;
			archive << info.msgType;
			archive << info.receiveNo;
			archive << info.done_time;
			archive << info.result;
			archive << info.telecom;
			archive << info.price;
			archive << info.reg_time;
			archive << info.result_time;
			archive << info.doneflag;
			archive << info.completeflag;

		}

	}
	else
	{
		//archive >> _userid >> _pwd >> _callbackNo >> _receiveNo >> _subject >> _message;

		archive >> result_count;

		CString uid;
		CString pid;
		CString send_number;
		unsigned int kshot_number;
		int msgType;
		CString receiveNo;
		CString done_time;
		int result;
		int telecom;
		float price;
		CTime reg_time;
		DWORD result_time;
		BOOL doneflag;
		BOOL completeflag;

		for( int i = result_count; i > 0; i-- ) 
		{
			archive >> uid;
			archive >> pid;
			archive >> send_number;
			archive >> kshot_number;
			archive >> msgType;
			archive >> receiveNo;
			archive >> done_time;
			archive >> result;
			archive >> telecom;
			archive >> price;
			archive >> reg_time;
			archive >> result_time;
			archive >> doneflag;
			archive >> completeflag;

			AddResult(uid, pid, send_number, kshot_number, msgType, receiveNo, done_time, result, telecom, price, reg_time, result_time, doneflag, completeflag);
		}

	}	
}

void CSendResultInfo::SaveInfo(const char *filePath)
{
	CFile file;
	if (file.Open(filePath, CFile::modeCreate | CFile::modeWrite) == FALSE)
		return;

	CArchive  ar(&file, CArchive::store);

	try {
		// ERect ��ü�� �����͸� �����Ѵ�.
		Serialize(ar);
	}
	catch (CFileException *fe) {
		// ���ܰ� �߻��ϸ� �޼����ڽ��� ���Ͽ� ����ڿ��� �˸���.
		fe->ReportError();
	}
	catch (CArchiveException *ae) {
		// ���ܰ� �߻��ϸ� �޼����ڽ��� ���Ͽ� ����ڿ��� �˸���.
		ae->ReportError();
	}
	catch (CMemoryException *me) {
		// ���ܰ� �߻��ϸ� �޼����ڽ��� ���Ͽ� ����ڿ��� �˸���.
		me->ReportError();
	}

	ar.Close();
	file.Close();
}

void CSendResultInfo::LoadInfo(const char *filePath)
{
	CFile file;
	if (file.Open(filePath, CFile::modeRead) == FALSE)
		return;

	CArchive  ar(&file, CArchive::load);

	try {
		// ���Ͽ� ����� ERect ��ü�� �����͸� �ҷ��´�.
		Serialize(ar);

	}
	catch (CFileException *fe) {
		// ���ܰ� �߻��ϸ� �޼����ڽ��� ���Ͽ� ����ڿ��� �˸���.
		fe->ReportError();
	}
	catch (CArchiveException *ae) {
		// ���ܰ� �߻��ϸ� �޼����ڽ��� ���Ͽ� ����ڿ��� �˸���.
		ae->ReportError();
	}
	catch (CMemoryException *me) {
		// ���ܰ� �߻��ϸ� �޼����ڽ��� ���Ͽ� ����ڿ��� �˸���.
		me->ReportError();
	}

	ar.Close();
	file.Close();
}