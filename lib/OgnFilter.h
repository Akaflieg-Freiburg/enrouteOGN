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

#pragma once

#include "OgnParser.h"

#include <map>
#include <set>
#include <string>

namespace Ogn {

/*! \brief Persistent OGN data entry for a single aircraft.
 *
 *  Lightweight cache entry containing only the most relevant tracking data:
 *  timestamp, aircraft type, address information, and call sign.
 */
struct OgnData {
    std::chrono::system_clock::time_point timestamp; /*!< UTC time of the message */
    std::set<OgnAircraftType> aircraftTypes;          /*!< all aircraft types seen (e.g. Glider, TowPlane) */
    std::set<OgnAddressType> addressTypes;            /*!< all address types seen (e.g. ICAO, FLARM) */
};


/*! \brief Cache of OGN traffic data, keyed by aircraft ID.
 *
 *  Maintains a map from aircraftID to the most recent OgnData for that
 *  aircraft. Feed incoming parsed messages via update().
 */
class OgnCache {
    // std::map with std::less<> enables heterogeneous lookup in C++17:
    // find() and insert() accept string_view without allocating a temporary string.
    // Key: std::string (aircraft ID, e.g., "id0ADDE626")
    // Value: OgnData (cached aircraft information including timestamp, type, address types)
    using CacheMap = std::map<std::string, OgnData, std::less<>>;

public:
    /*! \brief Update the cache with data from a new OGN traffic message.
     *
     *  Processing rules:
     *  - Only TRAFFIC_REPORT messages with a non-empty aircraftID are accepted.
     *  - The stored entry is updated only if \p msg carries a strictly newer
     *    timestamp than the cached one (epoch timestamp counts as "no data yet").
     *  - Fields that carry no valid information are not overwritten:
     *    - aircraftType: skipped when OgnAircraftType::unknown.
     *    - addressType: added to the set of known address types.
     *    - All string fields: skipped when empty.
     *  - callSign is set to flightnumber when flightnumber is non-empty;
     *    otherwise it is initialised from sourceId if not yet populated.
     *
     *  \param msg  A parsed OGN message (produced by OgnParser::parseAprsisMessage).
     */
    void update(const OgnMessage& msg);

    /*! \brief Remove all cached entries with timestamps older than 1 hour.
     *
     *  This is useful to purge stale aircraft data from the cache.
     *  Entries with an epoch timestamp (no valid data) are also removed.
     */
    void clean();

    /*! \brief Read-only access to the internal map. */
    const CacheMap& data() const { return m_cache; }

private:
    CacheMap m_cache;
};


/*! \brief Filter for OGN traffic messages based on timestamp and cached data.
 *
 *  Evaluates incoming OGN messages and determines if they contain new data
 *  by comparing timestamps against a cached history. Also enriches messages
 *  with previously known aircraft type information.
 */
class OgnFilter {
public:
    /*! \brief Evaluate a traffic message and update it based on cached data.
     *
     *  Processing rules:
     *  - Only TRAFFIC_REPORT messages are accepted.
     *  - Returns false (no new data) if the message timestamp is older than
     *    or equal to what is cached for this aircraftID.
     *  - If the message timestamp is strictly newer than the cached entry:
     *    - If msg.aircraftType is unknown but the cache has a known type,
     *      msg.aircraftType is updated from the cache.
     *    - Returns true (new data).
     *  - The cache is updated with the new message data.
     *
     *  \param msg  A parsed OGN message to filter and enrich. May be modified
     *              if a known aircraft type is applied from cache.
     *  \return     True if the message contains new data, false otherwise.
     */
    bool filter(OgnMessage& msg);

    /*! \brief Remove all cached entries with timestamps older than 1 hour. */
    void clean() { m_cache.clean(); }

    /*! \brief Read-only access to the internal cache. */
    const OgnCache& cache() const { return m_cache; }

private:
    OgnCache m_cache;
};

} // namespace Ogn
