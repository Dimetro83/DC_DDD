#ifndef PGLOADER_H
#define PGLOADER_H

#include <string>
#include <vector>

#include "Singleton.h"
#include "Thread.h"


namespace dcpp {
class PGLoader : public Singleton<PGLoader>{
public:
	PGLoader();
	~PGLoader()
	{
	}
	bool getIPBlockBool(const string& p_IP) const;
    void LoadIPFilters();
private:
       mutable CriticalSection m_cs;
       struct CustomIPFilter
		{
                uint32_t m_startIP; 
				uint32_t m_endIP;
				bool     m_is_block;
#ifdef PPA_INCLUDE_TODO
				string   m_message;
#endif
  	    };
        typedef vector<CustomIPFilter> CustomIPFilterList;
        CustomIPFilterList m_IPTrust;
		uint32_t m_count_trust;
};
}
#endif
