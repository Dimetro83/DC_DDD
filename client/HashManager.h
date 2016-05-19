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

#ifndef DCPLUSPLUS_DCPP_HASH_MANAGER_H
#define DCPLUSPLUS_DCPP_HASH_MANAGER_H

#include "Singleton.h"
#include "MerkleTree.h"
#include "Thread.h"
#include "Semaphore.h"
#include "TimerManager.h"
#include "Util.h"
#include "FastAlloc.h"
#include "Text.h"
#include "Streams.h"
#include "CFlylinkDBManager.h"

#define IRAINMAN_NTFS_STREAM_TTH

namespace dcpp {

STANDARD_EXCEPTION(HashException);
class File;

class HashManagerListener {
public:
	virtual ~HashManagerListener() { }
		template<int I> struct X
		{
			enum { TYPE = I };
		};

	typedef X<0> TTHDone;

	virtual void on(TTHDone, const string& /* fileName */, const TTHValue& /* root */,
		                int64_t aTimeStamp, const CFlyMediaInfo& p_out_media, int64_t p_size) noexcept = 0;
};

class FileException;

class HashManager : public Singleton<HashManager>, public Speaker<HashManagerListener>,
	private TimerManagerListener 
{
public:

	HashManager() {
		TimerManager::getInstance()->addListener(this);
	}
	virtual ~HashManager() {
		TimerManager::getInstance()->removeListener(this);
		hasher.join();
	}

		void hashFile(const string& fileName, int64_t aSize)
	{
			// Lock l(cs); [-] IRainman fix: this lock is epic fail...
			hasher.hashFile(fileName, aSize);
	}

	/**
	 * Check if the TTH tree associated with the filename is current.
	 */
    bool checkTTH(const string& fname,const string& fpath,int64_t p_path_id, int64_t aSize, int64_t aTimeStamp, TTHValue& p_out_tth);
		void stopHashing(const string& baseDir)
		{
			hasher.stopHashing(baseDir);
		}
		void setPriority(Thread::Priority p)
		{
			hasher.setThreadPriority(p);
		}

	/** Return block size of the tree associated with root, or 0 if no such tree is in the store */
	size_t getBlockSize(const TTHValue& root);

	/** @return TTH root */
	const TTHValue getTTH(const string& fname,const string& fpath, int64_t aSize) throw(HashException);

		void addTree(const string& aFileName, uint64_t aTimeStamp, const TigerTree& tt, int64_t p_Size)
      { 
			hashDone(aFileName, aTimeStamp, tt, -1, false, p_Size);
      }
		void addTree(const TigerTree& p_tree);

		void getStats(string& curFile, int64_t& bytesLeft, size_t& filesLeft)
		{
		hasher.getStats(curFile, bytesLeft, filesLeft);
	}

	/**
	 * Rebuild hash data file
	 */
		void rebuild()
		{
			hasher.scheduleRebuild();
		}

	void startup() 
        { 
          hasher.start(); 
	}

		void shutdown()
		{
		// [-] brain-ripper
		// Removed critical section - it caused deadlock on client exit:
		// "join" hangs on WaitForSingleObject(threadHandle, INFINITE),
		// and threadHandle thread hangs on this very critical section
		// in HashManager::hashDone
		//Lock l(cs); //[!]IRainman
		
		hasher.shutdown();
		hasher.join();
	}
		void getMediaInfo(const string& p_name, CFlyMediaInfo& p_media, int64_t p_size, const TigerTree& p_tth);

		struct HashPauser
		{
		HashPauser();
		~HashPauser();

	private:
		bool resume;
	};
	
	/// @return whether hashing was already paused
	bool pauseHashing();
	void resumeHashing();	
	bool isHashingPaused() const;


//[+] Greylink
	#ifdef IRAINMAN_NTFS_STREAM_TTH
	class StreamStore { // greylink dc++: work with ntfs stream
		public:
			//StreamStore(const string& aFileName) : fileName(aFileName) { } [-] IRainman
			bool loadTree(const string& p_filePath, TigerTree& p_Tree, int64_t p_aFileSize = -1);// [+] IRainman const string& p_filePath
			bool saveTree(const string& p_filePath, const TigerTree& p_Tree);// [+] IRainman const string& p_filePath
			void deleteStream(const string& p_filePath);//[+] IRainman
		private:
			struct TTHStreamHeader {
				uint32_t magic;
				uint32_t checksum;	// xor of other TTHStreamHeader DWORDs
				uint64_t fileSize;
				uint64_t timeStamp;
				uint64_t blockSize;
				TTHValue root;
			};
			static const uint64_t g_minStreamedFileSize = 16*1048576;
			static const uint32_t g_MAGIC = '++lg';
			static const string g_streamName;
			static std::set<char> g_error_tth_stream; //[+]PPA
			static void addBan(const string& p_filePath)
			{
	          if(p_filePath.size())
				  g_error_tth_stream.insert(p_filePath[0]);
			}
			static bool isBan(const string& p_filePath)
			{
	          return p_filePath.size() && g_error_tth_stream.count(p_filePath[0]) == 1;
			}
			//string fileName; [-] IRainman

			void setCheckSum(TTHStreamHeader& p_header);
			bool validateCheckSum(const TTHStreamHeader& p_header);
		};
//[~] Greylink
	StreamStore m_streamstore;
	#endif // IRAINMAN_NTFS_STREAM_TTH

private:
	class Hasher : public Thread {
	public:
		Hasher() : stop(false), running(false), paused(0), rebuild(false), currentSize(0) { }

		void hashFile(const string& fileName, int64_t size);

		/// @return whether hashing was already paused
		bool pause();
		void resume();
		bool isPaused() const;

		void stopHashing(const string& baseDir);
		int run();
		bool fastHash(const string& fname, uint8_t* buf, TigerTree& tth, int64_t size);
		void getStats(string& curFile, int64_t& bytesLeft, size_t& filesLeft);
		void shutdown() { stop = true; if(paused) s.signal(); s.signal(); }
		void scheduleRebuild() { rebuild = true; if(paused) s.signal(); s.signal(); }

	private:
		// Case-sensitive (faster), it is rather unlikely that case changes, and if it does it's harmless.
		// map because it's sorted (to avoid random hash order that would create quite strange shares while hashing)
		typedef map<string, int64_t> WorkMap;	
		typedef WorkMap::iterator WorkIter;

		WorkMap w;
		mutable CriticalSection cs;
		Semaphore s;

		bool stop;
		bool running;
		int64_t paused; //[!] PPA -> int
		bool rebuild;
		string currentFile;
		int64_t currentSize;

		void instantPause();
	};

	friend class Hasher;

		bool addFile(const string& p_FileName, int64_t p_TimeStamp, const TigerTree& p_tth, int64_t p_size, CFlyMediaInfo& p_out_media);
	#ifdef IRAINMAN_NTFS_STREAM_TTH
		bool HashManager::addFileFromStream(const string& p_name, const TigerTree& p_TT, int64_t p_size);
	#endif

	Hasher hasher;

	mutable CriticalSection cs;

	/** Single node tree where node = root, no storage in HashData.dat */
	static const int64_t SMALL_TREE = -1;

		void hashDone(const string& aFileName, uint64_t aTimeStamp, const TigerTree& tth, int64_t speed, 
		              bool p_is_ntfs,
		              int64_t p_Size);
		void doRebuild()
		{
		Lock l(cs);
		rebuild();
	}
};

} // namespace dcpp

#endif // !defined(HASH_MANAGER_H)

/**
 * @file
 * $Id: HashManager.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
