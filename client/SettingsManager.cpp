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
#include "SettingsManager.h"

#include "ResourceManager.h"
#include "SimpleXML.h"
#include "Util.h"
#include "File.h"
#include "version.h"
#include "AdcHub.h"
#include "CID.h"
#include "SearchManager.h"
#include "StringTokenizer.h"

#include "../dht/dht.h"

#include <boost/static_assert.hpp> // [+]IRainman

namespace dcpp {

StringList SettingsManager::connectionSpeeds;

const string SettingsManager::settingTags[] =
{
	// Strings
	"Nick", "UploadSpeed", "Description", "DownloadDirectory", "EMail", "ExternalIp",
	"Font", "MainFrameOrder", "MainFrameWidths", "HubFrameOrder", "HubFrameWidths", 
	"LanguageFile", "SearchFrameOrder", "SearchFrameWidths", "FavoritesFrameOrder", "FavoritesFrameWidths", 
	"HublistServers", "QueueFrameOrder", "QueueFrameWidths", "PublicHubsFrameOrder", "PublicHubsFrameWidths", 
	"UsersFrameOrder", "UsersFrameWidths", "HttpProxy", "LogDirectory", "LogFormatPostDownload", 
	"LogFormatPostUpload", "LogFormatMainChat", "LogFormatPrivateChat", "FinishedOrder", "FinishedWidths",	 
	"TempDownloadDirectory", "BindInterface", "SocksServer", "SocksUser", "SocksPassword", "ConfigVersion", 
	"DefaultAwayMessage", "TimeStampsFormat", "ADLSearchFrameOrder", "ADLSearchFrameWidths", 
	"FinishedULWidths", "FinishedULOrder", "CID", "SpyFrameWidths", "SpyFrameOrder", 
	"BeepFile", "BeginFile", "FinishedFile", "SourceFile", "UploadFile", "FakerFile", "ChatNameFile", "WinampFormat",
	"KickMsgRecent01", "KickMsgRecent02", "KickMsgRecent03", "KickMsgRecent04", "KickMsgRecent05", 
	"KickMsgRecent06", "KickMsgRecent07", "KickMsgRecent08", "KickMsgRecent09", "KickMsgRecent10", 
	"KickMsgRecent11", "KickMsgRecent12", "KickMsgRecent13", "KickMsgRecent14", "KickMsgRecent15", 
	"KickMsgRecent16", "KickMsgRecent17", "KickMsgRecent18", "KickMsgRecent19", "KickMsgRecent20",
	"Toolbar", "ToolbarImage", "ToolbarHot", "UserListImage", "UploadQueueFrameOrder", "UploadQueueFrameWidths",
	"ProfilesURL", "SoundTTH", "SoundException", "SoundHubConnected", "SoundHubDisconnected", "SoundFavUserOnline", "SoundTypingNotify",
	"WebServerLogFormat", "WebServerUser", "WebServerPass", "LogFileMainChat", 
	"LogFilePrivateChat", "LogFileStatus", "LogFileUpload", "LogFileDownload", "LogFileSystem", "LogFormatSystem", 
	"LogFormatStatus", "LogFileWebServer", "DirectoryListingFrameOrder", "DirectoryListingFrameWidths", 
	"MainFrameVisible", "SearchFrameVisible", "QueueFrameVisible", "HubFrameVisible", "UploadQueueFrameVisible", 
	"EmoticonsFile", "TLSPrivateKeyFile", "TLSCertificateFile", "TLSTrustedCertificatesPath",
	"FinishedVisible", "FinishedULVisible", "DirectoryListingFrameVisible",
	"RecentFrameOrder", "RecentFrameWidths", "ToolbarSettings", "DHTKey", "Mapper",
	"ISPResourceRootURL",// //[+]FlylinkDC++ Team
	"SENTRY", 
	// Ints
	"IncomingConnections", "InPort", "Slots", "AutoFollow", "ClearSearch",
	"BackgroundColor", "TextColor", "ShareHidden", "FilterMessages", "MinimizeToTray",
	"AutoSearch", "TimeStamps", "ConfirmExit", "PopupHubPms", "PopupBotPms", "IgnoreHubPms", "IgnoreBotPms",
	"BufferSize", "DownloadSlots", "MaxDownloadSpeed", "LogMainChat", "LogPrivateChat",
	"LogDownloads", "LogUploads", "StatusInChat", "ShowJoins", "PrivateMessageBeep", "PrivateMessageBeepOpen",
	"UseSystemIcons", "PopupPMs", "MinUploadSpeed", "GetUserInfo", "UrlHandler", "MainWindowState", 
	"MainWindowSizeX", "MainWindowSizeY", "MainWindowPosX", "MainWindowPosY", "AutoAway",
	"SocksPort", "SocksResolve", "KeepLists", "AutoKick", "QueueFrameShowTree", 
	"CompressTransfers", "ShowProgressBars", "MaxTabRows",
	"MaxCompression", "AntiFragMethod", "NoAwayMsgToBots",
	"SkipZeroByte", "AdlsBreakOnFirst",
	"HubUserCommands", "AutoSearchAutoMatch", "DownloadBarColor", "UploadBarColor", "LogSystem",
	"LogFilelistTransfers", "ShowStatusbar", "BandwidthSettingMode", "ShowToolbar", "ShowTransferview", 
	"SearchPassiveAlways", "SetMinislotSize", "ShutdownInterval", "DontAnnounceNewVersions", 
	"CzertHiddenSettingA", "CzertHiddenSettingB", "ExtraSlots", "ExtraPartialSlots",
	"TextGeneralBackColor", "TextGeneralForeColor", "TextGeneralBold", "TextGeneralItalic", 
	"TextMyOwnBackColor", "TextMyOwnForeColor", "TextMyOwnBold", "TextMyOwnItalic", 
	"TextPrivateBackColor", "TextPrivateForeColor", "TextPrivateBold", "TextPrivateItalic", 
	"TextSystemBackColor", "TextSystemForeColor", "TextSystemBold", "TextSystemItalic", 
	"TextServerBackColor", "TextServerForeColor", "TextServerBold", "TextServerItalic", 
	"TextTimestampBackColor", "TextTimestampForeColor", "TextTimestampBold", "TextTimestampItalic", 
	"TextMyNickBackColor", "TextMyNickForeColor", "TextMyNickBold", "TextMyNickItalic", 
	"TextFavBackColor", "TextFavForeColor", "TextFavBold", "TextFavItalic", 
	"TextOPBackColor", "TextOPForeColor", "TextOPBold", "TextOPItalic", 
	"TextURLBackColor", "TextURLForeColor", "TextURLBold", "TextURLItalic", 
	"BoldAuthorsMess", "ThrottleEnable", "UploadLimitTime", "DownloadLimitTime", "HubSlots",
	"TimeThrottle", "TimeLimitStart", "TimeLimitEnd", "RemoveForbidden",
	"ProgressTextDown", "ProgressTextUp", "ShowInfoTips", "ExtraDownloadSlots",
	"MinimizeOnStratup", "ConfirmDelete", "DefaultSearchFreeSlots", "SendUnknownCommands",
	"ErrorColor", "ExpandQueue", "TransferSplitSize",
	"DisconnectSpeed", "DisconnectFileSpeed", "DisconnectTime", "RemoveSpeed",
	"ProgressOverrideColors", "Progress3DDepth", "ProgressOverrideColors2",
	"MenubarTwoColors", "MenubarLeftColor", "MenubarRightColor", "MenubarBumped", 
	"DisconnectFileSize", "UploadQueueFrameShowTree",
	"SegmentsManual", "NumberOfSegments", "PercentFakeShareTolerated",
	"AutoUpdateIP", "MaxHashSpeed",
	"SaveTthInNtfsFilestream",
	"GetUserCountry", "DisableCZDiacritic",
	"DebugCommands", "UseAutoPriorityByDefault", "UseOldSharingUI",
	"FavShowJoins", "LogStatusMessages", "PMLogLines", "SearchAlternateColour", "SoundsDisabled",
	"ReportFoundAlternates", "CheckNewUsers",
	"SearchTime", "DontBeginSegment", "DontBeginSegmentSpeed", "PopunderPm", "PopunderFilelist",
	"DropMultiSourceOnly", "DisplayCheatsInMainChat", "MagnetAsk", "MagnetAction", "MagnetRegister",
	"DisconnectRaw", "TimeoutRaw", "FakeShareRaw", "ListLenMismatch", "FileListTooSmall", "FileListUnavailable",
	"AddFinishedInstantly", "Away", "UseCTRLForLineHistory",
	"PopupHubConnected", "PopupHubDisconnected", "PopupFavoriteConnected", "PopupCheatingUser", "PopupDownloadStart", 
	"PopupDownloadFailed", "PopupDownloadFinished", "PopupUploadFinished", "PopupPm", "PopupNewPM", 
	"PopupType", "WebServer", "WebServerPort", "WebServerLog", "ShutdownAction", "MinimumSearchInterval",
	"PopupAway", "PopupMinimized", "ShowShareCheckedUsers", "MaxAutoMatchSource",
    "IgnoredColor", "FavoriteColor",
	"NormalColour", "PasiveColor", "OpColor", 
	"DontDLAlreadyShared",
	"ConfirmHubRemoval", "SuppressMainChat", "ProgressBackColor", "ProgressCompressColor", "ProgressSegmentColor",
	"OpenNewWindow", "FileSlots",  "UDPPort", "MultiChunk",
 	"UserListDoubleClick", "TransferListDoubleClick", "ChatDoubleClick", "AdcDebug",
	"ToggleActiveWindow", "ProgressbaroDCStyle", "SearchHistory", 
	"AcceptedDisconnects", "AcceptedTimeouts",
	"OpenPublic", "OpenFavoriteHubs", "OpenFavoriteUsers", "OpenRecentHubs", "OpenQueue", "OpenFinishedDownloads",
	"OpenFinishedUploads", "OpenSearchSpy", "OpenNetworkStatistics", "OpenNotepad", "OutgoingConnections",
	"NoIPOverride", "GroupSearchResults", "BoldFinishedDownloads", "BoldFinishedUploads", "BoldQueue", 
	"BoldHub", "BoldPm", "BoldSearch", "TabsPos", "SocketInBuffer", "SocketOutBuffer", 
	"ColorDownloaded", "ColorRunning", "ColorDone",
        "DupeColor",
        "AutoRefreshTime", "UseTLS", "OpenUploadQueue",
	"BoldUploadQueue", "AutoSearchLimit", "AutoKickNoFavs", "PromptPassword", "SpyFrameIgnoreTthSearches",
 	"AllowUntrustedHubs", "AllowUntrustedClients", "TLSPort", "FastHash", "DownConnPerSec",
	"HighestPrioSize", "HighPrioSize", "NormalPrioSize", "LowPrioSize", "LowestPrio",
	"FilterEnter", "SortFavUsersFirst", "ShowShellMenu", "SendBloom", "OverlapChunks", "ShowQuickSearch",
	"EnableIpGuard", "DefaultPolicy",
	"UcSubMenu", "AutoSlots", "Coral", "UseDHT", "DHTPort", "UpdateIP", "KeepFinishedFiles",
	"EnableLastIp",
	"SqliteUseJournalMemory", 
	"SqliteUseExclusiveLockMode", 
	"AllowNATTraversal", "UseExplorerTheme", "AutoDetectIncomingConnection",
	"SENTRY",
	// Int64
	"TotalUpload", "TotalDownload",
	"SENTRY"
};

SettingsManager::SettingsManager()
{
	//[+]IRainman
	// SettingsManager::SETTINGS_LAST and SettingsManager::settingTags[] size do not match ;) Check them out!
	BOOST_STATIC_ASSERT(_countof(settingTags) == SETTINGS_LAST + 1);
	// Thanks Boost developers for this wonderful functional

	connectionSpeeds.push_back("0.01");
	connectionSpeeds.push_back("0.02");
	connectionSpeeds.push_back("0.05");
	connectionSpeeds.push_back("0.1");
	connectionSpeeds.push_back("0.2");
	connectionSpeeds.push_back("0.5");
	connectionSpeeds.push_back("1");
	connectionSpeeds.push_back("2");
	connectionSpeeds.push_back("5");
	connectionSpeeds.push_back("10");
	connectionSpeeds.push_back("20");
	connectionSpeeds.push_back("50");
	connectionSpeeds.push_back("100");
	connectionSpeeds.push_back("1000");

	for(int i=0; i<SETTINGS_LAST; i++)
		isSet[i] = false;

	for(int i=0; i<INT_LAST-INT_FIRST; i++) {
		intDefaults[i] = 0;
		intSettings[i] = 0;
	}
	for(int i=0; i<INT64_LAST-INT64_FIRST; i++) {
		int64Defaults[i] = 0;
		int64Settings[i] = 0;
	}

	const string l_dir = Util::getRegistryValueString("DownloadDir",true);
	if(!l_dir.empty())
		setDefault(DOWNLOAD_DIRECTORY, l_dir);
        else
	   setDefault(DOWNLOAD_DIRECTORY, Util::getPath(Util::PATH_DOWNLOADS));
	setDefault(TEMP_DOWNLOAD_DIRECTORY, "");
	setDefault(SLOTS, 10);
	setDefault(TCP_PORT, 0);
	setDefault(UDP_PORT, 0);
	setDefault(TLS_PORT, 0);
	setDefault(DHT_PORT, DHT_UDPPORT);
	setDefault(INCOMING_CONNECTIONS, INCOMING_DIRECT);
	setDefault(OUTGOING_CONNECTIONS, OUTGOING_DIRECT);
	setDefault(AUTO_DETECT_CONNECTION, true);
#ifdef PPA_INCLUDE_AUTO_FOLLOW
	setDefault(AUTO_FOLLOW, true);
#endif
	setDefault(CLEAR_SEARCH, false);
	setDefault(SHARE_HIDDEN, false);
	setDefault(FILTER_MESSAGES, true);
	setDefault(MINIMIZE_TRAY, true);
	setDefault(AUTO_SEARCH, true);
	setDefault(TIME_STAMPS, true);
	setDefault(CONFIRM_EXIT, true);
	setDefault(POPUP_HUB_PMS, true);
	setDefault(POPUP_BOT_PMS, true);
	setDefault(IGNORE_HUB_PMS, false);
	setDefault(IGNORE_BOT_PMS, false);
	setDefault(BUFFER_SIZE, 64);
	setDefault(HUBLIST_SERVERS, "http://dchublist.ru/hublist.xml.bz2;http://dchublist.com/hublist.xml.bz2;http://ddd-dc.ru/hublist/svao.xml.bz2;http://ddd-dc.ru/hublist/home.xml.bz2;http://ddd-dc.ru/hublist/netbynet.xml.bz2;http://ddd-dc.ru/hublist/triolan.xml.bz2;http://ddd-dc.ru/hublist/akadoural.xml.bz2;");
	setDefault(DOWNLOAD_SLOTS, 100);
    setDefault(FILE_SLOTS, 15);
	setDefault(MAX_DOWNLOAD_SPEED, 0);
	setDefault(LOG_DIRECTORY, Util::getPath(Util::PATH_USER_LOCAL) + "Logs" PATH_SEPARATOR_STR);
	setDefault(LOG_UPLOADS, false);
	setDefault(LOG_DOWNLOADS, false);
	setDefault(LOG_PRIVATE_CHAT, true);
	setDefault(LOG_MAIN_CHAT, false);
	setDefault(STATUS_IN_CHAT, false);
	setDefault(SHOW_JOINS, false);
	setDefault(UPLOAD_SPEED, connectionSpeeds[0]);
	setDefault(PRIVATE_MESSAGE_BEEP, true);
	setDefault(PRIVATE_MESSAGE_BEEP_OPEN, false);
	setDefault(USE_SYSTEM_ICONS, true);
	setDefault(POPUP_PMS, true);
	setDefault(MIN_UPLOAD_SPEED, 0);
	setDefault(LOG_FORMAT_POST_DOWNLOAD, "%Y-%m-%d %H:%M: %[target] " + STRING(DOWNLOADED_FROM) + " %[userNI] (%[userCID]), %[fileSI] (%[fileSIchunk]), %[speed], %[time]");
	setDefault(LOG_FORMAT_POST_UPLOAD, "%Y-%m-%d %H:%M: %[source] " + STRING(UPLOADED_TO) + " %[userNI] (%[userCID]), %[fileSI] (%[fileSIchunk]), %[speed], %[time]");
	setDefault(LOG_FORMAT_MAIN_CHAT, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FORMAT_PRIVATE_CHAT, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FORMAT_STATUS, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FORMAT_SYSTEM, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FILE_MAIN_CHAT, "%[hubURL].log");
	setDefault(LOG_FILE_STATUS, "%[hubURL]_status.log");
	setDefault(LOG_FILE_PRIVATE_CHAT, "PM\\%[userCID].log");
	setDefault(LOG_FILE_UPLOAD, "Uploads.log");
	setDefault(LOG_FILE_DOWNLOAD, "Downloads.log");
	setDefault(LOG_FILE_SYSTEM, "system.log");
	setDefault(LOG_FILE_WEBSERVER, "Webserver.log");
	setDefault(GET_USER_INFO, true);
	setDefault(URL_HANDLER, true);
	setDefault(AUTO_AWAY, false);
	setDefault(SOCKS_PORT, 1080);
	setDefault(SOCKS_RESOLVE, 1);
	setDefault(CONFIG_VERSION, "0.181");		// 0.181 is the last version missing configversion
	setDefault(KEEP_LISTS, false);
	setDefault(AUTO_KICK, true);
	setDefault(QUEUEFRAME_SHOW_TREE, true);
	setDefault(COMPRESS_TRANSFERS, true);
	setDefault(SHOW_PROGRESS_BARS, true);
	setDefault(DEFAULT_AWAY_MESSAGE, "I'm away. State your business and I might answer later if you're lucky.");
	setDefault(TIME_STAMPS_FORMAT, "%H:%M:%S");
	setDefault(MAX_TAB_ROWS, 8);
	setDefault(MAX_COMPRESSION, 6);
	setDefault(ANTI_FRAG, true);
	setDefault(NO_AWAYMSG_TO_BOTS, true);
	setDefault(SKIP_ZERO_BYTE, false);
	setDefault(ADLS_BREAK_ON_FIRST, false);
	setDefault(HUB_USER_COMMANDS, true);
	setDefault(AUTO_SEARCH_AUTO_MATCH, true);
	setDefault(LOG_FILELIST_TRANSFERS, false);
	setDefault(LOG_SYSTEM, false);
	setDefault(SEND_UNKNOWN_COMMANDS, false);
	setDefault(MAX_HASH_SPEED, 0);
	setDefault(SAVE_TTH_IN_NTFS_FILESTREAM, true);
	setDefault(GET_USER_COUNTRY, true);
	setDefault(FAV_SHOW_JOINS, false);
	setDefault(LOG_STATUS_MESSAGES, false);
	setDefault(SHOW_TRANSFERVIEW, true);
	setDefault(SHOW_STATUSBAR, true);
	setDefault(SHOW_TOOLBAR, true);
	setDefault(POPUNDER_PM, false);
	setDefault(POPUNDER_FILELIST, false);
	setDefault(MAGNET_REGISTER, true);
	setDefault(MAGNET_ASK, true);
	setDefault(MAGNET_ACTION, MAGNET_AUTO_SEARCH);
#ifdef PPA_INCLUDE_ADD_FINISHED_INSTANTLY
	setDefault(ADD_FINISHED_INSTANTLY, false);
#endif
	setDefault(DONT_DL_ALREADY_SHARED, false);
	setDefault(CONFIRM_HUB_REMOVAL, true);
	setDefault(USE_CTRL_FOR_LINE_HISTORY, true);
	setDefault(JOIN_OPEN_NEW_WINDOW, false);
	setDefault(SHOW_LAST_LINES_LOG, 20);
	setDefault(CONFIRM_DELETE, true);
	setDefault(ADC_DEBUG, false);
	setDefault(TOGGLE_ACTIVE_WINDOW, false);
	setDefault(SEARCH_HISTORY, 15);
	setDefault(SET_MINISLOT_SIZE, 1024);
	setDefault(PRIO_HIGHEST_SIZE, 64);
	setDefault(PRIO_HIGH_SIZE, 4096);
	setDefault(PRIO_NORMAL_SIZE, 262144);
	setDefault(PRIO_LOW_SIZE, 1048576);
	setDefault(PRIO_LOWEST, false);
	setDefault(OPEN_PUBLIC, false);
	setDefault(OPEN_FAVORITE_HUBS, false);
	setDefault(OPEN_FAVORITE_USERS, false);
	setDefault(OPEN_RECENT_HUBS, false);
	setDefault(OPEN_QUEUE, false);
	setDefault(OPEN_FINISHED_DOWNLOADS, false);
	setDefault(OPEN_FINISHED_UPLOADS, false);
	setDefault(OPEN_SEARCH_SPY, false);
	setDefault(OPEN_NETWORK_STATISTICS, false);
	setDefault(OPEN_NOTEPAD, false);
	setDefault(NO_IP_OVERRIDE, false);
#ifdef FLYLINKDC_BUG_WIN8_DP
	setDefault(SOCKET_IN_BUFFER, 64*1024);
#endif
	setDefault(SOCKET_OUT_BUFFER, 64*1024);
	setDefault(OPEN_UPLOAD_QUEUE, false);
	setDefault(TLS_TRUSTED_CERTIFICATES_PATH, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR);
	setDefault(TLS_PRIVATE_KEY_FILE, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR "client.key");
	setDefault(TLS_CERTIFICATE_FILE, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR "client.crt");
	setDefault(BOLD_FINISHED_DOWNLOADS, true);
	setDefault(BOLD_FINISHED_UPLOADS, true);
	setDefault(BOLD_QUEUE, true);
	setDefault(BOLD_HUB, true);
	setDefault(BOLD_PM, true);
	setDefault(BOLD_SEARCH, true);
	setDefault(BOLD_UPLOAD_QUEUE, true);
	setDefault(AUTO_REFRESH_TIME, 60);
	setDefault(USE_TLS, true);
	setDefault(AUTO_SEARCH_LIMIT, 15);
	setDefault(AUTO_KICK_NO_FAVS, false);
	setDefault(PROMPT_PASSWORD, true);
	setDefault(SPY_FRAME_IGNORE_TTH_SEARCHES, false);
	setDefault(ALLOW_UNTRUSTED_HUBS, true);
	setDefault(ALLOW_UNTRUSTED_CLIENTS, true);		
	setDefault(FAST_HASH, true);
	setDefault(SORT_FAVUSERS_FIRST, true);
	setDefault(SHOW_SHELL_MENU, false);
	setDefault(SEND_BLOOM, true);
	setDefault(CORAL, false);	
	setDefault(ENABLE_LAST_IP, false); //[+]PPA	
	setDefault(SQLITE_USE_JOURNAL_MEMORY, false); //[+]PPA	
	setDefault(SQLITE_USE_EXCLUSIVE_LOCK_MODE, false); //[+]PPA	
	setDefault(NUMBER_OF_SEGMENTS, 10); //[+]PPA
	setDefault(SEGMENTS_MANUAL, false);
	setDefault(HUB_SLOTS, 1);
	setDefault(DEBUG_COMMANDS, false);
	setDefault(PROFILES_URL, "https://dcaml.svn.sourceforge.net/svnroot/dcaml/");	
	setDefault(EXTRA_SLOTS, 3);
	setDefault(EXTRA_PARTIAL_SLOTS, 1);
	setDefault(SHUTDOWN_TIMEOUT, 150);
	setDefault(SEARCH_PASSIVE, false);
	setDefault(MAX_UPLOAD_SPEED_LIMIT, 0);
	setDefault(MAX_DOWNLOAD_SPEED_LIMIT, 0);
	setDefault(MAX_UPLOAD_SPEED_LIMIT_TIME, 0);
	setDefault(MAX_DOWNLOAD_SPEED_LIMIT_TIME, 0);
	setDefault(TOOLBAR, "0,5,-1,1,2,-1,3,4,-1,6,7,8,9,-1,10,11,12,-1,14,15,16,-1,17,21,19,-1,18,13,20");
	setDefault(WEBSERVER, false);
	setDefault(WEBSERVER_PORT, (int)Util::rand(80, 1024));
	setDefault(WEBSERVER_FORMAT,"%Y-%m-%d %H:%M: %[ip] tried getting %[file]");
	setDefault(LOG_WEBSERVER, false);
	setDefault(WEBSERVER_USER, "ddd");
	setDefault(WEBSERVER_PASS, "ddd");
	setDefault(AUTO_PRIORITY_DEFAULT, true);
	setDefault(TOOLBARIMAGE,"");
	setDefault(TOOLBARHOTIMAGE,"");
	setDefault(TIME_DEPENDENT_THROTTLE, false);
	setDefault(BANDWIDTH_LIMIT_START, 0);
	setDefault(BANDWIDTH_LIMIT_END, 0);
	setDefault(REMOVE_FORBIDDEN, true);
	setDefault(THROTTLE_ENABLE, false);
	setDefault(EXTRA_DOWNLOAD_SLOTS, 3);

	setDefault(BOLD_AUTHOR_MESS, true);
	setDefault(KICK_MSG_RECENT_01, "");
	setDefault(KICK_MSG_RECENT_02, "");
	setDefault(KICK_MSG_RECENT_03, "");
	setDefault(KICK_MSG_RECENT_04, "");
	setDefault(KICK_MSG_RECENT_05, "");
	setDefault(KICK_MSG_RECENT_06, "");
	setDefault(KICK_MSG_RECENT_07, "");
	setDefault(KICK_MSG_RECENT_08, "");
	setDefault(KICK_MSG_RECENT_09, "");
	setDefault(KICK_MSG_RECENT_10, "");
	setDefault(KICK_MSG_RECENT_11, "");
	setDefault(KICK_MSG_RECENT_12, "");
	setDefault(KICK_MSG_RECENT_13, "");
	setDefault(KICK_MSG_RECENT_14, "");
	setDefault(KICK_MSG_RECENT_15, "");
	setDefault(KICK_MSG_RECENT_16, "");
	setDefault(KICK_MSG_RECENT_17, "");
	setDefault(KICK_MSG_RECENT_18, "");
	setDefault(KICK_MSG_RECENT_19, "");
	setDefault(KICK_MSG_RECENT_20, "");
	setDefault(WINAMP_FORMAT, "winamp((%[title]) stats(%[percent] of %[length])");
	setDefault(SHOW_INFOTIPS, true);
	setDefault(MINIMIZE_ON_STARTUP, false);
	setDefault(FREE_SLOTS_DEFAULT, false);
	setDefault(EXPAND_QUEUE, true);
	setDefault(TRANSFER_SPLIT_SIZE, 8000);

	setDefault(PERCENT_FAKE_SHARE_TOLERATED, 20);
#ifdef FLY_INCLUDE_CZCHARS_DISABLE
	setDefault(CZCHARS_DISABLE, false);
#endif
	setDefault(REPORT_ALTERNATES, true);	

	setDefault(SOUNDS_DISABLED, false);
	setDefault(CHECK_NEW_USERS, false);
	setDefault(UPLOADQUEUEFRAME_SHOW_TREE, true);	
	setDefault(DONT_BEGIN_SEGMENT, true);
	setDefault(DONT_BEGIN_SEGMENT_SPEED, 512);

	setDefault(DISCONNECT_RAW, 0);
	setDefault(TIMEOUT_RAW, 0);
	setDefault(FAKESHARE_RAW, 0);
	setDefault(LISTLEN_MISMATCH, 0);
	setDefault(FILELIST_TOO_SMALL, 0);
	setDefault(FILELIST_UNAVAILABLE, 0);
	setDefault(DISPLAY_CHEATS_IN_MAIN_CHAT, true);	
	setDefault(SEARCH_TIME, 10);
	setDefault(SUPPRESS_MAIN_CHAT, false);
	setDefault(AUTO_SLOTS, 5);	
	
	// default sounds
	setDefault(BEGINFILE, Util::emptyString);
	setDefault(BEEPFILE, Util::emptyString);
	setDefault(FINISHFILE, Util::emptyString);
	setDefault(SOURCEFILE, Util::emptyString);
	setDefault(UPLOADFILE, Util::emptyString);
	setDefault(FAKERFILE, Util::emptyString);
	setDefault(CHATNAMEFILE, Util::emptyString);
	setDefault(SOUND_TTH, Util::emptyString);
	setDefault(SOUND_EXC, Util::emptyString);
	setDefault(SOUND_HUBCON, Util::emptyString);
	setDefault(SOUND_HUBDISCON, Util::emptyString);
	setDefault(SOUND_FAVUSER, Util::emptyString);
	setDefault(SOUND_TYPING_NOTIFY, Util::emptyString);

	setDefault(POPUP_HUB_CONNECTED, false);
	setDefault(POPUP_HUB_DISCONNECTED, false);
	setDefault(POPUP_FAVORITE_CONNECTED, true);
	setDefault(POPUP_CHEATING_USER, true);
	setDefault(POPUP_DOWNLOAD_START, true);
	setDefault(POPUP_DOWNLOAD_FAILED, false);
	setDefault(POPUP_DOWNLOAD_FINISHED, true);
	setDefault(POPUP_UPLOAD_FINISHED, false);
	setDefault(POPUP_PM, false);
	setDefault(POPUP_NEW_PM, true);
	setDefault(POPUP_TYPE, 0);
	setDefault(POPUP_AWAY, false);
	setDefault(POPUP_MINIMIZED, true);

	setDefault(AWAY, false);
	setDefault(SHUTDOWN_ACTION, 0);
	setDefault(MINIMUM_SEARCH_INTERVAL, 20);
	setDefault(PROGRESSBAR_ODC_STYLE, true);

	setDefault(MAX_AUTO_MATCH_SOURCES, 5);
	setDefault(MULTI_CHUNK, true);
	setDefault(USERLIST_DBLCLICK, 2);
	setDefault(TRANSFERLIST_DBLCLICK, 0);
	setDefault(CHAT_DBLCLICK, 1);	
	setDefault(HUBFRAME_VISIBLE, "1,1,0,1,1,1,1,0,0,0,1,0,1,1,1,0");
	setDefault(DIRECTORYLISTINGFRAME_VISIBLE, "1,1,0,1,1,1,1,1,1,1,1,1");	
	setDefault(FINISHED_VISIBLE, "1,1,1,1,1,1,1,1");
	setDefault(FINISHED_UL_VISIBLE, "1,1,1,1,1,1,1");
	setDefault(ACCEPTED_DISCONNECTS, 5);
	setDefault(ACCEPTED_TIMEOUTS, 10);
	setDefault(EMOTICONS_FILE, "ddd_mini_smiles");
	setDefault(GROUP_SEARCH_RESULTS, true);
	setDefault(TABS_POS, 1);
	setDefault(DONT_ANNOUNCE_NEW_VERSIONS, false);
	setDefault(DOWNCONN_PER_SEC, 2);
	setDefault(FILTER_ENTER, false);
	setDefault(SHOW_QUICK_SEARCH, true);	
	setDefault(UC_SUBMENU, false);
// TODO:	setDefault(TOOLBAR_SIZE, 800);

        setDefault(DUPE_COLOR, 0x73F7E6);

	setDefault(DROP_MULTISOURCE_ONLY, true);
	setDefault(DISCONNECT_SPEED, 5);
	setDefault(DISCONNECT_FILE_SPEED, 15);
	setDefault(DISCONNECT_TIME, 40);
	setDefault(DISCONNECT_FILESIZE, 50);
    setDefault(REMOVE_SPEED, 2);
	setDefault(OVERLAP_CHUNKS, true);
	setDefault(KEEP_FINISHED_FILES, false);

	setDefault(ENABLE_IPGUARD, false);
	setDefault(DEFAULT_POLICY, false);


	setDefault(MAIN_WINDOW_STATE, SW_SHOWNORMAL);
	setDefault(MAIN_WINDOW_SIZE_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_SIZE_Y, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_Y, CW_USEDEFAULT);

	/* Theme settings */
	setDefault(TEXT_FONT, "Segoe UI,-11,400,0");

	setDefault(BACKGROUND_COLOR, RGB(242,245,255));
	setDefault(TEXT_COLOR, RGB(0,0,128));

	setDefault(TEXT_GENERAL_BACK_COLOR, RGB(242,245,255));
	setDefault(TEXT_GENERAL_FORE_COLOR, RGB(67,98,154));
	setDefault(TEXT_GENERAL_BOLD, false);
	setDefault(TEXT_GENERAL_ITALIC, false);

	setDefault(TEXT_MYOWN_BACK_COLOR, RGB(242,245,255));
	setDefault(TEXT_MYOWN_FORE_COLOR, RGB(128,0,0));
	setDefault(TEXT_MYOWN_BOLD, true);
	setDefault(TEXT_MYOWN_ITALIC, false);

	setDefault(TEXT_PRIVATE_BACK_COLOR, RGB(242,245,255));
	setDefault(TEXT_PRIVATE_FORE_COLOR, RGB(67,98,154));
	setDefault(TEXT_PRIVATE_BOLD, true);
	setDefault(TEXT_PRIVATE_ITALIC, false);

	setDefault(TEXT_SYSTEM_BACK_COLOR, RGB(242,245,255));
	setDefault(TEXT_SYSTEM_FORE_COLOR, RGB(0,128,64));
	setDefault(TEXT_SYSTEM_BOLD, true);
	setDefault(TEXT_SYSTEM_ITALIC, true);

	setDefault(TEXT_SERVER_BACK_COLOR, RGB(242,245,255));
	setDefault(TEXT_SERVER_FORE_COLOR, RGB(0,128,64));
	setDefault(TEXT_SERVER_BOLD, true);
	setDefault(TEXT_SERVER_ITALIC, false);

	setDefault(TEXT_TIMESTAMP_BACK_COLOR, RGB(242,245,255));
	setDefault(TEXT_TIMESTAMP_FORE_COLOR, RGB(67,98,154));
	setDefault(TEXT_TIMESTAMP_BOLD, false);
	setDefault(TEXT_TIMESTAMP_ITALIC, false);

	setDefault(TEXT_MYNICK_BACK_COLOR, RGB(242,245,255));
	setDefault(TEXT_MYNICK_FORE_COLOR, RGB(128,0,0));
	setDefault(TEXT_MYNICK_BOLD, true);
	setDefault(TEXT_MYNICK_ITALIC, false);

	setDefault(TEXT_FAV_BACK_COLOR, RGB(242,245,255));
	setDefault(TEXT_FAV_FORE_COLOR, RGB(255,128,64));
	setDefault(TEXT_FAV_BOLD, true);
	setDefault(TEXT_FAV_ITALIC, true);

	setDefault(TEXT_OP_BACK_COLOR, RGB(242,245,255));
	setDefault(TEXT_OP_FORE_COLOR, RGB(0,128,0));
	setDefault(TEXT_OP_BOLD, true);
	setDefault(TEXT_OP_ITALIC, false);

	setDefault(TEXT_URL_BACK_COLOR, RGB(242,245,255));
	setDefault(TEXT_URL_FORE_COLOR, RGB(0,102,204));
	setDefault(TEXT_URL_BOLD, false);
	setDefault(TEXT_URL_ITALIC, false);

	setDefault(ERROR_COLOR, RGB(255, 0, 0));
	setDefault(SEARCH_ALTERNATE_COLOUR, RGB(255,0,128));

	setDefault(MENUBAR_TWO_COLORS, true);
	setDefault(MENUBAR_LEFT_COLOR, RGB(254, 157, 27));
	setDefault(MENUBAR_RIGHT_COLOR, RGB(194, 78, 7));
	setDefault(MENUBAR_BUMPED, true);

	setDefault(OP_COLOR, RGB(0,128,0));
	setDefault(FAVORITE_COLOR, RGB(255,128,64));	
	setDefault(IGNORED_COLOR, RGB(64,128,128));	
	setDefault(PASIVE_COLOR, RGB(67,98,154));
	setDefault(NORMAL_COLOUR, RGB(67,98,154));
	setDefault(PROGRESS_3DDEPTH, 4);
	setDefault(PROGRESS_OVERRIDE_COLORS, true);
	setDefault(PROGRESS_TEXT_COLOR_DOWN, RGB(255, 255, 255));
	setDefault(PROGRESS_TEXT_COLOR_UP, RGB(255, 255, 255));
	setDefault(UPLOAD_BAR_COLOR, RGB(255, 64, 0));
	setDefault(DOWNLOAD_BAR_COLOR, RGB(55, 170, 85));
	setDefault(PROGRESS_BACK_COLOR, RGB(95, 95, 95));
	setDefault(PROGRESS_COMPRESS_COLOR, RGB(222, 160, 0));
	setDefault(PROGRESS_SEGMENT_COLOR, RGB(49, 106, 197));
	setDefault(COLOR_RUNNING, RGB(0, 150, 0));
	setDefault(COLOR_DOWNLOADED, RGB(255, 255, 100));
	setDefault(COLOR_DONE, RGB(222, 160, 0));
	
	setDefault(USE_DHT, false);
	setDefault(UPDATE_IP, false);
	setDefault(ALLOW_NAT_TRAVERSAL, true);
	setDefault(USE_EXPLORER_THEME, true);
	
#ifdef _WIN32
	OSVERSIONINFO ver  = {0};
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((OSVERSIONINFO*)&ver);

	setDefault(USE_OLD_SHARING_UI, (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) ? false : true);
#endif

	setSearchTypeDefaults();
}

void SettingsManager::load(string const& aFileName)
{
	try {
		SimpleXML xml;

		xml.fromXML(File(aFileName, File::READ, File::OPEN).read());

		xml.resetCurrentChild();

		xml.stepIn();

		if(xml.findChild("Settings"))
		{
			xml.stepIn();

			int i;

			for(i=STR_FIRST; i<STR_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(StrSetting(i), xml.getChildData());
				xml.resetCurrentChild();
			}
			for(i=INT_FIRST; i<INT_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(IntSetting(i), Util::toInt(xml.getChildData()));
				xml.resetCurrentChild();
			}
			for(i=INT64_FIRST; i<INT64_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(Int64Setting(i), Util::toInt64(xml.getChildData()));
				xml.resetCurrentChild();
			}

			xml.stepOut();
		}

		xml.resetCurrentChild();
		if(xml.findChild("SearchTypes")) {
			try {
				searchTypes.clear();
				xml.stepIn();
				while(xml.findChild("SearchType")) {
					const string& extensions = xml.getChildData();
					if(extensions.empty()) {
						continue;
					}
					const string& name = xml.getChildAttrib("Id");
					if(name.empty()) {
						continue;
					}
					searchTypes[name] = StringTokenizer<string>(extensions, ';').getTokens();
				}
				xml.stepOut();
			} catch(const SimpleXMLException&) {
				setSearchTypeDefaults();
			}
		}

		if(SETTING(PRIVATE_ID).length() != 39 || CID(SETTING(PRIVATE_ID)).isZero()) {
			set(PRIVATE_ID, CID::generate().toBase32());
		}

		double v = Util::toDouble(SETTING(CONFIG_VERSION));
		// if(v < 0.x) { // Fix old settings here }

		if(v <= 0.674) {

			// Formats changed, might as well remove these...
			set(LOG_FORMAT_POST_DOWNLOAD, Util::emptyString);
			set(LOG_FORMAT_POST_UPLOAD, Util::emptyString);
			set(LOG_FORMAT_MAIN_CHAT, Util::emptyString);
			set(LOG_FORMAT_PRIVATE_CHAT, Util::emptyString);
			set(LOG_FORMAT_STATUS, Util::emptyString);
			set(LOG_FORMAT_SYSTEM, Util::emptyString);
			set(LOG_FILE_MAIN_CHAT, Util::emptyString);
			set(LOG_FILE_STATUS, Util::emptyString);
			set(LOG_FILE_PRIVATE_CHAT, Util::emptyString);
			set(LOG_FILE_UPLOAD, Util::emptyString);
			set(LOG_FILE_DOWNLOAD, Util::emptyString);
			set(LOG_FILE_SYSTEM, Util::emptyString);
		}

		if(v <= 0.770 && SETTING(INCOMING_CONNECTIONS) != INCOMING_FIREWALL_PASSIVE) {
			set(AUTO_DETECT_CONNECTION, false); //Don't touch if it works
		}

		setDefault(UDP_PORT, SETTING(TCP_PORT));

		File::ensureDirectory(SETTING(TLS_TRUSTED_CERTIFICATES_PATH));

		fire(SettingsManagerListener::Load(), xml);

		xml.stepOut();

	} catch(const Exception&) {
		if(CID(SETTING(PRIVATE_ID)).isZero())
			set(PRIVATE_ID, CID::generate().toBase32());
	}

	if(SETTING(INCOMING_CONNECTIONS) == INCOMING_DIRECT) {
		set(TCP_PORT, (int)Util::rand(10000, 32000));
		set(UDP_PORT, (int)Util::rand(10000, 32000));
		set(TLS_PORT, (int)Util::rand(10000, 32000));
	}

	if(SETTING(DHT_PORT) == SETTING(UDP_PORT))
		set(DHT_PORT, SETTING(DHT_PORT) + 1); //[+] PPA

	if(SETTING(DHT_KEY).length() != 39 || CID(SETTING(DHT_KEY)).isZero())
		set(DHT_KEY, CID::generate().toBase32());
}

void SettingsManager::save(string const& aFileName) {

	SimpleXML xml;
	xml.addTag("DCPlusPlus");
	xml.stepIn();
	xml.addTag("Settings");
	xml.stepIn();

	int i;
	string type("type"), curType("string");

	for(i=STR_FIRST; i<STR_LAST; i++)
	{
		if(i == CONFIG_VERSION) {
			xml.addTag(settingTags[i], A_VERSIONSTRING);
			xml.addChildAttrib(type, curType);
		} else if(isSet[i]) {
			xml.addTag(settingTags[i], get(StrSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}

	curType = "int";
	for(i=INT_FIRST; i<INT_LAST; i++)
	{
		if(isSet[i]) {
			xml.addTag(settingTags[i], get(IntSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	curType = "int64";
	for(i=INT64_FIRST; i<INT64_LAST; i++)
	{
		if(isSet[i])
		{
			xml.addTag(settingTags[i], get(Int64Setting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	xml.stepOut();
	
	/*xml.addTag("SearchTypes");
	xml.stepIn();
	for(SearchTypesIterC i = searchTypes.begin(); i != searchTypes.end(); ++i) {
		xml.addTag("SearchType", Util::toString(";", i->second));
		xml.addChildAttrib("Id", i->first);
	}
	xml.stepOut();*/

	fire(SettingsManagerListener::Save(), xml);

	try {
		File out(aFileName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&out);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
		f.flush();
		out.close();
		File::deleteFile(aFileName);
		File::renameFile(aFileName + ".tmp", aFileName);
	} catch(...) {
		// ...
	}
}

void SettingsManager::validateSearchTypeName(const string& name) const {
	if(name.empty() || (name.size() == 1 && name[0] >= '1' && name[0] <= '6')) {
		throw SearchTypeException("Invalid search type name"); // TODO: localize
	}
	for(int type = SearchManager::TYPE_ANY; type != SearchManager::TYPE_LAST; ++type) {
		if(SearchManager::getTypeStr(type) == name) {
			throw SearchTypeException("This search type already exists"); // TODO: localize
		}
	}
}

void SettingsManager::setSearchTypeDefaults() {
	searchTypes.clear();

	// for conveniency, the default search exts will be the same as the ones defined by SEGA.
	const auto& searchExts = AdcHub::getSearchExts();
	for(size_t i = 0, n = searchExts.size(); i < n; ++i)
		searchTypes[string(1, '1' + i)] = searchExts[i];

	fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::addSearchType(const string& name, const StringList& extensions, bool validated) {
	if(!validated) {
		validateSearchTypeName(name);
	}

	if(searchTypes.find(name) != searchTypes.end()) {
		throw SearchTypeException("This search type already exists"); // TODO: localize
	}

	searchTypes[name] = extensions;
	fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::delSearchType(const string& name) {
	validateSearchTypeName(name);
	searchTypes.erase(name);
	fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::renameSearchType(const string& oldName, const string& newName) {
	validateSearchTypeName(newName);
	StringList exts = getSearchType(oldName)->second;
	addSearchType(newName, exts, true);
	searchTypes.erase(oldName);
}

void SettingsManager::modSearchType(const string& name, const StringList& extensions) {
	getSearchType(name)->second = extensions;
	fire(SettingsManagerListener::SearchTypesChanged());
}

const StringList& SettingsManager::getExtensions(const string& name) {
	return getSearchType(name)->second;
}

SettingsManager::SearchTypesIter SettingsManager::getSearchType(const string& name) {
	SearchTypesIter ret = searchTypes.find(name);
	if(ret == searchTypes.end()) {
		throw SearchTypeException("No such search type"); // TODO: localize
	}
	return ret;
}

} // namespace dcpp

/**
 * @file
 * $Id: SettingsManager.cpp 575 2011-08-25 19:38:04Z bigmuscle $
 */
