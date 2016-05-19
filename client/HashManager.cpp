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

#include "stdinc.h"
#include "HashManager.h"

#include "ResourceManager.h"
#include "SimpleXML.h"
#include "LogManager.h"
#if defined(USE_MEDIAINFO_DLL)
#include "mp3info/MediaInfoDLL.h" //[+]PPA
#else
#include "..\MediaInfoLib\Source\MediaInfo\MediaInfo.h"
#endif

#ifndef _WIN32
#include <sys/mman.h> // mmap, munmap, madvise
#include <signal.h>  // for handling read errors from previous trio
#include <setjmp.h>
#endif
#include <boost/algorithm/string.hpp>

std::string g_cur_mediainfo_file;

namespace dcpp {

#ifdef IRAINMAN_NTFS_STREAM_TTH
//[+] Greylink
const string HashManager::StreamStore::g_streamName(".gltth");
std::set<char> HashManager::StreamStore::g_error_tth_stream;


void HashManager::StreamStore::setCheckSum(TTHStreamHeader& p_header) {
	p_header.magic = g_MAGIC;
	uint32_t l_sum = 0;
	for(size_t i = 0; i < sizeof(TTHStreamHeader) / sizeof(uint32_t); i++)
		l_sum ^= ((uint32_t*)&p_header)[i];
	p_header.checksum ^= l_sum;
}

bool HashManager::StreamStore::validateCheckSum(const TTHStreamHeader& p_header) {
	if(p_header.magic != g_MAGIC)
		return false;
	uint32_t l_sum = 0;
	for(size_t i = 0; i < sizeof(TTHStreamHeader) / sizeof(uint32_t); i++)
		l_sum ^= ((uint32_t*)&p_header)[i];
	return (l_sum == 0);
}
void HashManager::addTree(const TigerTree& p_tree)
{
	CFlylinkDBManager::getInstance()->addTree(p_tree);
}

bool HashManager::StreamStore::loadTree(const string& p_filePath, TigerTree& p_Tree, int64_t p_FileSize) 
{
	//[!] TODO выполнить проверку на тип файловой системы + wine
	try {
#ifdef RIP_USE_STREAM_SUPPORT_DETECTION
		if (!m_FsDetect.IsStreamSupported(p_filePath.c_str()))
#else
		if(isBan(p_filePath))
#endif
			return false;
		const int64_t fileSize = (p_FileSize == -1)? File::getSize(p_filePath) : p_FileSize;
		if(fileSize < g_minStreamedFileSize) // that's why minStreamedFileSize never be changed!
			return false;
		const uint64_t timeStamp = File::getTimeStamp(p_filePath);
		{
			File stream(p_filePath + ":" + g_streamName, File::READ, File::OPEN);
			size_t sz = sizeof(TTHStreamHeader);
			TTHStreamHeader h;
			if(stream.read(&h, sz) != sizeof(TTHStreamHeader))
				return false;
			if((uint64_t)fileSize != h.fileSize || timeStamp != h.timeStamp || !validateCheckSum(h))
				return false;
			size_t datalen = TigerTree::calcBlocks(fileSize, h.blockSize) * TTHValue::BYTES;
			sz = datalen;
			AutoArray<uint8_t> buf(datalen);
			if(stream.read((uint8_t*)buf, sz) != datalen)
				return false;
			p_Tree = TigerTree(fileSize, h.blockSize, buf,datalen);
			if(!(p_Tree.getRoot() == h.root))
				return false;
			}
	} catch(const Exception& /*e*/)
	{
		//LogManager::getInstance()->message("ERROR_GET_TTH_STREAM " + p_filePath + " : " + e.getError());// [+]IRainman
		return false;
	}
/*
if (speed > 0) {
		LogManager::getInstance()->message(STRING(HASHING_FINISHED) + " " + fn + " (" + Util::formatBytes(speed) + "/s)");
	} else {
		LogManager::getInstance()->message(STRING(HASHING_FINISHED) + " " + fn);
	}
*/
	return true;
}

bool HashManager::StreamStore::saveTree(const string& p_filePath, const TigerTree& p_Tree) {
	if(g_isWineCheck || //[+]PPA под линуксом не пишем в поток
		 !BOOLSETTING(SAVE_TTH_IN_NTFS_FILESTREAM))
		return false;
#ifdef RIP_USE_STREAM_SUPPORT_DETECTION
	if (!m_FsDetect.IsStreamSupported(p_filePath.c_str()))
#else
	if(isBan(p_filePath))
#endif
		return false;
	try {
		TTHStreamHeader h;
		h.fileSize = File::getSize(p_filePath);
		if (h.fileSize < g_minStreamedFileSize || h.fileSize != (uint64_t)p_Tree.getFileSize())
			return false; // that's why minStreamedFileSize never be changed!
		h.timeStamp = File::getTimeStamp(p_filePath);
		h.root = p_Tree.getRoot();
		h.blockSize = p_Tree.getBlockSize();
		setCheckSum(h);
		{
			File stream(p_filePath + ":" + g_streamName, File::WRITE, File::CREATE | File::TRUNCATE);
			stream.write(&h, sizeof(TTHStreamHeader));
			stream.write(p_Tree.getLeaves()[0].data, p_Tree.getLeaves().size() * TTHValue::BYTES);
		}
		File::setTimeStamp(p_filePath, h.timeStamp);
		LogManager::getInstance()->message("[OK] TTH -> NTFS Stream " + p_filePath );
	} catch(const Exception& e) {
		addBan(p_filePath);
		LogManager::getInstance()->message("ERROR_ADD_TTH_STREAM) " + p_filePath + " : " + e.getError());// [+]IRainman
		return false;
	}
	return true;
}
//[~] Greylink

void HashManager::StreamStore::deleteStream(const string& p_filePath) {
	try {
		File::deleteFile(p_filePath + ":" + g_streamName);
	} catch
		(const FileException& e) {
			LogManager::getInstance()->message("ERROR_DELETE_TTH_STREAM " + p_filePath + " : " + e.getError());
		}
}

#ifdef RIP_USE_STREAM_SUPPORT_DETECTION
void HashManager::SetFsDetectorNotifyWnd(HWND hWnd)
{
	m_streamstore.SetFsDetectorNotifyWnd(hWnd);
}
#endif

bool HashManager::addFileFromStream(const string& p_name, const TigerTree& p_TT, int64_t p_size)
{
	const int64_t l_TimeStamp = File::getTimeStamp(p_name);
	CFlyMediaInfo l_out_media;
	return addFile(p_name, l_TimeStamp, p_TT, p_size, l_out_media);
}

void HashManager::getMediaInfo(const string& p_name, CFlyMediaInfo& p_media, int64_t p_size, const TigerTree& p_tth)
{
	try
	{
#if defined(USE_MEDIAINFO_DLL)
  static MediaInfoDLL::MediaInfo g_media_info_dll;
#else
  static MediaInfoLib::MediaInfo g_media_info_dll;
#endif
  static unordered_set<string> g_media_ext_set; 

  p_media.init();

#if defined(USE_MEDIAINFO_DLL)
  if(!g_media_info_dll.IsReady())
	return;
#endif
  extern const char* g_media_ext[];
		if (g_media_ext_set.empty())
	for (size_t i = 0; g_media_ext[i]; ++i)
	  g_media_ext_set.insert(string(".") + g_media_ext[i]);

  if (!g_media_ext_set.count(Text::toLower(Util::getFileExt(p_name)))) 
		return;
		char l_size[64];
		l_size[0] = 0;
		_snprintf(l_size, sizeof(l_size), "%I64d", p_size);
		g_cur_mediainfo_file = p_name + "\r\nTTH = " + p_tth.getRoot().toBase32() + "\r\nFile size = " + string(l_size);
	if(g_media_info_dll.Open(Text::toT(p_name)) )
 	{
	 const size_t audioCount =  g_media_info_dll.Count_Get(MediaInfoLib::Stream_Audio);
	 p_media.m_bitrate  = 0;
	 std::string audioFormatString;
	 for (size_t i = 0; i<audioCount; i++)
	 {
		 const string l_sinfo = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Audio, i, _T("BitRate")));
		 uint16_t bitRate = ( atoi(l_sinfo.c_str())/1000.0 + 0.5);
		 if ( bitRate > p_media.m_bitrate)
			 p_media.m_bitrate = bitRate;
		  
		 
		 std::string sFormat = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Audio, i, _T("Format")));
#if defined (SSA_REMOVE_NEEDLESS_WORDS_FROM_VIDEO_AUDIO_INFO)
		 boost::replace_all(sFormat, "Audio", "");
#endif
		 std::string sBitRate = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Audio, i, _T("BitRate/String")));
		 std::string sChannels = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Audio, i, _T("Channel(s)/String")));
		 std::string sLanguage = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Audio, i, _T("Language/String1")));

		 if ( !sFormat.empty() || !sBitRate.empty() || !sChannels.empty() || !sLanguage.empty())
		 {
			 audioFormatString += " |";
					if (!sFormat.empty())
					{
						audioFormatString += ' ';
				 audioFormatString += sFormat;
				 audioFormatString += ",";
		}
					if (!sBitRate.empty())
					{
						audioFormatString += ' ';
				 audioFormatString += sBitRate;
				 audioFormatString += ",";
			 }
					if (!sChannels.empty())
					{
						audioFormatString += ' ';
				 audioFormatString += sChannels;
				 audioFormatString += ",";
			 }
					if (!sLanguage.empty())
					{
						audioFormatString += ' ';
				 audioFormatString += sLanguage;
				 audioFormatString += ",";
			 }
			 audioFormatString = audioFormatString.substr(0, audioFormatString.length() - 1); // Remove last , 
		 }

	 }
			string width;
			string height;
#ifdef USE_MEDIAINFO_IMAGES
			width = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Image, 0, _T("Width")));
			height = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Image, 0, _T("Height")));
	 if (width.empty() && height.empty())
#endif
		{
		 width = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Video, 0, _T("Width")));
		 height = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Video, 0, _T("Height")));
		}
	 p_media.m_mediaX = atoi(width.c_str());
	 p_media.m_mediaY = atoi(height.c_str());

#if defined (SSA_USE_OVERALBITRATE_IN_AUDIO_INFO)
	 std::string sOveralBitRate = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_General, 0, _T("OverallBitRate/String")));
#endif
	 std::string sDuration = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_General, 0, _T("Duration/String")));
	 if (!sDuration.empty() 
#if defined (SSA_USE_OVERALBITRATE_IN_AUDIO_INFO)
		 || !sOveralBitRate.empty()
#endif
		 )
	 {
		 std::string audioGeneral;
#if defined (SSA_USE_OVERALBITRATE_IN_AUDIO_INFO)
		 if (!sOveralBitRate.empty())
		 {
			 audioGeneral += sOveralBitRate;
			 audioGeneral += ",";
		 }
#endif
		 if (!sDuration.empty())
		 {
			 audioGeneral += sDuration;
			 audioGeneral += ",";
		 }
		 p_media.m_audio = audioGeneral.substr(0, audioGeneral.length() - 1); // Remove last , 
		 // No Duration => No sound
		 if ( !audioFormatString.empty())
			p_media.m_audio += audioFormatString;
	 }

			const size_t videoCount =  g_media_info_dll.Count_Get(MediaInfoLib::Stream_Video);
	 if (videoCount > 0 )
	 {
		 string videoString;
		 for (size_t i = 0; i< videoCount; i++)
		 {
			 std::string sVFormat = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Video, i, _T("Format")));
#if defined (SSA_REMOVE_NEEDLESS_WORDS_FROM_VIDEO_AUDIO_INFO)
			 boost::replace_all(sVFormat, "Video", "");
			 boost::replace_all(sVFormat, "Visual", "");
#endif
			 std::string sVBitrate = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Video, i, _T("BitRate/String")));
			 std::string sVAspectRatio = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Video, i, _T("DisplayAspectRatio/String")));
			 std::string sVFrameRate = Text::fromT(g_media_info_dll.Get(MediaInfoLib::Stream_Video, i, _T("FrameRate/String")));
			 if ( !sVFormat.empty() || !sVBitrate.empty() || !sVAspectRatio.empty() || !sVFrameRate.empty())
			 {

						if (!sVFormat.empty())
						{
					 videoString += sVFormat;
					 videoString += ", ";
				 }
						if (!sVBitrate.empty())
						{
					 videoString += sVBitrate;
					 videoString += ", ";
				 }
						if (!sVAspectRatio.empty())
						{
					 videoString += sVAspectRatio;
					 videoString += ", ";
				 }
						if (!sVFrameRate.empty())
						{
					 videoString += sVFrameRate;
					 videoString += ", ";
				 }
				 videoString = videoString.substr(0, videoString.length() - 2); // Remove last ,
				 videoString += " | ";
			 }
		 }

		 if (videoString.length() > 3) // This is false only in theorical way.
			p_media.m_video = videoString.substr(0, videoString.length() - 3); // Remove last |
	 }
     g_media_info_dll.Close();
     }
	g_cur_mediainfo_file = Util::emptyString;
	}
	catch(std::exception& e)
	{
		LogManager::getInstance()->message("HashManager::getMediaInfo: " + p_name + "TTH = " + p_tth.getRoot().toBase32() + " error =" + string(e.what()));
		char l_buf[4000];
		sprintf_s(l_buf, _countof(l_buf), "При сканировании файла %s в MediaInfoLib возникла ошибка:\r\n%s\r\nЕсли возможно, вышлите данный файл для анализа на адрес ppa74@ya.ru", // TODO translate
		  p_name.c_str(), 
		  e.what());
		::MessageBox(0,Text::toT(l_buf).c_str(), _T("Error mediainfo!"), MB_ICONERROR);
	}
}
#endif // IRAINMAN_NTFS_STREAM_TTH

bool HashManager::checkTTH(const string& fname,const string& fpath,int64_t p_path_id, int64_t aSize, int64_t aTimeStamp, TTHValue& p_out_tth) {
	const bool l_db = CFlylinkDBManager::getInstance()->checkTTH(fname, p_path_id, aSize, aTimeStamp, p_out_tth);
	#ifdef IRAINMAN_NTFS_STREAM_TTH
	const string name = fpath + fname;
	#endif // IRAINMAN_NTFS_STREAM_TTH
	if(!l_db) 
	{
		#ifdef IRAINMAN_NTFS_STREAM_TTH
		TigerTree l_TT;
		if(m_streamstore.loadTree(name, l_TT)) {
			addFileFromStream(name, l_TT, aSize);
		} else {
		#endif // IRAINMAN_NTFS_STREAM_TTH

		hasher.hashFile(name, aSize);
		return false;

		#ifdef IRAINMAN_NTFS_STREAM_TTH
		}
		#endif // IRAINMAN_NTFS_STREAM_TTH
	}
	#ifdef IRAINMAN_NTFS_STREAM_TTH
	else {// if(!l_db)
		if(File::isExist(name)) {
			TigerTree l_TT;
			if(CFlylinkDBManager::getInstance()->getTree(p_out_tth, l_TT))
				m_streamstore.saveTree(name, l_TT);
		}
	}
	#endif // IRAINMAN_NTFS_STREAM_TTH
	return true;
}

const TTHValue HashManager::getTTH(const string& fname, const string& fpath, int64_t aSize) throw(HashException)
{
	const TTHValue* tth = CFlylinkDBManager::getInstance()->findTTH(fname, fpath); // [!] IRainman fix: no needs to lock.
	if (tth == NULL)
	{
		const string name = fpath + fname;
		#ifdef IRAINMAN_NTFS_STREAM_TTH
		TigerTree l_TT;
		if (m_streamstore.loadTree(name, l_TT)) // [!] IRainman fix: no needs to lock.
		{
			addFileFromStream(name, l_TT, aSize); // [!] IRainman fix: no needs to lock.
			tth = CFlylinkDBManager::getInstance()->findTTH(fname, fpath); // [!] IRainman fix: no needs to lock.
		}
		else
		{
		#endif // IRAINMAN_NTFS_STREAM_TTH

			hasher.hashFile(name , aSize); // [!] IRainman fix: no needs to lock.
		throw HashException(Util::emptyString);		

		#ifdef IRAINMAN_NTFS_STREAM_TTH
		}
		if(!BOOLSETTING(SAVE_TTH_IN_NTFS_FILESTREAM))
			HashManager::getInstance()->m_streamstore.deleteStream(name); // [!] IRainman fix: no needs to lock.
		#endif // IRAINMAN_NTFS_STREAM_TTH
	}
	return *tth;
}

void HashManager::hashDone(const string& aFileName, uint64_t aTimeStamp, const TigerTree& tth, int64_t speed, 
                           bool p_is_ntfs, int64_t p_size)
{
	Lock l(cs);
	dcassert(!aFileName.empty());
	if (aFileName.empty())
	{
		LogManager::getInstance()->message("HashManager::hashDone - aFileName.empty()");
		return;
	}
	CFlyMediaInfo l_out_media;
	try
	{
		addFile(aFileName, aTimeStamp, tth, p_size, l_out_media);
#ifdef IRAINMAN_NTFS_STREAM_TTH
		if (!p_is_ntfs) // [+] PPA TTH received from the NTFS stream, do not write back!
			if (BOOLSETTING(SAVE_TTH_IN_NTFS_FILESTREAM))
			HashManager::getInstance()->m_streamstore.saveTree(aFileName, tth);
		else
			HashManager::getInstance()->m_streamstore.deleteStream(aFileName);
		#endif // IRAINMAN_NTFS_STREAM_TTH
	}
	catch (const Exception& e)
	{
		LogManager::getInstance()->message(STRING(HASHING_FAILED) + ' ' + aFileName + e.getError());
		return;
	}

	fire(HashManagerListener::TTHDone(), aFileName, tth.getRoot(), aTimeStamp, l_out_media, p_size);
	     
	string fn = aFileName;
	if (count(fn.begin(), fn.end(), PATH_SEPARATOR) >= 2)
	{
		string::size_type i = fn.rfind(PATH_SEPARATOR);
		i = fn.rfind(PATH_SEPARATOR, i - 1);
		fn.erase(0, i);
		fn.insert(0, "...");
	}
	if (speed > 0)
	{
		LogManager::getInstance()->message(STRING(HASHING_FINISHED) + ' ' + fn + " (" + Util::formatBytes(speed) + "/s)");
	}
	else
	{
		LogManager::getInstance()->message(STRING(HASHING_FINISHED) + ' ' + fn);
	}
}

bool HashManager::addFile(const string& p_FileName, int64_t p_TimeStamp, const TigerTree& p_tth, int64_t p_size, CFlyMediaInfo& p_out_media)
{
	const string l_name = Text::toLower(Util::getFileName(p_FileName));
	const string l_path = Text::toLower(Util::getFilePath(p_FileName));
	p_out_media.init();
	getMediaInfo(p_FileName, p_out_media, p_size, p_tth);
	return CFlylinkDBManager::getInstance()->addFile(l_path, l_name, p_TimeStamp, p_tth, p_out_media, false);
}

void HashManager::Hasher::hashFile(const string& fileName, int64_t size) {
	Lock l(cs);
	if (w.insert(make_pair(fileName, size)).second) {
		if(paused > 0)
			paused++;
		else
			s.signal();
	}
}
bool HashManager::Hasher::pause() {
	Lock l(cs);
	return paused++ > 0;
}

void HashManager::Hasher::resume() {
	Lock l(cs);
	while(--paused > 0)
		s.signal();
}

bool HashManager::Hasher::isPaused() const {
	Lock l(cs);
	return paused > 0;
}

void HashManager::Hasher::stopHashing(const string& baseDir) {
	Lock l(cs);
	for (WorkIter i = w.begin(); i != w.end();) {
		if (strnicmp(baseDir, i->first, baseDir.length()) == 0) {
			w.erase(i++);
		} else {
			++i;
		}
	}
}
size_t HashManager::getBlockSize(const TTHValue& p_root) 
{
	return size_t(CFlylinkDBManager::getInstance()->getBlockSize(p_root));
}
void HashManager::Hasher::getStats(string& curFile, int64_t& bytesLeft, size_t& filesLeft) {
	Lock l(cs);
	curFile = currentFile;
	filesLeft = w.size();
	if (running)
		filesLeft++;
	bytesLeft = 0;
	for (WorkMap::const_iterator i = w.begin(); i != w.end(); ++i) {
		bytesLeft += i->second;
	}
	bytesLeft += currentSize;
}

void HashManager::Hasher::instantPause() {
	bool wait = false;
	{
		Lock l(cs);
		if(paused > 0) {
			paused++;
			wait = true;
		}
	}
	if(wait)
		s.wait();
}

#ifdef _WIN32
#define BUF_SIZE (2048*1024) //(c)Alexey Vinogradov

bool HashManager::Hasher::fastHash(const string& fname, uint8_t* buf, TigerTree& tth, int64_t size) {
	HANDLE h = INVALID_HANDLE_VALUE;
	DWORD x, y;
	if (!GetDiskFreeSpace(Text::toT(Util::getFilePath(fname)).c_str(), &y, &x, &y, &y)) {
		return false;
	} else {
		if ((BUF_SIZE % x) != 0) {
			return false;
		} else {
			h = ::CreateFile(Text::toT(fname).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
				FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);
			if (h == INVALID_HANDLE_VALUE)
				return false;
		}
	}
	DWORD hn = 0;
	DWORD rn = 0;
	uint8_t* hbuf = buf + BUF_SIZE;
	uint8_t* rbuf = buf;

	OVERLAPPED over = { 0 };
	BOOL res = TRUE;
	over.hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	
	bool ok = false;

	uint64_t lastRead = GET_TICK();
	if (!::ReadFile(h, hbuf, BUF_SIZE, &hn, &over)) {
		if (GetLastError() == ERROR_HANDLE_EOF) {
			hn = 0;
		} else if (GetLastError() == ERROR_IO_PENDING) {
			if (!GetOverlappedResult(h, &over, &hn, TRUE)) {
				if (GetLastError() == ERROR_HANDLE_EOF) {
					hn = 0;
				} else {
					goto cleanup;
				}
			}
		} else {
			goto cleanup;
		}
	}

	over.Offset = hn;
	size -= hn;
	while (!stop) {
		if (size > 0) {
			// Start a new overlapped read
			ResetEvent(over.hEvent);
			if (SETTING(MAX_HASH_SPEED) > 0) {
				uint64_t now = GET_TICK();
				uint64_t minTime = hn * 1000LL / (SETTING(MAX_HASH_SPEED) * 1024LL * 1024LL);
				if (lastRead + minTime > now) {
					uint64_t diff = now - lastRead;
					Thread::sleep(minTime - diff);
				} 
				lastRead = lastRead + minTime;
			} else {
				lastRead = GET_TICK();
			}
			res = ReadFile(h, rbuf, BUF_SIZE, &rn, &over);
		} else {
			rn = 0;
		}

		tth.update(hbuf, hn);
	
		{
			Lock l(cs);
			currentSize = max(currentSize - hn, _LL(0));
		}
		
		if (size == 0) {
			ok = true;
			break;
		}

		if (!res) { 
			// deal with the error code 
			switch (GetLastError()) { 
			case ERROR_IO_PENDING: 
				if (!GetOverlappedResult(h, &over, &rn, TRUE)) {
					dcdebug("Error 0x%x: %s\n", GetLastError(), Util::translateError(GetLastError()).c_str());
						goto cleanup;
					}
				break;
			default:
				dcdebug("Error 0x%x: %s\n", GetLastError(), Util::translateError(GetLastError()).c_str());
				goto cleanup;
			}
		}

		instantPause();

		*((uint64_t*)&over.Offset) += rn;
		size -= rn;
		
		swap(rbuf, hbuf);
		swap(rn, hn);
	}

	cleanup:
	::CloseHandle(over.hEvent);
	::CloseHandle(h);
	return ok;
}

#else // !_WIN32

static const int64_t BUF_SIZE = 0x1000000 - (0x1000000 % getpagesize());
static sigjmp_buf sb_env;

static void sigbus_handler(int signum, siginfo_t* info, void* context) {
	// Jump back to the fastHash which will return error. Apparently truncating
	// a file in Solaris sets si_code to BUS_OBJERR
	if (signum == SIGBUS && (info->si_code == BUS_ADRERR || info->si_code == BUS_OBJERR))
		siglongjmp(sb_env, 1);
}

bool HashManager::Hasher::fastHash(const string& filename, uint8_t* , TigerTree& tth, int64_t size) {
	int fd = open(Text::fromUtf8(filename).c_str(), O_RDONLY);
	if(fd == -1) {
		dcdebug("Error opening file %s: %s\n", filename.c_str(), Util::translateError(errno).c_str());
		return false;
	}

	int64_t pos = 0;
	int64_t size_read = 0;
	void *buf = NULL;
	bool ok = false;

	// Prepare and setup a signal handler in case of SIGBUS during mmapped file reads.
	// SIGBUS can be sent when the file is truncated or in case of read errors.	 
	struct sigaction act, oldact;
	sigset_t signalset;

	sigemptyset(&signalset);

	act.sa_handler = NULL;
	act.sa_sigaction = sigbus_handler;
	act.sa_mask = signalset;
	act.sa_flags = SA_SIGINFO | SA_RESETHAND;

	if (sigaction(SIGBUS, &act, &oldact) == -1) {
		dcdebug("Failed to set signal handler for fastHash\n");
		close(fd);
		return false;	// Better luck with the slow hash.
	}

	uint64_t lastRead = GET_TICK();
	while (pos < size && !stop) {
		size_read = std::min(size - pos, BUF_SIZE);
			buf = mmap(0, size_read, PROT_READ, MAP_SHARED, fd, pos);
			if(buf == MAP_FAILED) {
				dcdebug("Error calling mmap for file %s: %s\n", filename.c_str(), Util::translateError(errno).c_str());
				break;
			}

		if (sigsetjmp(sb_env, 1)) {
			dcdebug("Caught SIGBUS for file %s\n", filename.c_str());
			break;
		}

		if (posix_madvise(buf, size_read, POSIX_MADV_SEQUENTIAL | POSIX_MADV_WILLNEED) == -1) {
				dcdebug("Error calling madvise for file %s: %s\n", filename.c_str(), Util::translateError(errno).c_str());
				break;
			}

			if (SETTING(MAX_HASH_SPEED) > 0) {
			uint64_t now = GET_TICK();
			uint64_t minTime = size_read * 1000LL / (SETTING(MAX_HASH_SPEED) * 1024LL * 1024LL);
				if (lastRead + minTime > now) {
				uint64_t diff = now - lastRead;
					Thread::sleep(minTime - diff);
				} 
				lastRead = lastRead + minTime;
			} else {
				lastRead = GET_TICK();
			}

		tth.update(buf, size_read);

		{
			Lock l(cs);
			currentSize = max(static_cast<uint64_t>(currentSize - size_read), static_cast<uint64_t>(0));		
		}
		
		if (munmap(buf, size_read) == -1) {
			dcdebug("Error calling munmap for file %s: %s\n", filename.c_str(), Util::translateError(errno).c_str());
			break;
		}

		buf = NULL;
		pos += size_read;

		instantPause();

		if (pos == size) {
			ok = true;
		}
	}

	if (buf != NULL && buf != MAP_FAILED && munmap(buf, size_read) == -1) {
			dcdebug("Error calling munmap for file %s: %s\n", filename.c_str(), Util::translateError(errno).c_str());
		}

	close(fd);

	if (sigaction(SIGBUS, &oldact, NULL) == -1) {
		dcdebug("Failed to reset old signal handler for SIGBUS\n");
	}

	return ok;
}

#endif // !_WIN32

int HashManager::Hasher::run() {
	setThreadPriority(Thread::IDLE);

	uint8_t* buf = NULL;
	bool virtualBuf = true;

	string fname;
	bool last = false;
	for(;;) {
		s.wait();
		if(stop)
			break;
		if(rebuild) {
			HashManager::getInstance()->doRebuild();
			rebuild = false;
			LogManager::getInstance()->message(STRING(HASH_REBUILT));
			continue;
		}
		{
			Lock l(cs);
			if(!w.empty()) {
				currentFile = fname = w.begin()->first;
				currentSize = w.begin()->second;
				w.erase(w.begin());
				last = w.empty();
			} else {
				last = true;
				fname.clear();
			}
		}
		running = true;

		if(!fname.empty()) {
			const int64_t size = File::getSize(fname);
			int64_t sizeLeft = size;
#ifdef _WIN32
			if(buf == NULL) {
				virtualBuf = true;
				buf = (uint8_t*)VirtualAlloc(NULL, 2*BUF_SIZE, MEM_COMMIT, PAGE_READWRITE);
			}
#endif
			if(buf == NULL) {
				virtualBuf = false;
				buf = new uint8_t[BUF_SIZE]; // bad_alloc! https://www.box.net/shared/d07faa588d5f44d577a0
			}
			try {
				File l_f(fname, File::READ, File::OPEN);
				const int64_t bs = TigerTree::getMaxBlockSize(l_f.getSize());
				const uint64_t start = GET_TICK();
				const uint64_t timestamp = l_f.getLastWriteTime(); //[!]PPA
				int64_t speed = 0;
				size_t n = 0;
				TigerTree fastTTH(bs);
				TigerTree slowTTH(bs); //[+]PPA
				TigerTree* tth = &fastTTH;
				bool l_is_ntfs = false;
				if(HashManager::getInstance()->m_streamstore.loadTree(fname, fastTTH, size)) {//[+]IRainman
					l_is_ntfs = true; //[+]PPA
					l_f.close();
					LogManager::getInstance()->message("[OK] NTFS Stream->TTH: " + fname);
					goto Done; //TODO переписать без goto
				}
#ifdef _WIN32
				if(!virtualBuf || !BOOLSETTING(FAST_HASH) || !fastHash(fname, buf, fastTTH, size)) {
#else
				if(!BOOLSETTING(FAST_HASH) || !fastHash(fname, 0, fastTTH, size)) {
#endif
					tth = &slowTTH;
					uint64_t lastRead = GET_TICK();

					do {
						size_t bufSize = BUF_SIZE;
						if(SETTING(MAX_HASH_SPEED) > 0) {
							uint64_t now = GET_TICK();
							uint64_t minTime = n * 1000LL / (SETTING(MAX_HASH_SPEED) * 1024LL * 1024LL);
							if(lastRead + minTime > now) {
								Thread::sleep(minTime - (now - lastRead));
							}
							lastRead = lastRead + minTime;
						} else {
							lastRead = GET_TICK();
						}
						n = l_f.read(buf, bufSize);
						tth->update(buf, n);

						{
							Lock l(cs);
							currentSize = max(static_cast<uint64_t>(currentSize - n), static_cast<uint64_t>(0));
						}
						sizeLeft -= n;

						instantPause();
					} while (n > 0 && !stop);
				} else {
					sizeLeft = 0;
				}

				l_f.close();
				tth->finalize();
				uint64_t end = GET_TICK();
				if(end > start) {
					speed = size * _LL(1000) / (end - start);
				}
Done:
				HashManager::getInstance()->hashDone(fname, timestamp, *tth, speed, l_is_ntfs,size);
			} catch(const FileException& e) {
				LogManager::getInstance()->message(STRING(ERROR_HASHING) + " " + fname + ": " + e.getError());
			}
		}
		{
			Lock l(cs);
			currentFile.clear();
			currentSize = 0;
		}		
		running = false;
		if(buf != NULL && (last || stop)) {
			if(virtualBuf) {
#ifdef _WIN32
				VirtualFree(buf, 0, MEM_RELEASE);
#endif
			} else {
				delete [] buf;
			}
			buf = NULL;
		}
	}
	return 0;
}

HashManager::HashPauser::HashPauser() {
	resume = !HashManager::getInstance()->pauseHashing();
}

HashManager::HashPauser::~HashPauser() {
	if(resume)
	HashManager::getInstance()->resumeHashing();
}

bool HashManager::pauseHashing() {
	return hasher.pause();
}

void HashManager::resumeHashing() {
	hasher.resume();
}

bool HashManager::isHashingPaused() const {
	return hasher.isPaused();
}

} // namespace dcpp

/**
 * @file
 * $Id: HashManager.cpp 568 2011-07-24 18:28:43Z bigmuscle $
 */
