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

#ifndef DCPLUSPLUS_DCPP_DIRECTORY_LISTING_H
#define DCPLUSPLUS_DCPP_DIRECTORY_LISTING_H

#include "forward.h"
#include "noexcept.h"

#include "HintedUser.h"
#include "FastAlloc.h"
#include "MerkleTree.h"
#include "Streams.h"
#include "QueueItem.h"
#include "CFlyMediaInfo.h"
#include "UserInfoBase.h"

namespace dcpp {
	
class ListLoader;
STANDARD_EXCEPTION(AbortException);

class DirectoryListing : boost::noncopyable, public UserInfoBase
{
public:
	class Directory;
        // !SMT!-UI  dupe/downloads search results in both class File and class Directory
        enum {
                FLAG_SHARED             = 1 << 0,
                FLAG_NOT_SHARED         = 1 << 1,
		FLAG_DOWNLOAD           = 1 << 2, //[+]PPA
                FLAG_OLD_TTH            = 1 << 3 //[+]PPA
        };

	class File : public FastAlloc<File>, public Flags {
	public:
		typedef File* Ptr;
		struct FileSort {
			bool operator()(const Ptr& a, const Ptr& b) const {
				return stricmp(a->getName().c_str(), b->getName().c_str()) < 0;
			}
		};
		typedef vector<Ptr> List;
		typedef List::const_iterator Iter;
		
				File(Directory* aDir, const string& aName, int64_t aSize, const string& aTTH, uint32_t p_Hit, uint32_t p_ts, const CFlyMediaInfo& p_media) noexcept :
					name(aName), size(aSize), parent(aDir), tthRoot(aTTH), adls(false), hit(p_Hit), ts(p_ts), m_media(p_media)
		{
		} 
				File(const File& rhs, bool _adls = false) : name(rhs.name), size(rhs.size), parent(rhs.parent), tthRoot(rhs.tthRoot), adls(_adls), 
					hit(rhs.hit), ts(rhs.ts),m_media(rhs.m_media)
		{
		}
				
				File& operator=(const File& rhs)
				{
					name = rhs.name;
					size = rhs.size;
					parent = rhs.parent;
					tthRoot = rhs.tthRoot;
					hit = rhs.hit;
					ts = rhs.ts;
					m_media = rhs.m_media;
			return *this;
		}

		~File() { }

		GETSET(TTHValue, tthRoot, TTH);
		GETSET(string, name, Name);
		GETSET(int64_t, size, Size);
		GETSET(Directory*, parent, Parent);
		GETSET(uint32_t, hit, Hit);
		GETSET(uint32_t, ts, TS);
				CFlyMediaInfo m_media;
		GETSET(bool, adls, Adls);
	};

	class Directory : public FastAlloc<Directory> 
                        , public Flags //!fulDC! !SMT!-UI 
, boost::noncopyable
 {
	public:
		typedef Directory* Ptr;
		struct DirSort {
			bool operator()(const Ptr& a, const Ptr& b) const {
				return stricmp(a->getName().c_str(), b->getName().c_str()) < 0;
			}
		};
		typedef vector<Ptr> List;
		typedef List::const_iterator Iter;

		typedef unordered_set<TTHValue> TTHSet;
		
		List directories;
		File::List files;
		
		Directory(Directory* aParent, const string& aName, bool _adls, bool aComplete) 
			: name(aName), parent(aParent), adls(_adls), complete(aComplete) { }
		
		virtual ~Directory();

		size_t getTotalFileCount(bool adls = false) const;		
		int64_t getTotalSize(bool adls = false) const;
		int64_t getTotalHit() const;
		uint32_t getTotalTS() const;
		uint16_t getTotalBitrate() const;
		void filterList(DirectoryListing& dirList);
		void filterList(TTHSet& l);
		void getHashList(TTHSet& l);
		
		size_t getFileCount() const { return files.size(); }
		
		int64_t getSize() const {
			int64_t x = 0;
			for(File::Iter i = files.begin(); i != files.end(); ++i) {
				x+=(*i)->getSize();
			}
			return x;
		}
		int64_t getHit() const {
			int64_t x = 0;
			for(File::Iter i = files.begin(); i != files.end(); ++i) {
				x+=(*i)->getHit();
			}
			return x;
		}
		uint16_t getBitrate() const {
			uint16_t x = 0;
					for (File::Iter i = files.begin(); i != files.end(); ++i)
					{
						x = std::max((*i)->m_media.m_bitrate, x);
			}
			return x;
		}
		uint32_t getTS() const {
			uint32_t x = 0;
			for(File::Iter i = files.begin(); i != files.end(); ++i) {
				x = std::max((*i)->getTS(),x);
			}
			return x;
		}
		
        void checkDupes(const DirectoryListing* lst); // !SMT!-UI
		GETSET(string, name, Name);
		GETSET(Directory*, parent, Parent);		
		GETSET(bool, adls, Adls);		
		GETSET(bool, complete, Complete);
	};

	class AdlDirectory : public Directory {
	public:
		AdlDirectory(const string& aFullPath, Directory* aParent, const string& aName) : Directory(aParent, aName, true, true), fullPath(aFullPath) { }

		GETSET(string, fullPath, FullPath);
	};

	DirectoryListing(const HintedUser& aUser);
	~DirectoryListing();
	
	void loadFile(const string& name, bool p_own_list = false);

	string updateXML(const std::string&, bool p_own_list);
	string loadXML(InputStream& xml, bool updating, bool p_own_list);

	void download(const string& aDir, const string& aTarget, bool highPrio, QueueItem::Priority prio = QueueItem::DEFAULT);
	void download(Directory* aDir, const string& aTarget, bool highPrio, QueueItem::Priority prio = QueueItem::DEFAULT);
	void download(File* aFile, const string& aTarget, bool view, bool highPrio, QueueItem::Priority prio = QueueItem::DEFAULT);

	string getPath(const Directory* d) const;
	string getPath(const File* f) const { return getPath(f->getParent()); }

	int64_t getTotalSize(bool adls = false) { return root->getTotalSize(adls); }
	size_t getTotalFileCount(bool adls = false) { return root->getTotalFileCount(adls); }

	const Directory* getRoot() const { return root; }
	Directory* getRoot() { return root; }

	void checkDupes(); // !fulDC!
	static UserPtr getUserFromFilename(const string& fileName);
	
	const UserPtr& getUser() const { return hintedUser.user; }	
		
	GETSET(HintedUser, hintedUser, HintedUser);
	GETSET(bool, abort, Abort);
		void log_matched_files(const HintedUser& p_user,int p_count); //[+]PPA
private:
	friend class ListLoader;

	Directory* root;
		
	Directory* find(const string& aName, Directory* current);
};

inline bool operator==(DirectoryListing::Directory::Ptr a, const string& b) { return stricmp(a->getName(), b) == 0; }
inline bool operator==(DirectoryListing::File::Ptr a, const string& b) { return stricmp(a->getName(), b) == 0; }

} // namespace dcpp

#endif // !defined(DIRECTORY_LISTING_H)

/**
 * @file
 * $Id: DirectoryListing.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
