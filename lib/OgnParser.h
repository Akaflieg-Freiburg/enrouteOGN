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

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace Ogn {
struct OgnMessage;

/*! \brief Aircraft type for OGN messages
 *
 *  This enum defines aircraft types used in OGN/APRS messages.
 *  The list is modeled after the FLARM/NMEA specification.
 */
enum class OgnAircraftType
{
    unknown = 0,       /*!< Unknown aircraft type */
    Aircraft,          /*!< Fixed wing aircraft */
    Airship,           /*!< Airship, such as a zeppelin or a blimp */
    Balloon,           /*!< Balloon */
    Copter,            /*!< Helicopter, gyrocopter or rotorcraft */
    Drone,             /*!< Drone */
    Glider,            /*!< Glider, including powered gliders and touring motor gliders */
    HangGlider,        /*!< Hang glider */
    Jet,               /*!< Jet aircraft */
    Paraglider,        /*!< Paraglider */
    Skydiver,          /*!< Skydiver */
    StaticObstacle,    /*!< Static obstacle */
    TowPlane           /*!< Tow plane */
};

/*! \brief Parser for OGN glidernet.org traffic receiver.
* 
* sentences similar to NMEA like the following: 
* 
*   FLRDDE626>APRS,qAS,EGHL:/074548h5111.32N/00102.04W'086/007/A=000607 id0ADDE626 -019fpm +0.0rot 5.5dB 3e -4.3kHz
* 
* \see http://wiki.glidernet.org/wiki:subscribe-to-ogn-data
* \see http://wiki.glidernet.org/wiki:ogn-flavoured-aprs
* \see http://wiki.glidernet.org/aprs-interaction-examples
* \see https://github.com/svoop/ogn_client-ruby/wiki/SenderBeacon
*
* This class is used in the UnitTest. It should not have external dependencies like GlobalObject.
*/
class OgnParser {
public:
    static void parseAprsisMessage(OgnMessage& ognMessage);
    static std::string formatLoginString(std::string_view callSign,
                                         double latitude,
                                         double longitude,
                                         unsigned int receiveRadius,
                                         std::string_view appName,
                                         std::string_view appVersion);
    static std::string formatPositionReport(std::string_view callSign,
                                            double latitude,
                                            double longitude,
                                            double altitude,
                                            double course,
                                            double speed,
                                            OgnAircraftType aircraftType);
    static std::string formatFilterCommand(double latitude, double longitude, unsigned int receiveRadiusKm);

private:
    static std::string formatFilter(double latitude, double longitude, unsigned int receiveRadius);
    static std::string formatLatitude(double latitude);
    static std::string formatLongitude(double longitude);
    static std::string calculatePassword(std::string_view callSign);
    static double decodeLatitude(std::string_view nmeaLatitude, char latitudeDirection, char latEnhancement);
    static double decodeLongitude(std::string_view nmeaLongitude, char longitudeDirection, char lonEnhancement);
    static void parseTrafficReport(OgnMessage &ognMessage, std::string_view header, std::string_view body);
    static void parseCommentMessage(OgnMessage& ognMessage);
    static void parseStatusMessage(OgnMessage &ognMessage, std::string_view header, std::string_view body);
};

enum class OgnMessageType
{
    UNKNOWN,
    TRAFFIC_REPORT,
    COMMENT,
    STATUS,
    WEATHER,
};

// see http://wiki.glidernet.org/wiki:ogn-flavoured-aprs
enum class OgnAddressType
{ 
    UNKNOWN = 0,
    ICAO = 1,
    FLARM = 2,
    OGN_TRACKER = 3,
};

enum class OgnSymbol
{ 
    UNKNOWN,
    GLIDER,
    HELICOPTER,
    PARACHUTE,
    AIRCRAFT,
    JET,
    BALLOON,
    STATIC_OBJECT,
    UAV,
    WEATHERSTATION,
};

struct OgnMessage
{
    std::string sentence;       // e.g. "FLRDDE626>APRS,qAS,EGHL:/074548h5111.32N/00102.04W'086/007/A=000607 id0ADDE626 -019fpm +0.0rot 5.5dB 3e -4.3kHz"
    OgnMessageType type = OgnMessageType::UNKNOWN; // e.g. OgnMessageType::TRAFFIC_REPORT

    std::string_view sourceId;       // like ENROUTE12345
    std::string_view timestamp;      // hhmmss
    double latitude = std::numeric_limits<double>::quiet_NaN();  // Latitude in degrees (WGS84)
    double longitude = std::numeric_limits<double>::quiet_NaN(); // Longitude in degrees (WGS84)
    double altitude = std::numeric_limits<double>::quiet_NaN();  // Altitude in meters (MSL)
    OgnSymbol symbol = OgnSymbol::UNKNOWN; // the symbol that should be shown on the map, typically Aircraft.

    double course = {};         // course in degrees
    double speed = {};          // speed in knots
    std::string_view aircraftID;     // aircraft ID, e.g. "id0ADDE626"
    double verticalSpeed = {};  // in m/s
    std::string_view rotationRate;   // like "+0.0rot"
    std::string_view signalStrength; // like "5.5dB"
    std::string_view errorCount;     // like "3e"
    std::string_view frequencyOffset;// like "-4.3kHz"
    std::string_view squawk;         // like "sq2244"
    std::string_view flightlevel;    // like "FL350.00"
    std::string_view flightnumber;   // Flight number, e.g., "DLH2AV" or "SRR6119"
    std::string_view gpsInfo;        // like "gps:0.0"
    OgnAircraftType aircraftType = OgnAircraftType::unknown; // e.g., "Glider", "Tow Plane", etc.
    OgnAddressType addressType = OgnAddressType::UNKNOWN; // e.g., "ICAO", "FLARM", "OGN Tracker"
    std::string_view address;        // like "4D21C2"
    bool stealthMode = false;   // true if the aircraft shall be hidden
    bool noTrackingFlag = false;// true if the aircraft shall not be tracked

    uint32_t wind_direction = {};  // degree 0..359
    uint32_t wind_speed = {};      // m/s
    uint32_t wind_gust_speed = {}; // m/s
    uint32_t temperature = {};     // degree C
    uint32_t humidity = {};        // percent
    double pressure = {};          // hPa

    void reset()
    {
        sentence.clear();
        type = OgnMessageType::UNKNOWN;
        sourceId = std::string_view();       
        timestamp = std::string_view();      
        latitude = std::numeric_limits<double>::quiet_NaN();
        longitude = std::numeric_limits<double>::quiet_NaN();
        altitude = std::numeric_limits<double>::quiet_NaN();
        symbol = OgnSymbol::UNKNOWN;
        course = 0.0;
        speed = 0.0;
        aircraftID = std::string_view();     
        verticalSpeed = 0.0;
        rotationRate = std::string_view();   
        signalStrength = std::string_view(); 
        errorCount = std::string_view();     
        frequencyOffset = std::string_view();
        squawk = std::string_view();         
        flightlevel = std::string_view();    
        flightnumber = std::string_view();   
        gpsInfo = std::string_view();        
        aircraftType = OgnAircraftType::unknown;
        addressType = OgnAddressType::UNKNOWN;
        address = std::string_view();        
        stealthMode = false;
        noTrackingFlag = false;
        wind_direction = 0;
        wind_speed = 0;
        wind_gust_speed = 0;
        temperature = 0;
        humidity = 0;
        pressure = 0.0;
    }
};
}
