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

#include <QDateTime>
#include <QString>
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
    QString format(const Ogn::OgnMessage& message) override
    {
        // SBS-1 is only for traffic reports
        if (message.type != Ogn::OgnMessageType::TRAFFIC_REPORT) {
            return QString();
        }
        
        if (!message.coordinate.isValid()) {
            return QString();
        }
        
        // Get current timestamp
        const QDateTime now = QDateTime::currentDateTimeUtc();
        const QString dateStr = now.toString("yyyy/MM/dd");
        const QString timeStr = now.toString("HH:mm:ss.zzz");
        
        // Convert ICAO address to 6-character hex (pad with zeros if needed)
        QString icaoHex = QString(message.address).toUpper();
        while (icaoHex.length() < 6) {
            icaoHex.prepend('0');
        }
        
        // Convert altitude from meters to feet
        const int altitudeFeet = static_cast<int>(message.coordinate.altitude() * 3.28084);
        
        // Convert speed from m/s to knots (message.speed is already in knots as double)
        const int speedKnots = static_cast<int>(message.speed);
        
        // Convert course to integer degrees
        const int trackDegrees = static_cast<int>(message.course);
        
        // Convert vertical speed from m/s to feet/min
        const int verticalRateFpm = static_cast<int>(message.verticalSpeed * 196.85);
        
        // Format callsign (use flightnumber if available)
        QString callsign = QString(message.flightnumber);
        if (callsign.isEmpty()) {
            callsign = icaoHex; // Use ICAO as fallback
        }
        
        // Build MSG type 8 (all data) output
        return QString("MSG,8,111,11111,%1,111111,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,,,,,")
            .arg(icaoHex)
            .arg(dateStr)
            .arg(timeStr)
            .arg(dateStr)
            .arg(timeStr)
            .arg(callsign)
            .arg(altitudeFeet)
            .arg(speedKnots)
            .arg(trackDegrees)
            .arg(message.coordinate.latitude(), 0, 'f', 6)
            .arg(message.coordinate.longitude(), 0, 'f', 6)
            .arg(verticalRateFpm);
    }
};
