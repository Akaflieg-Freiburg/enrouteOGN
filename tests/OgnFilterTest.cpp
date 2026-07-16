/***************************************************************************
 *   Unit tests for OgnFilter and OgnCache                               *
 ***************************************************************************/

#include "OgnFilter.h"
#include "OgnParser.h"
#include "test_helpers.h"
#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <functional>

using namespace Ogn;

// Forward declarations
bool testCacheUpdate_newEntry();
bool testCacheUpdate_newerData();
bool testCacheUpdate_olderData();
bool testCacheUpdate_unknownAircraftType();
bool testCacheUpdate_multipleAddressTypes();
bool testCacheUpdate_multipleAircraftTypes();
bool testCacheUpdate_ignoresNonTrafficReport();
bool testCacheUpdate_ignoresEmptyAircraftID();
bool testCacheClean_removeOldEntries();
bool testFilterBasic_acceptNewData();
bool testFilterBasic_rejectOldData();
bool testFilterBasic_rejectEqualTimestamp();
bool testFilterBasic_ignoresNonTrafficReport();
bool testFilterBasic_ignoresEmptyAircraftID();
bool testFilterBasic_rejectEpochTimestamp();
bool testFilterEnrichment_applyKnownAircraftType();
bool testFilterEnrichment_preserveUnknownType();

// Test registry
struct Test {
    const char* name;
    std::function<bool()> func;
};

const Test tests[] = {
    {"testCacheUpdate_newEntry", testCacheUpdate_newEntry},
    {"testCacheUpdate_newerData", testCacheUpdate_newerData},
    {"testCacheUpdate_olderData", testCacheUpdate_olderData},
    {"testCacheUpdate_unknownAircraftType", testCacheUpdate_unknownAircraftType},
    {"testCacheUpdate_multipleAddressTypes", testCacheUpdate_multipleAddressTypes},
    {"testCacheUpdate_multipleAircraftTypes", testCacheUpdate_multipleAircraftTypes},
    {"testCacheUpdate_ignoresNonTrafficReport", testCacheUpdate_ignoresNonTrafficReport},
    {"testCacheUpdate_ignoresEmptyAircraftID", testCacheUpdate_ignoresEmptyAircraftID},
    {"testCacheClean_removeOldEntries", testCacheClean_removeOldEntries},
    {"testFilterBasic_acceptNewData", testFilterBasic_acceptNewData},
    {"testFilterBasic_rejectOldData", testFilterBasic_rejectOldData},
    {"testFilterBasic_rejectEqualTimestamp", testFilterBasic_rejectEqualTimestamp},
    {"testFilterBasic_ignoresNonTrafficReport", testFilterBasic_ignoresNonTrafficReport},
    {"testFilterBasic_ignoresEmptyAircraftID", testFilterBasic_ignoresEmptyAircraftID},
    {"testFilterBasic_rejectEpochTimestamp", testFilterBasic_rejectEpochTimestamp},
    {"testFilterEnrichment_applyKnownAircraftType", testFilterEnrichment_applyKnownAircraftType},
    {"testFilterEnrichment_preserveUnknownType", testFilterEnrichment_preserveUnknownType},
};

// Cache stores a new aircraft entry correctly
bool testCacheUpdate_newEntry() {
    OgnCache cache;
    OgnMessage msg;
    msg.type = OgnMessageType::TRAFFIC_REPORT;
    msg.aircraftID = "id0ADDE626";
    msg.timestamp = std::chrono::system_clock::now();
    msg.aircraftType = OgnAircraftType::Glider;
    msg.addressType = OgnAddressType::FLARM;

    cache.update(msg);

    const auto& data = cache.data();
    ASSERT_EQ(data.size(), size_t(1));
    ASSERT_EQ(data.count("id0ADDE626"), size_t(1));

    const auto& entry = data.at("id0ADDE626");
    ASSERT_TRUE(entry.aircraftTypes.count(OgnAircraftType::Glider) > 0);
    ASSERT_TRUE(entry.addressTypes.count(OgnAddressType::FLARM) > 0);

    return true;
}

// Cache updates entry when newer timestamp is received
bool testCacheUpdate_newerData() {
    OgnCache cache;
    auto now = std::chrono::system_clock::now();

    // First update
    OgnMessage msg1;
    msg1.type = OgnMessageType::TRAFFIC_REPORT;
    msg1.aircraftID = "id0ADDE626";
    msg1.timestamp = now;
    msg1.aircraftType = OgnAircraftType::Glider;
    msg1.addressType = OgnAddressType::FLARM;
    cache.update(msg1);

    // Newer update
    OgnMessage msg2;
    msg2.type = OgnMessageType::TRAFFIC_REPORT;
    msg2.aircraftID = "id0ADDE626";
    msg2.timestamp = now + std::chrono::seconds(1);
    msg2.aircraftType = OgnAircraftType::TowPlane;
    msg2.addressType = OgnAddressType::ICAO;
    cache.update(msg2);

    const auto& data = cache.data();
    const auto& entry = data.at("id0ADDE626");
    ASSERT_TRUE(entry.aircraftTypes.count(OgnAircraftType::Glider) > 0);
    ASSERT_TRUE(entry.aircraftTypes.count(OgnAircraftType::TowPlane) > 0);
    ASSERT_TRUE(entry.addressTypes.count(OgnAddressType::FLARM) > 0);
    ASSERT_TRUE(entry.addressTypes.count(OgnAddressType::ICAO) > 0);

    return true;
}

// Cache ignores update with older timestamp
bool testCacheUpdate_olderData() {
    OgnCache cache;
    auto now = std::chrono::system_clock::now();

    // First update
    OgnMessage msg1;
    msg1.type = OgnMessageType::TRAFFIC_REPORT;
    msg1.aircraftID = "id0ADDE626";
    msg1.timestamp = now;
    msg1.aircraftType = OgnAircraftType::Glider;
    cache.update(msg1);

    // Older update (should be ignored)
    OgnMessage msg2;
    msg2.type = OgnMessageType::TRAFFIC_REPORT;
    msg2.aircraftID = "id0ADDE626";
    msg2.timestamp = now - std::chrono::seconds(1);
    msg2.aircraftType = OgnAircraftType::TowPlane;
    cache.update(msg2);

    const auto& data = cache.data();
    const auto& entry = data.at("id0ADDE626");
    // Older update ignored, only Glider should be in the set
    ASSERT_TRUE(entry.aircraftTypes.count(OgnAircraftType::Glider) > 0);
    ASSERT_EQ(entry.aircraftTypes.count(OgnAircraftType::TowPlane), size_t(0));

    return true;
}

// Cache preserves known aircraft type when unknown type is received
bool testCacheUpdate_unknownAircraftType() {
    OgnCache cache;
    auto now = std::chrono::system_clock::now();

    // First update with known type
    OgnMessage msg1;
    msg1.type = OgnMessageType::TRAFFIC_REPORT;
    msg1.aircraftID = "id0ADDE626";
    msg1.timestamp = now;
    msg1.aircraftType = OgnAircraftType::Glider;
    cache.update(msg1);

    // Newer update with unknown type (should not overwrite)
    OgnMessage msg2;
    msg2.type = OgnMessageType::TRAFFIC_REPORT;
    msg2.aircraftID = "id0ADDE626";
    msg2.timestamp = now + std::chrono::seconds(1);
    msg2.aircraftType = OgnAircraftType::unknown;
    cache.update(msg2);

    const auto& data = cache.data();
    const auto& entry = data.at("id0ADDE626");
    // Unknown type is not added; Glider should still be in the set
    ASSERT_TRUE(entry.aircraftTypes.count(OgnAircraftType::Glider) > 0);
    ASSERT_EQ(entry.aircraftTypes.size(), size_t(1));

    return true;
}

// Cache accumulates multiple address types in a set
bool testCacheUpdate_multipleAddressTypes() {
    OgnCache cache;
    auto now = std::chrono::system_clock::now();

    // First update with FLARM
    OgnMessage msg1;
    msg1.type = OgnMessageType::TRAFFIC_REPORT;
    msg1.aircraftID = "id0ADDE626";
    msg1.timestamp = now;
    msg1.addressType = OgnAddressType::FLARM;
    cache.update(msg1);

    // Second update with ICAO
    OgnMessage msg2;
    msg2.type = OgnMessageType::TRAFFIC_REPORT;
    msg2.aircraftID = "id0ADDE626";
    msg2.timestamp = now + std::chrono::seconds(1);
    msg2.addressType = OgnAddressType::ICAO;
    cache.update(msg2);

    const auto& data = cache.data();
    const auto& entry = data.at("id0ADDE626");
    ASSERT_EQ(entry.addressTypes.size(), size_t(2));
    ASSERT_TRUE(entry.addressTypes.count(OgnAddressType::FLARM) > 0);
    ASSERT_TRUE(entry.addressTypes.count(OgnAddressType::ICAO) > 0);

    return true;
}

// Cache cleanup removes entries older than 1 hour
bool testCacheClean_removeOldEntries() {
    OgnCache cache;
    auto now = std::chrono::system_clock::now();
    auto old_time = now - std::chrono::hours(2);
    auto recent_time = now - std::chrono::minutes(10);

    // Add old entry
    OgnMessage msg_old;
    msg_old.type = OgnMessageType::TRAFFIC_REPORT;
    msg_old.aircraftID = "id0ADDE626";
    msg_old.timestamp = old_time;
    msg_old.aircraftType = OgnAircraftType::Glider;
    cache.update(msg_old);

    // Add recent entry
    OgnMessage msg_recent;
    msg_recent.type = OgnMessageType::TRAFFIC_REPORT;
    msg_recent.aircraftID = "id0BBBBBB";
    msg_recent.timestamp = recent_time;
    msg_recent.aircraftType = OgnAircraftType::TowPlane;
    cache.update(msg_recent);

    ASSERT_EQ(cache.data().size(), size_t(2));

    cache.clean();

    // Old entry should be removed, recent entry should remain
    ASSERT_EQ(cache.data().size(), size_t(1));
    ASSERT_EQ(cache.data().count("id0BBBBBB"), size_t(1));
    ASSERT_EQ(cache.data().count("id0ADDE626"), size_t(0));

    return true;
}

// Cache accumulates multiple aircraft types in a set
bool testCacheUpdate_multipleAircraftTypes() {
    OgnCache cache;
    auto now = std::chrono::system_clock::now();

    OgnMessage msg1;
    msg1.type = OgnMessageType::TRAFFIC_REPORT;
    msg1.aircraftID = "id0ADDE626";
    msg1.timestamp = now;
    msg1.aircraftType = OgnAircraftType::Glider;
    cache.update(msg1);

    OgnMessage msg2;
    msg2.type = OgnMessageType::TRAFFIC_REPORT;
    msg2.aircraftID = "id0ADDE626";
    msg2.timestamp = now + std::chrono::seconds(1);
    msg2.aircraftType = OgnAircraftType::TowPlane;
    cache.update(msg2);

    const auto& entry = cache.data().at("id0ADDE626");
    ASSERT_EQ(entry.aircraftTypes.size(), size_t(2));
    ASSERT_TRUE(entry.aircraftTypes.count(OgnAircraftType::Glider) > 0);
    ASSERT_TRUE(entry.aircraftTypes.count(OgnAircraftType::TowPlane) > 0);

    return true;
}

// Cache ignores non-TRAFFIC_REPORT messages
bool testCacheUpdate_ignoresNonTrafficReport() {
    OgnCache cache;
    OgnMessage msg;
    msg.type = OgnMessageType::COMMENT;
    msg.aircraftID = "id0ADDE626";
    msg.timestamp = std::chrono::system_clock::now();
    cache.update(msg);

    ASSERT_EQ(cache.data().size(), size_t(0));

    return true;
}

// Cache ignores messages with empty aircraftID
bool testCacheUpdate_ignoresEmptyAircraftID() {
    OgnCache cache;
    OgnMessage msg;
    msg.type = OgnMessageType::TRAFFIC_REPORT;
    msg.aircraftID = "";
    msg.timestamp = std::chrono::system_clock::now();
    cache.update(msg);

    ASSERT_EQ(cache.data().size(), size_t(0));

    return true;
}

// Filter accepts and caches new traffic report messages
bool testFilterBasic_acceptNewData() {
    OgnFilter filter;
    OgnMessage msg;
    msg.type = OgnMessageType::TRAFFIC_REPORT;
    msg.aircraftID = "id0ADDE626";
    msg.timestamp = std::chrono::system_clock::now();
    msg.aircraftType = OgnAircraftType::Glider;

    bool result = filter.filter(msg);
    ASSERT_TRUE(result);

    return true;
}

// Filter rejects messages with timestamp older than cached data
bool testFilterBasic_rejectOldData() {
    OgnFilter filter;
    auto now = std::chrono::system_clock::now();

    // First message
    OgnMessage msg1;
    msg1.type = OgnMessageType::TRAFFIC_REPORT;
    msg1.aircraftID = "id0ADDE626";
    msg1.timestamp = now;
    msg1.aircraftType = OgnAircraftType::Glider;
    filter.filter(msg1);

    // Older message (should be rejected)
    OgnMessage msg2;
    msg2.type = OgnMessageType::TRAFFIC_REPORT;
    msg2.aircraftID = "id0ADDE626";
    msg2.timestamp = now - std::chrono::seconds(1);
    msg2.aircraftType = OgnAircraftType::TowPlane;
    
    bool result = filter.filter(msg2);
    ASSERT_FALSE(result);

    return true;
}

// Filter rejects message with the same timestamp as cached (not strictly newer)
bool testFilterBasic_rejectEqualTimestamp() {
    OgnFilter filter;
    auto now = std::chrono::system_clock::now();

    OgnMessage msg1;
    msg1.type = OgnMessageType::TRAFFIC_REPORT;
    msg1.aircraftID = "id0ADDE626";
    msg1.timestamp = now;
    filter.filter(msg1);

    OgnMessage msg2;
    msg2.type = OgnMessageType::TRAFFIC_REPORT;
    msg2.aircraftID = "id0ADDE626";
    msg2.timestamp = now; // same timestamp

    bool result = filter.filter(msg2);
    ASSERT_FALSE(result);

    return true;
}

// Filter returns false for non-TRAFFIC_REPORT messages
bool testFilterBasic_ignoresNonTrafficReport() {
    OgnFilter filter;
    OgnMessage msg;
    msg.type = OgnMessageType::COMMENT;
    msg.aircraftID = "id0ADDE626";
    msg.timestamp = std::chrono::system_clock::now();

    bool result = filter.filter(msg);
    ASSERT_FALSE(result);
    ASSERT_EQ(filter.cache().data().size(), size_t(0));

    return true;
}

// Filter returns false for messages with empty aircraftID
bool testFilterBasic_ignoresEmptyAircraftID() {
    OgnFilter filter;
    OgnMessage msg;
    msg.type = OgnMessageType::TRAFFIC_REPORT;
    msg.aircraftID = "";
    msg.timestamp = std::chrono::system_clock::now();

    bool result = filter.filter(msg);
    ASSERT_FALSE(result);
    ASSERT_EQ(filter.cache().data().size(), size_t(0));

    return true;
}

// Filter returns false for messages with epoch (invalid) timestamp
bool testFilterBasic_rejectEpochTimestamp() {
    OgnFilter filter;
    OgnMessage msg;
    msg.type = OgnMessageType::TRAFFIC_REPORT;
    msg.aircraftID = "id0ADDE626";
    msg.timestamp = std::chrono::system_clock::time_point{}; // epoch = invalid

    bool result = filter.filter(msg);
    ASSERT_FALSE(result);
    ASSERT_EQ(filter.cache().data().size(), size_t(0));

    return true;
}

// Filter enriches message with cached aircraft type when message type is unknown
bool testFilterEnrichment_applyKnownAircraftType() {
    OgnFilter filter;
    auto now = std::chrono::system_clock::now();

    // First message with known type
    OgnMessage msg1;
    msg1.type = OgnMessageType::TRAFFIC_REPORT;
    msg1.aircraftID = "id0ADDE626";
    msg1.timestamp = now;
    msg1.aircraftType = OgnAircraftType::Glider;
    filter.filter(msg1);

    // Second message with unknown type
    OgnMessage msg2;
    msg2.type = OgnMessageType::TRAFFIC_REPORT;
    msg2.aircraftID = "id0ADDE626";
    msg2.timestamp = now + std::chrono::seconds(1);
    msg2.aircraftType = OgnAircraftType::unknown;
    
    bool result = filter.filter(msg2);
    ASSERT_TRUE(result);
    // Message should be enriched with cached aircraft type
    ASSERT_EQ(static_cast<int>(msg2.aircraftType), static_cast<int>(OgnAircraftType::Glider));

    return true;
}

// Filter preserves unknown type when cache has no known type
bool testFilterEnrichment_preserveUnknownType() {
    OgnFilter filter;
    auto now = std::chrono::system_clock::now();

    // First message with unknown type
    OgnMessage msg1;
    msg1.type = OgnMessageType::TRAFFIC_REPORT;
    msg1.aircraftID = "id0ADDE626";
    msg1.timestamp = now;
    msg1.aircraftType = OgnAircraftType::unknown;
    filter.filter(msg1);

    // Second message with unknown type (cache also unknown, so stays unknown)
    OgnMessage msg2;
    msg2.type = OgnMessageType::TRAFFIC_REPORT;
    msg2.aircraftID = "id0ADDE626";
    msg2.timestamp = now + std::chrono::seconds(1);
    msg2.aircraftType = OgnAircraftType::unknown;
    
    bool result = filter.filter(msg2);
    ASSERT_TRUE(result);
    // Message type should remain unknown
    ASSERT_EQ(static_cast<int>(msg2.aircraftType), static_cast<int>(OgnAircraftType::unknown));

    return true;
}

// Test runner
int main() {
    std::cout << "********* Start testing of OgnFilter *********" << std::endl;

    int totalPassed = 0;
    int totalFailed = 0;

    for (const auto& test : tests) {
        std::cout << "Running: " << test.name << " ... ";
        if (test.func()) {
            std::cout << "PASS" << std::endl;
            totalPassed++;
        } else {
            std::cout << "FAIL" << std::endl;
            totalFailed++;
        }
    }

    std::cout << "\nTotals: " << totalPassed << " passed, " << totalFailed << " failed" << std::endl;
    std::cout << "********* Finished testing of OgnFilter *********" << std::endl;

    return totalFailed > 0 ? 1 : 0;
}
