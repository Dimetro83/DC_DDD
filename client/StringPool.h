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
 
#ifndef _STRINGPOOL_H
#define _STRINGPOOL_H

#include "CriticalSection.h"

namespace dcpp
{

	class StringPool 
	{

	typedef std::tr1::unordered_map<std::string, size_t> StringMap;
    StringMap sm;
	    
	    FastCriticalSection cs;	// I hope this is enough and normal CS won't be needed

	public:

    StringPool() 
    {
    }

    const std::string& addString(const std::string& s);
    
    void removeString(const std::string& s);
	};

	class pooled_string 
	{

    const std::string* data;
	static StringPool* pool;

	public:

	pooled_string() : data(NULL) 
	{
	}
	
		pooled_string(const pooled_string& s) : data(NULL)
		{
			if(s.data)
				put(*s.data);
		}
	
		~pooled_string()
		{ 
			if(data) 
				pool->removeString(*data); 
		}		
	
		pooled_string& operator=(const std::string& s) 
		{ 
			if(data == NULL || s != *data)
			{
				if(data) 
					pool->removeString(*data);
	
				put(s); 
			}
			return *this;
		}

		pooled_string& operator=(const pooled_string& sp) 
		{ 
			return operator=(*sp.data); 
		}
		
		operator const std::string&() const 
		{ 
			return get(); 
		}

		void put(const std::string& s) 
		{ 
			data = &pool->addString(s); 
		}
		
		const std::string& get() const 
		{ 
			return *data; 
		}

	};
	
}	// namespace dcpp

#endif	// _STRINGPOOL_H