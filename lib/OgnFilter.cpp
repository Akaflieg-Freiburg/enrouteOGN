/***************************************************************************
 *   Copyright (C) 2021-2025 by Stefan Kebekus                             *
 *   stefan.kebekus@gmail.com                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "OgnFilter.h"

#include <chrono>
#include <cmath>

namespace Ogn {

void OgnCache::update(const OgnMessage& msg)
{
    // Only process traffic reports with a valid aircraft identifier
    if (msg.type != OgnMessageType::TRAFFIC_REPORT) {
        return;
    }
    if (msg.aircraftID.empty()) {
        return;
    }

    // find() with std::less<> accepts string_view directly — no string allocation.
    // Only convert to std::string when we actually need to insert a new key.
    auto it = m_cache.find(msg.aircraftID);
    const bool inserted = (it == m_cache.end());
    if (inserted) {
        it = m_cache.emplace(std::string(msg.aircraftID), OgnData{}).first;
    } else {
        // Reject if not strictly newer than what is already cached
        if (msg.timestamp <= it->second.timestamp) {
            return;
        }
    }
    OgnData& entry = it->second;

    // --- Timestamp ----------------------------------------------------------
    entry.timestamp = msg.timestamp;

    // --- Aircraft identification (skip unknown / default values) -------------
    if (msg.aircraftType != OgnAircraftType::unknown) {
        entry.aircraftTypes.insert(msg.aircraftType);
    }
    if (msg.addressType != OgnAddressType::UNKNOWN) {
        entry.addressTypes.insert(msg.addressType);
    }
}

void OgnCache::clean()
{
    auto now = std::chrono::system_clock::now();
    auto one_hour_ago = now - std::chrono::hours(1);

    for (auto it = m_cache.begin(); it != m_cache.end(); ) {
        if (it->second.timestamp < one_hour_ago) {
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

bool OgnFilter::filter(OgnMessage& msg)
{
    // Only process traffic reports
    if (msg.type != OgnMessageType::TRAFFIC_REPORT) {
        return false;
    }
    if (msg.aircraftID.empty()) {
        return false;
    }

    // Heterogeneous lookup: find() accepts string_view without allocating a temporary string.
    const auto& cache_data = m_cache.data();
    auto it = cache_data.find(msg.aircraftID);
    if (it != cache_data.end()) {
        const OgnData& cached = it->second;

        // Reject if message is not strictly newer
        if (msg.timestamp <= cached.timestamp) {
            return false;
        }

        // If message has unknown aircraft type but cache has known types, apply the first
        if (msg.aircraftType == OgnAircraftType::unknown &&
            !cached.aircraftTypes.empty()) {
            msg.aircraftType = *cached.aircraftTypes.begin();
        }
    }

    // Update the cache with this new message
    m_cache.update(msg);

    // Message is new
    return true;
}

} // namespace Ogn
