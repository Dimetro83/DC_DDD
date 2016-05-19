#include "stdinc.h"
#include "DCPlusPlus.h"

#include "IpGuard.h"
#include "ResourceManager.h"

namespace dcpp {

inline static uint32_t make_ip(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
	return ((a << 24) | (b << 16) | (c << 8) | d);
}


bool IpGuard::load() {
	if(noSave) {
		Lock l(cs);
		noSave = false;

		try {
			SimpleXML xml;
			xml.fromXML(File(Util::getPath(Util::PATH_USER_CONFIG) + "IPGuard.xml", File::READ, File::OPEN).read());
			if(xml.findChild("IpRanges")) {
				xml.stepIn();
				while(xml.findChild("Range")) {
					uint32_t start = Util::toUInt32(xml.getChildAttrib("Start"));
					uint32_t end = Util::toUInt32(xml.getChildAttrib("End"));
					ranges.push_front(range(++lastRange, xml.getChildAttrib("Comment"), ip(std::min(start, end)), ip(std::max(start, end))));
				}
				xml.resetCurrentChild();
				xml.stepOut();
			}
		} catch(const Exception& e) {
			dcdebug("IpGuard::load: %s\n", e.getError().c_str());
		}

		// Optimize ranges
		ranges.sort();
		ranges.unique(merger);

		return true;
	}

	return false;
}

void IpGuard::save() {
	Lock l(cs);

	SimpleXML xml;
	xml.addTag("IpRanges");
	xml.stepIn();
	for(RangeIter i = ranges.begin(); i != ranges.end(); ++i) {
		xml.addTag("Range");
		xml.addChildAttrib("Start", Util::toString(i->start.iIP));
		xml.addChildAttrib("End", Util::toString(i->end.iIP));
		xml.addChildAttrib("Comment", i->name);
	}
	xml.stepOut();

	try {
		string fname = Util::getPath(Util::PATH_USER_CONFIG) + "IPGuard.xml";

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);
	} catch(const Exception& e) {
		dcdebug("IpGuard::save: %s\n", e.getError().c_str());
	}
}

bool IpGuard::check(const string& aIP, string& reason) {
	Lock l(cs);

	if(aIP.empty() || ranges.size() == 0)
		return false;

	unsigned int a, b, c, d;
	if(sscanf(aIP.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || a > 255 || b > 255 || c > 255 || d > 255) 
		return false;

	uint32_t iIP = make_ip(a, b, c, d);

	if(iIP == 0)
		return false;

	RangeIter find;
	if((find = range_search(ranges.begin(), ranges.end(), ip(iIP))) != ranges.end()) {
		reason = find->name;
		return !BOOLSETTING(DEFAULT_POLICY);
   }

	reason = STRING(IPGUARD_DEFAULT_POLICY);
	return BOOLSETTING(DEFAULT_POLICY);
}

void IpGuard::check(uint32_t aIP, Socket* socket /*= NULL*/) throw(SocketException) {
	Lock l(cs);

	if(aIP == 0 || ranges.size() == 0)
		return;

	RangeIter find;
	if((find = range_search(ranges.begin(), ranges.end(), ip(htonl(aIP)))) != ranges.end()) {
		if(!BOOLSETTING(DEFAULT_POLICY)) {
			if(socket != NULL)
				socket->disconnect();
			throw SocketException(STRING(IPGUARD) + ": " + find->name + " (" + inet_ntoa(*(in_addr*)&aIP) + ")");
		}

		return;
   }

	if(BOOLSETTING(DEFAULT_POLICY)) {
		if(socket != NULL)
			socket->disconnect();
		throw SocketException(STRING(IPGUARD) + ": " + STRING(IPGUARD_DEFAULT_POLICY) + " (" + inet_ntoa(*(in_addr*)&aIP) + ")");
	}
}

void IpGuard::addRange(const string& aName, const string& aStart, const string& aEnd) throw(Exception) {
	Lock l(cs);
	unsigned int sa, sb, sc, sd, ea, eb, ec, ed;

	// Validate parameters
	if(sscanf(aStart.c_str(), "%u.%u.%u.%u", &sa, &sb, &sc, &sd) != 4 || sa > 255 || sb > 255 || sc > 255 || sd > 255)
		throw Exception(STRING(IPGUARD_INVALID_START));

	if(sscanf(aEnd.c_str(), "%u.%u.%u.%u", &ea, &eb, &ec, &ed) != 4 || ea > 255 || eb > 255 || ec > 255 || ed > 255)
		throw Exception(STRING(IPGUARD_INVALID_END));

	uint32_t start = make_ip(sa, sb, sc, sd);
	uint32_t end = make_ip(ea, eb, ec, ed);

	if(start == 0 || end == 0)
		throw Exception(STRING(IPGUARD_INVALID_RANGE));

	ranges.push_front(range(++lastRange, aName, ip(std::min(start, end)), ip(std::max(start, end))));

	// Optimize ranges
	ranges.sort();
	ranges.unique(merger);
}

void IpGuard::updateRange(int aId, const string& aName, const string& aStart, const string& aEnd) throw(Exception) {
	Lock l(cs);
	unsigned int sa, sb, sc, sd, ea, eb, ec, ed;

	// Validate parameters
	if(sscanf(aStart.c_str(), "%u.%u.%u.%u", &sa, &sb, &sc, &sd) != 4 || sa > 255 || sb > 255 || sc > 255 || sd > 255)
		throw Exception(STRING(IPGUARD_INVALID_START));

	if(sscanf(aEnd.c_str(), "%u.%u.%u.%u", &ea, &eb, &ec, &ed) != 4 || ea > 255 || eb > 255 || ec > 255 || ed > 255)
		throw Exception(STRING(IPGUARD_INVALID_END));

	uint32_t start = make_ip(sa, sb, sc, sd);
	uint32_t end = make_ip(ea, eb, ec, ed);

	if(start == 0 || end == 0)
		throw Exception(STRING(IPGUARD_INVALID_RANGE));

	for(RangeList::iterator i = ranges.begin(); i != ranges.end(); ++i) {
		if(i->id == aId) {
			(*i).copy(aName, ip(std::min(start, end)), ip(std::max(start, end)));
			break;
		}
	}

	// Optimize ranges
	ranges.sort();
	ranges.unique(merger);
}

void IpGuard::removeRange(int aId) {
	Lock l(cs);
	for(RangeList::iterator i = ranges.begin(); i != ranges.end(); ++i) {
		if(i->id == aId) {
			ranges.erase(i);
			ranges.sort();
			break;
		}
	}
}

}