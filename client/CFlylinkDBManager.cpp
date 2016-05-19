//-----------------------------------------------------------------------------
//(c) 2007-2012 pavel.pimenov@gmail.com
//-----------------------------------------------------------------------------
#include "stdinc.h"
#include "DCPlusPlus.h"
#include "Pointer.h"
#include "CFlylinkDBManager.h"
#include "Util.h"
#include "SettingsManager.h"
#include "LogManager.h"
#include "ShareManager.h"
bool g_DisableSQLiteWAL    = false;
const char* g_media_ext[] = {
"3gp", 
"avi", 
"divx",
"flv", 
"m4v", 
"mkv", 
"dts",
"mov", 
"mp4", 
"mpg", 
"mpeg",
"vob", 
"wmv", 
"bik", 
"qt", 
"rm", 
"aac", 
"ac3", 
"ape", 
"fla", 
"flac",
"m4a", 
"mp1", 
"mp2", 
"mp3", 
"ogg",
"wma", 
"wv", 
"mka",
"vqf",
"lqt",
	"ts",
	"tp",
	"mod",
#if defined(USE_MEDIAINFO_IMAGES) // Картинки не анализируем
"tiff",
"tif",
"jpg",
"ico",
"jpeg",
"gif",
"bmp",
"png",
"pcx",
"tga",
#endif
 0 // Не убирать терминатор!
//[20:13:46] pavel.pimenov: а если кто-то добавит к char* g_media_ext[] = { в верхнем?
//[20:14:06 | Изменены 20:14:39] Третьяков Вячеслав: [20:13] pavel.pimenov: 
//<<< а если кто-то добавит к char* g_media_ext[] = { в верхнем?по лапам ему и по наглой морде ((blush))
};

//========================================================================================================
bool CFlylinkDBManager::safeAlter(const char* p_sql)
{
   try
	{ 
     m_flySQLiteDB.executenonquery(p_sql);
		return true;
    }
	catch(const database_error& e)
	{
		LogManager::getInstance()->message("safeAlter: " + e.getError());
	}
    return false;
}
//========================================================================================================
void CFlylinkDBManager::errorDB(const string& p_txt, bool p_critical)
{
	const string l_message = p_txt + string("\r\nВероятно, файл базны данных FlylinkDC.sqlite поврежден.\r\nВариант решения проблемы:\r\n\r\n"
		"* Провести диагностику и ремонт файла по инструкции http://flylinkdc.blogspot.com/2010/10/flylinkdcsqlite.html\r\n");
	MessageBox(NULL, Text::toT(l_message).c_str(), _T("ddd++.sqlite database error!"), MB_OK | MB_ICONERROR | MB_TOPMOST);
	LogManager::getInstance()->message(p_txt, true); // Всегда логируем в файл (т.к. база может быть битой)
	if(p_critical)
		exit(-1);
}
//========================================================================================================
CFlylinkDBManager::CFlylinkDBManager()
{
	m_last_path_id = -1;
	m_convert_ftype_stop_key = 0; 
	m_first_ratio_cache = false;
	m_DIC.resize(e_DIC_LAST-1); 
	const string l_pragma_page_size = "PRAGMA page_size=4096;";
	const string l_pragma_cache_size = "PRAGMA cache_size=2048;";
	const string l_pragma_default_cache_size = "PRAGMA default_cache_size=2048;";
		try
		{
		const string l_log_db_name = "ddd++_log.sqlite";
		const string l_log_db = string(Util::getPath(Util::PATH_USER_CONFIG) + l_log_db_name);
		if(!Util::fileExists(l_log_db))
		{
		 sqlite3_connection l_flySQLiteDB_LOG;
		 l_flySQLiteDB_LOG.open(l_log_db.c_str());
 		 l_flySQLiteDB_LOG.executenonquery(
		    "CREATE TABLE IF NOT EXISTS fly_log(id integer PRIMARY KEY AUTOINCREMENT,"
		    "sdate text not null, type integer not null,body text, hub text, nick text,\n"
		    "ip text, file text, source text, target text,fsize int64,fchunk int64,extra text,userCID text);");
		}

			 m_flySQLiteDB.open(string(Util::getPath(Util::PATH_USER_CONFIG) + "ddd++.sqlite").c_str());
             m_flySQLiteDB.executenonquery("PRAGMA page_size=4096;");
	     m_flySQLiteDB.executenonquery((g_DisableSQLiteWAL || BOOLSETTING(SQLITE_USE_JOURNAL_MEMORY)) ? "PRAGMA journal_mode=MEMORY;" : "PRAGMA journal_mode=WAL;");
             m_flySQLiteDB.executenonquery("PRAGMA temp_store=MEMORY;");
		m_flySQLiteDB.executenonquery("attach database '" + l_log_db + "' as LOGDB");
			 

		     m_flySQLiteDB.executenonquery("create table IF NOT EXISTS fly_revision(rev integer NOT NULL);");
            
			 m_flySQLiteDB.executenonquery("create table IF NOT EXISTS fly_tth(\n"
			                                "tth char(24) PRIMARY KEY NOT NULL);"); 
			 m_flySQLiteDB.executenonquery("create table IF NOT EXISTS fly_hash(\n"
			                                "id integer PRIMARY KEY AUTOINCREMENT NOT NULL, tth char(24) NOT NULL UNIQUE);");
             m_flySQLiteDB.executenonquery(
				 "CREATE TABLE IF NOT EXISTS fly_file(dic_path integer not null,\n"
				 "name text not null,\n" 
				 "size int64 not null,\n"
				 "stamp int64 not null,\n"
				 "tth_id int64 not null,\n"
				 "hit int64 default 0,\n"
				 "stamp_share int64 default 0,\n"
				 "bitrate integer default 0,\n"
		    "ftype integer default -1,\n"
		    "media_x integer,\n"
		    "media_y integer,\n"
		    "media_video text,\n"
		    "media_audio text\n"
				 ");");
		
			 m_flySQLiteDB.executenonquery("CREATE UNIQUE INDEX IF NOT EXISTS iu_fly_file_name ON fly_file(dic_path,name);");
			 m_flySQLiteDB.executenonquery("CREATE INDEX IF NOT EXISTS i_fly_file_tth_id ON fly_file(tth_id);");
			 //[-] TODO потестить на большой шаре
			 // m_flySQLiteDB.executenonquery("CREATE INDEX IF NOT EXISTS i_fly_file_dic_path ON fly_file(dic_path);");
             m_flySQLiteDB.executenonquery(
				 "CREATE TABLE IF NOT EXISTS fly_hash_block(tth_id integer PRIMARY KEY NOT NULL,\n"
				     "tiger_tree blob,file_size int64 not null,block_size int64);");
			 m_flySQLiteDB.executenonquery("create table IF NOT EXISTS fly_path(\n"
			 "id integer PRIMARY KEY AUTOINCREMENT NOT NULL, name text NOT NULL UNIQUE);");  
			 m_flySQLiteDB.executenonquery("create table IF NOT EXISTS fly_dic(\n"
		     "id integer PRIMARY KEY AUTOINCREMENT NOT NULL,dic integer NOT NULL, name text NOT NULL);");
			m_flySQLiteDB.executenonquery("CREATE UNIQUE INDEX IF NOT EXISTS iu_fly_dic_name ON fly_dic(name,dic);");
             m_flySQLiteDB.executenonquery(
				 "CREATE TABLE IF NOT EXISTS fly_ratio(id integer PRIMARY KEY AUTOINCREMENT NOT NULL,\n"
				    "dic_ip integer not null,dic_nick integer not null, dic_hub integer not null,\n"
				    "upload int64 default 0,download int64 default 0);");
			m_flySQLiteDB.executenonquery("CREATE UNIQUE INDEX IF NOT EXISTS iu_fly_ratio ON fly_ratio(dic_nick,dic_hub,dic_ip);");

			m_flySQLiteDB.executenonquery("CREATE VIEW IF NOT EXISTS v_fly_ratio AS\n"
		"SELECT fly_ratio.id id, fly_ratio.upload upload,\n"
		"fly_ratio.download download,\n"
		"userip.name userip,\n"
		"nick.name nick,\n"
		"hub.name hub\n"
		"FROM fly_ratio\n"
		"INNER JOIN fly_dic userip ON fly_ratio.dic_ip = userip.id\n"
	    "INNER JOIN fly_dic nick ON fly_ratio.dic_nick = nick.id\n"
        "INNER JOIN fly_dic hub ON fly_ratio.dic_hub = hub.id");

		m_flySQLiteDB.executenonquery("CREATE VIEW IF NOT EXISTS v_fly_ratio_all AS\n"
		"SELECT fly_ratio.id id, fly_ratio.upload upload,\n"
		"fly_ratio.download download,\n"
		"nick.name nick,\n"
		"hub.name hub,\n"
		"fly_ratio.dic_ip dic_ip,\n"
        "fly_ratio.dic_nick dic_nick,\n"
		"fly_ratio.dic_hub dic_hub\n"
		"FROM fly_ratio\n"
	    "INNER JOIN fly_dic nick ON fly_ratio.dic_nick = nick.id\n"
        "INNER JOIN fly_dic hub ON fly_ratio.dic_hub = hub.id");

		m_flySQLiteDB.executenonquery("CREATE VIEW IF NOT EXISTS v_fly_dup_file AS\n"
          "SELECT tth_id,count(*) cnt_dup,\n"
             "max((select name from fly_path where id = dic_path)) path_1,\n"
             "min((select name from fly_path where id = dic_path)) path_2,\n"
             "max(name) name_max,\n"
             "min(name) name_min\n"
             "FROM fly_file group by tth_id having count(*) > 1");

		const int l_rev = m_flySQLiteDB.executeint("select max(rev) from fly_revision");
        if(l_rev < 322)
		{
          safeAlter("ALTER TABLE fly_file add column hit int64 default 0");
          safeAlter("ALTER TABLE fly_file add column stamp_share int64 default 0");
          safeAlter("ALTER TABLE fly_file add column bitrate integer default 0");
          sqlite3_transaction l_trans(m_flySQLiteDB);
			 // рехеш всего музона
          m_flySQLiteDB.executenonquery("delete from fly_file where name like '%.mp3' and (bitrate=0 or bitrate is null)");
		  l_trans.commit();
		}
        if(l_rev < 358)
		{
             safeAlter("ALTER TABLE fly_file add column ftype integer default -1");
             sqlite3_transaction l_trans(m_flySQLiteDB);
	         m_flySQLiteDB.executenonquery("update fly_file set ftype=1 where ftype=-1 and "
				            "(name like '%.mp3' or name like '%.ogg' or name like '%.wav' or name like '%.flac' or name like '%.wma')");
	         m_flySQLiteDB.executenonquery("update fly_file set ftype=2 where ftype=-1 and "
				            "(name like '%.rar' or name like '%.zip' or name like '%.7z' or name like '%.gz')");
	         m_flySQLiteDB.executenonquery("update fly_file set ftype=3 where ftype=-1 and "
				            "(name like '%.doc' or name like '%.pdf' or name like '%.chm' or name like '%.txt' or name like '%.rtf')");
	         m_flySQLiteDB.executenonquery("update fly_file set ftype=4 where ftype=-1 and "
				            "(name like '%.exe' or name like '%.com' or name like '%.msi')");
	         m_flySQLiteDB.executenonquery("update fly_file set ftype=5 where ftype=-1 and "
				            "(name like '%.jpg' or name like '%.gif' or name like '%.png')");
	         m_flySQLiteDB.executenonquery("update fly_file set ftype=6 where ftype=-1 and "
				            "(name like '%.avi' or name like '%.mpg' or name like '%.mov' or name like '%.divx')");
             l_trans.commit();
		}
        if(l_rev < 341)
		{
             sqlite3_transaction l_trans(m_flySQLiteDB);
             m_flySQLiteDB.executenonquery("delete from fly_file where tth_id=0");
			 l_trans.commit();
		}
        if(l_rev < 365)
		{
             sqlite3_transaction l_trans(m_flySQLiteDB);
	         m_flySQLiteDB.executenonquery("update fly_file set ftype=6 where name like '%.mp4' or name like '%.fly'");
             l_trans.commit();
		}
//	if (l_rev <= 381)
	{
			m_flySQLiteDB.executenonquery(
			    "CREATE TABLE IF NOT EXISTS fly_registry(segment integer not null, key text not null,val_str text, val_number int64,tick_count int not null);");
			m_flySQLiteDB.executenonquery("CREATE UNIQUE INDEX IF NOT EXISTS\n"
			                              "iu_fly_registry_key ON fly_registry(segment,key);");
	}
        if(l_rev <= 388)
		{
  		    const bool l_first_convert = is_table_exists("fly_last_ip");
			if(!l_first_convert)
			{
            m_flySQLiteDB.executenonquery("CREATE TABLE IF NOT EXISTS fly_last_ip(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\n"
		    "dic_nick integer not null, dic_hub integer not null,dic_ip integer not null);");
			m_flySQLiteDB.executenonquery("CREATE UNIQUE INDEX IF NOT EXISTS iu_fly_last_ip ON fly_last_ip(dic_nick,dic_hub);");
            sqlite3_transaction l_trans(m_flySQLiteDB);
            m_flySQLiteDB.executenonquery("insert into fly_last_ip(dic_nick,dic_hub,dic_ip)\n"
				"select dic_nick,dic_hub,max(dic_ip) from fly_ratio where download=0 or upload=0\n"
				"group by dic_nick,dic_hub");
            m_flySQLiteDB.executenonquery("delete from fly_ratio where download=0 and upload=0");
		    l_trans.commit();
			}
		}

		safeAlter("ALTER TABLE fly_hash_block add column block_size int64");
		safeAlter("ALTER TABLE fly_file add column media_x integer");
		safeAlter("ALTER TABLE fly_file add column media_y integer");
		safeAlter("ALTER TABLE fly_file add column media_video text");
		if(safeAlter("ALTER TABLE fly_file add column media_audio text"))
		{ // получилось добавить колонку - значит стартанули первый раз - удалим все записи для перехеша с помощью либы
				sqlite3_transaction l_trans(m_flySQLiteDB);
				string l_where;
				string l_or;
					for (size_t i = 0; g_media_ext[i]; ++i)
						{
							l_where = l_where + l_or + string("lower(name) like '%.") + string(g_media_ext[i]) + string("'");
							if(l_or.empty())
							   l_or = " or ";
					    }
			l_where = "delete from fly_file where " + l_where;
			m_flySQLiteDB.executenonquery(l_where);
			l_trans.commit();
		}

        if(l_rev < 388)
		{
             sqlite3_transaction l_trans(m_flySQLiteDB);
		     m_flySQLiteDB.executenonquery("insert into fly_revision(rev) values(388);");
			 l_trans.commit();
		}
		}
	    catch(const database_error& e)
	    {
		 errorDB("SQLite: " + e.getError());
    	}
}
//========================================================================================================
void CFlylinkDBManager::load_registry(TStringSet& p_values, int p_Segment)
{
	Lock l(m_cs);
	 p_values.clear();
	 CFlyRegistryMap l_values;
	 load_registry(l_values, p_Segment);
	 for (CFlyRegistryMapIterC k = l_values.begin(); k != l_values.end(); ++k)
		p_values.insert(Text::toT(k->first));
}
//========================================================================================================
void CFlylinkDBManager::save_registry(const TStringSet& p_values, int p_Segment)
{
	Lock l(m_cs);
	 CFlyRegistryMap l_values;
	for (TStringSet::const_iterator i = p_values.begin(); i != p_values.end(); ++i)
		l_values.insert(CFlyRegistryMap::value_type(
		                    Text::fromT(*i),
		                    CFlyRegistryValue()));
	 save_registry(l_values, p_Segment);
}
//========================================================================================================
void CFlylinkDBManager::load_registry(CFlyRegistryMap& p_values, int p_Segment)
{
	Lock l(m_cs);
	try
	{
		if (!m_get_registry.get())
			m_get_registry = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
			                                                               "select key,val_str,val_number from fly_registry where segment=?"));
		m_get_registry->bind(1, p_Segment);
		sqlite3_reader l_q = m_get_registry.get()->executereader();
		while (l_q.read())
			p_values.insert(CFlyRegistryMap::value_type(
			                    l_q.getstring(0),
			                    CFlyRegistryValue(l_q.getstring(1), l_q.getint64(2))));
	}
	catch (const database_error& e)
	{
		errorDB("SQLite - load_registry: " + e.getError());
	}
}
//========================================================================================================
void CFlylinkDBManager::save_registry(const CFlyRegistryMap& p_values, int p_Segment)
{
	const int l_tick = GetTickCount();
	Lock l(m_cs);
	try
	{
		sqlite3_transaction l_trans(m_flySQLiteDB);
		if (!m_insert_registry.get())
			m_insert_registry = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
			                                                                  "insert or replace into fly_registry (segment,key,val_str,val_number,tick_count) values(?,?,?,?,?)"));
		for (CFlyRegistryMapIterC k = p_values.begin(); k != p_values.end(); ++k)
		{
			m_insert_registry.get()->bind(1, p_Segment);
			m_insert_registry.get()->bind(2, k->first,SQLITE_TRANSIENT);
			m_insert_registry.get()->bind(3, k->second.m_val_str,SQLITE_TRANSIENT);
			m_insert_registry.get()->bind(4, k->second.m_val_int64);
			m_insert_registry.get()->bind(5, l_tick);
			m_insert_registry.get()->executenonquery();
		}
		if (!m_delete_registry.get())
			m_delete_registry = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
			                                                                  "delete from fly_registry where segment=? and tick_count<>?"));
		m_delete_registry->bind(1, p_Segment);
		m_delete_registry->bind(2, l_tick);
		m_delete_registry.get()->executenonquery();
		l_trans.commit();
	}
	catch (const database_error& e)
	{
			errorDB("SQLite - save_registry: " + e.getError());
	}
}
//========================================================================================================
CFlyRatioItem CFlylinkDBManager::LoadRatio(const string& p_hub,const string& p_nick)
{
 try {
    Lock l(m_cs);
	if(m_first_ratio_cache == false)
	{ 
    m_first_ratio_cache = true;
	string l_sql = "SELECT nick,dic_hub, sum(upload) u,sum(download) d,'' ip\n"
		           "FROM v_fly_ratio_all group by nick,dic_hub\n";
	if(BOOLSETTING(ENABLE_LAST_IP))
	{
	 l_sql +=  "union all\n"
			   "SELECT nick.name nick, fly_last_ip.dic_hub dic_hub,0,0,userip.name userip\n"
               "FROM fly_last_ip\n"
			   "INNER JOIN fly_dic userip ON fly_last_ip.dic_ip = userip.id\n"
               "INNER JOIN fly_dic nick ON  fly_last_ip.dic_nick = nick.id\n";
	}
	if(!m_select_ratio_load.get())
        m_select_ratio_load = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,l_sql));
	sqlite3_reader l_q = m_select_ratio_load.get()->executereader();
    while(l_q.read())
     {    
	   CFlyRatioItem& l_item = m_ratio_cache[getCFlyRatioKey(l_q.getstring(0), l_q.getint64(1))];
       if(BOOLSETTING(ENABLE_LAST_IP))
           l_item.m_last_ip = l_q.getstring(4); 
	   const __int64 l_u = l_q.getint64(2);
	   const __int64 l_d = l_q.getint64(3);
	   l_item.m_upload   += l_u; 
	   l_item.m_download += l_d; 
	   m_global_ratio.m_download += l_d;
	   m_global_ratio.m_upload   += l_u;
	 }
	}
	__int64 l_hub_id;
	const string l_key = getCFlyRatioKey(p_nick,p_hub,l_hub_id);
	CFlyRatioCache::const_iterator l_result = m_ratio_cache.find(l_key);
	if(l_result != m_ratio_cache.end())
 	   return l_result->second;
   }
   catch(const database_error& e)
	{
		errorDB("SQLite - LoadRatio: " + e.getError());
	}
   return CFlyRatioItem();
}
//========================================================================================================
string CFlylinkDBManager::getCFlyRatioKey(const string& p_nick,const __int64 p_dic_hub)
{
  return p_nick + '+' + Util::toString(p_dic_hub);
}
//========================================================================================================
string CFlylinkDBManager::getCFlyRatioKey(const string& p_nick,const string& p_hub, __int64& p_dic_hub)
{
	p_dic_hub = getDIC_ID(p_hub.length() ? p_hub : "DHT" ,e_DIC_HUB);
	if(!p_dic_hub)
	    LogManager::getInstance()->message("Error SQLite - getCFlyRatioKey - p_dic_hub = 0 + p_hub = " + p_hub);
  return getCFlyRatioKey(p_nick,p_dic_hub);
}
//========================================================================================================
string CFlylinkDBManager::getCFlyDeferredRatioKey(const __int64 p_dic_nick,const __int64 p_dic_hub,const __int64 p_dic_ip)
{
  return Util::toString(p_dic_nick) + Util::toString(p_dic_ip) + Util::toString(p_dic_hub);
}
//========================================================================================================
void CFlylinkDBManager::store_last_ip(const string& p_hub,const string& p_nick,const string& p_ip)
{
	if (p_hub.empty() || p_ip.empty())
		return;
	try
	{
    Lock l(m_cs);
		__int64 l_hub_id = 0;
	const string l_key = getCFlyRatioKey(p_nick,p_hub,l_hub_id);
		if(!l_hub_id)
			return;
	CFlyRatioItem& l_new_item = m_ratio_cache[l_key]; 
	if(l_new_item.m_last_ip != p_ip)
	{
	l_new_item.m_last_ip = p_ip;
    const __int64 l_nick =  getDIC_ID(p_nick,e_DIC_NICK);
    const __int64 l_ip   =  getDIC_ID(p_ip,e_DIC_IP);
      sqlite3_transaction l_trans(m_flySQLiteDB);
	  if(!m_insert_store_ip.get())
         m_insert_store_ip = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
        "insert or replace into fly_last_ip (dic_nick,dic_hub,dic_ip) values(?,?,?)"));
  	     sqlite3_command* l_sql = m_insert_store_ip.get();
         l_sql->bind(1, l_nick);
         l_sql->bind(2, l_hub_id);
         l_sql->bind(3, l_ip);
	     l_sql->executenonquery();
         l_trans.commit();
	}
	}
 	catch(const database_error& e)
	{
		errorDB("SQLite - store_last_ip: " + e.getError());
	}
}
//========================================================================================================
void CFlylinkDBManager::store_ratio(const string& p_hub,const string& p_nick, 
							  __int64 p_size, const string& p_ip, bool p_is_download)
{
 if(!p_size)
     return;
	try
	{
    Lock l(m_cs);
		__int64 l_hub_id = 0;
	const string l_key = getCFlyRatioKey(p_nick,p_hub,l_hub_id);
		if(!l_hub_id)
			return;
	CFlyRatioItem& l_new_item = m_ratio_cache[l_key]; 
    if(BOOLSETTING(ENABLE_LAST_IP))
   	   l_new_item.m_last_ip = p_ip;
     if(p_is_download)
	 {
		l_new_item.m_download += p_size;
  	    m_global_ratio.m_download += p_size;
 	 }
	 else
	 {
		l_new_item.m_upload   += p_size; 
  	    m_global_ratio.m_upload   += p_size;
	 }
    const __int64 l_nick =  getDIC_ID(p_nick,e_DIC_NICK);
    const __int64 l_ip   =  getDIC_ID(p_ip,e_DIC_IP);
	const string l_defer_key = getCFlyDeferredRatioKey(l_nick,l_hub_id,l_ip);
	CFlyDeferredRatio& l_defer_item = m_defer_ratio[l_defer_key];
	 l_defer_item.m_dic_nick   = l_nick;
     l_defer_item.m_dic_hub    = l_hub_id;
     l_defer_item.m_dic_ip     = l_ip;
     if(p_is_download)
       l_defer_item.m_download  += p_size;
	 else
       l_defer_item.m_upload    += p_size;
 }
	catch(const database_error& e)
	{
		errorDB("SQLite - store_ratio: " + e.getError());
	}
}
//========================================================================================================
void CFlylinkDBManager::flush_ratio()
{
 try 
    {
      Lock l(m_cs);
	  for(CFlyDeferredArray::const_iterator i = m_defer_ratio.begin(); i != m_defer_ratio.end(); ++i)
	  {
	   sqlite3_command* l_sql = 0;
	   int l_count_changes = 0;
	   sqlite3_transaction l_trans(m_flySQLiteDB);
	   if(!m_update_ratio.get())
         m_update_ratio = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
        "update fly_ratio set upload = upload+?, download=download+? where dic_nick=? and dic_hub=? and dic_ip=?"));
	     l_sql = m_update_ratio.get();
		 l_sql->bind(1, i->second.m_upload);
		 l_sql->bind(2, i->second.m_download);
         l_sql->bind(3, i->second.m_dic_nick);
         l_sql->bind(4, i->second.m_dic_hub);
         l_sql->bind(5, i->second.m_dic_ip);
 	     l_sql->executenonquery();
		 l_count_changes = m_flySQLiteDB.sqlite3_changes();
         l_trans.commit();
     if(l_count_changes == 0)  
	 {
      sqlite3_transaction l_trans_insert(m_flySQLiteDB);
	  if(!m_insert_ratio.get())
         m_insert_ratio = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
        "insert into fly_ratio (dic_ip,dic_nick,dic_hub,upload,download) values(?,?,?,?,?)"));
	     l_sql = m_insert_ratio.get();
             l_sql->bind(1, i->second.m_dic_ip);
             l_sql->bind(2, i->second.m_dic_nick);
             l_sql->bind(3, i->second.m_dic_hub);
  		     l_sql->bind(4, i->second.m_upload);
		     l_sql->bind(5, i->second.m_download);
 	      l_sql->executenonquery();
         l_trans_insert.commit();
	}
	}
	  m_defer_ratio.clear(); //TODO
    }
 	catch(const database_error& e)
	{
		errorDB("flush_ratio - ratio: " + e.getError());
	}
}
//========================================================================================================
bool CFlylinkDBManager::is_table_exists(const string& p_table_name)
{
	return m_flySQLiteDB.executeint(
		"select count(*) from sqlite_master where lower(name) = lower('"+ p_table_name +"');") != 0;
}
//========================================================================================================
__int64 CFlylinkDBManager::getDIC_ID(const string& p_name, const eTypeDIC p_DIC)
{
 if(p_name.empty())
	return 0;
	try
	{
 __int64& l_Cache_ID = m_DIC[p_DIC-1][p_name];
 if(l_Cache_ID)
	return l_Cache_ID;  
   sqlite3_command* l_sql = 0;
   if(!m_select_fly_dic.get())
         m_select_fly_dic = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
        "select id from fly_dic where name=? and dic=?"));
    l_sql = m_select_fly_dic.get();
    l_sql->bind(1, p_name,SQLITE_STATIC);
    l_sql->bind(2, p_DIC);
    if (const __int64 l_ID = l_sql->executeint64_no_throw())
	{
		l_Cache_ID = l_ID;
    	return l_Cache_ID;
	}
	else 
	{
    sqlite3_command* l_sql = 0;
	sqlite3_transaction l_trans(m_flySQLiteDB);
    if(!m_insert_fly_dic.get())
         m_insert_fly_dic = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
        "insert into fly_dic (dic,name) values(?,?)"));
    l_sql = m_insert_fly_dic.get();
    l_sql->bind(1, p_DIC);
    l_sql->bind(2, p_name,SQLITE_STATIC);
	l_sql->executenonquery();
    l_trans.commit();
	l_Cache_ID = m_flySQLiteDB.insertid();
	return l_Cache_ID;
	}
 }
	catch(const database_error& e)
	{
		errorDB("SQLite - getDIC_ID: " + e.getError());
	}
	return 0;
}
//========================================================================================================
/*
void CFlylinkDBManager::IPLog(const string& p_hub_ip,
		       const string& p_ip,
			   const string& p_nick)
{
 Lock l(m_cs);
 try {
    const __int64 l_hub_ip =  getDIC_ID(p_hub_ip,e_DIC_HUB);
    const __int64 l_nick =  getDIC_ID(p_nick,e_DIC_NICK);
    const __int64 l_ip =  getDIC_ID(p_ip,e_DIC_IP);
    sqlite3_transaction l_trans(m_flySQLiteDB);
    sqlite3_command* l_sql = prepareSQL(
	 "insert into fly_ip_log(DIC_HUBIP,DIC_NICK,DIC_USERIP,STAMP) values (?,?,?,?);");
    l_sql->bind(1, l_hub_ip);
    l_sql->bind(2, l_nick);
    l_sql->bind(3, l_ip);
    SYSTEMTIME t; 
	::GetLocalTime(&t);
	char l_buf[64];
	sprintf(l_buf,"%02d.%02d.%04d %02d:%02d:%02d",t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute,t.wSecond);
	l_sql->bind(4, l_buf,strlen(l_buf));
	l_sql->executenonquery();
    l_trans.commit();
 }
	catch(const database_error& e)
	{
		errorDB("SQLite - IPLog: " + e.getError());
	}
}
*/
//========================================================================================================
void CFlylinkDBManager::SweepPath()
{
    Lock l(m_cs);
	try
	{
	for(CFlyPathCache::const_iterator i = m_path_cache.begin();i!=m_path_cache.end();++i)
	{
		if(i->second.m_found == false)
		{
		 {
	     if(!m_sweep_path_file.get())
		        m_sweep_path_file = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		        "delete from fly_file where dic_path=?"));
                sqlite3_transaction l_trans(m_flySQLiteDB);
				m_sweep_path_file.get()->bind(1, i->second.m_path_id);
                m_sweep_path_file.get()->executenonquery();
                l_trans.commit();
		 }
	     if(!m_sweep_path.get())
		     m_sweep_path = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		    "delete from fly_path where id=?"));
         sqlite3_transaction l_trans(m_flySQLiteDB);
		 m_sweep_path.get()->bind(1, i->second.m_path_id);
         m_sweep_path.get()->executenonquery();
         l_trans.commit();
		}
	}
	LoadPathCache();
	LogManager::getInstance()->message("reload dir...");
	LogManager::getInstance()->message("delete from fly_hash_block...");
        sqlite3_transaction l_trans(m_flySQLiteDB);
	m_flySQLiteDB.executenonquery("delete FROM fly_hash_block where tth_id not in(select tth_id from fly_file)");
    l_trans.commit();
    }
	catch(const database_error& e)
	{
		errorDB("SQLite - SweepPath: " + e.getError());
	}
	
}
//========================================================================================================
void CFlylinkDBManager::LoadPathCache()
{
    Lock l(m_cs);
	m_convert_ftype_stop_key = 0;
	m_path_cache.clear();
	try
	{
	if(!m_load_path_cache.get())
		m_load_path_cache = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		"select id,name from fly_path"));
	sqlite3_reader l_q = m_load_path_cache.get()->executereader();
    while(l_q.read())
	{
		CFlyPathItem& l_rec = m_path_cache[l_q.getstring(1)];
		l_rec.m_path_id  = l_q.getint64(0); 
		l_rec.m_found    = false;
	}
	}
	catch(const database_error& e)
	{
		errorDB("SQLite - LoadPathCache: " + e.getError());
		return;
	}
}
//========================================================================================================
__int64 CFlylinkDBManager::get_path_id(const string& p_path, bool p_create )
{
    Lock l(m_cs);
	if(m_last_path_id != -1 && p_path == m_last_path)
       return m_last_path_id; 
	m_last_path = p_path;
	const string l_path = Text::toLower(p_path);
	try
	{
	CFlyPathCache::iterator l_item_iter = m_path_cache.find(l_path);
	if(l_item_iter != m_path_cache.end())
	  {
		l_item_iter->second.m_found = true;
		m_last_path_id = l_item_iter->second.m_path_id;
		return m_last_path_id;
	  }
	if(!m_get_path_id.get())
		m_get_path_id = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
                       "select id from fly_path where name=?;")); 
    m_get_path_id.get()->bind(1, p_path,SQLITE_STATIC);
	m_last_path_id = m_get_path_id.get()->executeint64_no_throw();
	if (m_last_path_id)
		return m_last_path_id;
	else 
    if(p_create)
	{
	sqlite3_transaction l_trans(m_flySQLiteDB);
	if(!m_insert_fly_path.get())
	     m_insert_fly_path = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
                    "insert into fly_path (name) values(?)"));
    m_insert_fly_path.get()->bind(1, l_path,SQLITE_STATIC);
	m_insert_fly_path.get()->executenonquery();
    l_trans.commit();
	CFlyPathItem l_item;
	l_item.m_found = true;
	l_item.m_path_id = m_flySQLiteDB.insertid();
	m_path_cache[l_path] = l_item;
	m_last_path_id = l_item.m_path_id; 
	return m_last_path_id;
	}
	else
     return 0;
	}
	catch(const database_error& e)
	{
		errorDB("SQLite - get_path_id: " + e.getError());
		return 0;
	}
}
//========================================================================================================
const TTHValue*  CFlylinkDBManager::findTTH(const string& p_fname,const string& p_fpath)
{
    Lock l(m_cs);
	try
	{
	const __int64 l_path_id = get_path_id(p_fpath, false);
	if(!l_path_id)
	   return 0; 	 
	if(!m_findTTH.get())
		m_findTTH = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
         "select tth_id from fly_file where name=? and dic_path=?;"));
    sqlite3_command* l_sql = m_findTTH.get();
		l_sql->bind(1, p_fname,SQLITE_STATIC);
	l_sql->bind(2, l_path_id);
	if (const __int64 l_tth_id = l_sql->executeint64_no_throw())
    	return get_tth(l_tth_id);
	else
       return 0;
    }
	catch(const database_error& e)
	{
		errorDB("SQLite - findTTH: " + e.getError());
	    return 0;
	}
}	 
//========================================================================================================
#pragma optimize("t", on)
void CFlylinkDBManager::SweepFiles(__int64 p_path_id, const CFlyDirMap& p_sweep_files)
{
    Lock l(m_cs);
	try
	{
	for(CFlyDirMap::const_iterator i = p_sweep_files.begin();i!=p_sweep_files.end();++i)
	{
		if(i->second.m_found == false)
		{
	     if(!m_sweep_dir_sql.get())
		 m_sweep_dir_sql = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		    "delete from fly_file where dic_path=? and name=?"));
         sqlite3_transaction l_trans(m_flySQLiteDB);
	     m_sweep_dir_sql.get()->bind(1, p_path_id);
	     m_sweep_dir_sql.get()->bind(2, i->first,SQLITE_STATIC);
         m_sweep_dir_sql.get()->executenonquery();
         l_trans.commit();
		}
	}
    }
	catch(const database_error& e)
	{
		errorDB("SQLite - SweepFiles: " + e.getError());
	}
}
//========================================================================================================
void CFlylinkDBManager::LoadDir(__int64 p_path_id, CFlyDirMap& p_dir_map)
{
    Lock l(m_cs);
	try
	{
	if(!m_load_dir_sql.get())
		m_load_dir_sql = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
            "select size,stamp,tth,name,hit,stamp_share,bitrate,ftype,media_x,media_y,media_video,media_audio from fly_file ff,fly_hash fh where\n"
        "ff.dic_path=? and ff.tth_id=fh.id"));
	m_load_dir_sql.get()->bind(1, p_path_id);
	sqlite3_reader l_q = m_load_dir_sql.get()->executereader();
	bool l_calc_ftype = false;
    while(l_q.read())
     {
		 const string l_name = l_q.getstring(3); 
		 CFlyFileInfo& l_info = p_dir_map[l_name]; 
		 l_info.m_found = false;
		 l_info.m_recalc_ftype = false;
		 l_info.m_size   = l_q.getint64(0);
		 l_info.m_TimeStamp  = l_q.getint64(1);
		 l_info.m_StampShare  = l_q.getint64(5);
		 if(!l_info.m_StampShare)
			 l_info.m_StampShare = l_info.m_TimeStamp;
		 l_info.m_hit = uint32_t(l_q.getint(4));
			l_info.m_media.m_bitrate = short(l_q.getint(6));
			l_info.m_media.m_mediaX  = short(l_q.getint(8)); 
			l_info.m_media.m_mediaY  = short(l_q.getint(9));
			l_info.m_media.m_video   = l_q.getstring(10);
			l_info.m_media.m_audio   = l_q.getstring(11);
		 const int l_ftype = l_q.getint(7);
		 if(l_ftype == -1)
		 {
 		     l_info.m_recalc_ftype = true;
		     l_calc_ftype = true;	  
			 l_info.m_ftype = ShareManager::getFType(l_name);
		 } 
		 else
		 {
			 l_info.m_ftype = SearchManager::TypeModes(l_ftype);
		 }
		 l_q.getblob(2,&l_info.m_tth,24);
	 }
	// тормозит...
	if(l_calc_ftype && m_convert_ftype_stop_key < 200)
	{
     if(!m_set_ftype.get())
         m_set_ftype  = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
	              "update fly_file set ftype=? where name=? and dic_path=? and ftype=-1"));
     sqlite3_transaction l_trans(m_flySQLiteDB);
     m_set_ftype.get()->bind(3, p_path_id);
	 for(CFlyDirMap::const_iterator i = p_dir_map.begin(); i != p_dir_map.end(); ++i)
	 {
		 if(i->second.m_recalc_ftype)
		 {
			 m_convert_ftype_stop_key++; 
	         m_set_ftype.get()->bind(1, i->second.m_ftype);
					m_set_ftype.get()->bind(2, i->first,SQLITE_STATIC);
             m_set_ftype.get()->executenonquery();
		 }
	 }
     l_trans.commit();
	}
	
    }
	catch(const database_error& e)
	{
		errorDB("SQLite - LoadDir: " + e.getError());
	}
}
#pragma optimize("", on)
//========================================================================================================
void CFlylinkDBManager::updateFileInfo(const string& p_fname,__int64 p_path_id,
									   int64_t p_Size, int64_t p_TimeStamp,__int64 p_tth_id)
{
  if(!m_update_file.get())
		m_update_file = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		"update fly_file set size=?,stamp=?,tth_id=?,stamp_share=? where name=? and dic_path=?;"));
   sqlite3_transaction l_trans(m_flySQLiteDB);
   m_update_file.get()->bind(1, p_Size);
   m_update_file.get()->bind(2, p_TimeStamp);
   m_update_file.get()->bind(3, p_tth_id);
   m_update_file.get()->bind(4, int64_t(File::currentTime()));
	m_update_file.get()->bind(5, p_fname,SQLITE_STATIC);
   m_update_file.get()->bind(6, p_path_id);
   m_update_file.get()->executenonquery();
   l_trans.commit();
}
//========================================================================================================
bool CFlylinkDBManager::checkTTH(const string& p_fname,__int64 p_path_id, 
									int64_t p_Size, int64_t p_TimeStamp, TTHValue& p_out_tth)
{
    Lock l(m_cs);
	try
	{
	if(!m_check_tth_sql.get())
		m_check_tth_sql = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		"select size,stamp,tth from fly_file ff,fly_hash fh where\n"
                "fh.id=ff.tth_id and ff.name=? and ff.dic_path=?"));
		m_check_tth_sql.get()->bind(1, Text::toLower(p_fname),SQLITE_TRANSIENT);
	m_check_tth_sql.get()->bind(2, p_path_id);
	sqlite3_reader l_q = m_check_tth_sql.get()->executereader();
    while(l_q.read())
     {
		 const int64_t l_size   = l_q.getint64(0);
		 const int64_t l_stamp  = l_q.getint64(1);
		 l_q.getblob(2,p_out_tth.data,24);
		 if(l_stamp != p_TimeStamp || l_size != p_Size)
		 {
 	          l_q.close();
		      updateFileInfo(p_fname,p_path_id,-1,-1,-1);
			  return false;
		 }
      return true;
	 }
    return false;
    }
	catch(const database_error& e)
	{
		errorDB("SQLite - checkTTH: " + e.getError());
	    return false;
	}
}
//========================================================================================================
__int64 CFlylinkDBManager::getBlockSize(const TTHValue& p_root)
{
  Lock l(m_cs);
  if(const __int64 l_tth_id = get_tth_id(p_root,false))
  {
	try
	{
	if(!m_get_blocksize.get())
		m_get_blocksize = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		"select file_size,block_size from fly_hash_block where tth_id=?"));
    m_get_blocksize.get()->bind(1, l_tth_id);
	sqlite3_reader l_q = m_get_blocksize.get()->executereader();
    while(l_q.read())
     {
		const __int64 l_file_size = l_q.getint64(0);
		__int64 l_blocksize = l_q.getint64(1);
		if(l_blocksize == 0)
		   l_blocksize = TigerTree::getMaxBlockSize(l_file_size);
		return l_blocksize;
	 }
	}
	catch(const database_error& e)
	{
		errorDB("SQLite - getBlockSize: " + e.getError());
	}
  }
  return 0;
}
//========================================================================================================
bool CFlylinkDBManager::getTree(const TTHValue& p_root, TigerTree& p_tt)
{
  Lock l(m_cs);
  if(const __int64 l_tth_id = get_tth_id(p_root,false))
  {
	try
	{
	if(!m_get_tree.get())
		m_get_tree = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		"select tiger_tree,file_size,block_size from fly_hash_block where tth_id=?"));
    m_get_tree.get()->bind(1, l_tth_id);
	sqlite3_reader l_q = m_get_tree.get()->executereader();
    while(l_q.read())
     {
		const __int64 l_file_size = l_q.getint64(1);
		__int64 l_blocksize = l_q.getint64(2);
		if(l_blocksize == 0)
		  l_blocksize = TigerTree::getMaxBlockSize(l_file_size);
		if (l_file_size <= MIN_BLOCK_SIZE)
		{
	    	p_tt = TigerTree(l_file_size, l_blocksize, p_root);
		    return true;
	    }
		static vector<uint8_t> g_buf;   
		l_q.getblob(0,g_buf);
		if(g_buf.size() > 0)
		{
			p_tt = TigerTree(l_file_size, l_blocksize, &g_buf[0],g_buf.size());
	 	    return p_tt.getRoot() == p_root;
		}
		else
			return false;
	}
	}
	catch(const database_error& e)
	{
		errorDB("SQLite - getTree: " + e.getError());
		return false;
	}
  }
  return false;
}
//========================================================================================================
const TTHValue* CFlylinkDBManager::get_tth(const __int64 p_tth_id)
{
    Lock l(m_cs);
	try
	{
    if(!m_get_tth.get())
		m_get_tth = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
	          "select tth from fly_hash where id=?;"));
    m_get_tth.get()->bind(1, p_tth_id);
	static TTHValue g_TTH;
	const string l_tth = m_get_tth.get()->executestring();
	if (l_tth.size())
	{
		g_TTH =  TTHValue(l_tth);
		return &g_TTH; 
	}
	return NULL;
	}
	catch(const database_error& e)
	{
		errorDB("SQLite - get_tth: " + e.getError()); // TODO translate
		return NULL;
	}
}
//========================================================================================================
bool CFlylinkDBManager::is_old_tth(const TTHValue& p_tth)
{
    Lock l(m_cs);
	return get_tth_id(p_tth,false) != 0;
}
//========================================================================================================
__int64 CFlylinkDBManager::get_tth_id(const TTHValue& p_tth, bool p_create /*= true*/)
{
	try
	{
	if(!m_get_tth_id.get())
		m_get_tth_id = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		"select id from fly_hash where tth=?;"));
    sqlite3_command* l_sql = m_get_tth_id.get();
		l_sql->bind(1, p_tth.data, 24,SQLITE_STATIC);
	if (const __int64 l_ID = l_sql->executeint64_no_throw())
    	return l_ID;
		else if (p_create)
	{
	 sqlite3_transaction l_trans(m_flySQLiteDB);
	 if(!m_insert_fly_hash.get())
		m_insert_fly_hash = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
        "insert into fly_hash (tth) values(?)"));
     sqlite3_command* l_sql_insert = m_insert_fly_hash.get();
			l_sql_insert->bind(1, p_tth.data, 24,SQLITE_STATIC);
 	 l_sql_insert->executenonquery();
     l_trans.commit();
	 return m_flySQLiteDB.insertid();
	}
	else
 	  return 0;
	}
	catch(const database_error& e)
	{
		errorDB("SQLite - get_tth_id: " + e.getError());
		return 0;
	}
}
//========================================================================================================
void CFlylinkDBManager::Hit(const string& p_Path,const string& p_FileName)
{
    Lock l(m_cs);
	try
	{
	const __int64 l_path_id = get_path_id(p_Path,false);
	if(!l_path_id)
		return; 
	if(!m_upload_file.get())
		m_upload_file = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		"update fly_file set hit=hit+1 where name=? and dic_path=?"));
	sqlite3_transaction l_trans(m_flySQLiteDB);
	sqlite3_command* l_sql = m_upload_file.get();
		l_sql->bind(1, p_FileName,SQLITE_STATIC);
	l_sql->bind(2, l_path_id);
	l_sql->executenonquery();
    l_trans.commit();
	}
	catch(const database_error& e)
	{
		errorDB("SQLite - Hit: " + e.getError());
	}
}
//========================================================================================================
bool CFlylinkDBManager::addFile(const string& p_Path,const string& p_FileName, 
								int64_t p_TimeStamp, const TigerTree& p_tth,
                                const CFlyMediaInfo& p_media,bool p_case_convet)
{
    Lock l(m_cs);
	try
	{
	const __int64 l_path_id = get_path_id(p_Path,true);
	if(!l_path_id)
		return false;
	const __int64 l_tth_id = addTree(p_tth); 
	if(!l_tth_id)
		return false;
	sqlite3_transaction l_trans(m_flySQLiteDB);
	if(!m_insert_file.get())
		m_insert_file = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
                                "insert or replace into fly_file (tth_id,dic_path,name,size,stamp,stamp_share,bitrate,ftype,media_x,media_y,media_video,media_audio) values(?,?,?,?,?,?,?,?,?,?,?,?);"));
	sqlite3_command* l_sql = m_insert_file.get();
    l_sql->bind(1, l_tth_id);
    l_sql->bind(2, l_path_id);
		l_sql->bind(3, p_FileName,SQLITE_STATIC);
    l_sql->bind(4, p_tth.getFileSize());
    l_sql->bind(5, p_TimeStamp);
    l_sql->bind(6, int64_t(File::currentTime()));
		l_sql->bind(7, p_media.m_bitrate);
		l_sql->bind(8, ShareManager::getFType(p_FileName));
		l_sql->bind(9, p_media.m_mediaX);
		l_sql->bind(10, p_media.m_mediaY);
		l_sql->bind(11, p_media.m_video,SQLITE_STATIC);
		l_sql->bind(12, p_media.m_audio,SQLITE_STATIC);
	l_sql->executenonquery();
    l_trans.commit();
	return true;
	}
	catch(const database_error& e)
	{
		errorDB("SQLite - addFile: " + e.getError());
		return false;
	}
}
//========================================================================================================
__int64 CFlylinkDBManager::addTree(const TigerTree& p_tt)
{
	Lock l(m_cs);
	const __int64 l_tth_id = get_tth_id(p_tt.getRoot(), true);
	if(!l_tth_id)
		return 0;
	try
	{
		int l_size = p_tt.getLeaves().size() * TTHValue::BYTES;
		const __int64 l_file_size = p_tt.getFileSize();
	sqlite3_transaction l_trans(m_flySQLiteDB);
	sqlite3_command* l_sql = 0;
	      if(!m_ins_fly_hash_block.get())
		     m_ins_fly_hash_block = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
       "insert or replace into fly_hash_block (tth_id,file_size,tiger_tree,block_size) values(?,?,?,?)"));
		l_sql = m_ins_fly_hash_block.get();
        l_sql->bind(1, l_tth_id);
        l_sql->bind(2, l_file_size);
		l_sql->bind(3, p_tt.getLeaves()[0].data,  l_file_size > MIN_BLOCK_SIZE ? l_size : 0,SQLITE_STATIC);
		l_sql->bind(4, p_tt.getBlockSize());
		l_sql->executenonquery();
        l_trans.commit();
	 return l_tth_id;
	}
	catch(const database_error& e)
	{
		errorDB("SQLite - addTree: " + e.getError());
		return 0;
	}
}
//========================================================================================================
CFlylinkDBManager::~CFlylinkDBManager()
{
    flush_ratio();
}
//========================================================================================================
double CFlylinkDBManager::get_ratio() const
{
    if(m_global_ratio.m_download > 0 )
		 return (double)m_global_ratio.m_upload / (double)m_global_ratio.m_download;
	else
		return 0;
}
//========================================================================================================
tstring CFlylinkDBManager::get_ratioW() const
{
    if(m_global_ratio.m_download > 0 )
	{
	 TCHAR buf[256];
	 snwprintf(buf, _countof(buf), _T("Ratio (up/down): %.2f"), get_ratio());
	 return buf;
	}
	return _T("");
}
//========================================================================================================
void CFlylinkDBManager::push_download_tth(const TTHValue& p_tth)
{
 Lock l(m_cs);
 if (!is_download_tth(p_tth))
 {
	try {
	sqlite3_transaction l_trans(m_flySQLiteDB);
    if(!m_insert_fly_tth.get())
	    m_insert_fly_tth = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
               "insert into fly_tth(tth) values(?)"));
			m_insert_fly_tth.get()->bind(1, p_tth.data, 24,SQLITE_STATIC);
    m_insert_fly_tth.get()->executenonquery();
    l_trans.commit();
	}
	catch(const database_error& e)
	{
		errorDB("SQLite-push_download_tth: " + e.getError());
	}
 }
}
//========================================================================================================
bool CFlylinkDBManager::is_download_tth(const TTHValue& p_tth)
{
 Lock l(m_cs);
 try {
	if(!m_is_download_tth.get())
		m_is_download_tth = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
		"select count(*) from fly_tth where tth=?;"));
    sqlite3_command* l_sql = m_is_download_tth.get();
		l_sql->bind(1, p_tth.data, 24,SQLITE_STATIC);
	return l_sql->executeint() != 0;
	}
	catch(const database_error& e)
	{
		errorDB("is_download_tth: " + e.getError());
		return false;
	}
}
//========================================================================================================
void CFlylinkDBManager::log(const int p_area,const StringMap& p_params)
{
 Lock l(m_cs);
	try
	{
		sqlite3_transaction l_trans(m_flySQLiteDB);
    if(!m_insert_fly_message.get())
			m_insert_fly_message = auto_ptr<sqlite3_command>(new sqlite3_command(m_flySQLiteDB,
			                                                                     "insert into LOGDB.fly_log(sdate,type,body,hub,nick,ip,file,source,target,fsize,fchunk,extra,userCID)"
			                                                                     " values(datetime('now','localtime'),?,?,?,?,?,?,?,?,?,?,?,?);")); 
    m_insert_fly_message.get()->bind(1, p_area);
		m_insert_fly_message.get()->bind(2, getString(p_params, "message"),SQLITE_TRANSIENT);
		m_insert_fly_message.get()->bind(3, getString(p_params, "hubURL"),SQLITE_TRANSIENT);
		m_insert_fly_message.get()->bind(4, getString(p_params, "myNI"),SQLITE_TRANSIENT);
		m_insert_fly_message.get()->bind(5, getString(p_params, "ip"),SQLITE_TRANSIENT);
		m_insert_fly_message.get()->bind(6, getString(p_params, "file"),SQLITE_TRANSIENT);
		m_insert_fly_message.get()->bind(7, getString(p_params, "source"),SQLITE_TRANSIENT);
		m_insert_fly_message.get()->bind(8, getString(p_params, "target"),SQLITE_TRANSIENT);
		m_insert_fly_message.get()->bind(9, getString(p_params, "fileSI"),SQLITE_TRANSIENT);
		m_insert_fly_message.get()->bind(10, getString(p_params, "fileSIchunk"),SQLITE_TRANSIENT);
		m_insert_fly_message.get()->bind(11, getString(p_params, "extra"),SQLITE_TRANSIENT);
		m_insert_fly_message.get()->bind(12, getString(p_params, "userCID"),SQLITE_TRANSIENT);
    m_insert_fly_message.get()->executenonquery();
    l_trans.commit();
	}
	catch(const database_error& e)
	{
		errorDB("SQLite - log: " + e.getError(), true); // TODO translate
	}
}
