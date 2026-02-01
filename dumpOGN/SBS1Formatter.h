/***************************************************************************
 *   Copyright (C) 2025 by Stefan Kebekus                                 *
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

#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include "OutputFormatter.h"

/*! \brief SBS-1 BaseStation format (dump1090-compatible)
 *
 *  Outputs messages in the SBS-1 format used by dump1090 and compatible tools.
 *  This is the text-based format that tools like tar1090, VirtualRadarServer, etc. understand.
 *  
 *  SBS-1 Format Field breakdown (22 fields):
 *  1. Message type: MSG
 *  2. Transmission type: 1-8 (1=callsign, 3=position, 4=speed/heading, 8=all data)
 *  3. Session ID
 *  4. Aircraft ID  
 *  5. ICAO Hex Ident (6 characters)
 *  6. Flight ID
 *  7. Date generated (YYYY/MM/DD)
 *  8. Time generated (HH:MM:SS.SSS)
 *  9. Date logged (YYYY/MM/DD)
 *  10. Time logged (HH:MM:SS.SSS)
 *  11. Callsign
 *  12. Altitude (feet)
 *  13. Ground speed (knots)
 *  14. Track (degrees)
 *  15. Latitude
 *  16. Longitude
 *  17. Vertical rate (feet/min)
 *  18. Squawk code
 *  19. Alert flag
 *  20. Emergency flag
 *  21. SPI flag
 *  22. Is on ground
 */
class SBS1Formatter : public OutputFormatter
{
public:
    std::string format(const Ogn::OgnMessage& message) override
    {
        // SBS-1 is only for traffic reports
        if (message.type != Ogn::OgnMessageType::TRAFFIC_REPORT) {
            return std::string();
        }
        
        if (std::isnan(message.latitude) || std::isnan(message.longitude)) {
            return std::string();
        }
        
        // Get current timestamp
        std::time_t now = std::time(nullptr);
        std::tm* utc_tm = std::gmtime(&now);
        
        char dateStr[32];
        char timeStr[32];
        std::snprintf(dateStr, sizeof(dateStr), "%04d/%02d/%02d", 
                     utc_tm->tm_year + 1900, utc_tm->tm_mon + 1, utc_tm->tm_mday);
        std::snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d.000",
                     utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec);
        
        // Convert ICAO address to 6-character hex (pad with zeros if needed)
        std::string icaoHex(message.address);
        // Convert to uppercase
        for (char& c : icaoHex) {
            c = std::toupper(static_cast<unsigned char>(c));
        }
        while (icaoHex.length() < 6) {
            icaoHex = "0" + icaoHex;
        }
        
        // Convert altitude from meters to feet
        const int altitudeFeet = static_cast<int>(message.altitude * 3.28084);
        
        // Convert speed from m/s to knots (message.speed is already in knots as double)
        const int speedKnots = static_cast<int>(message.speed);
        
        // Convert course to integer degrees
        const int trackDegrees = static_cast<int>(message.course);
        
        // Convert vertical speed from m/s to feet/min
        const int verticalRateFpm = static_cast<int>(message.verticalSpeed * 196.85);
        
        // Format callsign (use flightnumber if available)
        std::string callsign(message.flightnumber);
        if (callsign.empty()) {
            callsign = icaoHex; // Use ICAO as fallback
        }
        
        // Build MSG type 8 (all data) output
        std::ostringstream oss;
        oss << "MSG,8,111,11111," << icaoHex << ",111111,"
            << dateStr << "," << timeStr << ","
            << dateStr << "," << timeStr << ","
            << callsign << ","
            << altitudeFeet << ","
            << speedKnots << ","
            << trackDegrees << ","
            << std::fixed << std::setprecision(6) << message.latitude << ","
            << std::fixed << std::setprecision(6) << message.longitude << ","
            << verticalRateFpm << ",,,,,";
        
        return oss.str();
    }
};
