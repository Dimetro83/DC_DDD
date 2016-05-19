#ifndef RANGES_PAGE_H
#define RANGES_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class RangesPage : public CPropertyPage<IDD_RANGES>, public PropPage
{
public:
	RangesPage(SettingsManager *s) : PropPage(s) {
//		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_CERTIFICATES) + _T('\\') + TSTRING(IPGUARD)).c_str());
				title = _tcsdup(TSTRING(IPGUARD).c_str());
		SetTitle(title);
	};

	~RangesPage() { 
		ctrlRanges.Detach();
		ctrlPolicy.Detach();
		free(title);
	};

	BEGIN_MSG_MAP_EX(RangesPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_CHANGE, onChange)
		NOTIFY_HANDLER(IDC_RANGE_ITEMS, NM_DBLCLK, onDblClick)
		NOTIFY_HANDLER(IDC_RANGE_ITEMS, LVN_ITEMCHANGED, onItemchangedDirectories)
		NOTIFY_HANDLER(IDC_RANGE_ITEMS, LVN_KEYDOWN, onKeyDown)
//		NOTIFY_HANDLER(IDC_RANGE_ITEMS, NM_CUSTOMDRAW, ctrlRanges.onCustomDraw)
		COMMAND_ID_HANDLER(IDC_ENABLE_IPGUARD, onFixControls)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);

	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT onFixControls(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		fixControls(ctrlRanges.GetItemCount() == 0);
		return 0;
	}

	LRESULT onDblClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

		if(item->iItem >= 0) {
			PostMessage(WM_COMMAND, IDC_CHANGE, 0);
		} else if(item->iItem == -1) {
			PostMessage(WM_COMMAND, IDC_ADD, 0);
		}

		return 0;
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
protected:
	ExListViewCtrl ctrlRanges;
	CComboBox ctrlPolicy;

	static Item items[];
	static TextItem texts[];
	TCHAR* title;
	void fixControls(bool aUpdate);
	void updateItems(int aSel = -1);
private:
    string m_IPFilter;
	string m_IPFilterPATH;

};

#endif // RANGES_PAGE_H