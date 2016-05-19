/**
*    Wrapper для HIcon
*/

/*
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

#ifndef _H_ICON_WRAPPER_H_
#define _H_ICON_WRAPPER_H_

#pragma once

class HIconWrapper
{
	public:
		HIconWrapper(LPCWSTR name, HMODULE hInst = NULL, int cx = 16, int cy = 16, UINT fuLoad = LR_DEFAULTCOLOR)
			: m_id(0), m_name(name), m_hInst(hInst), m_cx(cx), m_cy(cy), m_fuLoad(fuLoad), m_icon(NULL) {}
		HIconWrapper(WORD id, HMODULE hInst = NULL, int cx = 16, int cy = 16, UINT fuLoad = LR_DEFAULTCOLOR)
			: m_id(id), m_hInst(hInst), m_cx(cx), m_cy(cy), m_fuLoad(fuLoad), m_icon(NULL) {}
		HIconWrapper(HICON p_icon) : m_icon(p_icon)
		{
		}
		~HIconWrapper()
		{
			if (m_icon)
			{
				const BOOL l_res = DestroyIcon(m_icon);
				dcassert(l_res);
				m_icon = NULL;
			}
		}
		
		operator HICON()
		{
			if (!m_icon)
				load();
			return m_icon;
		}
		
	private:
		void HIconWrapper::load()
		{
			if (m_icon)
				return; // don't call twice
			if (m_id)
			{
				m_icon = (HICON)::LoadImage(m_hInst, MAKEINTRESOURCE(m_id), IMAGE_ICON, m_cx, m_cy, m_fuLoad); //TODO Crash wine
				if (!m_icon && m_hInst != GetModuleHandle(NULL))
				{
					m_icon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(m_id), IMAGE_ICON, m_cx, m_cy, m_fuLoad);
				}
			}
			else
			{
				m_icon = (HICON)::LoadImage(m_hInst, m_name, IMAGE_ICON, m_cx, m_cy, m_fuLoad);
				if (!m_icon && m_hInst != NULL)
				{
					m_icon = (HICON)::LoadImage(NULL , m_name, IMAGE_ICON, m_cx, m_cy, m_fuLoad);
				}
				
			}
		}
		
	private:
		HICON m_icon;
		WORD m_id;
		LPCWSTR m_name;
		int m_cx;
		int m_cy;
		UINT m_fuLoad;
		HMODULE m_hInst;
};


#endif //_H_ICON_WRAPPER_H_