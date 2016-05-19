/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_COMPILER_H
#define DCPLUSPLUS_DCPP_COMPILER_H

#if defined(__GNUC__)
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5)
#error GCC 4.5 is required

#endif

#elif defined(_MSC_VER)
#if _MSC_VER < 1600
#error MSVC 10 (2010) is required
#endif

#define _SECURE_SCL_THROWS 0

#ifndef _DEBUG
#define _SECURE_SCL  0
#define _ITERATOR_DEBUG_LEVEL 0 // VC++ 2010
#define _HAS_ITERATOR_DEBUGGING 0
#else
#define _SECURE_SCL  1
#define _ITERATOR_DEBUG_LEVEL 2 // VC++ 2010 
#define _HAS_ITERATOR_DEBUGGING 1
#endif

#define memzero(dest, n) memset(dest, 0, n)

//disable the deprecated warnings for the CRT functions.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _ATL_SECURE_NO_DEPRECATE 1
#define _CRT_NON_CONFORMING_SWPRINTFS 1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

#define strtoll _strtoi64

#define snprintf _snprintf
#define snwprintf _snwprintf

#else
#error No supported compiler found

#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#define _LL(x) x##ll
#define _ULL(x) x##ull
#define I64_FMT "%I64d"
#define U64_FMT "%I64u" // PVS-Studio] V576. Incorrect format. Consider checking the N actual argument of the 'Foo' function

#elif defined(SIZEOF_LONG) && SIZEOF_LONG == 8
#define _LL(x) x##l
#define _ULL(x) x##ul
#define I64_FMT "%ld"
#define U64_FMT "%lu" // PVS-Studio] V576. Incorrect format. Consider checking the N actual argument of the 'Foo' function
#else
#define _LL(x) x##ll
#define _ULL(x) x##ull
#define I64_FMT "%lld"
#define U64_FMT "%llu" // PVS-Studio] V576. Incorrect format. Consider checking the N actual argument of the 'Foo' function
#endif

#ifndef _REENTRANT
# define _REENTRANT 1
#endif

# pragma warning(disable: 4996)
# pragma warning(disable: 4711) // function 'xxx' selected for automatic inline expansion
# pragma warning(disable: 4786) // identifier was truncated to '255' characters in the debug information
# pragma warning(disable: 4290) // C++ Exception Specification ignored
# pragma warning(disable: 4127) // constant expression
# pragma warning(disable: 4710) // function not inlined
# pragma warning(disable: 4503) // decorated name length exceeded, name was truncated
# pragma warning(disable: 4428) // universal-character-name encountered in source
# pragma warning(disable: 4201) // nonstadard extension used : nameless struct/union
//[+]PPA
# pragma warning(disable: 4244) // 'argument' : conversion from 'int' to 'unsigned short', possible loss of data
# pragma warning(disable: 4512) // 'boost::detail::future_object_base::relocker' : assignment operator could not be generated
# pragma warning(disable: 4100) //unreferenced formal parameter

#ifdef _WIN64
# pragma warning(disable: 4244) // conversion from 'xxx' to 'yyy', possible loss of data
# pragma warning(disable: 4267) // conversion from 'xxx' to 'yyy', possible loss of data
#endif

//[+]PPA
typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;

typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#endif // DCPLUSPLUS_DCPP_COMPILER_H
