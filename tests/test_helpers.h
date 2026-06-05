/***************************************************************************
 *   Test helpers - simple testing framework without Qt dependencies      *
 ***************************************************************************/

#pragma once

#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>

// Simple test macros for standalone testing
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

#define ASSERT_FALSE(condition) \
    if ((condition)) { \
        std::cerr << "FAIL at " << __FILE__ << ":" << __LINE__ << ": Condition is true" << std::endl; \
        return false; \
    }

#define ASSERT_NE(actual, unexpected) \
    if ((actual) == (unexpected)) { \
        std::cerr << "FAIL at " << __FILE__ << ":" << __LINE__ << ": Values should not be equal: " << (actual) << std::endl; \
        return false; \
    }

