/* 
 * Copyright (C) 2009 Big Muscle
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
 
#include "stdinc.h"
#include "DCPlusPlus.h"

#include "StringPool.h"

namespace dcpp
{

	StringPool* pooled_string::pool = new StringPool();

	const std::string& StringPool::addString(const std::string& s) 
	{
		FastLock l(cs);
		
	StringMap::iterator i = sm.find(s);
	if(i == sm.end())
	{
		return sm.insert(std::make_pair(s, 1)).first->first;
	}
	else
	{
		i->second++;
		return i->first;
	}
	}

	void StringPool::removeString(const std::string& s)
	{
		FastLock l(cs);
		
	StringMap::iterator i = sm.find(s); 
		dcassert(i != sm.end());
	
		if(--(i->second) == 0)
		sm.erase(i);
	}

}	// namespace dcpp