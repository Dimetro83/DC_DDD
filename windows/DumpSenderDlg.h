#if !defined(DUMP_SENDER_DLG_H)
#define DUMP_SENDER_DLG_H

#ifdef NIGHT_ORION_SEND_STACK_DUMP_TO_SERVER
#include "WinUtil.h"
#include "../client/ResourceManager.h"
// for Custom Themes Bitmap
#include "Resource.h"
#include "ResourceLoader.h"
#include "ExListViewCtrl.h"
#include "HIconWrapper.h"
#include "wtl_flylinkdc.h"

namespace dcpp {

class CDumpSenderDlg : public CDialogImpl<CDumpSenderDlg>
{
	public:
		enum { IDD = IDD_DUMP_SENDER };
		CDumpSenderDlg(const wstring& p_DumpFileName, const string& p_mediainfo_crash_info)
			: m_DumpFileName(p_DumpFileName), m_mediainfo_crash_info(p_mediainfo_crash_info)
		{
		}
		
		~CDumpSenderDlg()
		{
			ctrlCommands.Detach();
		}
		
		BEGIN_MSG_MAP(CDumpSenderDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDYES, BN_CLICKED, OnOk)
		COMMAND_HANDLER(IDNO, BN_CLICKED, OnCancel)
		COMMAND_ID_HANDLER(IDC_DUMP_LINK, onLink)
		END_MSG_MAP()

		LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
		{
			SetWindowText(CTSTRING(FTP_SEND_HEADER));
			SetDlgItemText(IDC_DUMP_SEND_QUESTION, CTSTRING(FTP_SEND_QUESTION));
			SetDlgItemText(IDC_DUMP_SEND_QUESTION2, CTSTRING(FTP_SEND_QUESTION2));
			SetDlgItemText(IDYES, CTSTRING(YES));
			SetDlgItemText(IDNO, CTSTRING(NO));
			m_hIcon = std::unique_ptr<HIconWrapper>(new HIconWrapper(IDR_ERROR));
			SetIcon(*m_hIcon, FALSE);
			SetIcon(*m_hIcon, TRUE);
			// for Custom Themes Bitmap
			img.LoadFromResource(IDR_ERROR_PNG, _T("PNG"));
			GetDlgItem(IDC_STATIC).SendMessage(STM_SETIMAGE, IMAGE_BITMAP, LPARAM((HBITMAP)img));
			
			SetDlgItemText(IDC_DUMP_FOOTER, CTSTRING(PATH));
			SetDlgItemText(IDC_DUMP_FOOTER_TEXT, Util::getFilePath(m_DumpFileName).c_str());
			CenterWindow();
			
			CRect rc;
			ctrlCommands.Attach(GetDlgItem(IDC_DUMP_DETAILS));
			ctrlCommands.GetClientRect(rc);
			ctrlCommands.InsertColumn(0, CTSTRING(FILE), LVCFMT_LEFT, (rc.Width() / 8) * 6, 0);
			ctrlCommands.InsertColumn(1, CTSTRING(SIZE), LVCFMT_LEFT, rc.Width() / 8, 1);
			ctrlCommands.InsertColumn(2, _T("Traffic"), LVCFMT_LEFT, rc.Width() / 8, 2);
			ctrlCommands.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);
			int i = ctrlCommands.insert(ctrlCommands.GetItemCount(), FormatFileName().c_str());
			ctrlCommands.SetItemText(i, 1, _T("140 KB"));
			ctrlCommands.SetItemText(i, 2, _T("20 KB"));
			if (!m_mediainfo_crash_info.empty())
			{
				const tstring l_name = Text::toT(m_mediainfo_crash_info);
				if (l_name != FormatFileName())
				{
					i = ctrlCommands.insert(ctrlCommands.GetItemCount(), Text::toT(m_mediainfo_crash_info).c_str());
					ctrlCommands.SetItemText(i, 1, _T("100 B"));
					ctrlCommands.SetItemText(i, 2, _T("20 B"));
				}
			}
			
			SetDlgItemText(IDC_DUMP_LINK, CTSTRING(FTP_SEND_LINK));
			m_url.init(GetDlgItem(IDC_DUMP_LINK), CTSTRING(FTP_SEND_LINK));
			
			return TRUE;
		}
		
		std::wstring FormatFileName() const
		{
			const wstring str = m_DumpFileName;
			return Util::getFileName(str);
		}
		
		LRESULT OnOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
		{
			EndDialog(IDYES);
			return 0;
		}
		
		LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
		{
			EndDialog(IDNO);
			return 0;
		}
		
		LRESULT onLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
		{
			WinUtil::openLink(_T("http://www.flylinkdc.ru/2012/05/blog-post.html")); 
			return 0;
		}
		
	private:
		ExCImage img;
		wstring m_DumpFileName;
		string  m_mediainfo_crash_info;
		ExListViewCtrl ctrlCommands;
		std::unique_ptr<HIconWrapper> m_hIcon;
		CFlyHyperLink m_url;
		CDumpSenderDlg(const CDumpSenderDlg&);
		void operator=(const CDumpSenderDlg&);
};
}
#endif // NIGHT_ORION_SEND_STACK_DUMP_TO_SERVER
#endif // !defined(DUMP_SENDER_DLG_H)
