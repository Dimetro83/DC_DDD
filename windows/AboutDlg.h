/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(ABOUT_DLG_H)
#define ABOUT_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlctrlx.h>

static const TCHAR g_thanks[] = 
_T("BigMuscle  (http://strongdc.sourceforge.net)\r\n")
_T("Pavel Pimenov  (http://code.google.com/p/flylinkdc)\r\n")
_T("KauH\r\n")
;
class AboutDlg : public CDialogImpl<AboutDlg>
{
public:
	enum { IDD = IDD_ABOUTBOX };

	AboutDlg() { }
		virtual ~AboutDlg() { }

	BEGIN_MSG_MAP(AboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_LINK_BLOG, onBlogLink)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		SetDlgItemText(IDC_VERSION, 
	   _T("DDD++ ") T_VERSIONSTRING
	   _T("\r\nDamaks © 2011-2012")
 	   _T("\r\n\r\nЛёгкий и быстрый DC++ клиент"));


	   CEdit ctrlThanks(GetDlgItem(IDC_THANKS));
		ctrlThanks.FmtLines(TRUE);
		ctrlThanks.AppendText(g_thanks, TRUE);
		ctrlThanks.Detach();
#ifdef FLY_INCLUDE_EXE_TTH
		SetDlgItemText(IDC_TTH, WinUtil::tth.c_str());
#endif
		SetDlgItemText(IDC_TOTALS, (_T("Upload: ") + Util::formatBytesW(SETTING(TOTAL_UPLOAD)) + _T(", Download: ") + 
			Util::formatBytesW(SETTING(TOTAL_DOWNLOAD))).c_str());

			SetDlgItemText(IDC_LINK_BLOG, _T("http://ddd-dc.ru"));
			m_url_blog.SubclassWindow(GetDlgItem(IDC_LINK_BLOG));
			m_url_blog.SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON | HLINK_UNDERLINEHOVER);

		if(SETTING(TOTAL_DOWNLOAD) > 0) {
			TCHAR buf[64];
			snwprintf(buf, sizeof(buf), _T("Ratio (up/down): %.2f"), ((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD)));

			SetDlgItemText(IDC_RATIO, buf);
		/*	sprintf(buf, "Uptime: %s", Util::formatTime(Util::getUptime()));
			SetDlgItemText(IDC_UPTIME, Text::toT(buf).c_str());*/
		}
		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT onVersionData(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		tstring* x = (tstring*) wParam;
		SetDlgItemText(IDC_LATEST, x->c_str());
		delete x;
		return 0;
	}
		LRESULT onBlogLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
		{
			WinUtil::openLink(_T("http://ddd-dc.ru"));
			return 0;
		}
		
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		EndDialog(wID);
		return 0;
	}

private:
	CHyperLink m_url_blog;
	AboutDlg(const AboutDlg&) { dcassert(0); }
	
};

#endif // !defined(ABOUT_DLG_H)

/**
 * @file
 * $Id: AboutDlg.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
