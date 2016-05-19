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
#include "FinishedManager.h"

#include "ClientManager.h"
#include "FinishedManagerListener.h"
#include "Download.h"
#include "Upload.h"
#include "QueueManager.h"
#include "UploadManager.h"

#include "LogManager.h"
#include "ResourceManager.h"
#include "CFlylinkDBManager.h"

namespace dcpp {

FinishedManager::FinishedManager() { 
	QueueManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
}
	
FinishedManager::~FinishedManager() {
	QueueManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);

	Lock l(cs);
	for_each(downloads.begin(), downloads.end(), DeleteFunction());
	for_each(uploads.begin(), uploads.end(), DeleteFunction());
}

void FinishedManager::remove(FinishedItemPtr item, bool upload /* = false */) {
	{
		Lock l(cs);
		FinishedItemList *listptr = upload ? &uploads : &downloads;
		FinishedItemList::iterator it = find(listptr->begin(), listptr->end(), item);

		if(it != listptr->end())
			listptr->erase(it);
		else
			return;
	}
}
	
void FinishedManager::removeAll(bool upload /* = false */) {
	{
		Lock l(cs);
		FinishedItemList *listptr = upload ? &uploads : &downloads;
		for_each(listptr->begin(), listptr->end(), DeleteFunction());
		listptr->clear();
	}
}

void FinishedManager::on(QueueManagerListener::Finished, const QueueItem* qi, const string&, const Download* d) noexcept
{
	bool isFile = !qi->isSet(QueueItem::FLAG_USER_LIST);

	if(isFile && !SETTING(FINISHFILE).empty() && !BOOLSETTING(SOUNDS_DISABLED)) {
		PlaySound(Text::toT(SETTING(FINISHFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
	}
		
	if(isFile || (qi->isSet(QueueItem::FLAG_USER_LIST) && BOOLSETTING(LOG_FILELIST_TRANSFERS))) {
		
		FinishedItemPtr item = new FinishedItem(qi->getTarget(), d->getHintedUser(), qi->getSize(), static_cast<int64_t>(d->getAverageSpeed()), GET_TIME(), qi->getTTH().toBase32(),d->getUser()->getLastIP());
		{
			Lock l(cs);
			downloads.push_back(item);
		}
			
		fire(FinishedManagerListener::AddedDl(), item);
	
		const size_t BUF_SIZE = STRING(FINISHED_DOWNLOAD).size() + MAX_PATH + 128;
		boost::scoped_array<char> buf(new char[BUF_SIZE]);
		snprintf(&buf[0], BUF_SIZE, CSTRING(FINISHED_DOWNLOAD), Util::getFileName(qi->getTarget()).c_str(),
			Util::toString(ClientManager::getInstance()->getNicks(d->getHintedUser())).c_str());
		LogManager::getInstance()->message(&buf[0]);
	}
}

void FinishedManager::on(UploadManagerListener::Complete, const Upload* u) noexcept
{
	if(u->getType() == Transfer::TYPE_FILE || (u->getType() == Transfer::TYPE_FULL_LIST && BOOLSETTING(LOG_FILELIST_TRANSFERS))) {
		if ((!SETTING(UPLOADFILE).empty() && (!BOOLSETTING(SOUNDS_DISABLED))))
			PlaySound(Text::toT(SETTING(UPLOADFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

		FinishedItemPtr item = new FinishedItem(u->getPath(), u->getHintedUser(),	u->getFileSize(), static_cast<int64_t>(u->getAverageSpeed()), GET_TIME(),Util::emptyString,u->getUser()->getLastIP());
		{
			Lock l(cs);
			uploads.push_back(item);
		}

		fire(FinishedManagerListener::AddedUl(), item);

		const size_t BUF_SIZE = STRING(FINISHED_UPLOAD).size() + MAX_PATH + 128;
		{
			boost::scoped_array<char> buf(new char[BUF_SIZE]);
			snprintf(&buf[0], BUF_SIZE, CSTRING(FINISHED_UPLOAD), (Util::getFileName(u->getPath())).c_str(),
			Util::toString(ClientManager::getInstance()->getNicks(u->getHintedUser())).c_str());

			LogManager::getInstance()->message(&buf[0]);
		}
	    const string l_name = Text::toLower(Util::getFileName(u->getPath()));
	    const string l_path = Text::toLower(Util::getFilePath(u->getPath()));
        CFlylinkDBManager::getInstance()->Hit(l_path,l_name);
	}
}

} // namespace dcpp

/**
 * @file
 * $Id: FinishedManager.cpp 571 2011-07-27 19:18:04Z bigmuscle $
 */
