//////////////////////////////////////////////////////////////////////////
///
///	[CMdlLog] Application log module
///
///	@ Author	: Ju-heon Song
///	@ Date		: 10/26/2011 20:40:25
/// @ Version	: 1.0.0.0
///
///	Copyright(C) 2010 oopman.com, All rights reserved.
///
//////////////////////////////////////////////////////////////////////////

#pragma once
#ifdef MDLLOG_EXPORTS
#define MDLLOG_API __declspec(dllexport)
#else
#define MDLLOG_API __declspec(dllimport)
#endif

class MDLLOG_API CMdlLog
{
public:
	enum Level { LEVEL_ERROR, LEVEL_EVENT, LEVEL_DEBUG };
	enum Mode  { MODE_SINGLE, MODE_MULTI };

	CMdlLog(BOOL bIsConApp = FALSE, UINT nLogMaxLen = 1024, INT nLogActiveLevel = LEVEL_ERROR,
			LPCTSTR lpszPreFix = NULL, LPCTSTR lpszPath = NULL, LPCTSTR lpszName = NULL, UINT nMode = MODE_SINGLE);
	virtual ~CMdlLog(void);

private:
	HANDLE				m_hConsole;
	BOOL				m_bActiveConsole;
	BOOL				m_bIsConApp;
	UINT				m_nMode;
	UINT				m_nLogMaxLen;
	INT					m_nLogActiveLevel;
	TCHAR				m_szPath[MAX_PATH + 1];
	TCHAR				m_szName[MAX_PATH + 1];
	TCHAR				m_szTime[128];
	TCHAR				*m_lpszPreFix;
	CRITICAL_SECTION	m_cs;

protected:
	virtual LPCTSTR	GetFormatLevel(INT nLevel);
	virtual LPCTSTR	GetFormatTime(void);
	virtual BOOL WriteToFiles(LPCTSTR lpszLog, INT nLevel);
	virtual void WriteToConsoles(LPCTSTR lpszLog, INT nLevel);

public:
	// @desc   : Module Initialize setting
	// @para   : Console application?, Log string max length, Log active level, Log string pre fix, Log file directory, Log file name, Module object mode
	// @result : TRUE, FALSE
	BOOL SetInit(BOOL bIsConApp, UINT nLogMaxLen, INT nLogActiveLevel,
					LPCTSTR lpszPreFix = NULL, LPCTSTR lpszPath = NULL, LPCTSTR lpszName = NULL, UINT nMode = MODE_SINGLE);

	// @desc   : Set the active level of the log
	// @para   : Active level
	// @result : none
	void SetActiveLevel(INT nActiveLevel);

	// @desc   : Setting for console displays
	// @para   : TRUE ? FALSE
	// @result : none
	void ShowConsole(BOOL bIsShow = TRUE);

	// @desc   : Log write
	// @para   : Log level, Log (format) string, (values)...
	// @result : TRUE, FALSE
	BOOL Write(INT nLevel, LPCTSTR lpszFormat, ...);
};