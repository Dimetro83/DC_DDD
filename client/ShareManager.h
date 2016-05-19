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

#ifndef DCPLUSPLUS_DCPP_SHARE_MANAGER_H
#define DCPLUSPLUS_DCPP_SHARE_MANAGER_H

#include <atomic>

#include "TimerManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "HashManager.h"
#include "QueueManagerListener.h"

#include "Exception.h"
#include "StringSearch.h"
#include "Singleton.h"
#include "BloomFilter.h"
#include "FastAlloc.h"
#include "MerkleTree.h"
#include "Pointer.h"
#include "CFlyMediaInfo.h"
#ifdef _WIN32
# include <ShlObj.h> //[+]PPA
#endif

namespace dht
{
	class IndexManager;
}

namespace dcpp {

STANDARD_EXCEPTION(ShareException);

class SimpleXML;
class Client;
class File;
class OutputStream;
class MemoryInputStream;

struct ShareLoader;
class ShareManager : public Singleton<ShareManager>, private SettingsManagerListener, private Thread, private TimerManagerListener,
	private HashManagerListener, private QueueManagerListener
{
public:
	/**
	 * @param aDirectory Physical directory location
	 * @param aName Virtual name
	 */
	void addDirectory(const string& realPath, const string &virtualName);
	void removeDirectory(const string& realPath);
	void renameDirectory(const string& realPath, const string& virtualName);

        string toRealPath(const TTHValue& tth) const; // !PPA!
#ifdef _DEBUG
	string toVirtual(const TTHValue& tth) const;
#endif
	string toReal(const string& virtualFile);
	TTHValue getTTH(const string& virtualFile) const;
	
	void refresh(bool dirs = false, bool aUpdate = true, bool block = false) noexcept;
	void setDirty() { xmlDirty = true; }
	void setPurgeTTH() { m_sweep_path = true; }
	
	
	bool shareFolder(const string& path, bool thoroughCheck = false) const;
	int64_t removeExcludeFolder(const string &path, bool returnSize = true);
	int64_t addExcludeFolder(const string &path);

	void search(SearchResultList& l, const string& aString, int aSearchType, int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults) noexcept;
	void search(SearchResultList& l, const StringList& params, StringList::size_type maxResults) noexcept;

	StringPairList getDirectories() const noexcept;

	MemoryInputStream* generatePartialList(const string& dir, bool recurse) const;
	MemoryInputStream* getTree(const string& virtualFile) const;

	AdcCommand getFileInfo(const string& aFile);

	int64_t getShareSize() const noexcept;
	int64_t getShareSize(const string& realPath) const noexcept;

	size_t getSharedFiles() const noexcept;

	string getShareSizeString() const { return Util::toString(getShareSize()); }
	string getShareSizeString(const string& aDir) const { return Util::toString(getShareSize(aDir)); }
	
	void getBloom(ByteVector& v, size_t k, size_t m, size_t h) const;

	static SearchManager::TypeModes getFType(const string& fileName) noexcept;
	string validateVirtual(const string& /*aVirt*/) const noexcept;
	bool hasVirtual(const string& name) const noexcept;

	void addHits(uint32_t aHits) {
		hits += aHits;
	}

	const string& getOwnListFile() {
		generateXmlList();
		return getBZXmlFile();
	}

	bool isTTHShared(const TTHValue& tth) const {
		Lock l(cs);
		return tthIndex.find(tth) != tthIndex.end();
	}
	
	GETSET(size_t, hits, Hits);
	GETSET(string, bzXmlFile, BZXmlFile);
	GETSET(int64_t, sharedSize, SharedSize);

private:
	struct AdcSearch;
	class Directory : public FastAlloc<Directory>, public intrusive_ptr_base<Directory>, boost::noncopyable {
	public:
		typedef boost::intrusive_ptr<Directory> Ptr;
		typedef std::map<string, Ptr> Map;
		typedef Map::iterator MapIter;

		struct File {
			struct StringComp {
				StringComp(const string& s) : a(s) { }
				bool operator()(const File& b) const { return stricmp(a, b.getName()) == 0; }
				const string& a;
			private:
				StringComp& operator=(const StringComp&);
			};
			struct FileTraits {
				size_t operator()( const File& a ) const
				  {
					  return hash<std::string>()(a.getName());
 			      }
			 bool operator()( const File &a, const File& b ) const
				 {
				     return a.getName() == b.getName();
				  }
			};
			typedef unordered_set<File, FileTraits,FileTraits> Set;


						File() : size(0), parent(0), hit(0), ts(0), ftype(SearchManager::TYPE_ANY) { }
						File(const string& aName, int64_t aSize, Directory::Ptr aParent, const TTHValue& aRoot, uint32_t aHit, uint32_t aTs, 
							 SearchManager::TypeModes aftype, const CFlyMediaInfo& p_media) :
							    tth(aRoot), size(aSize), parent(aParent.get()), hit(aHit), ts(aTs), ftype(aftype),
								m_media(p_media)
						{
							setName(aName);
				}
			File(const File& rhs) : 
			ftype(rhs.ftype),
							m_media(rhs.m_media),
							ts(rhs.ts), hit(rhs.hit), tth(rhs.getTTH()), size(rhs.getSize()), parent(rhs.getParent()
)
						{
			setName(rhs.getName());
			}

			~File() { }
						
						File& operator=(const File& rhs)
						{
							setName(rhs.m_name);
							size = rhs.size;
							parent = rhs.parent;
							tth = rhs.tth;
							hit = rhs.hit;
							ts = rhs.ts;
							m_media = rhs.m_media;
							ftype = rhs.ftype;
				return *this;
			}

			bool operator==(const File& rhs) const {
				return getParent() == rhs.getParent() && (stricmp(getName(), rhs.getName()) == 0);
			}
			string getADCPath() const { return parent->getADCPath() + getName(); }
			string getFullName() const { return parent->getFullName() + getName(); }
			string getRealPath() const { return parent->getRealPath(getName()); }

private: 
		 string m_name;
		 string m_low_name;
public: 
	       const string& getName() const { return m_name; } 
		   const string& getLowName() const //[+]PPA http://flylinkdc.blogspot.com/2010/08/1.html
		      {
			      return m_low_name; 
              } 
	       void setName(const string& p_name) 
		     { 
				  m_name = p_name; 
				  m_low_name = Text::toLower(m_name);
	         }
			GETSET(int64_t, size, Size);
			GETSET(Directory*, parent, Parent);
		    GETSET(uint32_t, hit, Hit);
		    GETSET(uint32_t, ts, TS);
						CFlyMediaInfo m_media;
			const TTHValue& getTTH() const
			{
				return tth;
			}
			void setTTH(const TTHValue& p_tth)
			{
				tth = p_tth;
			}
            SearchManager::TypeModes getFType() const
			{
				return ftype;
			}
		private:
			TTHValue tth;
			SearchManager::TypeModes ftype;
			
		};

		Map directories;
		File::Set files;
		int64_t size;

		static Ptr create(const string& aName, const Ptr& aParent = Ptr()) { return Ptr(new Directory(aName, aParent)); }

		bool hasType(uint32_t type) const noexcept {
			return ( (type == SearchManager::TYPE_ANY) || (fileTypes & (1 << type)) );
		}
		void addType(uint32_t type) noexcept;

		string getADCPath() const noexcept;
		string getFullName() const noexcept; 
		string getRealPath(const std::string& path) const;

		int64_t getSize() const noexcept;

		void search(SearchResultList& aResults, StringSearch::List& aStrings, int aSearchType, int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults) const noexcept;
		void search(SearchResultList& aResults, AdcSearch& aStrings, StringList::size_type maxResults) const noexcept;

		void toXml(OutputStream& xmlFile, string& indent, string& tmp2, bool fullList) const;
		void filesToXml(OutputStream& xmlFile, string& indent, string& tmp2) const;

		File::Set::const_iterator findFile(const string& aFile) const { return find_if(files.begin(), files.end(), Directory::File::StringComp(aFile)); }

		void merge(const Ptr& source);
		
		const string& getName() const
		{
			return m_name;
		}
		const string& getLowName() const  
		{
			return m_low_name;
		}
		void setName(const string& p_name)
		{
			m_name = p_name;
			m_low_name = Text::toLower(m_name);
		}
		GETSET(Directory*, parent, Parent);
	private:
		friend void intrusive_ptr_release(intrusive_ptr_base<Directory>*);

		Directory(const string& aName, const Ptr& aParent);
		~Directory() { }

		string m_name; 
		string m_low_name;  //[+]PPA http://flylinkdc.blogspot.com/2010/08/1.html
		/** Set of flags that say which SearchManager::TYPE_* a directory contains */
		uint32_t fileTypes;

	};

	friend class Directory;
	friend struct ShareLoader;

	friend class Singleton<ShareManager>;
	ShareManager();
	
	~ShareManager();
    class CFlyGetSysPath
	{
		string m_path;
	public:
        const string& getPath(int p_Type) //CSIDL_WINDOWS || CSIDL_PROGRAM_FILES
		{
		 if(!m_path.length())
		 {
		  TCHAR path[MAX_PATH];
						if(::SHGetFolderPath(NULL, p_Type, NULL, SHGFP_TYPE_CURRENT, path) == S_OK)
		  m_path = Text::fromT(tstring(path)) + PATH_SEPARATOR;
						else
							{
							m_path = Util::emptyString;
							}
		 }
		 return m_path;
		}
	};
	CFlyGetSysPath m_windows_path;
	CFlyGetSysPath m_pf_path;
		CFlyGetSysPath m_pf_x86_path;
		CFlyGetSysPath m_appdata_path;
		CFlyGetSysPath m_local_appdata_path;
	
	struct AdcSearch {
		AdcSearch(const StringList& params);

		bool isExcluded(const string& str);
		bool hasExt(const string& name);

		StringSearch::List* include;
		StringSearch::List includeX;
		StringSearch::List exclude;
		StringList ext;
		StringList noExt;

		int64_t gt;
		int64_t lt;

		TTHValue root;
		bool hasRoot;

		bool isDirectory;
	};

	int64_t xmlListLen;
	TTHValue xmlRoot;
	int64_t bzXmlListLen;
	TTHValue bzXmlRoot;
	unique_ptr<File> bzXmlRef;

	bool xmlDirty;
	bool forceXmlRefresh; /// bypass the 15-minutes guard
	bool refreshDirs;
	bool update;
	bool initial;

	int listN;

	atomic_flag refreshing;
	
	uint64_t lastXmlUpdate;
	uint64_t lastFullUpdate;

	mutable CriticalSection cs;

	// List of root directory items
	typedef std::list<Directory::Ptr> DirList;
	DirList directories;

	/** Map real name to virtual name - multiple real names may be mapped to a single virtual one */
	StringMap shares;

	friend class ::dht::IndexManager;
	
	typedef unordered_map<TTHValue, Directory::File::Set::const_iterator> HashFileMap;
	typedef HashFileMap::const_iterator HashFileIter;

	HashFileMap tthIndex;

	BloomFilter<5> bloom;
	
	Directory::File::Set::const_iterator findFile(const string& virtualFile) const;
	void inc_Hit(const string& p_Path,const string& p_FileName);

	Directory::Ptr buildTree(const string& aName, const Directory::Ptr& aParent, bool p_is_job);
	bool m_sweep_guard;
	bool m_sweep_path;
	bool checkHidden(const string& aName) const;

	void rebuildIndices();

	void updateIndices(Directory& aDirectory);
	void updateIndices(Directory& dir, const Directory::File::Set::iterator& i);
	
	Directory::Ptr merge(const Directory::Ptr& directory);
	
	void generateXmlList();
	StringList notShared;
	bool loadCache() noexcept;
	DirList::const_iterator getByVirtual(const string& virtualName) const noexcept;
	pair<Directory::Ptr, string> splitVirtual(const string& virtualPath) const;
	string findRealRoot(const string& virtualRoot, const string& virtualLeaf) const;

	Directory::Ptr getDirectory(const string& fname);

	int run();

	// QueueManagerListener
	void on(QueueManagerListener::FileMoved, const string& n) noexcept;

	// HashManagerListener
	void on(HashManagerListener::TTHDone, const string& fname, const TTHValue& root,
		        int64_t aTimeStamp, const CFlyMediaInfo& p_out_media, int64_t p_size) noexcept;

	// SettingsManagerListener
	void on(SettingsManagerListener::Save, SimpleXML& xml) noexcept {
		save(xml);
	}
	void on(SettingsManagerListener::Load, SimpleXML& xml) noexcept {
		load(xml);
	}
	
	// TimerManagerListener
	void on(TimerManagerListener::Minute, uint64_t tick) noexcept;
	void load(SimpleXML& aXml);
	void save(SimpleXML& aXml);
	
};

} // namespace dcpp

#endif // !defined(SHARE_MANAGER_H)

/**
 * @file
 * $Id: ShareManager.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
