/***************************************************************************
 *   Test helpers - simple testing framework without Qt dependencies      *
 ***************************************************************************/

#pragma once

#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>

namespace TestHelpers {

inline int testsPassed = 0;
inline int testsFailed = 0;
inline int testsSkipped = 0;
inline std::string currentTest;

#define TEST_CASE(name) \
    void name(); \
    struct name##_register { \
        name##_register() { TestHelpers::registerTest(#name, name); } \
    } name##_instance; \
    void name()

inline void fail(const std::string& message, const char* file, int line) {
    std::cerr << "FAIL: " << currentTest << " - " << message 
              << " (" << file << ":" << line << ")" << std::endl;
    testsFailed++;
    exit(1);
}

inline void pass(const std::string& message = "") {
    if (!message.empty()) {
        std::cout << "PASS: " << currentTest << " - " << message << std::endl;
    }
}

template<typename T>
inline void assertEqual(const T& actual, const T& expected, const char* file, int line) {
    if (actual != expected) {
        std::cerr << "FAIL: " << currentTest << " - Expected: " << expected 
                  << ", Got: " << actual << " (" << file << ":" << line << ")" << std::endl;
        testsFailed++;
        exit(1);
    }
}

inline void assertEqual(const std::string& actual, const std::string& expected, const char* file, int line) {
    if (actual != expected) {
        std::cerr << "FAIL: " << currentTest << " - Expected: \"" << expected 
                  << "\", Got: \"" << actual << "\" (" << file << ":" << line << ")" << std::endl;
        testsFailed++;
        exit(1);
    }
}

inline void assertEqual(double actual, double expected, const char* file, int line) {
    if (std::abs(actual - expected) > 0.0000001) {
        std::cerr << "FAIL: " << currentTest << " - Expected: " << expected 
                  << ", Got: " << actual << " (" << file << ":" << line << ")" << std::endl;
        testsFailed++;
        exit(1);
    }
}

inline void assertGreaterOrEqual(double actual, double expected, const char* file, int line) {
    if (actual < expected) {
        std::cerr << "FAIL: " << currentTest << " - Expected >= " << expected 
                  << ", Got: " << actual << " (" << file << ":" << line << ")" << std::endl;
        testsFailed++;
        exit(1);
    }
}

inline void assertLessOrEqual(double actual, double expected, const char* file, int line) {
    if (actual > expected) {
        std::cerr << "FAIL: " << currentTest << " - Expected <= " << expected 
                  << ", Got: " << actual << " (" << file << ":" << line << ")" << std::endl;
        testsFailed++;
        exit(1);
    }
}

inline void assertNotEqual(int actual, int expected, const char* file, int line) {
    if (actual == expected) {
        std::cerr << "FAIL: " << currentTest << " - Values should not be equal: " << actual 
                  << " (" << file << ":" << line << ")" << std::endl;
        testsFailed++;
        exit(1);
    }
}

inline void assertTrue(bool condition, const char* file, int line) {
    if (!condition) {
        std::cerr << "FAIL: " << currentTest << " - Condition is false"
                  << " (" << file << ":" << line << ")" << std::endl;
        testsFailed++;
        exit(1);
    }
}

inline void skip(const std::string& reason) {
    std::cout << "SKIP: " << currentTest << " - " << reason << std::endl;
    testsSkipped++;
    exit(0);
}

using TestFunc = void(*)();
inline std::vector<std::pair<std::string, TestFunc>>& getTests() {
    static std::vector<std::pair<std::string, TestFunc>> tests;
    return tests;
}

inline void registerTest(const std::string& name, TestFunc func) {
    getTests().push_back({name, func});
}

inline int runAllTests() {
    std::cout << "********* Start testing *********" << std::endl;
    
    for (const auto& [name, func] : getTests()) {
        currentTest = name;
        std::cout << "Running: " << name << std::endl;
        
        int result = system((std::string("./") + name).c_str());
        if (result == 0) {
            std::cout << "PASS: " << name << std::endl;
            testsPassed++;
        } else if (WEXITSTATUS(result) == 77) {  // Skip code
            testsSkipped++;
        } else {
            testsFailed++;
        }
    }
    
    std::cout << "Totals: " << testsPassed << " passed, " 
              << testsFailed << " failed, " 
              << testsSkipped << " skipped" << std::endl;
    std::cout << "********* Finished testing *********" << std::endl;
    
    return testsFailed > 0 ? 1 : 0;
}

} // namespace TestHelpers

#define ASSERT_EQ(actual, expected) TestHelpers::assertEqual(actual, expected, __FILE__, __LINE__)
#define ASSERT_NE(actual, expected) TestHelpers::assertNotEqual(actual, expected, __FILE__, __LINE__)
#define ASSERT_GE(actual, expected) TestHelpers::assertGreaterOrEqual(actual, expected, __FILE__, __LINE__)
#define ASSERT_LE(actual, expected) TestHelpers::assertLessOrEqual(actual, expected, __FILE__, __LINE__)
#define ASSERT_TRUE(condition) TestHelpers::assertTrue(condition, __FILE__, __LINE__)
#define SKIP_TEST(reason) TestHelpers::skip(reason)
#define FAIL(message) TestHelpers::fail(message, __FILE__, __LINE__)
