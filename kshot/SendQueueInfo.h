#pragma once

struct SEND_QUEUE_INFO
{
	/*
	char uid[20];
	int msgtype; 
	char send_number[20];
	char callback[20];
	char receive[20];
	char subject[100];
	char msg[2000];
	char filename[50];
	char filepath[200];
	int	filesize;
	unsigned int kshot_number;
	char isReserved;
	char reservedTime[30];
	char registTime[30];
	char line[30];
	*/

	CString uid;
	int msgtype;
	CString send_number;
	CString callback;
	CString receive;
	CString subject;
	CString msg;
	CString filename;
	CString filepath;
	int	filesize;
	unsigned int kshot_number;
	char isReserved;
	CString reservedTime;
	CString registTime;
	CString line;

	SEND_QUEUE_INFO() {
		msgtype = 0;
		filesize = 0;
		kshot_number = 0;
		isReserved = 0;
	}

	SEND_QUEUE_INFO(SEND_QUEUE_INFO& info) {
		/*
		strcpy(uid, info.uid);
		msgtype = info.msgtype;
		strcpy(send_number, info.send_number);
		strcpy(callback, info.callback);
		strcpy(receive, info.receive);
		strcpy(subject, info.subject);
		strcpy(msg, info.msg);
		strcpy(filename, info.filename);
		strcpy(filepath, info.filepath);
		filesize = info.filesize;
		kshot_number = info.kshot_number;
		isReserved = info.isReserved;
		strcpy(reservedTime, info.reservedTime);
		strcpy(registTime, info.registTime);
		strcpy(line, info.line);
		*/

		uid = info.uid;
		msgtype = info.msgtype;
		send_number = info.send_number;
		callback = info.callback;
		receive = info.receive;
		subject = info.subject;
		msg = info.msg;
		filename = info.filename;
		filepath = info.filepath;
		filesize = info.filesize;
		kshot_number = info.kshot_number;
		isReserved = info.isReserved;
		reservedTime = info.reservedTime;
		registTime = info.registTime;
		line = info.line;
	}
};