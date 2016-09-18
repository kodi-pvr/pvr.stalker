#pragma once

/*
 *      Copyright (C) 2016  Jamal Edey
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

#include <algorithm>
#include <string>
#include <vector>

namespace Base {
    struct Channel {
        unsigned int uniqueId;
        int number;
        std::string name;
    };

    template<class ChannelType>
    class ChannelManager {
    public:
        typedef typename std::vector<ChannelType>::iterator ChannelIterator;

        ChannelManager() { }

        virtual ~ChannelManager() {
            m_channels.clear();
        }

        virtual ChannelIterator GetChannelIterator(unsigned int uniqueId) {
            ChannelIterator it;
            it = std::find_if(m_channels.begin(), m_channels.end(), [uniqueId](const Channel &c) {
                return c.uniqueId == uniqueId;
            });
            return it;
        }

        virtual ChannelType *GetChannel(unsigned int uniqueId) {
            ChannelIterator it = GetChannelIterator(uniqueId);
            return it != m_channels.end() ? &(*it) : nullptr;
        }

        virtual std::vector<ChannelType> GetChannels() {
            return m_channels;
        }

    protected:
        std::vector<ChannelType> m_channels;
    };
}
