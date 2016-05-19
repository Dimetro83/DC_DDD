/* 
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"


#ifdef _DEBUG_VLD
/** 
	Memory leak detector
	You can remove following 3 lines if you don't want to detect memory leaks.
 */
#define VLD_MAX_DATA_DUMP 0
#define VLD_AGGREGATE_DUPLICATES
#ifdef _WIN32
#include <vld.h>
#endif
#endif

#include "../client/DCPlusPlus.h"
#include "SingleInstance.h"
#include "WinUtil.h"

#include "Mapper_NATPMP.h"
#include "Mapper_MiniUPnPc.h"
#include "Mapper_WinUPnP.h"

#include "../client/MerkleTree.h"
#include "../client/MappingManager.h"
#include "../client/SSLSocket.h"

#include "Resource.h"

#include "MainFrm.h"
#include "PopupManager.h"

#include <delayimp.h>
#include <Dbghelp.h>
#ifdef NIGHT_ORION_SEND_STACK_DUMP_TO_SERVER
#include "DumpSenderDlg.h"
#endif


typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(
		__in          HANDLE hProcess,
		__in          DWORD ProcessId,
		__in          HANDLE hFile,
		__in          MINIDUMP_TYPE DumpType,
		__in          PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
		__in          PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
		__in          PMINIDUMP_CALLBACK_INFORMATION CallbackParam
		);
MINIDUMPWRITEDUMP g_pMiniDumpWriteDump = NULL;


CAppModule _Module;

CriticalSection cs;
const int DEBUG_BUFSIZE = 8192;
static bool firstException = true;

EXCEPTION_RECORD CurrExceptionRecord;
CONTEXT CurrContext;

// start directory string primary to use in unhandled exception filter to generate crash dump
static std::wstring g_strHomeDir;
static size_t g_strHomeDirLen; // [!] PVS V104 Implicit conversion of 'g_strHomeDirLen' to memsize type in an arithmetic expression: & g_strHomeDir [0] + g_strHomeDirLen main.cpp 125

#ifdef _DEBUG
#define DUMP_STACK_FILENAME L"crash-strong-stack-debug-" L_VERSIONSTRING L".dmp"
#define DUMP_FULL_FILENAME L"crash-strong-full-debug-" L_VERSIONSTRING L".dmp"
#else
#define DUMP_STACK_FILENAME L"crash-strong-stack-" L_VERSIONSTRING L".dmp"
#define DUMP_FULL_FILENAME L"crash-strong-full-" L_VERSIONSTRING L".dmp"
#endif
#define MAX_HOME_DIR_FILE_NAME_LEN ::max(_countof(DUMP_STACK_FILENAME), _countof(DUMP_FULL_FILENAME))
// g_strHomeDir's size initializes with size of start dir name + MAX_HOME_DIR_FILE_NAME_LEN.
// g_strHomeDir used to generate full qualified file name in start dir in that way:
// strcpy(&g_strHomeDir.at(g_strHomeDir.size() - MAX_DUMP_FILE_NAME_LEN), wcFileNameWithoutPath);
// lenght of wcFileNameWithoutPath MUST be less or equal MAX_HOME_DIR_FILE_NAME_LEN.
//
// One should use GetHomeDirFileName function to get full path file name.
// Remember this function and g_strHomeDir itself - is NOT multithread aware!
//
// MAX_HOME_DIR_FILE_NAME_LEN made fixed size to avoid memory allocation in exception filter

void InitHomeDirFileName()
{
	 g_strHomeDir.resize(MAX_PATH);
	 SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, &g_strHomeDir[0]);
	 wcscat(&g_strHomeDir[0], L"\\SSQLite++CrashDump\\");
	 g_strHomeDirLen = wcslen(&g_strHomeDir[0]);
	const string l_path = Text::fromT(g_strHomeDir.c_str()); //[!]PPA .c_str() не убирать!
	const string l_stack_dump = Text::fromT(DUMP_STACK_FILENAME);
    const string l_full_dump = Text::fromT(DUMP_FULL_FILENAME);
	const StringList& l_ToDelete = File::findFiles(l_path, "crash-ddd++-*-r*.dmp",false);
	for (StringIterC i = l_ToDelete.cbegin(); i != l_ToDelete.cend(); ++i)
		{
		  const string l_cur = *i;
		  if (l_cur != l_stack_dump && l_cur != l_full_dump)
			File::deleteFile(l_path + *i);
	    }
}
LPCWSTR GetHomeDirFileName(LPCWSTR pwcFileName)
{
	if (!pwcFileName || wcslen(pwcFileName) > MAX_HOME_DIR_FILE_NAME_LEN)
		return pwcFileName;
	CreateDirectory(&g_strHomeDir[0],NULL);
        wcscpy(&g_strHomeDir[0] + g_strHomeDirLen, pwcFileName);
	return g_strHomeDir.c_str();
}
// End CRASH DUMP stuff *********************************************************************************

void ExceptionFunction()
{
//	if (iLastExceptionDlgResult == IDCANCEL)
	{
		ExitProcess(1);
	}
}
#define MEDIAINFO_CRASH_INFO_FILENAME "mediainfo-crash-tth-info-sdc-sqlite.txt"
static void NotifyUserAboutException(LPEXCEPTION_POINTERS pException, bool bDumped, DWORD dwDumpError)
{
	if ((!SETTING(SOUND_EXC).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
		PlaySound(Text::toT(SETTING(SOUND_EXC)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

	NOTIFYICONDATA m_nid = {0};
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	if(MainFrame::getMainFrame())
	 m_nid.hWnd = MainFrame::getMainFrame()->m_hWnd;
	m_nid.uID = 0;
	m_nid.uFlags = NIF_INFO;
	m_nid.uTimeout = 5000;
	m_nid.dwInfoFlags = NIIF_WARNING;
	if (bDumped)
	{
		_tcsncpy(m_nid.szInfo, DUMP_STACK_FILENAME _T(" was generated!"), 255);
	}
	else
	{
		_tcsncpy(m_nid.szInfo, _T(" Error create crash-ddd++-*.dmp file..."), 255); 
	}
	_tcsncpy(m_nid.szInfoTitle, _T("DDD++ has crashed"),63);
	Shell_NotifyIcon(NIM_MODIFY, &m_nid);

//	wstring l_message = _T("Если Вы хотите помочь разработчику исправить ошибку,\n")
//		 _T("пошлите через файлообменник запакованные дампы падения (crash-strong-*.dmp)\n")
//		 _T("на адрес ppa74@ya.ru или pavel.pimenov@gmail.com");
	extern string g_cur_mediainfo_file;    
    if(!g_cur_mediainfo_file.empty())
	{
				string l_mediainfo_tmp = A_VERSIONSTRING;
				l_mediainfo_tmp += "\r\nCrash mediainfo file: " + g_cur_mediainfo_file;
				try
				{
					extern std::wstring g_strHomeDir;
					string l_file = Text::fromT(g_strHomeDir.c_str()); //[!]PPA .c_str() не убирать!
					const string::size_type l_pos = l_file.rfind('\\');
					if (l_pos != string::npos)
					{
						l_file = l_file.substr(0, l_pos + 1) + A_VERSIONSTRING + '-' + MEDIAINFO_CRASH_INFO_FILENAME;
						File(l_file, File::WRITE, File::CREATE | File::TRUNCATE).write(l_mediainfo_tmp);
					}
				}
				catch (FileException&)
				{
				}
	}
//	if(MessageBox(WinUtil::mainWnd, l_message.c_str(), _T("StrongDC++ sqlite упал!"), MB_YESNO | MB_ICONERROR) == IDYES) 
//    {
//		WinUtil::openLink(_T("mailto:ppa74@ya.ru"));
//	}
	ExceptionFunction();
//	CExceptionDlg dlg;
//	iLastExceptionDlgResult = dlg.DoModal(WinUtil::mainWnd);
//	ExceptionFunction();
}

static bool DoDump(LPCWSTR pwcDumpFileName, MINIDUMP_TYPE DumpType, LPEXCEPTION_POINTERS pExceptionInfo)
{
	if (!g_pMiniDumpWriteDump)
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		return false;
	}
		
	bool bRet = false;
	DWORD dwLastError = ERROR_SUCCESS;
	HANDLE hDumpFile = ::CreateFile(pwcDumpFileName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	                                
	dwLastError = GetLastError();

	if (hDumpFile && hDumpFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei; 

		mdei.ThreadId = GetCurrentThreadId(); 
		mdei.ExceptionPointers = pExceptionInfo; 
		mdei.ClientPointers = FALSE; 

		bRet = g_pMiniDumpWriteDump(GetCurrentProcess(), 
			GetCurrentProcessId(), 
			hDumpFile, 
			DumpType, 
			(pExceptionInfo != NULL) ? &mdei : NULL, 
			0, 
			0) != FALSE;
		                            
		dwLastError = GetLastError();
			
		CloseHandle(hDumpFile);
	}
	
	if (!bRet)
		SetLastError(dwLastError);
		
	return bRet;
}

static LONG __stdcall DCUnhandledExceptionFilter( LPEXCEPTION_POINTERS pExceptionInfo )
{
	bool bDumped = false;
	DWORD dwLastError = ERROR_SUCCESS;
	
	if (g_pMiniDumpWriteDump)
	{
		bDumped |= DoDump(GetHomeDirFileName(DUMP_STACK_FILENAME), MiniDumpWithHandleData , pExceptionInfo);
		if (!bDumped)
			{
				dwLastError = GetLastError();
//		        const string l_text_error = "dwDumpError=" + Util::toString(dwLastError);
//				MessageBox(WinUtil::mainWnd, Text::toT(l_text_error).c_str(), _T("DoDump error!"), MB_ICONERROR);
		}
//		else
//				MessageBox(WinUtil::mainWnd, _T("DoDump 1 ok!") , _T("DoDump 1 ok!"), MB_ICONERROR);

			
		bDumped |= DoDump(GetHomeDirFileName(DUMP_FULL_FILENAME), MiniDumpWithFullMemory, pExceptionInfo);
		if (!bDumped)
		{
			dwLastError = GetLastError();
//	        const string l_text_error = "dwDumpError=" + Util::toString(dwLastError);
//			MessageBox(WinUtil::mainWnd, Text::toT(l_text_error).c_str(), _T("DoDump 2 error!"), MB_ICONERROR);
		}
//		else
//				MessageBox(WinUtil::mainWnd, _T("DoDump 2 ok!") , _T("DoDump 2 ok!"), MB_ICONERROR);

	}
	
	NotifyUserAboutException(pExceptionInfo, bDumped, dwLastError);
	
#ifndef _DEBUG
	return EXCEPTION_CONTINUE_EXECUTION;
#else
	return EXCEPTION_CONTINUE_SEARCH;
#endif
}

static void sendCmdLine(HWND hOther, LPTSTR lpstrCmdLine)
{
	tstring cmdLine = lpstrCmdLine;
	LRESULT result;

	COPYDATASTRUCT cpd;
	cpd.dwData = 0;
	cpd.cbData = sizeof(TCHAR)*(cmdLine.length() + 1);
	cpd.lpData = (void *)cmdLine.c_str();
	result = SendMessage(hOther, WM_COPYDATA, NULL,	(LPARAM)&cpd);
}

BOOL CALLBACK searchOtherInstance(HWND hWnd, LPARAM lParam) {
	ULONG_PTR result;
	LRESULT ok = ::SendMessageTimeout(hWnd, WMU_WHERE_ARE_YOU, 0, 0,
		SMTO_BLOCK | SMTO_ABORTIFHUNG, 5000, &result);
	if(ok == 0)
		return TRUE;
	if(result == WMU_WHERE_ARE_YOU) {
		// found it
		HWND *target = (HWND *)lParam;
		*target = hWnd;
		return FALSE;
	}
	return TRUE;
}

static void checkCommonControls() {
#define PACKVERSION(major,minor) MAKELONG(minor,major)

	HINSTANCE hinstDll;
	DWORD dwVersion = 0;
	
	hinstDll = LoadLibrary(_T("comctl32.dll"));
	
	if(hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
	
		pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");
		
		if(pDllGetVersion)
		{
			DLLVERSIONINFO dvi = {0};
			HRESULT hr;
		
			dvi.cbSize = sizeof(dvi);
	
			hr = (*pDllGetVersion)(&dvi);
			
			if(SUCCEEDED(hr))
			{
				dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
			}
		}
		
		FreeLibrary(hinstDll);
	}

	if(dwVersion < PACKVERSION(5,80)) {
		MessageBox(NULL, _T("Your version of windows common controls is too old for DDD++ to run correctly, and you will most probably experience problems with the user interface. You should download version 5.80 or higher from the DC++ homepage or from Microsoft directly."), _T("User Interface Warning"), MB_OK);
	}
}

static HWND hWnd;
static tstring sText;
static tstring sTitle;

LRESULT CALLBACK splashCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_PAINT) {
		// Get some information
		HDC dc = GetDC(hwnd);
		RECT rc;
		GetWindowRect(hwnd, &rc);
		OffsetRect(&rc, -rc.left, -rc.top);
		RECT rc2 = rc;
		rc2.top = rc2.bottom - 96; 
		rc2.right = rc2.right - 10;
		::SetBkMode(dc, TRANSPARENT);
		
		// Draw the icon
		HBITMAP hi;
		hi = (HBITMAP)LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(IDB_SPLASH), IMAGE_BITMAP, 550, 200, LR_SHARED);
			 
		HDC comp=CreateCompatibleDC(dc);
		SelectObject(comp,hi);	

		BitBlt(dc,0, 0 , 550, 200,comp,0,0,SRCCOPY);

		DeleteObject(hi);
		DeleteDC(comp);
//		LOGFONT logFont = {0};
//		HFONT hFont;
//		GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
//		lstrcpy(logFont.lfFaceName, TEXT("Tahoma"));
//		logFont.lfHeight = 9;
//		logFont.lfWeight = 700;
//		hFont = CreateFontIndirect(&logFont);		
//		SelectObject(dc, hFont);
//		::SetTextColor(dc, RGB(128,128,128));
//		::DrawText(dc, sTitle.c_str(), sTitle.length(), &rc2, DT_RIGHT);
//		DeleteObject(hFont);

//		if(!sText.empty()) {
//			rc2 = rc;
//			rc2.top = rc2.bottom - 15;
//			GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
//			lstrcpy(logFont.lfFaceName, TEXT("Tahoma"));
//			logFont.lfHeight = 9;
//			logFont.lfWeight = 700;
//			hFont = CreateFontIndirect(&logFont);		
//			SelectObject(dc, hFont);
//			::SetTextColor(dc, RGB(128,128,128));
//			::DrawText(dc, (_T(".:: ") + sText + _T(" ::.")).c_str(), _tcslen((_T(".:: ") + sText + _T(" ::.")).c_str()), &rc2, DT_CENTER);
//			DeleteObject(hFont);
		}

		ReleaseDC(hwnd, dc);
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void callBack(void* x, const tstring& a) {
	sText = a;
	SendMessage((HWND)x, WM_PAINT, 0, 0);
}

bool g_isWineCheck = false;

static bool is_wine()
{
    HMODULE module = GetModuleHandleA("ntdll.dll");
    if (!module)
		return 0;
    return (GetProcAddress(module, "wine_server_call") != NULL);
}
#ifdef NIGHT_ORION_SEND_STACK_DUMP_TO_SERVER
static bool SendFileToServer(const wstring& p_DumpFileName, const string& p_mediainfo_crash_info, bool p_silient)
{
	if (!p_silient)
	{
			CDumpSenderDlg dlg(p_DumpFileName, p_mediainfo_crash_info);
			const INT_PTR iDumpSenderDlgResult = dlg.DoModal(WinUtil::mainWnd);
			if (iDumpSenderDlgResult != IDYES)
			{
				LogManager::getInstance()->getInstance()->message("[stack-dmp-sender] cancel");
				return false;
			}
	}
	
	const wstring l_bz2_name = p_DumpFileName + _T(".bz2");
	if (File::bz2CompressFile(p_DumpFileName, l_bz2_name) > 0)
	{
		const TCHAR* l_url = _T("nightorion.dyndns.ws");
		CInternetHandle hInternet(InternetOpen(T_VERSIONSTRING, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0));
		if (!hInternet)
			return false;
		CInternetHandle hFTP(InternetConnect(hInternet, l_url, INTERNET_DEFAULT_FTP_PORT, NULL, NULL, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE | INTERNET_FLAG_EXISTING_CONNECT, 0));
		if (!hFTP)
		{
			LogManager::getInstance()->getInstance()->message("[stack-dmp-sender][InternetConnect][Error] " + Util::translateError(GetLastError()));
			return false;
		}
		if (FtpSetCurrentDirectory(hFTP, _T("dumps")))
		{
			GUID l_Guid = { 0 };
			::CoCreateGuid(&l_Guid);
			OLECHAR l_szGuid[40] = {0};
			::StringFromGUID2(l_Guid, l_szGuid, 40);
			wstring l_guid_str, l_guid_str_guid(l_szGuid);
			if (l_guid_str_guid.length() > 1)
				l_guid_str_guid = l_guid_str_guid.substr(1, 8);
			l_guid_str += Util::getCompileDate();
			l_guid_str += L'_';
			l_guid_str += Util::getCompileTime();
			l_guid_str += L'_';
			l_guid_str += Text::acpToWide(SETTING(PRIVATE_ID));
			l_guid_str += L'_';
			l_guid_str += l_guid_str_guid;
			l_guid_str += L'_';
			if (g_isWineCheck)
				l_guid_str += L"wine_";
			if(!p_mediainfo_crash_info.empty())
				l_guid_str += L"mediainfo_";
			l_guid_str += Util::getFileName(l_bz2_name);
			LogManager::getInstance()->getInstance()->message("[stack-dmp-sender][FtpPutFile] Start upload: " + Text::fromT(l_bz2_name));
			if (FtpPutFile(hFTP, l_bz2_name.c_str(), l_guid_str.c_str(), FTP_TRANSFER_TYPE_BINARY, 0) != 0)
			{
				LogManager::getInstance()->getInstance()->message("[stack-dmp-sender][FtpPutFile][OK!]");
				File::deleteFileT(l_bz2_name);
				return true;
			}
			else
			{
				TCHAR l_msg[1000]; l_msg[0] = 0;
				_sntprintf(l_msg, sizeof(l_msg),_T("Ошибка передачи файлов на сервер разрабоитчиков\r\nЕсли не сложно вышлите их по почте на адрес ddd-dc@li.ru!\r\n")
					_T("Код ошибки GetLastError() = %d [%s]\r\n")
					_T("Файл l_bz2_name = %s\r\n")
					_T("Файл l_guid_str = %s\r\n"),
					GetLastError(),
					Text::toT(Util::translateError(GetLastError())).c_str(),
					l_bz2_name.c_str(),
					l_guid_str.c_str()
					);
				::MessageBox(NULL,l_msg, _T("Ошибка передачи дампа на ftp-сервер разработчиков"), MB_ICONSTOP | MB_OK);
				LogManager::getInstance()->getInstance()->message("[stack-dmp-sender][FtpPutFile][Error] " + Util::translateError(GetLastError()));
		}
		}
		else
			LogManager::getInstance()->getInstance()->message("[stack-dmp-sender][FtpSetCurrentDirectory][Error] " + Util::translateError(GetLastError()));
	}
	return false;
}

static void send_stack_dmp()
{
	try
	{
		extern std::wstring g_strHomeDir;
		const string l_path = Text::fromT(g_strHomeDir.c_str()); //[!]PPA .c_str() не убирать!
		const StringList& l_mediainfo_files = File::findFiles(l_path, string("*-") + MEDIAINFO_CRASH_INFO_FILENAME, false);
		StringList l_files = File::findFiles(l_path, "crash-ddd++-stack-*r*.dmp", false);
		string l_mediainfo_file;
		if (!l_mediainfo_files.empty())
		{
			l_mediainfo_file = l_mediainfo_files[0];
			l_files.push_back(l_mediainfo_file);
		}
		if (!l_files.empty())
		{
		bool l_silient_send_flag = false;
		for (StringIterC i = l_files.cbegin(); i != l_files.cend(); ++i)
		{
			if (SendFileToServer(Text::toT(l_path + *i), l_mediainfo_file, l_silient_send_flag))
				{
				File::deleteFile(l_path + *i);
					if (!l_mediainfo_file.empty())
					{
						l_mediainfo_file = "";
						l_silient_send_flag = true;
						if (SendFileToServer(Text::toT(l_path + l_mediainfo_file), l_mediainfo_file, l_silient_send_flag))
							File::deleteFile(l_path + l_mediainfo_file);
						else
							LogManager::getInstance()->getInstance()->message("[stack-dmp-sender] error SendFileToServer: " + l_path + l_mediainfo_file);
					}
				}
			else
				LogManager::getInstance()->getInstance()->message("[stack-dmp-sender] error SendFileToServer: " + l_path + *i);
		}
}
	}
	catch (const Exception& e)
	{
		LogManager::getInstance()->message("[stack-dmp-sender] exception = " + e.getError());
	}
}
#endif // NIGHT_ORION_SEND_STACK_DUMP_TO_SERVER

static int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	checkCommonControls();

	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);	

	CEdit dummy;
	CWindow splash;
	
	CRect rc;
	rc.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	rc.top = (rc.bottom / 2) - 80;

	rc.right = GetSystemMetrics(SM_CXFULLSCREEN);
	rc.left = rc.right / 2 - 85;
	
	dummy.Create(NULL, rc, _T(APPNAME) _T(" ") T_VERSIONSTRING, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_CENTER | ES_READONLY, WS_EX_STATICEDGE);
	splash.Create(_T("Static"), GetDesktopWindow(), splash.rcDefault, NULL, WS_POPUP | WS_VISIBLE | SS_USERITEM | WS_EX_TOOLWINDOW);
	splash.SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	
	HDC dc = splash.GetDC();
	rc.right = rc.left + 550;
	rc.bottom = rc.top + 200;
	splash.ReleaseDC(dc);
	splash.HideCaret();
	splash.SetWindowPos(NULL, &rc, SWP_SHOWWINDOW);
	splash.SetWindowLongPtr(GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&splashCallback));
	splash.CenterWindow();

	sTitle = _T("");
/*	sTitle = T_VERSIONSTRING;
	sTitle += _T(" (SQLite ") _T(SQLITE_VERSION) _T(")");
#ifdef _WIN64	
	sTitle += _T("x64");
#endif*/
	splash.SetFocus();
	splash.RedrawWindow();

	startup(callBack, (void*)splash.m_hWnd);
#ifdef NIGHT_ORION_SEND_STACK_DUMP_TO_SERVER
	send_stack_dmp();
#endif

	splash.DestroyWindow();
	dummy.DestroyWindow();

	if(ResourceManager::getInstance()->isRTL()) {
		SetProcessDefaultLayout(LAYOUT_RTL);
	}

	MappingManager::getInstance()->addMapper<Mapper_NATPMP>();
	MappingManager::getInstance()->addMapper<Mapper_MiniUPnPc>();
	MappingManager::getInstance()->addMapper<Mapper_WinUPnP>();

	MainFrame wndMain;

	rc = wndMain.rcDefault;

	if( (SETTING(MAIN_WINDOW_POS_X) != CW_USEDEFAULT) &&
		(SETTING(MAIN_WINDOW_POS_Y) != CW_USEDEFAULT) &&
		(SETTING(MAIN_WINDOW_SIZE_X) != CW_USEDEFAULT) &&
		(SETTING(MAIN_WINDOW_SIZE_Y) != CW_USEDEFAULT) ) {

		rc.left = SETTING(MAIN_WINDOW_POS_X);
		rc.top = SETTING(MAIN_WINDOW_POS_Y);
		rc.right = rc.left + SETTING(MAIN_WINDOW_SIZE_X);
		rc.bottom = rc.top + SETTING(MAIN_WINDOW_SIZE_Y);
		// Now, let's ensure we have sane values here...
		if( (rc.left < 0 ) || (rc.top < 0) || (rc.right - rc.left < 10) || ((rc.bottom - rc.top) < 10) ) {
			rc = wndMain.rcDefault;
		}
	}

	int rtl = ResourceManager::getInstance()->isRTL() ? WS_EX_RTLREADING : 0;
	if(wndMain.CreateEx(NULL, rc, 0, rtl | WS_EX_APPWINDOW | WS_EX_WINDOWEDGE) == NULL) {
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}
	
	if(BOOLSETTING(MINIMIZE_ON_STARTUP)) {
		wndMain.ShowWindow(SW_SHOWMINIMIZED);
	} else {
		wndMain.ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? SETTING(MAIN_WINDOW_STATE) : nCmdShow);
	}
	int nRet = theLoop.Run();
	
	_Module.RemoveMessageLoop();

	shutdown();

	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
    InitHomeDirFileName();

    g_isWineCheck = is_wine();

#ifndef _DEBUG
	SingleInstance dcapp(_T("{DDDCPLUS-AEE8350A-B49A-4753-AB4B-E55479A48351}"));
#else
	SingleInstance dcapp(_T("{DDDCPLUS-AEE8350A-B49A-4753-AB4B-E55479A48350}"));
#endif

	if(dcapp.IsAnotherInstanceRunning()) {
		// Allow for more than one instance...
				bool multiple = false;
		if(_tcslen(lpstrCmdLine) == 0) { //-V805
			if (::MessageBox(NULL, _T("There is already an instance of StrongDC++ sqlite running.\nDo you want to launch another instance anyway?"), 
				_T(APPNAME) _T(" ") T_VERSIONSTRING, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST) == IDYES) {
					multiple = true;
			}
		}
    extern bool g_DisableSQLiteWAL;
	if (_tcsstr(lpstrCmdLine, _T("/nowal")) != NULL)
	    g_DisableSQLiteWAL = true;

		if(multiple == false) {
			HWND hOther = NULL;
			EnumWindows(searchOtherInstance, (LPARAM)&hOther);

			if( hOther != NULL ) {
				// pop up
				::SetForegroundWindow(hOther);

				if( IsIconic(hOther)) {
					// restore
					::ShowWindow(hOther, SW_RESTORE);
				}
				sendCmdLine(hOther, lpstrCmdLine);
			}
			return FALSE;
		}
	}
	
	// For SHBrowseForFolder, UPnP_COM
	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); 
	ATLASSERT(SUCCEEDED(hRes)); // [+] IRainman

	// brain-ripper: init dbghlp.dll to use MiniDumpWriteDump in exception filter.
	HMODULE hDbgHlp = LoadLibrary(L"DbgHelp.dll");
	if (hDbgHlp)
	{
		g_pMiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress(hDbgHlp, "MiniDumpWriteDump");
		
		if (!g_pMiniDumpWriteDump)
		{
			FreeLibrary(hDbgHlp);
			hDbgHlp = NULL;
		}
	}

	
	LPTOP_LEVEL_EXCEPTION_FILTER pOldSEHFilter = NULL;
	pOldSEHFilter = SetUnhandledExceptionFilter(&DCUnhandledExceptionFilter);
	
	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES |
		ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_USEREX_CLASSES);	// add flags to support other controls
	
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));
	
	HINSTANCE hInstRich = ::LoadLibrary(_T("RICHED20.DLL"));	
	if (!hInstRich)
		hInstRich = ::LoadLibrary(_T("RICHED32.DLL"));

	const int nRet = Run(lpstrCmdLine, nCmdShow);
 
	if ( hInstRich ) {
		::FreeLibrary(hInstRich);
	}
	
	// Return back old VS SEH handler
	if (pOldSEHFilter != NULL)
		SetUnhandledExceptionFilter(pOldSEHFilter);
	
	g_pMiniDumpWriteDump = NULL;
	if (hDbgHlp)
		FreeLibrary(hDbgHlp);
	_Module.Term();
	::CoUninitialize();

	return nRet;
}

/**
 * @file
 * $Id: main.cpp 572 2011-07-27 19:28:15Z bigmuscle $
 */
