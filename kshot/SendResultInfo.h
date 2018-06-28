#pragma once

// CSendResultInfo 명령 대상입니다.

class CSendResultInfo : public CObject
{
public:
	DECLARE_SERIAL(CSendResultInfo)

	CSendResultInfo();
	virtual ~CSendResultInfo();

	// 전송 결과 내용
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
		
		// 등록(발송)시간
		CTime reg_time;

		// KT로 부터 결과 수신 시간
		DWORD result_time;

		// KT로 부터 결과 수신
		BOOL doneflag;

		// Agent 결과 송신 완료
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


