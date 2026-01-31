# enrouteOGN

A lightweight parser library for the [Open Glider Network](https://www.glidernet.org/).

## Library Structure

The library is organized as follows:

- **lib/**: Core library (Qt-free, C++17 only)
  - `OgnParser.h` - Public API
  - `OgnParser.cpp` - Implementation
- **tests/**: Unit tests (uses CTest)
- **dumpOGN/**: Utility for dumping OGN data 

## Building

The entire project is Qt-free and requires only C++17 and standard POSIX libraries.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
ctest --output-on-failure
```

Or use the convenience script:

```bash
./build-and-test.sh
```

If Qt is installed, the dumpOGN utility will also be built:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/qt
cmake --build .
ctest --output-on-failure
```

The build system will automatically detect Qt and build the dumpOGN utility if available.

## Usage

```cpp
#include "OgnParser.h"

// Connect to OGN APRS-IS server (using POSIX sockets)
int sock = connectToServer("aprs.glidernet.org", 14580);

// Send login with position filter
std::string login = Ogn::OgnParser::formatLoginString("MYCALL", 48.0, 11.0, 100, "MyApp", "1.0");
send(sock, login.c_str(), login.length(), 0);

// Read and parse messages
std::string buffer, line;
while (readLine(sock, line, buffer)) {
    Ogn::OgnMessage msg;
    msg.sentence = line;
    Ogn::OgnParser::parseAprsisMessage(msg);
    
    if (msg.type == Ogn::OgnMessageType::TRAFFIC_REPORT) {
        std::cout << "Latitude: " << msg.latitude << std::endl;
        std::cout << "Longitude: " << msg.longitude << std::endl;
        std::cout << "Altitude: " << msg.altitude << " m" << std::endl;
    }
}
```

See [dumpOGN/dumpOGN.cpp](dumpOGN/dumpOGN.cpp) for a complete working example.

## License

GPL v3 - See LICENSE file for details.

