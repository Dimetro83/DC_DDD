// ZenLib::OS_Utils - Cross platform OS utils
// Copyright (C) 2002-2011 MediaArea.net SARL, Info@MediaArea.net
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef ZenOS_UtilsH
#define ZenOS_UtilsH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Conf.h"
#include "ZenLib/Ztring.h"
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
// OS Information
//***************************************************************************

//---------------------------------------------------------------------------
// [!] FlylinkDC opt: We will help the optimizer to throw out the check and the code of it depends on your code: add "inline const static" and move to header to garantee inline implementation on this function.
inline static const bool IsWin9X  ()
{
    #ifdef ZENLIB_USEWX
        return true;
    #else //ZENLIB_USEWX
        #ifdef WINDOWS
			#ifdef ZENLIB_USE_IN_WIN98
	            if (GetVersion() >= 0x80000000)
		            return true;
			    else
			#endif //ZENLIB_USE_IN_WIN98
				    return false;
        #else //WINDOWS
            return true;
        #endif
    #endif //ZENLIB_USEWX
}
#ifdef WINDOWS
#ifndef ZENLIB_NO_WIN9X_SUPPORT
inline bool IsWin9X_Fast ()
{
    return GetVersion()>=0x80000000;
}
#endif //ZENLIB_NO_WIN9X_SUPPORT
#endif //WINDOWS

//***************************************************************************
// Execute
//***************************************************************************

void Shell_Execute(const Ztring &ToExecute);

//***************************************************************************
// Directorues
//***************************************************************************

Ztring OpenFolder_Show(void* Handle, const Ztring &Title, const Ztring &Caption);

} //namespace ZenLib
#endif
