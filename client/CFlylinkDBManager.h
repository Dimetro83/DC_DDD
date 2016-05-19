//-----------------------------------------------------------------------------
//(c) 2007-2011 pavel.pimenov@gmail.com
//-----------------------------------------------------------------------------
#ifndef CFlylinkDBManager_H
#define CFlylinkDBManager_H

#include <string>
#include <vector>

#include "Singleton.h"
#include "MerkleTree.h"
#include "SearchManager.h"

#include "sqlite/sqlite3x.hpp"
#include "Util.h"
#include "Thread.h"
#include "CFlyMediaInfo.h"

using namespace sqlite3x;

 enum eTypeDIC {
	  e_DIC_HUB = 1,
	  e_DIC_NICK = 2,
	  e_DIC_IP = 3,
	  e_DIC_LAST
 };
struct CFlyFileInfo
{
	int64_t  m_size;
	int64_t  m_TimeStamp;
	TTHValue m_tth;
	uint32_t m_hit; 
	int64_t  m_StampShare; 
	CFlyMediaInfo m_media;
	bool     m_found;
	bool     m_recalc_ftype;
    SearchManager::TypeModes m_ftype;
};
typedef unordered_map<string,CFlyFileInfo> CFlyDirMap;
struct CFlyPathItem
{
  __int64 m_path_id;
  bool    m_found;
};
struct CFlyDeferredRatio
{
  __int64  m_dic_nick;
  __int64  m_dic_hub;
  __int64  m_dic_ip;
  __int64  m_upload;
  __int64  m_download;
  CFlyDeferredRatio() :
   m_dic_nick(0),
   m_dic_hub(0),
   m_dic_ip(0),
   m_upload(0),
   m_download(0)
  {
  }
};
struct CFlyRatioItem //TODO унести в User
{
  __int64  m_upload;
  __int64  m_download;
    string m_last_ip;
  CFlyRatioItem() : m_upload(0), m_download(0)
  {
  }
};
enum eTypeSegment {
	  e_ExtraSlot = 1,
	e_RecentHub,
	e_SearchHistory
};
struct CFlyRegistryValue
{
    string m_val_str;
  __int64  m_val_int64;
  CFlyRegistryValue(__int64 p_val_int64 = 0)
	  :m_val_int64(p_val_int64)
  {
  }
  CFlyRegistryValue(const string& p_str,__int64 p_val_int64 = 0)
	  :m_val_int64(p_val_int64),m_val_str(p_str)
  {
  }
};
typedef unordered_map<string,CFlyRegistryValue> CFlyRegistryMap;
typedef CFlyRegistryMap::const_iterator CFlyRegistryMapIterC;

typedef unordered_map<string,CFlyPathItem> CFlyPathCache;
typedef unordered_map<string,CFlyRatioItem> CFlyRatioCache;
typedef unordered_map<string,CFlyDeferredRatio> CFlyDeferredArray;

class CFlylinkDBManager : public Singleton<CFlylinkDBManager>{
public:
	CFlylinkDBManager();
	~CFlylinkDBManager();
	void push_download_tth(const TTHValue& p_tth);
	void store_ratio(const string& p_hub,const string& p_nick, __int64 p_size, const string& p_ip , bool p_is_download);
	void store_last_ip(const string& p_hub,const string& p_nick,const string& p_ip);
	void flush_ratio();
	CFlyRatioItem LoadRatio(const string& p_hub,const string& p_nick);
private:
	CFlyRatioCache m_ratio_cache;
    string getCFlyRatioKey(const string& p_nick,const string& p_hub,__int64& p_dic_hub);
    string getCFlyRatioKey(const string& p_nick,const __int64 p_dic_hub);
    string getCFlyDeferredRatioKey(const __int64 p_dic_nick,const __int64 p_dic_hub,const __int64 p_dic_ip);
	bool m_first_ratio_cache;
	CFlyDeferredArray m_defer_ratio;

public:
	CFlyRatioItem  m_global_ratio;
    double get_ratio() const;
    tstring get_ratioW() const;

	bool is_table_exists(const string& p_table_name);
	bool is_download_tth(const TTHValue& p_tth);
	bool is_old_tth(const TTHValue& p_tth);
	const TTHValue* get_tth(const __int64 p_tth_id);
	bool getTree(const TTHValue& p_root, TigerTree& p_tt);
    __int64 getBlockSize(const TTHValue& p_root);
	__int64 get_path_id(const string& p_path, bool p_create);
	__int64 addTree(const TigerTree& tt);
	const TTHValue* findTTH(const string& aPath,const string& aFileName);
	bool addFile(const string& p_Path,const string& p_FileName, int64_t p_TimeStamp, 
		  const TigerTree& p_tth, const CFlyMediaInfo& p_media,bool p_case_convet);
	void Hit(const string& p_Path,const string& p_FileName);
	bool checkTTH(const string& fname,__int64 path_id, int64_t aSize, int64_t aTimeStamp, TTHValue& p_out_tth);
	void LoadPathCache();
	void SweepPath();
	void LoadDir(__int64 p_path_id, CFlyDirMap& p_dir_map);
	void SweepFiles(__int64 p_path_id, const CFlyDirMap& p_sweep_files);
	void log(const int p_area,const StringMap& p_params);

		void load_registry(CFlyRegistryMap& p_values, int p_Segment);
		void save_registry(const CFlyRegistryMap& p_values, int p_Segment);
		void load_registry(TStringSet& p_values, int p_Segment);
		void save_registry(const TStringSet& p_values, int p_Segment);
private:
	mutable CriticalSection m_cs;
	sqlite3_connection m_flySQLiteDB;
	CFlyPathCache m_path_cache; 
	auto_ptr<sqlite3_command> m_add_tree_find;
	auto_ptr<sqlite3_command> m_select_ratio_load;
	auto_ptr<sqlite3_command> m_update_ratio;
	auto_ptr<sqlite3_command> m_insert_ratio;
	auto_ptr<sqlite3_command> m_insert_store_ip;
	auto_ptr<sqlite3_command> m_ins_fly_hash_block;
	auto_ptr<sqlite3_command> m_insert_file;
	auto_ptr<sqlite3_command> m_update_file;
	auto_ptr<sqlite3_command> m_check_tth_sql;
	auto_ptr<sqlite3_command> m_load_dir_sql;
	auto_ptr<sqlite3_command> m_set_ftype;
	auto_ptr<sqlite3_command> m_load_path_cache;
	auto_ptr<sqlite3_command> m_sweep_dir_sql;
	auto_ptr<sqlite3_command> m_sweep_path;
	auto_ptr<sqlite3_command> m_sweep_path_file;
	auto_ptr<sqlite3_command> m_get_path_id;
	auto_ptr<sqlite3_command> m_get_tth_id;
	auto_ptr<sqlite3_command> m_findTTH;
	auto_ptr<sqlite3_command> m_get_tth;
	auto_ptr<sqlite3_command> m_is_download_tth;
	auto_ptr<sqlite3_command> m_upload_file;
	auto_ptr<sqlite3_command> m_get_tree;
	auto_ptr<sqlite3_command> m_get_blocksize;
	auto_ptr<sqlite3_command> m_insert_fly_hash;
	auto_ptr<sqlite3_command> m_insert_fly_path;
	auto_ptr<sqlite3_command> m_insert_fly_tth;
	auto_ptr<sqlite3_command> m_select_fly_dic;
	auto_ptr<sqlite3_command> m_insert_fly_dic;
		auto_ptr<sqlite3_command> m_get_registry;
		auto_ptr<sqlite3_command> m_insert_registry;
		auto_ptr<sqlite3_command> m_delete_registry;

	__int64 m_last_path_id;
	string m_last_path;

//logmanager
	auto_ptr<sqlite3_command> m_insert_fly_message;
    static inline const string& getString(const StringMap& p_params, const char* p_type)
    {
	StringMapIterC i = p_params.find(p_type);
	if(i != p_params.end())
		return i->second;
	else
      return Util::emptyString;
     }

	int m_convert_ftype_stop_key;

	vector< unordered_map<string, __int64> > m_DIC;
	__int64 getDIC_ID(const string& p_name, const eTypeDIC p_DIC);
	void errorDB(const string& p_txt, bool p_critical = true);

		bool safeAlter(const char* p_sql);
	void updateFileInfo(const string& p_fname,__int64 p_path_id, 
		 int64_t p_Size, int64_t p_TimeStamp,__int64 p_tth_id);
	__int64 get_tth_id(const TTHValue& p_tth, bool p_create);
};
#endif
