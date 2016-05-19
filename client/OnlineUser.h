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

#ifndef DCPLUSPLUS_DCPP_ONLINEUSER_H_
#define DCPLUSPLUS_DCPP_ONLINEUSER_H_

#include <map>

#include <boost/noncopyable.hpp>

#include "forward.h"
#include "Flags.h"
#include "FastAlloc.h"
#include "Util.h"
#include "User.h"
#include "UserInfoBase.h"

namespace dcpp {

class ClientBase;
class NmdcHub;

/** One of possibly many identities of a user, mainly for UI purposes */
class Identity {
public:
	enum ClientType {
		CT_BOT = 1,
		CT_REGGED = 2,
		CT_OP = 4,
		CT_SU = 8,
		CT_OWNER = 16,
		CT_HUB = 32,
		CT_HIDDEN = 64
	};
	
	Identity():m_info_bitmap(0) { }
	Identity(const UserPtr& ptr, uint32_t aSID) : user(ptr),m_info_bitmap(0) { setSID(aSID); }
	enum StatusFlags {
		NORMAL		= 0x01,
		AWAY		= 0x02,
		SERVER		= 0x04,
		FIREBALL	= 0x08,
		TLS			= 0x10,
		NAT			= 0x20
	};

	Identity(const Identity& rhs) { *this = rhs; } // Use operator= since we have to lock before reading...
	Identity& operator=(const Identity& rhs) 
	{ 
		Lock l(cs); 
		user = rhs.user; 
		info = rhs.info; 
		m_info_bitmap = rhs.m_info_bitmap;
	    return *this; 
	}
	~Identity() { }

// GS is already defined on some systems (e.g. OpenSolaris)
#ifdef GS
#undef GS
#endif

#define GS(n, x) string get##n() const { return get(x); } void set##n(const string& v) { set(x, v); }
	GS(Nick, "NI")
	GS(Description, "DE")
	GS(Ip, "I4")
//TODO
	GS(UdpPort, "U4")
	GS(Email, "EM")
	GS(T4, "T4")
	GS(Connection, "CO")

	void setBytesShared(const string& bs) { set("SS", bs); }
	int64_t getBytesShared() const { return Util::toInt64(get("SS")); }
	
	void setStatus(const string& st) { set("ST", st); }
	StatusFlags getStatus() const { return static_cast<StatusFlags>(Util::toInt(get("ST"))); }

private:    
	inline bool getBIT(uint32_t p_mask) const 
	{
      return (m_info_bitmap & p_mask) > 0;	
	}
	inline void setBIT(uint32_t p_mask, bool p_f)
	{
		 if(p_f) 
			 m_info_bitmap |= p_mask; 
		 else
			 m_info_bitmap &= ~p_mask;
	}
public:

#define GSBIT(x)\
	    bool isSet##x() const  { return getBIT(e_##x); }\
	    void set##x(bool p_f) { setBIT(e_##x, p_f);}

	enum {
		e_Op = 0x0001, // 'OP' - зовется чаще всех
		e_Hub = 0x0002, // 
		e_Hidden = 0x0004, // 
		e_Bot = 0x0008, // 
		e_Registered = 0x0010, //
		e_Away = 0x0020,  //
		e_BF = 0x0040, //
		e_TC = 0x0080, //
		e_FC = 0x0100, //
		e_BC = 0x0200 //
		//....
	};

	GSBIT(Op)
	GSBIT(Hub)
	GSBIT(Bot)
	GSBIT(Hidden)
	GSBIT(Registered)
	GSBIT(Away)
	GSBIT(BF)
	GSBIT(TC)

	string getTag() const;
	string getApplication() const; 
	const string getCountry() const;
	bool supports(const string& name) const;
	bool isHub() const { return isSetHub() || isClientType(CT_HUB);  }
	bool isOp() const { return isSetOp()   || isClientType(CT_OP); }
	bool isRegistered() const { return isSetRegistered() || isClientType(CT_REGGED); }
	bool isHidden() const { return isClientType(CT_HIDDEN) || isSetHidden(); }
	bool isBot() const { return isSetBot() || isClientType(CT_BOT); }
	bool isAway() const { return isSetAway() || (getStatus() & AWAY);  }
	bool isTcpActive(const Client* = NULL) const;
	bool isUdpActive() const;
	string get(const char* name) const;
	void set(const char* name, const string& val);
	string getSIDString() const { uint32_t sid = getSID(); return string((const char*)&sid, 4); }
	
	uint32_t getSID() const { return Util::toUInt32(get("SI")); }
	void setSID(uint32_t sid) { if(sid != 0) set("SI", Util::toString(sid)); }
	
	bool isClientType(ClientType ct) const;
		
	string setCheat(const ClientBase& c, const string& aCheatDescription, bool aBadClient);
	void getReport(map<string, string>& p_report) const;
	string updateClientType(const OnlineUser& ou);
#ifdef PPA_INCLUDE_BOOST_REGEXP
	bool matchProfile(const string& aString, const string& aProfile) const;
#endif

	static string getVersion(const string& aExp, string aTag);
	static string splitVersion(const string& aExp, string aTag, size_t part);
	
	void getParams(StringMap& map, const string& prefix, bool compatibility, bool dht = false) const;
	UserPtr& getUser() { return user; }
	GETSET(UserPtr, user, User);
private:
	typedef std::unordered_map<short, std::string> InfMap;
	typedef InfMap::const_iterator InfIter;
	InfMap info;
	uint32_t m_info_bitmap;

	static CriticalSection cs;

	string getDetectionField(const string& aName) const;
	void getDetectionParams(StringMap& p);
	string getPkVersion() const;
};

class OnlineUser : public FastAlloc<OnlineUser>, public intrusive_ptr_base<OnlineUser>, public UserInfoBase {
public:
	enum {
		COLUMN_FIRST,
		COLUMN_NICK = COLUMN_FIRST, 
		COLUMN_SHARED, 
		COLUMN_EXACT_SHARED, 
		COLUMN_DESCRIPTION, 
		COLUMN_TAG,
		COLUMN_CONNECTION, 
		COLUMN_IP,
		//[+]PPA
		COLUMN_LAST_IP,
        COLUMN_UPLOAD, 
        COLUMN_DOWNLOAD, 
		//[~]PPA
		COLUMN_EMAIL, 
		COLUMN_VERSION, 
		COLUMN_MODE, 
		COLUMN_HUBS, 
		COLUMN_SLOTS,
		COLUMN_CID,
		COLUMN_LAST
	};

	struct Hash {
		size_t operator()(const OnlineUserPtr& x) const { return ((size_t)(&(*x)))/sizeof(OnlineUser); }
	};

	OnlineUser(const UserPtr& ptr, ClientBase& client_, uint32_t sid_);
	virtual ~OnlineUser() throw() { }

	operator UserPtr&() { return getUser(); }
	operator const UserPtr&() const { return getUser(); }

	UserPtr& getUser() { return getIdentity().getUser(); }
	const UserPtr& getUser() const { return getIdentity().getUser(); }
	Identity& getIdentity() { return identity; }
	Client& getClient() { return (Client&)client; }
	const Client& getClient() const { return (const Client&)client; }
	
	ClientBase& getClientBase() { return client; }	
	const ClientBase& getClientBase() const { return client; }

	/* UserInfo */
	bool update(int sortCol, const tstring& oldText = Util::emptyStringT);
	uint8_t getImageIndex() const { return UserInfoBase::getImage(identity, &getClient()); }
	static int compareItems(const OnlineUser* a, const OnlineUser* b, uint8_t col);
	bool isHidden() const { return identity.isHidden(); }
	
	tstring getText(uint8_t col) const;

	bool isInList;
	GETSET(Identity, identity, Identity);
	int m_flagImage; //[+]PPA
private:
	friend class NmdcHub;

	OnlineUser(const OnlineUser&);
	OnlineUser& operator=(const OnlineUser&);

	ClientBase& client;
};


}

#endif /* ONLINEUSER_H_ */
