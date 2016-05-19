#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "RangesPage.h"
#include "RangeDlg.h"

#include "../client/SettingsManager.h"
#include "../client/version.h"
#include "../client/IpGuard.h"
#include "../client/PGLoader.h"

#define BUFLEN 256

PropPage::TextItem RangesPage::texts[] = {
	{ IDC_DEFAULT_POLICY_STR, ResourceManager::IPGUARD_DEFAULT_POLICY },
	{ IDC_ENABLE_IPGUARD, ResourceManager::IPGUARD_ENABLE },
	{ IDC_INTRO_IPGUARD, ResourceManager::IPGUARD_INTRO },
	{ IDC_ADD, ResourceManager::ADD },
	{ IDC_CHANGE, ResourceManager::SETTINGS_CHANGE },
	{ IDC_REMOVE, ResourceManager::REMOVE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item RangesPage::items[] = {
	{ IDC_ENABLE_IPGUARD, SettingsManager::ENABLE_IPGUARD, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

LRESULT RangesPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CRect rc;

	ctrlRanges.Attach(GetDlgItem(IDC_RANGE_ITEMS));
	ctrlRanges.GetClientRect(rc);

	ctrlRanges.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, (rc.Width() / 2) - 21, 0);
	ctrlRanges.InsertColumn(1, CTSTRING(START), LVCFMT_LEFT, (rc.Width() / 4) + 2, 1);
	ctrlRanges.InsertColumn(2, CTSTRING(END), LVCFMT_LEFT, (rc.Width() / 4) + 2, 2);
	ctrlRanges.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

	ctrlPolicy.Attach(GetDlgItem(IDC_DEFAULT_POLICY));
	ctrlPolicy.AddString(CTSTRING(ALLOW));
	ctrlPolicy.AddString(CTSTRING(DENY));
	ctrlPolicy.SetCurSel(BOOLSETTING(DEFAULT_POLICY) ? 1 : 0);

	// Do specialized reading here

	TStringList cols;
	const IpGuard::RangeList& ranges = IpGuard::getInstance()->getRanges();
	for(IpGuard::RangeIter j = ranges.begin(); j != ranges.end(); ++j) {
		cols.push_back(Text::toT(j->name));
		cols.push_back(Text::toT(j->start.str()));
		cols.push_back(Text::toT(j->end.str()));
		ctrlRanges.insert(cols, 0, (LPARAM)j->id);
		cols.clear();
	}

	fixControls(false);
        try {
                m_IPFilterPATH = Util::getPath(Util::PATH_USER_CONFIG) + "IPTrust.ini";
                string data = File(m_IPFilterPATH, File::READ, File::OPEN).read();
				m_IPFilter = data;
            	SetDlgItemText(IDC_FLYLINK_TRUST_IP, Text::toT(data).c_str());
            	SetDlgItemText(IDC_FLYLINK_PATH, Text::toT(m_IPFilterPATH).c_str());
        } 
				
		catch(const FileException&) { 
			SetDlgItemText(IDC_FLYLINK_PATH, _T("IPTrust.ini not found...")); 
        }
		//[+] Drakon
//			SetDlgItemText(IDC_FLYLINK_TRUST_IP_BOX, CTSTRING(IPFILTER_GROUPBOX));
//			SetDlgItemText(IDC_FLYLINK_PATH_WAY, CTSTRING(IPFILTER_PATH_WAY)); 
//			SetDlgItemText(IDC_FLYLINK_TP_IP, CTSTRING(IPFILTER_CUR_RULES)); 
	// Do specialized reading here
	return TRUE;

	return TRUE;
}

LRESULT RangesPage::onAdd(WORD , WORD , HWND , BOOL& ) {
	RangeDlg dlg;

	if(dlg.DoModal() == IDOK) {
		try {
			IpGuard::getInstance()->addRange(dlg.name, dlg.start, dlg.end);
		} catch(const Exception &e) {
			MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") T_VERSIONSTRING, MB_ICONSTOP | MB_OK);
			return 0;
		}
		updateItems();
	}
	return 0;
}

LRESULT RangesPage::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_CHANGE), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	return 0;
}

LRESULT RangesPage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_ADD, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT RangesPage::onChange(WORD , WORD , HWND , BOOL& ) {
	if(ctrlRanges.GetSelectedCount() == 1) {
		int sel = ctrlRanges.GetSelectedIndex();
		TCHAR buf[BUFLEN];
		RangeDlg dlg;
		ctrlRanges.GetItemText(sel, 0, buf, BUFLEN);
		dlg.name = Text::fromT(buf);
		ctrlRanges.GetItemText(sel, 1, buf, BUFLEN);
		dlg.start = Text::fromT(buf);
		ctrlRanges.GetItemText(sel, 2, buf, BUFLEN);
		dlg.end = Text::fromT(buf);

		if(dlg.DoModal() == IDOK) {
			try {
				IpGuard::getInstance()->updateRange(ctrlRanges.GetItemData(sel), dlg.name, dlg.start, dlg.end);
			} catch(const Exception &e) {
				MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") T_VERSIONSTRING, MB_ICONSTOP | MB_OK);
				return 0;
			}
			updateItems(sel);
		}
	}

	return 0;
}

LRESULT RangesPage::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlRanges.GetSelectedCount() == 1) {
		int sel = ctrlRanges.GetSelectedIndex();
		IpGuard::getInstance()->removeRange(ctrlRanges.GetItemData(sel));
		ctrlRanges.DeleteItem(sel);
	}
	return 0;
}

void RangesPage::write() {
	PropPage::write((HWND)*this, items);
	settings->set(SettingsManager::DEFAULT_POLICY, ctrlPolicy.GetCurSel());
	AutoArray<TCHAR> l_buf(0xFFFFF);
	if(GetDlgItemText(IDC_FLYLINK_TRUST_IP, l_buf.get(), 0xFFFFF)) 
	{
		string l_new = Text::fromT(l_buf.get());
		if(l_new != m_IPFilter)
		{
		try {
			File fout(m_IPFilterPATH, File::WRITE, File::CREATE | File::TRUNCATE);
			fout.write(l_new);
			fout.close();
			PGLoader::getInstance()->LoadIPFilters();
		} catch(const FileException& ) {
			return;
		}
		}
	}
}

void RangesPage::updateItems(int aSel /*= -1*/) {
	ctrlRanges.SetRedraw(FALSE);
	ctrlRanges.DeleteAllItems();

	TStringList cols;
	const IpGuard::RangeList& ranges = IpGuard::getInstance()->getRanges();
	for(IpGuard::RangeIter j = ranges.begin(); j != ranges.end(); ++j) {
		cols.push_back(Text::toT(j->name));
		cols.push_back(Text::toT(j->start.str()));
		cols.push_back(Text::toT(j->end.str()));
		ctrlRanges.insert(cols, 0, (LPARAM)j->id);
		cols.clear();
	}

	if(aSel != -1) ctrlRanges.SelectItem(aSel);
	ctrlRanges.SetRedraw(TRUE);
}

void RangesPage::fixControls(bool aUpdate) {
	bool state = (IsDlgButtonChecked(IDC_ENABLE_IPGUARD) != 0);
	::EnableWindow(GetDlgItem(IDC_INTRO_IPGUARD), state);
	::EnableWindow(GetDlgItem(IDC_DEFAULT_POLICY_STR), state);
	::EnableWindow(GetDlgItem(IDC_ADD), state);
	::EnableWindow(GetDlgItem(IDC_RANGE_ITEMS), state);
	::EnableWindow(GetDlgItem(IDC_DEFAULT_POLICY), state);

	if(ctrlRanges.GetSelectedIndex() != -1) {
		::EnableWindow(GetDlgItem(IDC_CHANGE), state);
		::EnableWindow(GetDlgItem(IDC_REMOVE), state);
	}

	if(state && aUpdate) {
		if(IpGuard::getInstance()->load()) {
			updateItems();
		}
	}
}