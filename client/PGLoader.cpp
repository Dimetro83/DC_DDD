#include "stdinc.h"
#include "DCPlusPlus.h"
#include "PGLoader.h"
#include "Util.h"
#include "File.h"
#include "SettingsManager.h"
#include "LogManager.h"

namespace dcpp {

PGLoader::PGLoader(){
	LoadIPFilters();
}
bool PGLoader::getIPBlockBool(const string& p_IP) const 
	{
        Lock l(m_cs); 
        if (m_IPTrust.size() == 0)
		    return false;	 
		unsigned u1, u2, u3, u4;
        const int iItems = sscanf_s(p_IP.c_str(), "%u.%u.%u.%u", &u1, &u2, &u3, &u4);
		uint32_t l_ipnum = 0;
        if(iItems == 4)  
           l_ipnum = (u1 << 24) + (u2 << 16) + (u3 << 8) + u4;
		bool l_isBlock = m_count_trust > 0;
		if(l_ipnum)
          for (CustomIPFilterList::const_iterator j = m_IPTrust.begin(); j != m_IPTrust.end(); ++j)
		     {
			  if(l_ipnum >= j->m_startIP && l_ipnum <= j->m_endIP)
			    {
#if 0 //TODO
					if(j->m_is_block)
					{
                      if(p_out_message.is_empty()) 
						  p_out_message = ""
					  p_out_message = j->m_message;
					}
#endif
                   if(j->m_is_block)
					  return true;
				   else
				      l_isBlock &= false;
			    }
		  }
       return l_isBlock;
}
void PGLoader::LoadIPFilters()
{
Lock l(m_cs); 
m_count_trust = 0;
m_IPTrust.clear();
try {
   string file = Util::getPath(Util::PATH_USER_CONFIG) + "IPTrust.ini";
   string data = File(file, File::READ, File::OPEN).read();
   data += " \n~~";
   string::size_type linestart = 0;
   string::size_type lineend = 0;
   for(;;) {
           lineend = data.find('\n', linestart);
           if (lineend == string::npos) break;
           const string line = data.substr(linestart, lineend - linestart - 1);
           	    int u1, u2, u3, u4, u5, u6, u7, u8;
                   if(const int iItems = sscanf_s(line.c_str(), "%u.%u.%u.%u - %u.%u.%u.%u", &u1, &u2, &u3, &u4, &u5, &u6, &u7, &u8))
				   if(iItems == 8 || iItems == 4)
				   {
                    CustomIPFilter l_IPRange;
					l_IPRange.m_is_block = u1 < 0;
					if(l_IPRange.m_is_block)
						u1 = -u1;
					else
						m_count_trust++;
                    l_IPRange.m_startIP = (u1 << 24) + (u2 << 16) + (u3 << 8) + u4;
#if PPA_INCLUDE_TODO
					const string::size_type l_start_comment = line.find(';',0);
					if(l_start_comment != string::npos)
						l_IPRange.m_message = line.substr(linestart, lineend - l_start_comment - 1);
#endif
					if(iItems == 4)
                      l_IPRange.m_endIP = l_IPRange.m_startIP;
				    else
                      l_IPRange.m_endIP = (u5 << 24) + (u6 << 16) + (u7 << 8) + u8;
                    m_IPTrust.push_back(l_IPRange);
					if(l_IPRange.m_is_block)
					  LogManager::getInstance()->message("IPTrust.ini Block:[" + line + "]");
					else
					  LogManager::getInstance()->message("IPTrust.ini Trust:[" + line + "]");
				   }
           linestart = lineend + 1;
   }
 } catch(const FileException&)
 {
 }
}

}