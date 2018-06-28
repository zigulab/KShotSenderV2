#pragma once

class DB_MEMBER
{
	int			user_no;
	CString		pid;
	CString		uid;
	CString		password;
	CString		name;
	CString		tel;
	CString		address;
	CString		manager;
	CString		handphone;
	CString		email;
	int			price_id;
	int			balance_id;
	CString		regdate;
	CString		mmsFolder;
	int			userType;
	int			limitYN;
	int			limitDailySend;
	int			denyYN;
};

class DB_PRICE
{
	int			price_id;
	double		price_sms;
	double		price_lms;
	double		price_mms;
	CString		uid;
};

class DB_BALANCE
{
	int			balance_id;
	int			rest_money;
	int			isPlusMinus;
	int			plus_minus_money;
	CString		regdate;
	CString		uid;
};

class DB_MSGQUEUE
{
	int			send_queue_id;
	CString		uid;
	CString		sendNo;
	int			kind;
	int			line_id;
	CString		callbackNo;
	CString		receiveNo;
	CString		subject;
	CString		msg;
	CString		mmsfile_id;
	int			isReserved;
	CString		registTime;
	CString		sendTime;
	CString		reponseTime;
	CString		resultTime;
	int			state;
	int			result;
};

class DB_MSRESULT
{
	int			send_queue_id;
	CString		uid;
	CString		sendNo;
	int			kind;
	int			line_id;
	CString		callbackNo;
	CString		receiveNo;
	CString		subject;
	CString		msg;
	CString		mmsfile_id;
	int			isReserved;
	CString		registTime;
	CString		sendTime;
	CString		reponseTime;
	CString		resultTime;
	int			state;
	int			result;
};

class DB_MSGAGGRE
{
	int			aggre_id;
	CString		uid;
	CString		sendNo;
	int			kind;
	int			line_id;
	CString		sendTime;
	int			total;
	int			success;
	int			fail;
	int			nontry;
	CString		aggreTime;
};

class DB_DAILYSTATISTICS
{
	int			statistic_id;
	CString		uid;
	CString		statisDate;
	int			kind;
	int			total;
	int			success;
	int			fail;
	int			nontry;
	CString		processTime;
};

class DBTableInfo
{
public:
	DBTableInfo(void);
	virtual ~DBTableInfo(void);
};

