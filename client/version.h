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
#include "..\revision.h"  // call UpdateRevision.bat (fatal error C1083: Cannot open include file: '..\revision.h': No such file or directory)

#ifdef _WIN64
#define A_POSTFIX64 "-x64"
#define L_POSTFIX64 L"-x64"
#else
#define A_POSTFIX64 ""
#define L_POSTFIX64 L""
#endif

#define APPNAME "DDD++"
  #define L_VERSIONSTRING L"" L_REVISION_STR L_POSTFIX64
  #define A_VERSIONSTRING  "" REVISION_STR   A_POSTFIX64
#ifdef UNICODE
#define T_VERSIONSTRING L_VERSIONSTRING
#else
#define T_VERSIONSTRING A_VERSIONSTRING
#endif

#define VERSIONFLOAT REVISION

#define DCVERSIONSTRING "0.785"
#define CURRENT_STRONG_DC_VERSION "2.43" //[+]IRainman DHT hack. Pleace always update this!
#ifdef PPA_INCLUDE_VERSION_CHECK
#define VERSION_URL "http://ddd-dc.ru/version.xml"
#endif

#define SVNVERSION "svn580"

#ifdef _WIN64
# define CONFIGURATION_TYPE "x86-64"
#else
# define CONFIGURATION_TYPE "x86-32"
#endif

#ifdef SVNVERSION
# define COMPLETEVERSIONSTRING	_T(APPNAME) _T(" ") _T(VERSIONSTRING) _T(" ") _T(CONFIGURATION_TYPE) _T(" ") _T(SVNVERSION) _T(" / ") _T(DCVERSIONSTRING)
#else
# define COMPLETEVERSIONSTRING	_T(APPNAME) _T(" ") _T(VERSIONSTRING) _T(" ") _T(CONFIGURATION_TYPE)
#endif

/* Update the .rc file as well... */

/**
 * @file
 * $Id: version.h 581 2011-11-02 18:59:46Z bigmuscle $
 */
