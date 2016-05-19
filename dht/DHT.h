/*
 * Copyright (C) 2009-2011 Big Muscle, http://strongdc.sf.net
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

#ifndef _DHT_H
#define _DHT_H

#include "BootstrapManager.h"
#include "Constants.h"
#include "KBucket.h"
#include "UDPSocket.h"

#include "../client/AdcCommand.h"
#include "../client/CID.h"
#include "../client/MerkleTree.h"
#include "../client/Singleton.h"
#include "../client/TimerManager.h"

namespace dht
{

	class DHT :	
		public Singleton<DHT>, public Speaker<ClientListener>, public ClientBase
	{
	public:
		DHT(void);
		~DHT(void);
		
		enum InfType { NONE = 0, PING = 1, CONNECTION = 2 };
		
		/** ClientBase derived functions */
		const string& getHubUrl() const { return NetworkName; }
		string getHubName() const { return NetworkName; }
		bool isOp() const { return false; }

		/** Starts DHT. */
		void start();
		void stop(bool exiting = false);

		uint16_t getPort() const { return BOOLSETTING(USE_DHT) ? socket.getPort() : 0; }
		
		/** Process incoming command */
		void dispatch(const string& aLine, const string& ip, uint16_t port, bool isUdpKeyValid);
		
		/** Sends command to ip and port */
		void send(AdcCommand& cmd, const string& ip, uint16_t port, const CID& targetCID, const UDPKey& udpKey);
		
		/** Adds node to routing table */
		Node::Ptr addNode(const CID& cid, const string& ip, uint16_t port, const UDPKey& udpKey, bool update, bool isUdpKeyValid);
		
		/** Returns counts of nodes available in k-buckets */
		size_t getNodesCount() { return RoutingTable::getNodesCount(); }
		
		/** Removes dead nodes */
		void checkExpiration(uint64_t aTick);
		
		/** Finds the file in the network */
		void findFile(const string& tth, const string& token = Util::toString(Util::rand()));
		
		/** Sends our info to specified ip:port */
		void info(const string& ip, uint16_t port, uint32_t type, const CID& targetCID, const UDPKey& udpKey);
		
		/** Sends Connect To Me request to online node */
		void connect(const OnlineUser& ou, const string& token);
		
		/** Sends private message to online node */
		void privateMessage(const OnlineUserPtr& ou, const string& aMessage, bool thirdPerson);
		
		/** Is DHT connected? */
		bool isConnected() const { return lastPacket && (GET_TICK() - lastPacket < CONNECTED_TIMEOUT); }
		
		/** Mark that network data should be saved */
		void setDirty() { dirty = true; }
		
		/** Saves network information to XML file */
		void saveData();
				
		/** Returns if our UDP port is open */
		bool isFirewalled() const { return firewalled; }
				
		/** Returns our IP got from the last firewall check */
		string getLastExternalIP() const { return lastExternalIP; }
		void setLastExternalIP(const string& ip) { lastExternalIP = ip; }
		
		void setRequestFWCheck() { Lock l(cs); requestFWCheck = true; firewalledWanted.clear(); firewalledChecks.clear(); }

		void lock(const function<void()>& f) noexcept { Lock l(cs); f(); } ;
				
	private:
		/** Classes that can access to my private members */
		friend class Singleton<DHT>;
		friend class SearchManager;
		
		void handle(AdcCommand::INF, const string& ip, uint16_t port, const UDPKey& udpKey, bool isUdpKeyValid, AdcCommand& c) noexcept;	// user's info
		void handle(AdcCommand::SCH, const string& ip, uint16_t port, const UDPKey& udpKey, AdcCommand& c) noexcept;	// incoming search request
		void handle(AdcCommand::RES, const string& ip, uint16_t port, const UDPKey& udpKey, AdcCommand& c) noexcept;	// incoming search result
		void handle(AdcCommand::PUB, const string& ip, uint16_t port, const UDPKey& udpKey, AdcCommand& c) noexcept;	// incoming publish request
		void handle(AdcCommand::CTM, const string& ip, uint16_t port, const UDPKey& udpKey, AdcCommand& c) noexcept;	// connection request
		void handle(AdcCommand::RCM, const string& ip, uint16_t port, const UDPKey& udpKey, AdcCommand& c) noexcept;	// reverse connection request
		void handle(AdcCommand::STA, const string& ip, uint16_t port, const UDPKey& udpKey, AdcCommand& c) noexcept;	// status message
		void handle(AdcCommand::PSR, const string& ip, uint16_t port, const UDPKey& udpKey, AdcCommand& c) noexcept;	// partial file request
		void handle(AdcCommand::MSG, const string& ip, uint16_t port, const UDPKey& udpKey, AdcCommand& c) noexcept; // private message
		void handle(AdcCommand::GET, const string& ip, uint16_t port, const UDPKey& udpKey, AdcCommand& c) noexcept;
		void handle(AdcCommand::SND, const string& ip, uint16_t port, const UDPKey& udpKey, bool isUdpKeyValid, AdcCommand& c) noexcept;
			
		/** Unsupported command */
		template<typename T> void handle(T, const Node::Ptr&user, AdcCommand&) { }
		
		/** UDP socket */
		UDPSocket	socket;
		
		/** Routing table */
		RoutingTable*	bucket;
		
		/** Lock to routing table */
		CriticalSection	cs;
		CriticalSection fwCheckCs;
		
		/** Our external IP got from last firewalled check */
		string lastExternalIP;
		
		/** Time when last packet was received */
		uint64_t lastPacket;
		
		/** IPs who we received firewalled status from */
		std::unordered_set<string> firewalledWanted;
		std::unordered_map<string, std::pair<string, uint16_t>> firewalledChecks;
		bool firewalled;
		bool requestFWCheck;
		
		/** Should the network data be saved? */
		bool dirty;

		/** Finds "max" closest nodes and stores them to the list */
		void getClosestNodes(const CID& cid, std::map<CID, Node::Ptr>& closest, unsigned int max, uint8_t maxType);
		
		/** Loads network information from XML file */
		void loadData();
	};

}

#endif	// _DHT_H