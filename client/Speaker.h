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

#ifndef DCPLUSPLUS_DCPP_SPEAKER_H
#define DCPLUSPLUS_DCPP_SPEAKER_H

#include <boost/range/algorithm/find.hpp>
#include <utility>
#include <vector>
#include "Thread.h"
#include "noexcept.h"
template<typename Listener>
class Speaker
{
		typedef std::vector<Listener*> ListenerList;
		class CBusy
		{
				bool& m_flag;
			public:
				explicit CBusy(bool& p_flag) : m_flag(p_flag)
				{
					m_flag = true;
				}
				~CBusy()
				{
					m_flag = false;
				}
		};
		void log_listener_list(const ListenerList& p_list, const char* p_log_message)
		{
#ifdef _DEBUG
			dcdebug("[log_listener_list][%s][tid = 0x%04X] [this=%p] count = %d ", p_log_message, ::GetCurrentThreadId(), this, p_list.size());
			int l_index = 0;
			for (auto i = p_list.cbegin(); i != p_list.cend(); ++i, ++l_index)
			{
				dcdebug("[%d] = %p", l_index, *i);
			}
			dcdebug("\r\n");
#endif
		}
	public:
		explicit Speaker() noexcept
		{
			m_fire_process = false;
		}
		virtual ~Speaker()
		{
			dcassert(m_listeners.empty());
			dcassert(m_remove_listeners.empty());
			dcassert(m_add_listeners.empty());
		}
		
		/// @todo simplify when we have variadic templates
		
		template<typename T0>
		void fire(T0 && type) noexcept
		{
			Lock l(m_listenerCS);
			log_listener_list(m_listeners, "fire-1");
			{
				CBusy l_fire_process(m_fire_process);
				for (auto i = m_listeners.cbegin(); i != m_listeners.cend(); ++i)
				{
					(*i)->on(std::forward<T0>(type));
				}
			}
			after_fire_process();
		}
		template<typename T0, typename T1>
		void fire(T0 && type, T1 && p1) noexcept
		{
			Lock l(m_listenerCS);
			log_listener_list(m_listeners, "fire-2");
			{
				CBusy l_fire_process(m_fire_process);
				for (auto i = m_listeners.cbegin(); i != m_listeners.cend(); ++i)
				{
					(*i)->on(std::forward<T0>(type), std::forward<T1>(p1)); // https://www.box.net/shared/da9ee6ddd7ec801b1a86
				}
			}
			after_fire_process();
		}
		template<typename T0, typename T1, typename T2>
		void fire(T0 && type, T1 && p1, T2 && p2) noexcept
		{
			Lock l(m_listenerCS);
			log_listener_list(m_listeners, "fire-3");
			{
				CBusy l_fire_process(m_fire_process);
				for (auto i = m_listeners.cbegin(); i != m_listeners.cend(); ++i)
				{
					(*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2)); // Venturi Firewall 2012-04-23_22-28-18_A6JRQEPFW5263A7S7ZOBOAJGFCMET3YJCUYOVCQ_0E0D7D71_crash-stack-r501-build-9812.dmp.bz2
				}
			}
			after_fire_process();
		}
		template<typename T0, typename T1, typename T2, typename T3>
		void fire(T0 && type, T1 && p1, T2 && p2, T3 && p3) noexcept
		{
			Lock l(m_listenerCS);
			log_listener_list(m_listeners, "fire-4");
			{
				CBusy l_fire_process(m_fire_process);
				for (auto i = m_listeners.cbegin(); i != m_listeners.cend(); ++i)
				{
					(*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2), std::forward<T3>(p3));
				}
			}
			after_fire_process();
		}
		template<typename T0, typename T1, typename T2, typename T3, typename T4>
		void fire(T0 && type, T1 && p1, T2 && p2, T3 && p3, T4 && p4) noexcept
		{
			Lock l(m_listenerCS);
			log_listener_list(m_listeners, "fire-5");
			{
				CBusy l_fire_process(m_fire_process);
				for (auto i = m_listeners.cbegin(); i != m_listeners.cend(); ++i)
				{
					(*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2), std::forward<T3>(p3), std::forward<T4>(p4));
				}
			}
			after_fire_process();
		}
		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
		void fire(T0 && type, T1 && p1, T2 && p2, T3 && p3, T4 && p4, T5 && p5) noexcept
		{
			Lock l(m_listenerCS);
			log_listener_list(m_listeners, "fire-6");
			{
				CBusy l_fire_process(m_fire_process);
				for (auto i = m_listeners.cbegin(); i != m_listeners.cend(); ++i)
				{
					(*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2), std::forward<T3>(p3), std::forward<T4>(p4), std::forward<T5>(p5));
				}
			}
			after_fire_process();
		}
		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
		void fire(T0 && type, T1 && p1, T2 && p2, T3 && p3, T4 && p4, T5 && p5, T6 && p6) noexcept
		{
			Lock l(m_listenerCS);
			log_listener_list(m_listeners, "fire-7");
			{
				CBusy l_fire_process(m_fire_process);
				for (auto i = m_listeners.cbegin(); i != m_listeners.cend(); ++i)
				{
					(*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2), std::forward<T3>(p3), std::forward<T4>(p4), std::forward<T5>(p5), std::forward<T6>(p6));
				}
			}
			after_fire_process();
		}
		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
		void fire(T0 && type, T1 && p1, T2 && p2, T3 && p3, T4 && p4, T5 && p5, T6 && p6, T7 && p7) noexcept
		{
			Lock l(m_listenerCS);
			log_listener_list(m_listeners, "fire-8");
			{
				CBusy l_fire_process(m_fire_process);
				for (auto i = m_listeners.cbegin(); i != m_listeners.cend(); ++i)
				{
					(*i)->on(std::forward<T0>(type), std::forward<T1>(p1), std::forward<T2>(p2), std::forward<T3>(p3), std::forward<T4>(p4), std::forward<T5>(p5), std::forward<T6>(p6), std::forward<T7>(p7));
				}
			}
			after_fire_process();
		}
		void addListener(Listener* p_Listener)
		{
			Lock l(m_listenerCS);
			if (m_fire_process)
				m_add_listeners.push_back(p_Listener);
			else
				internal_add(p_Listener);
		}
		
		void removeListener(Listener* p_Listener)
		{
			Lock l(m_listenerCS);
			if (m_fire_process)
				m_remove_listeners.push_back(p_Listener);
			else
				internal_remove(p_Listener);
		}
		
		void removeListeners()
		{
			Lock l(m_listenerCS);
			if (m_fire_process)
				m_remove_listeners.insert(m_remove_listeners.end(), m_listeners.begin(), m_listeners.end());
			else
			{
				log_listener_list(m_listeners, "removeListenerAll");
				m_listeners.clear();
				dcassert(m_add_listeners.empty());
				m_add_listeners.clear();
				dcassert(m_remove_listeners.empty());
				m_remove_listeners.clear();
			}
		}
		
	private:
		void after_fire_process()
		{
			for (auto i = m_add_listeners.cbegin(); i != m_add_listeners.cend(); ++i)
				internal_add(*i);
				
			m_add_listeners.clear();
			
			for (auto i = m_remove_listeners.cbegin(); i != m_remove_listeners.cend(); ++i)
				internal_remove(*i);
				
			m_remove_listeners.clear();
		}
		
		void internal_add(Listener* p_Listener)
		{
			log_listener_list(m_listeners, "addListener-before");
			dcdebug("[tid = 0x%04X][this=%p] addListener[%p]: count = %d\n", ::GetCurrentThreadId(), this, p_Listener, m_listeners.size());
			if (boost::range::find(m_listeners, p_Listener) == m_listeners.cend())
			{
				m_listeners.push_back(p_Listener);
				log_listener_list(m_listeners, "addListener-after");
			}
			else
			{
				dcassert(0);
				log_listener_list(m_listeners, "addListener-twice!!!");
			}
		}
		
		void internal_remove(Listener* p_Listener)
		{
			log_listener_list(m_listeners, "removeListener-before");
			dcdebug("[tid = 0x%04X][this=%p] removeListener[%p]: count = %d\n", ::GetCurrentThreadId(), this, p_Listener, m_listeners.size());
			const auto it = boost::range::find(m_listeners, p_Listener);
			if (it != m_listeners.cend())
			{
				m_listeners.erase(it);
				log_listener_list(m_listeners, "removeListener-after");
			}
			else
			{
				dcassert(0);
				log_listener_list(m_listeners, "removeListener-zombie!!!");
			}
		}
		
		bool m_fire_process;
		ListenerList m_listeners;
		ListenerList m_remove_listeners;
		ListenerList m_add_listeners;
		CriticalSection m_listenerCS;
};

#endif // !defined(SPEAKER_H)

/**
 * @file
 * $Id: Speaker.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
