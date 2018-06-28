#pragma once

// CSendResultInfo ��� ����Դϴ�.

class CSendResultInfo : public CObject
{
public:
	DECLARE_SERIAL(CSendResultInfo)

	CSendResultInfo();
	virtual ~CSendResultInfo();

	// ���� ��� ����
	struct SEND_RESULT_INFO
	{
		//Client* client;
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
		
		// ���(�߼�)�ð�
		CTime reg_time;

		// KT�� ���� ��� ���� �ð�
		DWORD result_time;

		// KT�� ���� ��� ����
		BOOL doneflag;

		// Agent ��� �۽� �Ϸ�
		BOOL completeflag;
	};

	vector<SEND_RESULT_INFO> vtSendResultInfo;

	int result_count;

public:

	void AddResult(CString uid, CString pid, CString send_number, unsigned int kshot_number, int msgType, CString receiveNo, CString done_time,
															int result, int telecom, float pirce, CTime reg_time, DWORD result_time, BOOL doneflag, BOOL completeflag);

	void Serialize(CArchive& archive);

	void SaveInfo(const char *filePath);

	void LoadInfo(const char *filePath);
};


