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

#include "OgnParser.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <ctime>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <map>
#include <cassert>

#define OGNPARSER_DEBUG 0

namespace {

// Helper functions for string_view operations (C++17 compatible)
inline bool starts_with(std::string_view sv, std::string_view prefix) {
    return sv.size() >= prefix.size() && sv.substr(0, prefix.size()) == prefix;
}

inline bool ends_with(std::string_view sv, std::string_view suffix) {
    return sv.size() >= suffix.size() && sv.substr(sv.size() - suffix.size()) == suffix;
}

// see http://wiki.glidernet.org/wiki:ogn-flavoured-aprs
static const std::unordered_map<std::string, Ogn::OgnAircraftType> AircraftTypeMap = {
    {"/z",  Ogn::OgnAircraftType::unknown},         // Unknown
    {"/'",  Ogn::OgnAircraftType::Glider},          // Glider
    {"/X",  Ogn::OgnAircraftType::Copter},          // Helicopter
    {"/g",  Ogn::OgnAircraftType::Paraglider},      // Parachute, Hang Glider, Paraglider
    {"\\^", Ogn::OgnAircraftType::Aircraft},        // Drop Plane, Powered Aircraft
    {"/^",  Ogn::OgnAircraftType::Jet},             // Jet Aircraft
    {"/O",  Ogn::OgnAircraftType::Balloon},         // Balloon, Airship
    {"\\n", Ogn::OgnAircraftType::StaticObstacle},  // Static Object
};

static const std::unordered_map<std::string, Ogn::OgnSymbol> AprsSymbolMap = {
    {"/z",  Ogn::OgnSymbol::UNKNOWN},        // Unknown
    {"/'",  Ogn::OgnSymbol::GLIDER},         // Glider
    {"/X",  Ogn::OgnSymbol::HELICOPTER},     // Helicopter
    {"/g",  Ogn::OgnSymbol::PARACHUTE},      // Parachute, Hang Glider, Paraglider
    {"\\^", Ogn::OgnSymbol::AIRCRAFT},       // Drop Plane, Powered Aircraft
    {"/^",  Ogn::OgnSymbol::JET},            // Jet Aircraft
    {"/O",  Ogn::OgnSymbol::BALLOON},        // Balloon, Airship
    {"\\n", Ogn::OgnSymbol::STATIC_OBJECT},  // Static Object
    {"/_",  Ogn::OgnSymbol::WEATHERSTATION},  // WeatherStation
};

// see http://wiki.glidernet.org/wiki:ogn-flavoured-aprs
static const std::map<uint32_t, Ogn::OgnAircraftType> AircraftCategoryMap = {
    {0x0, Ogn::OgnAircraftType::unknown},         // Reserved
    {0x1, Ogn::OgnAircraftType::Glider},          // Glider/Motor Glider/TMG
    {0x2, Ogn::OgnAircraftType::TowPlane},        // Tow Plane/Tug Plane
    {0x3, Ogn::OgnAircraftType::Copter},          // Helicopter/Gyrocopter/Rotorcraft
    {0x4, Ogn::OgnAircraftType::Skydiver},        // Skydiver/Parachute
    {0x5, Ogn::OgnAircraftType::Aircraft},        // Drop Plane for Skydivers
    {0x6, Ogn::OgnAircraftType::HangGlider},      // Hang Glider (hard)
    {0x7, Ogn::OgnAircraftType::Paraglider},      // Paraglider (soft)
    {0x8, Ogn::OgnAircraftType::Aircraft},        // Aircraft with reciprocating engine(s)
    {0x9, Ogn::OgnAircraftType::Jet},             // Aircraft with jet/turboprop engine(s)
    {0xA, Ogn::OgnAircraftType::unknown},         // Unknown
    {0xB, Ogn::OgnAircraftType::Balloon},         // Balloon (hot, gas, weather, static)
    {0xC, Ogn::OgnAircraftType::Airship},         // Airship/Blimp/Zeppelin
    {0xD, Ogn::OgnAircraftType::Drone},           // UAV/RPAS/Drone
    {0xE, Ogn::OgnAircraftType::unknown},         // Reserved
    {0xF, Ogn::OgnAircraftType::StaticObstacle}   // Static Obstacle
};

} // namespace

namespace Ogn {


void OgnParser::parseAprsisMessage(OgnMessage& ognMessage)
{
    // In this function 
    // avoid heap allocations for performance reasons.

    // Expect that data Structure OgnMessage is reset or initialized to default values.
    assert(ognMessage.type == OgnMessageType::UNKNOWN);

    const std::string_view sentence(ognMessage.sentence);
    if (starts_with(sentence, "#"))
    {
        // Comment message  
        parseCommentMessage(ognMessage);
        return;
    }

    // Split the sentence into header and body at the first colon
    auto const colonIndex = sentence.find(':');
    if (colonIndex == std::string_view::npos)
    {
#if OGNPARSER_DEBUG
        // Debug: Invalid message format
#endif
        ognMessage.type = OgnMessageType::UNKNOWN;
        return;
    }

    // This function runs all the time, so it is performance critical.
    // I try to avoid heap allocations and use a lot of std::string_view.
    std::string_view const header = sentence.substr(0, colonIndex);
    std::string_view const body = sentence.substr(colonIndex + 1);

    // Check if header and body are valid
    if (header.size() < 5 || body.size() < 5)
    {
#if OGNPARSER_DEBUG
        // Debug: Invalid message header or body
#endif
        ognMessage.type = OgnMessageType::UNKNOWN;
        return;
    }

    // Determine the type of message based on the first character in the body
    if (starts_with(body, "/"))
    {
        // "/" indicates a Traffic Report
        parseTrafficReport(ognMessage, header, body);
        return;
    }
    if (starts_with(body, ">"))
    {
        // ">" indicates a Receiver Status
        parseStatusMessage(ognMessage, header, body);
        return;
    }

    ognMessage.type = OgnMessageType::UNKNOWN;
#if OGNPARSER_DEBUG
    // Debug: Unknown message type
#endif
    return;
}

double OgnParser::decodeLatitude(std::string_view nmeaLatitude, char latitudeDirection, char latEnhancement)
{
    // In this function 
    // avoid heap allocations for performance reasons.

    // e.g. "5111.32"
    if (nmeaLatitude.size() < 7) {
        // Debug: invalid input
        return std::numeric_limits<double>::quiet_NaN(); // Invalid input
    }

    // Parse degrees (first 2 characters)
    double latitudeDegrees = 0.0;
#if defined(__ANDROID__) || defined(__APPLE__)
    // Android NDK and Apple platforms don't support std::from_chars for floating-point yet
    char* endPtr = nullptr;
    char degreesBuffer[3] = {nmeaLatitude[0], nmeaLatitude[1], '\0'};
    latitudeDegrees = std::strtod(degreesBuffer, &endPtr);
    if (endPtr == degreesBuffer) {
        return std::numeric_limits<double>::quiet_NaN();
    }
#else
    auto result = std::from_chars(nmeaLatitude.data(), nmeaLatitude.data() + 2, latitudeDegrees);
    if (result.ec != std::errc{}) {
        // Debug: decodeLatitude from_chars failed 1
        return std::numeric_limits<double>::quiet_NaN();
    }
#endif

    // Parse minutes (remaining characters after the first 2)
    double latitudeMinutes = 0.0;
    std::string_view const minutesStr = nmeaLatitude.substr(2);
#if defined(__ANDROID__) || defined(__APPLE__)
    // Android NDK and Apple platforms don't support std::from_chars for floating-point yet
    std::string minutesString(minutesStr);
    latitudeMinutes = std::strtod(minutesString.c_str(), &endPtr);
    if (endPtr == minutesString.c_str()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
#else
    auto minutesResult = std::from_chars(minutesStr.data(), minutesStr.data() + minutesStr.size(), latitudeMinutes);
    if (minutesResult.ec != std::errc{}) {
        // Debug: decodeLatitude from_chars failed 2
        return std::numeric_limits<double>::quiet_NaN();
    }
#endif

    // Combine degrees and minutes
    double latitude = latitudeDegrees + (latitudeMinutes / 60.0);

    // Apply precision enhancement
    if(latEnhancement != '\0') {
        if (latEnhancement >= '0' && latEnhancement <= '9') {
            latitude += static_cast<double>(latEnhancement - '0') * 0.001 / 60;
        }
    }

    // Adjust for direction (South is negative)
    if (latitudeDirection == 'S') {
        latitude = -latitude;
    }

    return latitude;
}

double OgnParser::decodeLongitude(std::string_view nmeaLongitude, char longitudeDirection, char lonEnhancement)
{
    // In this function 
    // avoid heap allocations for performance reasons.

    // e.g. "00102.04W"
    if (nmeaLongitude.size() < 8) {
        // Debug: lon invalid input
        return std::numeric_limits<double>::quiet_NaN(); // Invalid input
    }

    // Parse degrees (first 3 characters)
    double longitudeDegrees = 0.0;
#if defined(__ANDROID__) || defined(__APPLE__)
    // Android NDK and Apple platforms don't support std::from_chars for floating-point yet
    char* endPtr = nullptr;
    char degreesBuffer[4] = {nmeaLongitude[0], nmeaLongitude[1], nmeaLongitude[2], '\0'};
    longitudeDegrees = std::strtod(degreesBuffer, &endPtr);
    if (endPtr == degreesBuffer) {
        return std::numeric_limits<double>::quiet_NaN();
    }
#else
    auto result = std::from_chars(nmeaLongitude.data(), nmeaLongitude.data() + 3, longitudeDegrees);
    if (result.ec != std::errc{}) {
        // Debug: lon from_chars failed 1
        return std::numeric_limits<double>::quiet_NaN();
    }
#endif

    // Parse minutes (remaining characters after the first 3)
    double longitudeMinutes = 0.0;
    std::string_view const minutesStr = nmeaLongitude.substr(3);
#if defined(__ANDROID__) || defined(__APPLE__)
    // Android NDK and Apple platforms don't support std::from_chars for floating-point yet
    std::string minutesString(minutesStr);
    longitudeMinutes = std::strtod(minutesString.c_str(), &endPtr);
    if (endPtr == minutesString.c_str()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
#else
    auto minutesResult = std::from_chars(minutesStr.data(), minutesStr.data() + minutesStr.size(), longitudeMinutes);
    if (minutesResult.ec != std::errc{}) {
        // Debug: decodeLongitude from_chars failed 2
        return std::numeric_limits<double>::quiet_NaN();
    }
#endif

    // Combine degrees and minutes
    double longitude = longitudeDegrees + (longitudeMinutes / 60.0);

    // Apply precision enhancement
    if(lonEnhancement != '\0') {
        if (lonEnhancement >= '0' && lonEnhancement <= '9') {
            longitude += static_cast<double>(lonEnhancement - '0') * 0.001 / 60;
        }
    }

    // Adjust for direction (West is negative)
    if (longitudeDirection == 'W') {
        longitude = -longitude;
    }

    return longitude;
}

void OgnParser::parseTrafficReport(OgnMessage& ognMessage, const std::string_view header, const std::string_view body)
{
    // In this function 
    // avoid heap allocations for performance reasons.

    // e.g. header = "FLRDDE626>APRS,qAS,EGHL:"
    // e.g. body = "/074548h5111.32N/00102.04W'086/007/A=000607 id0ADDE626 -019fpm +0.0rot 5.5dB 3e -4.3kHz" (traffic report)
    // e.g. body = "/001140h4741.90N/01104.20E^/A=034868 !W91! id254D21C2 +128fpm FL350.00 A3:AXY547M Sq2244" (traffic report)
    // e.g. body = "/222245h4803.92N/00800.93E_292/005g010t030h00b65526 5.2dB" (weather report, but starting with '/' like a traffic report)
    #if OGNPARSER_DEBUG
    // Debug: Traffic Report
    #endif

    // Check if the body starts with a '/'
    // This is a requirement for the Traffic Report format.
    if(body[0] != '/') {
        #if OGNPARSER_DEBUG
        // Debug: Invalid body format in Traffic Report
        #endif
        ognMessage.type = OgnMessageType::UNKNOWN;
        return;
    }

    // Parse the Header
    auto const index = header.find('>');
    if (index == std::string_view::npos) {
        #if OGNPARSER_DEBUG
        // Debug: Invalid header format in Traffic Report
        #endif
        ognMessage.type = OgnMessageType::UNKNOWN;
        return;
    }
    ognMessage.type = OgnMessageType::TRAFFIC_REPORT;
    ognMessage.sourceId = header.substr(0, index);

    // Parse the body
    auto const blankIndex = body.find(' ');
    std::string_view aprsPart;
    std::string_view ognPart;
    if (blankIndex == std::string_view::npos) {
        aprsPart = body;
    } else {
        aprsPart = body.substr(0, blankIndex); // APRS Part before the first blank
        ognPart = body.substr(blankIndex + 1); // OGN Part after the first blank
    }

    // Parse aprsPart
    if (!starts_with(aprsPart, "/") || aprsPart.size() < 30) {
        ognMessage.type = OgnMessageType::UNKNOWN;
        return;
    }

    // Parse timestamp
    ognMessage.timestamp = aprsPart.substr(1, 6);

    // Parse coordinates
    {
        // latitude
        std::string_view const latString = aprsPart.substr(8, 7); // "4741.90"
        char const latDirection = aprsPart[15];      // "N" or "S"
        // longitude
        std::string_view const lonString = aprsPart.substr(17, 8); // "01104.20"
        char const lonDirection = aprsPart[25];       // "E" or "W"
        // optional precision enhancement, e.g. "!W91"
        char latEnhancement = '\0';
        char lonEnhancement = '\0';
        auto const precisionIndex = body.find("!W");
        if (precisionIndex != std::string_view::npos && body.size() > precisionIndex + 4) {
            latEnhancement = body[precisionIndex + 2];
            lonEnhancement = body[precisionIndex + 3];
        }
        // decode
        double const latitude = decodeLatitude(latString, latDirection, latEnhancement);
        double const longitude = decodeLongitude(lonString, lonDirection, lonEnhancement);
        ognMessage.latitude = latitude;
        ognMessage.longitude = longitude;
    }


    // Parse symbol
    char const symbolTable = aprsPart[16];
    char const symbolCode = aprsPart[26];
    char symbolKey[3] = {symbolTable, symbolCode, '\0'};
    auto it = AprsSymbolMap.find(symbolKey);
    if (it != AprsSymbolMap.end()) {
        ognMessage.symbol = it->second;
    } else {
        ognMessage.symbol = OgnSymbol::UNKNOWN;
    }

    // If the weather report is detected (e.g. an underscore appears after the longitude)
    if(ognMessage.symbol == OgnSymbol::WEATHERSTATION) {
        ognMessage.type = OgnMessageType::WEATHER;
        int const underscoreIndex = 26;
        // Decode wind direction: next 3 digits after the underscore
        std::string_view const windDirStr = aprsPart.substr(underscoreIndex + 1, 3);
        uint32_t windDir = 0;
        auto result = std::from_chars(windDirStr.data(), windDirStr.data() + windDirStr.size(), windDir);
        if (result.ec == std::errc{}) {
            ognMessage.wind_direction = windDir;
        }
        
        // Find the slash following the wind direction to decode wind speed
        auto const slashAfterUnderscore = aprsPart.find("/", underscoreIndex);
        if (slashAfterUnderscore != std::string_view::npos) {
            std::string_view const windSpeedStr = aprsPart.substr(slashAfterUnderscore + 1, 3);
            uint32_t windSpd = 0;
            auto result = std::from_chars(windSpeedStr.data(), windSpeedStr.data() + windSpeedStr.size(), windSpd);
            if (result.ec == std::errc{}) {
                ognMessage.wind_speed = windSpd;
            }
        }
        // Decode wind gust speed: look for 'g'
        auto const gIndex = aprsPart.find("g", underscoreIndex);
        if (gIndex != std::string_view::npos) {
            std::string_view const gustStr = aprsPart.substr(gIndex + 1, 3);
            uint32_t gust = 0;
            auto result = std::from_chars(gustStr.data(), gustStr.data() + gustStr.size(), gust);
            if (result.ec == std::errc{}) {
                ognMessage.wind_gust_speed = gust;
            }
        }
        // Decode temperature: look for 't'
        auto const tIndex = aprsPart.find("t", underscoreIndex);
        if (tIndex != std::string_view::npos) {
            std::string_view const tempStr = aprsPart.substr(tIndex + 1, 3);
            uint32_t temp = 0;
            auto result = std::from_chars(tempStr.data(), tempStr.data() + tempStr.size(), temp);
            if (result.ec == std::errc{}) {
                ognMessage.temperature = temp;
            }
        }
        // Decode humidity: look for 'h'
        auto const hIndex = aprsPart.find("h", underscoreIndex);
        if (hIndex != std::string_view::npos) {
            std::string_view const humStr = aprsPart.substr(hIndex + 1, 2);
            uint32_t hum = 0;
            auto result = std::from_chars(humStr.data(), humStr.data() + humStr.size(), hum);
            if (result.ec == std::errc{}) {
                ognMessage.humidity = hum;
            }
        }
        // Decode pressure: look for 'b'
        auto const bIndex = aprsPart.find("b", underscoreIndex);
        if (bIndex != std::string_view::npos) {
            std::string_view presStr = aprsPart.substr(bIndex + 1);
            // Find end of numeric part (space or end of string)
            auto endPos = presStr.find(' ');
            if (endPos != std::string_view::npos) {
                presStr = presStr.substr(0, endPos);
            }
            uint32_t pressureTenths = 0;
            auto result = std::from_chars(presStr.data(), presStr.data() + presStr.size(), pressureTenths);
            if (result.ec == std::errc{}) {
                ognMessage.pressure = pressureTenths / 10.0; // tenths of hectopascal
            }
        }
    } else {
        // Parse course, speed
        if (aprsPart.size() >= 34 && aprsPart[30] == '/') {
            std::string_view const courseStr = aprsPart.substr(27, 3);
            std::string_view const speedStr = aprsPart.substr(31, 3);
            int course = 0;
            int speed = 0;
            auto courseResult = std::from_chars(courseStr.data(), courseStr.data() + courseStr.size(), course);
            auto speedResult = std::from_chars(speedStr.data(), speedStr.data() + speedStr.size(), speed);
            if (courseResult.ec == std::errc{}) {
                ognMessage.course = static_cast<double>(course);
            }
            if (speedResult.ec == std::errc{}) {
                ognMessage.speed = static_cast<double>(speed);
            }
        }
        // Parse altitude
        auto const altitudeIndex = aprsPart.find("/A=");
        if (altitudeIndex != std::string_view::npos) {
            auto const altStart = altitudeIndex + 3;
            std::string_view const altitudeStr = aprsPart.substr(altStart, 6);
            int altitudeFeet = 0;
            auto result = std::from_chars(altitudeStr.data(), altitudeStr.data() + altitudeStr.size(), altitudeFeet);
            if (result.ec == std::errc{}) {
                // Convert feet to meters: 1 foot = 0.3048 meters
                ognMessage.altitude = altitudeFeet * 0.3048;
            }
        }
    }

    // Parse ognPart
    if (!ognPart.empty()) {
        auto it = ognPart.begin();
        while (it != ognPart.end()) {
            // Skip leading whitespace
            while (it != ognPart.end() && *it == ' ') {
                ++it;
            }
            if (it == ognPart.end()) break;
            
            // Find end of current item (next space or end)
            auto end = std::find(it, ognPart.end(), ' ');
            std::string_view const item(it, end - it);

            if (starts_with(item, "id")) {
                ognMessage.aircraftID = item.substr(2);
            } else if (starts_with(item, "t")) {
                std::string_view const tempStr = item.substr(1);
                int temp = 0;
                auto result = std::from_chars(tempStr.data(), tempStr.data() + tempStr.size(), temp);
                if (result.ec == std::errc{}) {
                    ognMessage.temperature = static_cast<uint32_t>(temp);
                }
            } else if (starts_with(item, "h")) {
                std::string_view const humStr = item.substr(1);
                uint32_t hum = 0;
                auto result = std::from_chars(humStr.data(), humStr.data() + humStr.size(), hum);
                if (result.ec == std::errc{}) {
                    ognMessage.humidity = hum;
                }
            } else if (starts_with(item, "b")) {
                std::string_view const presStr = item.substr(1);
                uint32_t pressureTenths = 0;
                auto result = std::from_chars(presStr.data(), presStr.data() + presStr.size(), pressureTenths);
                if (result.ec == std::errc{}) {
                    ognMessage.pressure = pressureTenths / 10.0; // Convert to hPa
                }
            } else if (ends_with(item, "fpm")) {
                // Convert feet per minute to meters per second: 1 fpm = 0.00508 m/s
                auto fpmIndex = item.find('f');
                if (fpmIndex != std::string_view::npos) {
                    std::string_view vspeedStr = item.substr(0, fpmIndex);
                    // Skip leading '+' if present (from_chars only handles '-' for signed types)
                    if (!vspeedStr.empty() && vspeedStr[0] == '+') {
                        vspeedStr = vspeedStr.substr(1);
                    }
                    int vspeedFpm = 0;
                    auto result = std::from_chars(vspeedStr.data(), vspeedStr.data() + vspeedStr.size(), vspeedFpm);
                    if (result.ec == std::errc{}) {
                        ognMessage.verticalSpeed = vspeedFpm * 0.00508;
                    }
                }
            } else if (ends_with(item, "rot")) {
                ognMessage.rotationRate = item;
            } else if (ends_with(item, "dB")) {
                ognMessage.signalStrength = item;
            } else if (ends_with(item, "e")) {
                ognMessage.errorCount = item;
            } else if (ends_with(item, "kHz")) {
                ognMessage.frequencyOffset = item;
            } else if (starts_with(item, "FL")) {
                ognMessage.flightlevel = item;
            } else if (starts_with(item, "A") && item.size() > 2 && item[2] == ':') {
                auto colonPos = item.find(':');
                if (colonPos != std::string_view::npos) {
                    ognMessage.flightnumber = item.substr(colonPos + 1);
                }
            } else if (starts_with(item, "Sq")) {
                ognMessage.squawk = item.substr(2);
            } else if (starts_with(item, "gps:")) {
                ognMessage.gpsInfo = item.substr(4);
            }

            it = (end != ognPart.end()) ? end + 1 : ognPart.end();
        }
    }

    // Parse aircraft type, address type, and address
    if (!ognMessage.aircraftID.empty()) {
        uint32_t hexcode = 0;
        auto result = std::from_chars(ognMessage.aircraftID.data(), 
                                      ognMessage.aircraftID.data() + ognMessage.aircraftID.size(), 
                                      hexcode, 16);
        if (result.ec == std::errc{}) {
            ognMessage.stealthMode = hexcode & 0x80000000;
            ognMessage.noTrackingFlag = hexcode & 0x40000000;
            uint32_t const aircraftCategory = ((hexcode >> 26) & 0xF);
            auto catIt = AircraftCategoryMap.find(aircraftCategory);
            if (catIt != AircraftCategoryMap.end()) {
                ognMessage.aircraftType = catIt->second;
            } else {
                ognMessage.aircraftType = Ogn::OgnAircraftType::unknown;
            }
            uint32_t const addressTypeValue = (hexcode >> 24) & 0x3;
            ognMessage.addressType = static_cast<OgnAddressType>(addressTypeValue);
            if (ognMessage.aircraftID.size() >= 8) {
                ognMessage.address = ognMessage.aircraftID.substr(2, 6);
            }
        }
    }

    #if OGNPARSER_DEBUG
    // Debug: Parsed Traffic Report
    #endif
}

void OgnParser::parseCommentMessage(OgnMessage& ognMessage)
{
    ognMessage.type = OgnMessageType::COMMENT;
}

void OgnParser::parseStatusMessage(OgnMessage &ognMessage,
                                                     const std::string_view /*header*/,
                                                     const std::string_view /*body*/)
{
    ognMessage.type = OgnMessageType::STATUS;
}

std::string OgnParser::formatPositionReport(const std::string_view callSign,
                                                          double latitude,
                                                          double longitude,
                                                          double altitude,
                                                          double course,
                                                          double speed,
                                                          OgnAircraftType aircraftType)
{
    // e.g. "ENR12345>APRS,TCPIP*: /074548h5111.32N/00102.04W'086/007/A=000607"

    // Static cache for the last lookup
    static OgnAircraftType lastAircraftType = OgnAircraftType::unknown;
    static std::string lastSymbol = "/z"; // Default to unknown symbol

    // Check if the cached value matches the current aircraft type
    if (lastAircraftType != aircraftType)
    {
        // Perform reverse lookup
        lastSymbol.clear();
        for (auto const& pair : AircraftTypeMap)
        {
            if (pair.second == aircraftType)
            {
                lastSymbol = pair.first;
                break;
            }
        }

        // Default to unknown symbol if no match is found
        if (lastSymbol.empty())
        {
            lastSymbol = "\\^";
        }

        // Update the cache
        lastAircraftType = aircraftType;
    }

    // Convert altitude from meters to feet: 1 meter = 3.28084 feet
    double const altitudeFeet = altitude * 3.28084;

    // Get current UTC time
    const std::time_t now = std::time(nullptr);
    const std::tm* utc_tm = std::gmtime(&now);
    char timeStr[7];
    std::snprintf(timeStr, sizeof(timeStr), "%02d%02d%02d", utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec);

    // Format course, speed, altitude with padding
    char courseStr[4], speedStr[4], altitudeStr[7];
    std::snprintf(courseStr, sizeof(courseStr), "%03d", static_cast<int>(course));
    std::snprintf(speedStr, sizeof(speedStr), "%03d", static_cast<int>(speed));
    std::snprintf(altitudeStr, sizeof(altitudeStr), "%06d", static_cast<int>(altitudeFeet));

    std::string result;
    result.reserve(100);
    result += callSign;
    result += ">APRS,TCPIP*: /";
    result += timeStr;
    result += "h";
    result += formatLatitude(latitude);
    result += lastSymbol[0];
    result += formatLongitude(longitude);
    result += lastSymbol[1];
    result += courseStr;
    result += "/";
    result += speedStr;
    result += "/A=";
    result += altitudeStr;
    result += "\n";

    return result;
}

std::string OgnParser::formatLatitude(double latitude)
{
    // e.g. "5111.32N"
    const char* direction = latitude >= 0 ? "N" : "S";
    latitude = std::abs(latitude);
    int const degrees = static_cast<int>(latitude);
    double const minutes = (latitude - degrees) * 60.0;
    
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d%05.2f%s", degrees, minutes, direction);
    return std::string(buffer);
}

std::string OgnParser::formatLongitude(double longitude)
{
    // e.g. "00102.04W"
    const char* direction = longitude >= 0 ? "E" : "W";
    longitude = std::abs(longitude);
    int const degrees = static_cast<int>(longitude);
    double const minutes = (longitude - degrees) * 60.0;
    
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%03d%05.2f%s", degrees, minutes, direction);
    return std::string(buffer);
}

std::string OgnParser::calculatePassword(std::string_view callSign)
{
    // APRS-IS passcode calculation: Sum of ASCII values of the first 6 characters of the call sign
    // e.g. "ENR12345" -> 379
    int sum = 0;
    for (size_t i = 0; i < callSign.length() && i < 6; ++i)
    {
        sum += static_cast<unsigned char>(callSign[i]);
    }
    return std::to_string(sum % 10000);
}

std::string OgnParser::formatLoginString(std::string_view callSign,
                                      double latitude,
                                      double longitude,
                                      unsigned int receiveRadius,
                                      std::string_view appName,
                                      std::string_view appVersion)
{
    // e.g. "user ENR12345 pass 379 vers Akaflieg-Freiburg Enroute 1.99 filter r/-48.0000/7.8512/99 t/o\n"
    std::string const password = calculatePassword(callSign);
    std::string const filter = formatFilter(latitude, longitude, receiveRadius);
    
    std::string result;
    result.reserve(200);
    result += "user ";
    result += callSign;
    result += " pass ";
    result += password;
    result += " vers ";
    result += appName;
    result += " ";
    result += appVersion;
    result += " ";
    result += filter;
    result += "\n";
    
    return result;
}

std::string OgnParser::formatFilter(double latitude, double longitude, unsigned int receiveRadius)
{
    // e.g. "filter r/-48.0000/7.8512/99 t/o"
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "filter r/%.4f/%.4f/%u t/o", latitude, longitude, receiveRadius);
    return std::string(buffer);
}

std::string OgnParser::formatFilterCommand(double latitude, double longitude, unsigned int receiveRadiusKm)
{
    // e.g. "# filter r/-48.0000/7.8512/99 t/o\n"
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "# filter r/%.4f/%.4f/%u t/o\n", latitude, longitude, receiveRadiusKm);
    return std::string(buffer);
}

} // namespace Ogn
