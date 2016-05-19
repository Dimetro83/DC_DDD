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

#include "stdafx.h"

#include "Constants.h"
#include "DHT.h"
#include "KBucket.h"
#include "TaskManager.h"
#include "Utils.h"

#include "../client/ClientManager.h"

namespace dht
{
	StringSet RoutingTable::ipMap;
	size_t RoutingTable::nodesCount = 0;

	// Set all new nodes' type to 3 to avoid spreading dead nodes..
	Node::Node(const UserPtr& u) :
		OnlineUser(u, *DHT::getInstance(), 0), created(GET_TICK()), type(3), expires(0), ipVerified(false), online(false)
	{
	}

	UDPKey Node::getUdpKey() const
	{
		// if our external IP changed from the last time, we can't encrypt packet with this key
		if(DHT::getInstance()->getLastExternalIP() == key.ip)
			return key;
		else
			return UDPKey();
	}

	void Node::setAlive()
	{
	const uint64_t l_CurrentTick = GET_TICK(); // [+] IRainman optimize
		// long existing nodes will probably be there for another long time
	const uint64_t hours = (l_CurrentTick - created) / 1000 / 60 / 60;
		switch(hours)
		{
			case 0:
				type = 2;
			expires = l_CurrentTick + (NODE_EXPIRATION / 2);
				break;
			case 1:
				type = 1;
			expires = l_CurrentTick + (uint64_t)(NODE_EXPIRATION / 1.5);
				break;
			default:	// long existing nodes
				type = 0;
			expires = l_CurrentTick + NODE_EXPIRATION;
		}
		}

	void Node::setTimeout(uint64_t now)
	{
		if(type == 4)
			return;

		type++;
		expires = now + NODE_RESPONSE_TIMEOUT;
	}


	RoutingTable::RoutingTable(int _level, bool _splitAllowed) : 
		level(_level), lastRandomLookup(GET_TICK()), splitAllowed(_splitAllowed)
	{
		bucket = new NodeList();

		zones[0] = NULL;
		zones[1] = NULL;

		TaskManager::getInstance()->addZone(this);
	}

	RoutingTable::~RoutingTable(void)
	{
		delete zones[0];
		delete zones[1];

		if(bucket)
		{
			TaskManager::getInstance()->removeZone(this);

			for(auto i = bucket->begin(); i != bucket->end(); ++i)
			{
				if((*i)->isOnline())
				{
					ClientManager::getInstance()->putOffline(i->get());
					(*i)->setOnline(false);
					(*i)->dec();
	}
			}	

			delete bucket;
		}
	}

	inline bool getBit(const uint8_t* value, int number)
	{
		uint8_t byte = value[number / 8];
		return (byte >> (7 - (number % 8))) & 1;
	}

	/*
	 * Creates new (or update existing) node which is NOT added to our routing table
	 */
	Node::Ptr RoutingTable::addOrUpdate(const UserPtr& u, const string& ip, uint16_t port, const UDPKey& udpKey, bool update, bool isUdpKeyValid)
	{
		if(bucket == NULL)
		{
			// get distance from me
			const uint8_t* distance = Utils::getDistance(u->getCID(), ClientManager::getInstance()->getMe()->getCID()).data();

			// iterate through tree structure
			return zones[getBit(distance, level)]->addOrUpdate(u, ip, port, udpKey, update, isUdpKeyValid);
		}
		else
			{
			Node::Ptr node = NULL;
			for(auto it = bucket->begin(); it != bucket->end(); ++it)
			{
				if((*it)->getUser() == u)
				{
					node = *it;

					// put node at the end of the list
					bucket->erase(it);
					bucket->push_back(node);
					break;
				}
			}
			
			if(node == NULL && u->isOnline())
			{
				// try to get node from ClientManager (user can be online but not in our routing table)
				node = (Node*)ClientManager::getInstance()->findDHTNode(u->getCID()).get();
			}

			if(node != NULL)
			{
				// fine, node found, update it and return it
				if(update)
				{	
					if(!node->getUdpKey().isZero() && !(node->getUdpKey().key == udpKey.key))
					{
						// if we haven't changed our IP in the last time, we require that node's UDP key is same as key sent
						// with this packet to avoid spoofing
						return node;
					}

						string oldIp = node->getIdentity().getIp();
						string oldPort	= node->getIdentity().getUdpPort();
						if(ip != oldIp || static_cast<uint16_t>(Util::toInt(oldPort)) != port)
						{
						// don't allow update when new IP already exists for different node
						string newIPPort = ip + ":" + Util::toString(port);
						if(ipMap.find(newIPPort) != ipMap.end())
							return node;

						// erase old IP
						ipMap.erase(oldIp + ":" + oldPort);		
						ipMap.insert(newIPPort);

						// since IP changed, this flag becomes invalid
						node->setIpVerified(false);
						}
							
					if(!node->isIpVerified())
						node->setIpVerified(isUdpKeyValid);

				node->setAlive();
					node->getIdentity().setIp(ip);
					node->getIdentity().setUdpPort(Util::toString(port));
					node->setUdpKey(udpKey);
						
				DHT::getInstance()->setDirty();
					}
			}
			else if(bucket->size() < K)
			{
				// bucket is not full, add node
				node = new Node(u);
				node->getIdentity().setIp(ip);
				node->getIdentity().setUdpPort(Util::toString(port));
				node->setIpVerified(isUdpKeyValid);
				node->setUdpKey(udpKey);

				bucket->push_back(node);

				DHT::getInstance()->setDirty();

				++nodesCount;
			}
			else if(level < ID_BITS - 1 && splitAllowed)
			{
				// bucket is full, split it
				split();

				// iterate through tree structure
				const uint8_t* distance = Utils::getDistance(u->getCID(), ClientManager::getInstance()->getMe()->getCID()).data();
				node = zones[getBit(distance, level)]->addOrUpdate(u, ip, port, udpKey, update, isUdpKeyValid);
		}
			else
			{
				node = new Node(u);
		node->getIdentity().setIp(ip);
		node->getIdentity().setUdpPort(Util::toString(port));
		node->setIpVerified(isUdpKeyValid);
			}

			u->setFlag(User::DHT);
		return node;
	}
	}

	void RoutingTable::split()
		{
		TaskManager::getInstance()->removeZone(this);

		// if we are in zero branch (very close nodes), we allow split always
		// branch with 1's is allowed to split only to level 4 and a few last buckets before routing table becomes full
		zones[0] = new RoutingTable(level + 1, splitAllowed);	// FIXME: only 000000000000 branches should be split and not 1110000000
		zones[1] = new RoutingTable(level + 1, (level < 4) || (level > ID_BITS - 5));

		for(auto it = bucket->begin(); it != bucket->end(); ++it)
			{
			// iterate through tree structure
			const uint8_t* distance = Utils::getDistance((*it)->getUser()->getCID(), ClientManager::getInstance()->getMe()->getCID()).data();
			zones[getBit(distance, level)]->bucket->push_back(*it);
		}

		delete bucket;
		bucket = NULL;
	}

	/*
	 * Finds "max" closest nodes and stores them to the list
	 */
	void RoutingTable::getClosestNodes(const CID& cid, Node::Map& closest, unsigned int max, uint8_t maxType) const
	{
		if(bucket != NULL)
		{
		for(auto it = bucket->begin(); it != bucket->end(); ++it)
			{
				Node::Ptr node = *it;
			if(node->getType() <= maxType && node->isIpVerified() && !node->getUser()->isSet(User::PASSIVE))
			{
				CID distance = Utils::getDistance(cid, node->getUser()->getCID());

			if(closest.size() < max)
			{
				// just insert
						dcassert(node != NULL);
					closest.insert(std::make_pair(distance, node));
			}
			else
			{
				// not enough room, so insert only closer nodes
				if(distance < closest.rbegin()->first)	// "closest" is sorted map, so just compare with last node
				{
							dcassert(node != NULL);

					closest.erase(closest.rbegin()->first);
						closest.insert(std::make_pair(distance, node));
				}
			}
		}
	}
	}
				else
				{
			const uint8_t* distance = Utils::getDistance(cid, ClientManager::getInstance()->getMe()->getCID()).data();
			bool bit = getBit(distance, level);
			zones[bit]->getClosestNodes(cid, closest, max, maxType);

			if(closest.size() < max)	// if not enough nodes found, try other buckets
				zones[!bit]->getClosestNodes(cid, closest, max, maxType);
			}
			}

	/*
	 * Loads existing nodes from disk
	 */
	void RoutingTable::loadNodes(SimpleXML& xml)
	{
		xml.resetCurrentChild();
		if(xml.findChild("Nodes"))
		{
			xml.stepIn();
			while(xml.findChild("Node"))
			{
				CID cid		= CID(xml.getChildAttrib("CID"));
				string i4	= xml.getChildAttrib("I4");
				uint16_t u4	= static_cast<uint16_t>(xml.getIntChildAttrib("U4"));

				if(Utils::isGoodIPPort(i4, u4))
				{
					UDPKey udpKey;
					string key		= xml.getChildAttrib("key");
					string keyIp	= xml.getChildAttrib("keyIP");

					if(!key.empty() && !keyIp.empty())
					{
						udpKey.key = CID(key);
						udpKey.ip = keyIp;

						// since we don't know our IP yet, we can use stored one
						if(DHT::getInstance()->getLastExternalIP() == "0.0.0.0")
							DHT::getInstance()->setLastExternalIP(keyIp);
					}

					//UserPtr u = ClientManager::getInstance()->getUser(cid);
					//addOrUpdate(u, i4, u4, udpKey, false, true);
					
					BootstrapManager::getInstance()->addBootstrapNode(i4, u4, cid, udpKey);
			}
			}
			xml.stepOut();
	}
	}

	/*
	 * Save bootstrap nodes to disk
	 */
	void RoutingTable::saveNodes(SimpleXML& xml)
	{
		xml.addTag("Nodes");
		xml.stepIn();

		// get 50 random nodes to bootstrap from them next time
		Node::Map closestToRandom;
		getClosestNodes(CID::generate(), closestToRandom, 200, 3);

		for(Node::Map::const_iterator j = closestToRandom.begin(); j != closestToRandom.end(); ++j)
		{
			const Node::Ptr& node = j->second;

			xml.addTag("Node");
			xml.addChildAttrib("CID", node->getUser()->getCID().toBase32());
			xml.addChildAttrib("type", node->getType());
			xml.addChildAttrib("verified", node->isIpVerified());

			UDPKey key = node->getUdpKey();
			if(!key.isZero())
			{
				xml.addChildAttrib("key", key.key.toBase32());
				xml.addChildAttrib("keyIP", key.ip);
			}

			StringMap params;
			node->getIdentity().getParams(params, Util::emptyString, false, true);

			for(StringMap::const_iterator i = params.begin(); i != params.end(); ++i)
				xml.addChildAttrib(i->first, i->second);
		}

		xml.stepOut();
	}

	void RoutingTable::checkExpiration(uint64_t aTick)
	{
		if(bucket == NULL)
			return;

		// first, remove dead nodes		
		auto i = bucket->begin();
		while(i != bucket->end())
		{
			Node::Ptr& node = *i;
		
			if(node->getType() == 4 && node->expires > 0 && node->expires <= aTick)
			{
				if(node->isOnline())
				{
					node->setOnline(false);

					// FIXME: what if we're still downloading/uploading from/to this node?
					// then it is weird why it does not responded to our INF
					ClientManager::getInstance()->putOffline(node.get());
					node->dec();
	}

				if(node->unique())	// is the only reference in this bucket?
				{
					// node is dead, remove it
					string ip	= node->getIdentity().getIp();
					string port = node->getIdentity().getUdpPort();
					ipMap.erase(ip + ":" + port);

					i = bucket->erase(i);
					DHT::getInstance()->setDirty();

					--nodesCount;
				}
				else
				{
					++i;
				}
			
				continue;
			}
	
			if(node->expires == 0)
				node->expires = aTick;
			++i;
		}

		// lookup for new random nodes
		if(aTick - lastRandomLookup >= RANDOM_LOOKUP)
		{
			if(bucket->size() <= 2)
			{
				
			}
			lastRandomLookup = aTick;
		}

		if(bucket->empty())
			return;

		// select the oldest expired node
		Node::Ptr node = bucket->front();
		if(node->getType() < 4 && node->expires <= aTick)
		{
			// ping the oldest (expired) node
			node->setTimeout(aTick);
			DHT::getInstance()->info(node->getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(node->getIdentity().getUdpPort())), DHT::PING, node->getUser()->getCID(), node->getUdpKey());
		}
		else
		{
			bucket->pop_front();
			bucket->push_back(node);
		}
/*
#ifdef _DEBUG
		int verified = 0; int types[5] = { 0 };
		for(auto j = bucket->begin(); j != bucket->end(); ++j)
		{
			Node::Ptr n = *j;
			if(n->isIpVerified()) verified++;
			
			dcassert(n->getType() >= 0 && n->getType() <= 4);
			types[n->getType()]++;
		}
			
		dcdebug("DHT Zone Level %d, Nodes: %d (%d verified), Types: %d/%d/%d/%d/%d\n", level, bucket->size(), verified, types[0], types[1], types[2], types[3], types[4]);
#endif*/
}

}
