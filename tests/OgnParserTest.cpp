/***************************************************************************
 *   Unit tests for OgnParser - Qt-free version                           *
 ***************************************************************************/

#include "OgnParser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <locale>
#include <cassert>
#include <cstring>
#include <ctime>
#include <functional>

using namespace Ogn;

// Simple test macros
#define ASSERT_EQ(actual, expected) \
    if ((actual) != (expected)) { \
        std::cerr << "FAIL at " << __FILE__ << ":" << __LINE__ << ": Expected " << (expected) << ", got " << (actual) << std::endl; \
        return false; \
    }

#define ASSERT_DOUBLE_EQ(actual, expected) \
    if (std::abs((actual) - (expected)) > 0.0000001) { \
        std::cerr << "FAIL at " << __FILE__ << ":" << __LINE__ << ": Expected " << (expected) << ", got " << (actual) << std::endl; \
        return false; \
    }

#define ASSERT_GE(actual, min_val) \
    if ((actual) < (min_val)) { \
        std::cerr << "FAIL at " << __FILE__ << ":" << __LINE__ << ": Expected >= " << (min_val) << ", got " << (actual) << std::endl; \
        return false; \
    }

#define ASSERT_LE(actual, max_val) \
    if ((actual) > (max_val)) { \
        std::cerr << "FAIL at " << __FILE__ << ":" << __LINE__ << ": Expected <= " << (max_val) << ", got " << (actual) << std::endl; \
        return false; \
    }

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "FAIL at " << __FILE__ << ":" << __LINE__ << ": Condition is false" << std::endl; \
        return false; \
    }

#define ASSERT_NE(actual, unexpected) \
    if ((actual) == (unexpected)) { \
        std::cerr << "FAIL at " << __FILE__ << ":" << __LINE__ << ": Values should not be equal: " << (actual) << std::endl; \
        return false; \
    }

// Helper function to get current UTC time string
std::string getCurrentUtcTimeString() {
    std::time_t now = std::time(nullptr);
    std::tm* utc_tm = std::gmtime(&now);
    char buffer[7];
    std::snprintf(buffer, sizeof(buffer), "%02d%02d%02d", utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec);
    return buffer;
}

// Forward declarations
bool testFormatLoginString();
bool testFormatFilterCommand();
bool testFormatPositionReport();
bool testParseAprsisMessage_validTrafficReport1();
bool testParseAprsisMessage_validTrafficReport2();
bool testParseAprsisMessage_validTrafficReport3();
bool testParseAprsisMessage_Docu();
bool testParseAprsisMessage_invalidMessage();
bool testParseAprsisMessage_commentMessage();
bool testParseAprsisMessage_receiverStatusMessage();
bool testParseAprsisMessage_weatherReport();
bool testParseAprsisMessage_multipleMessages();
bool testPerformanceOfParseAprsisMessage();

// Test registry
struct Test {
    const char* name;
    std::function<bool()> func;
};

const Test tests[] = {
    {"testFormatLoginString", testFormatLoginString},
    {"testFormatFilterCommand", testFormatFilterCommand},
    {"testFormatPositionReport", testFormatPositionReport},
    {"testParseAprsisMessage_validTrafficReport1", testParseAprsisMessage_validTrafficReport1},
    {"testParseAprsisMessage_validTrafficReport2", testParseAprsisMessage_validTrafficReport2},
    {"testParseAprsisMessage_validTrafficReport3", testParseAprsisMessage_validTrafficReport3},
    {"testParseAprsisMessage_Docu", testParseAprsisMessage_Docu},
    {"testParseAprsisMessage_invalidMessage", testParseAprsisMessage_invalidMessage},
    {"testParseAprsisMessage_commentMessage", testParseAprsisMessage_commentMessage},
    {"testParseAprsisMessage_receiverStatusMessage", testParseAprsisMessage_receiverStatusMessage},
    {"testParseAprsisMessage_weatherReport", testParseAprsisMessage_weatherReport},
    {"testParseAprsisMessage_multipleMessages", testParseAprsisMessage_multipleMessages},
    {"testPerformanceOfParseAprsisMessage", testPerformanceOfParseAprsisMessage},
};

bool testFormatLoginString() {
    const std::string_view callsign = "ENR12345";
    const double latitude = -48.0;
    const double longitude = 7.85123456;
    const unsigned int receiveRadius = 99;
    const std::string_view swname = "Enroute";
    const std::string_view swversion = "1.99";

    const std::string loginString = OgnParser::formatLoginString(
        callsign, latitude, longitude, receiveRadius, swname, swversion);

    ASSERT_EQ(loginString, "user ENR12345 pass 379 vers Enroute 1.99 filter r/-48.0000/7.8512/99 t/o\n");
    return true;
}

bool testFormatFilterCommand() {
    const double latitude = -48.0;
    const double longitude = 7.85123456;
    const unsigned int receiveRadius = 99;
    std::string command = OgnParser::formatFilterCommand(latitude, longitude, receiveRadius);
    ASSERT_EQ(command, "# filter r/-48.0000/7.8512/99 t/o\n");
    return true;
}

bool testFormatPositionReport() {
    std::string_view callSign = "ENR12345";
    double latitude = 51.1886666667;
    double longitude = -1.034;
    double altitude = 185.0136;
    double course = 86.0;
    double speed = 7.0;

    // Test with unknown aircraft type
    Ogn::OgnAircraftType aircraftType = Ogn::OgnAircraftType::unknown;
    std::string currentTimeStamp = getCurrentUtcTimeString();
    std::string positionReport = OgnParser::formatPositionReport(
        callSign, latitude, longitude, altitude, course, speed, aircraftType);
    std::string expected = "ENR12345>APRS,TCPIP*: /" + currentTimeStamp + "h5111.32N/00102.04Wz086/007/A=000607\n";
    ASSERT_EQ(positionReport, expected);

    // Test with Glider
    aircraftType = Ogn::OgnAircraftType::Glider;
    currentTimeStamp = getCurrentUtcTimeString();
    positionReport = OgnParser::formatPositionReport(
        callSign, latitude, longitude, altitude, course, speed, aircraftType);
    expected = "ENR12345>APRS,TCPIP*: /" + currentTimeStamp + "h5111.32N/00102.04W'086/007/A=000607\n";
    ASSERT_EQ(positionReport, expected);

    return true;
}

bool testParseAprsisMessage_validTrafficReport1() {
    std::string sentence = "FLRDDE626>APRS,qAS,EGHL:/074548h5111.32N/00102.04W'086/007/A=000607 id0ADDE626 -019fpm +0.0rot 5.5dB 3e -4.3kHz";
    OgnMessage message;
    message.sentence = sentence;
    OgnParser::parseAprsisMessage(message);

    ASSERT_EQ(message.sentence, sentence);
    ASSERT_EQ(static_cast<int>(message.type), static_cast<int>(OgnMessageType::TRAFFIC_REPORT));
    ASSERT_DOUBLE_EQ(message.latitude, +51.1886666667);
    ASSERT_DOUBLE_EQ(message.longitude, -1.034);
    ASSERT_DOUBLE_EQ(message.altitude, 185.0136);
    ASSERT_EQ(static_cast<int>(message.symbol), static_cast<int>(OgnSymbol::GLIDER));
    ASSERT_DOUBLE_EQ(message.course, 86.0);
    ASSERT_DOUBLE_EQ(message.speed, 7.0);
    ASSERT_EQ(std::string(message.aircraftID), std::string("0ADDE626"));
    ASSERT_DOUBLE_EQ(message.verticalSpeed, -0.09652);
    ASSERT_EQ(std::string(message.rotationRate), std::string("+0.0rot"));
    ASSERT_EQ(std::string(message.signalStrength), std::string("5.5dB"));
    ASSERT_EQ(std::string(message.errorCount), std::string("3e"));
    ASSERT_EQ(std::string(message.frequencyOffset), std::string("-4.3kHz"));
    ASSERT_EQ(static_cast<int>(message.aircraftType), static_cast<int>(Ogn::OgnAircraftType::TowPlane));
    ASSERT_EQ(static_cast<int>(message.addressType), static_cast<int>(OgnAddressType::FLARM));
    ASSERT_EQ(std::string(message.address), std::string("DDE626"));
    ASSERT_EQ(message.stealthMode, false);
    ASSERT_EQ(message.noTrackingFlag, false);
    return true;
}

bool testParseAprsisMessage_validTrafficReport2() {
    std::string sentence = "ICA4D21C2>OGADSB,qAS,HLST:/001140h4741.90N/01104.20E^124/460/A=034868 !W91! id254D21C2 +128fpm FL350.00 A3:AXY547M Sq2244";
    OgnMessage message;
    message.sentence = sentence;
    OgnParser::parseAprsisMessage(message);

    ASSERT_EQ(message.sentence, sentence);
    ASSERT_EQ(static_cast<int>(message.type), static_cast<int>(OgnMessageType::TRAFFIC_REPORT));
    ASSERT_DOUBLE_EQ(message.latitude, +47.6984833333);
    ASSERT_GE(message.longitude, +11.0700166667-0.0001);
    ASSERT_LE(message.longitude, +11.0700166667+0.0001);
    ASSERT_DOUBLE_EQ(message.altitude, 10627.7664);
    ASSERT_DOUBLE_EQ(message.course, 124.0);
    ASSERT_DOUBLE_EQ(message.speed, 460.0);
    ASSERT_EQ(std::string(message.aircraftID), std::string("254D21C2"));
    return true;
}

bool testParseAprsisMessage_invalidMessage() {
    std::string sentence = "INVALID MESSAGE FORMAT";
    OgnMessage message;
    message.sentence = sentence;
    OgnParser::parseAprsisMessage(message);

    ASSERT_EQ(message.sentence, sentence);
    ASSERT_EQ(static_cast<int>(message.type), static_cast<int>(OgnMessageType::UNKNOWN));
    ASSERT_TRUE(std::isnan(message.latitude));
    ASSERT_TRUE(std::isnan(message.longitude));
    ASSERT_TRUE(std::isnan(message.altitude));
    return true;
}

bool testParseAprsisMessage_commentMessage() {
    std::string sentence = "# This is a comment";
    OgnMessage message;
    message.sentence = sentence;
    OgnParser::parseAprsisMessage(message);

    ASSERT_EQ(message.sentence, sentence);
    ASSERT_EQ(static_cast<int>(message.type), static_cast<int>(OgnMessageType::COMMENT));
    ASSERT_TRUE(std::isnan(message.latitude));
    return true;
}

bool testParseAprsisMessage_receiverStatusMessage() {
    std::string sentence = "FLRDDE626>APRS,qAS,EGHL:>Receiver Status Message";
    OgnMessage message;
    message.sentence = sentence;
    OgnParser::parseAprsisMessage(message);

    ASSERT_EQ(message.sentence, sentence);
    ASSERT_EQ(static_cast<int>(message.type), static_cast<int>(OgnMessageType::STATUS));
    return true;
}

bool testParseAprsisMessage_weatherReport() {
    std::string sentence = "FNT08075C>OGNFNT,qAS,Hoernle2:/222245h4803.92N/00800.93E_292/005g010t030h01b65526 5.2dB";
    OgnMessage message;
    message.sentence = sentence;
    OgnParser::parseAprsisMessage(message);

    ASSERT_EQ(message.sentence, sentence);
    ASSERT_EQ(static_cast<int>(message.type), static_cast<int>(OgnMessageType::WEATHER));
    ASSERT_EQ(static_cast<int>(message.symbol), static_cast<int>(OgnSymbol::WEATHERSTATION));
    ASSERT_DOUBLE_EQ(message.latitude, 48.0653333333);
    ASSERT_DOUBLE_EQ(message.longitude, 8.0155);
    ASSERT_TRUE(std::isnan(message.altitude));
    ASSERT_EQ(message.wind_direction, 292u);
    ASSERT_EQ(message.wind_speed, 5u);
    return true;
}

bool testParseAprsisMessage_multipleMessages() {
    std::vector<std::string> sentences = {
        "FLRDDE626>APRS,qAS,EGHL:/074548h5111.32N/00102.04W'086/007/A=000607 id0ADDE626 -019fpm +0.0rot 5.5dB 3e -4.3kHz",
        "LFNW>APRS,TCPIP*,qAC,GLIDERN5:/183804h4254.53NI00203.90E&/A=001000",
        "# aprsc 2.0.14-g28c5a6a 29 Jun 2014 07:46:15 GMT GLIDERN1 37.187.40.234:14580"
    };

    for (const auto& sentence : sentences) {
        OgnMessage message;
        message.sentence = sentence;
        OgnParser::parseAprsisMessage(message);
        ASSERT_EQ(message.sentence, sentence);
        ASSERT_NE(static_cast<int>(message.type), static_cast<int>(OgnMessageType::UNKNOWN));
    }
    return true;
}

bool testParseAprsisMessage_validTrafficReport3() {
    std::string sentence = "ICA4D21C2>OGADSB,qAS,HLST:/001140h4741.90N/01104.20E^/A=034868 !W91! id254D21C2 +128fpm FL350.00 A3:AXY547M Sq2244";
    OgnMessage message;
    message.sentence = sentence;
    OgnParser::parseAprsisMessage(message);

    ASSERT_EQ(message.sentence, sentence);
    ASSERT_EQ(static_cast<int>(message.type), static_cast<int>(OgnMessageType::TRAFFIC_REPORT));
    ASSERT_DOUBLE_EQ(message.latitude, +47.6984833333);
    ASSERT_GE(message.longitude, +11.0700166667-0.0001);
    ASSERT_LE(message.longitude, +11.0700166667+0.0001);
    ASSERT_DOUBLE_EQ(message.altitude, 10627.7664);
    ASSERT_DOUBLE_EQ(message.course, 0.0);
    ASSERT_DOUBLE_EQ(message.speed, 0.0);
    ASSERT_EQ(message.aircraftID, "254D21C2");
    ASSERT_EQ(static_cast<int>(message.symbol), static_cast<int>(OgnSymbol::JET));
    ASSERT_DOUBLE_EQ(message.verticalSpeed, +0.65024);
    ASSERT_EQ(message.rotationRate, "");
    ASSERT_EQ(message.signalStrength, "");
    ASSERT_EQ(message.errorCount, "");
    ASSERT_EQ(message.frequencyOffset, "");
    ASSERT_EQ(static_cast<int>(message.aircraftType), static_cast<int>(OgnAircraftType::Jet));
    ASSERT_EQ(static_cast<int>(message.addressType), static_cast<int>(OgnAddressType::ICAO));
    ASSERT_EQ(message.address, "4D21C2");
    ASSERT_EQ(message.stealthMode, false);
    ASSERT_EQ(message.noTrackingFlag, false);
    return true;
}

bool testParseAprsisMessage_Docu() {
    std::vector<std::string> sentences = {
        "FLRDDE626>APRS,qAS,EGHL:/074548h5111.32N/00102.04W'086/007/A=000607 id0ADDE626 -019fpm +0.0rot 5.5dB 3e -4.3kHz",
        "LFNW>APRS,TCPIP*,qAC,GLIDERN5:/183804h4254.53NI00203.90E&/A=001000",
        "LFNW>APRS,TCPIP*,qAC,GLIDERN5:>183804h v0.2.6.ARM CPU:0.7 RAM:505.3/889.7MB NTP:0.4ms/+7.7ppm +0.0C 0/0Acfts[1h] RF:+69-4.0ppm/+1.77dB/+3.5dB@10km[184484]/+11.2dB@10km[1/1]",
        "FLRDDE626>APRS,qAS,EGHL:/074548h5111.32N/00102.04W'086/007/A=000607 id0ADDE626 -019fpm +0.0rot 5.5dB 3e -4.3kHz",
        "FLRDDE626>APRS,qAS,EGHL:/074557h5111.32N/00102.01W'086/006/A=000607 id0ADDE626 +020fpm +0.3rot 5.8dB 4e -4.3kHz",
        "FLRDDE626>APRS,qAS,EGHL:/074559h5111.32N/00102.00W'090/006/A=000607 id0ADDE626 +020fpm -0.7rot 8.8dB 0e -4.3kHz",
        "FLRDDE626>APRS,qAS,EGHL:/074605h5111.32N/00101.98W'090/006/A=000607 id0ADDE626 +020fpm +0.0rot 5.5dB 1e -4.2kHz",
        "# aprsc 2.0.14-g28c5a6a 29 Jun 2014 07:46:15 GMT GLIDERN1 37.187.40.234:14580"
    };
    
    for (const auto& sentence : sentences) {
        OgnMessage message;
        message.sentence = sentence;
        OgnParser::parseAprsisMessage(message);
        ASSERT_EQ(message.sentence, sentence);
        ASSERT_NE(static_cast<int>(message.type), static_cast<int>(OgnMessageType::UNKNOWN));
    }
    return true;
}

bool testPerformanceOfParseAprsisMessage() {
    // Simple performance test - parse same message 10000 times
    std::string sentence = "FLRDDE626>APRS,qAS,EGHL:/074548h5111.32N/00102.04W'086/007/A=000607 id0ADDE626 -019fpm +0.0rot 5.5dB 3e -4.3kHz";
    OgnMessage message;
    
    for (int i = 0; i < 10000; i++) {
        message.reset();
        message.sentence = sentence;
        OgnParser::parseAprsisMessage(message);
    }
    
    // If we got here without crashing, the test passes
    ASSERT_EQ(static_cast<int>(message.type), static_cast<int>(OgnMessageType::TRAFFIC_REPORT));
    return true;
}

// Test runner
int main() {
    std::cout << "********* Start testing of OgnParser *********" << std::endl;
    
    // List of locales to test - ensures locale independence
    // This addresses issue #613 where German locale caused commas instead of decimal points
    const std::vector<const char*> locales = {
        "C",              // Standard C locale (decimal point)
        "en_US.UTF-8",    // US English (decimal point)
        "de_DE.UTF-8",    // German (comma as decimal separator)
        "fr_FR.UTF-8"     // French (comma as decimal separator)
    };
    
    int totalPassed = 0;
    int totalFailed = 0;
    
    for (const auto& test : tests) {
        std::cout << "\nRunning: " << test.name << std::endl;
        
        int passed = 0;
        int failed = 0;
        
        for (const char* localeName : locales) {
            // Try to set the locale
            try {
                std::locale::global(std::locale(localeName));
            } catch (const std::runtime_error&) {
                std::cerr << "\033[31mWarning: Locale " << localeName << " not available on this system. Skipping tests for this locale.\033[0m" << std::endl;
                continue;
            }
            
            // Skip performance test for non-C locales to keep test time reasonable
            if (std::string(test.name) == "testPerformanceOfParseAprsisMessage" && 
                std::string(localeName) != "C") {
                continue;
            }
            
            std::cout << "  [" << localeName << "] ";
            if (test.func()) {
                std::cout << "PASS" << std::endl;
                passed++;
            } else {
                std::cout << "FAIL" << std::endl;
                failed++;
            }
        }
        
        std::cout << "  " << test.name << ": " << passed << " passed, " << failed << " failed" << std::endl;
        totalPassed += passed;
        totalFailed += failed;
    }
    
    // Restore C locale
    std::locale::global(std::locale::classic());
    
    std::cout << "\nTotals: " << totalPassed << " passed, " << totalFailed << " failed" << std::endl;
    std::cout << "********* Finished testing of OgnParser *********" << std::endl;
    
    return totalFailed > 0 ? 1 : 0;
}
