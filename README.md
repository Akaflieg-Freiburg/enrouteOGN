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

## dumpOGN Utility

The `dumpOGN` command-line tool connects to the OGN APRS-IS network and outputs live aircraft traffic data. It can output data in two formats:

- **Raw OGN APRS format** (default): Original APRS-IS sentences as received from the network
- **SBS-1 BaseStation format** (--sbs1): Compatible with dump1090, tar1090, VirtualRadarServer, and other aviation tools

### Usage

```bash
dumpOGN --lat 48.3537 --lon 11.7860 [OPTIONS]
```

### Options

- `--lat LATITUDE` - Latitude for position filter (required)
- `--lon LONGITUDE` - Longitude for position filter (required)
- `--radius KM` - Radius for position filter in km (default: 50)
- `--sbs1` - Output in SBS-1 BaseStation format instead of raw APRS
- `-s, --server HOST` - OGN APRS-IS server (default: aprs.glidernet.org)
- `-p, --port PORT` - Server port (default: 14580)
- `-h, --help` - Show help message
- `-v, --version` - Show version

### Example: Raw OGN APRS Output

```bash
dumpOGN --lat 48.3537 --lon 11.7860 --radius 100
```

Output:
```
ICA3C6590>OGADSB,qAS,EDMQ:/103649h4818.00N/01200.69E^164/253/A=007829 !W65! id253C6590 +3712fpm FL081.58 A3:ABC123
ICA440066>OGFLR,qAS,ETSI8:/103641h4840.94N\01131.25E^098/080/A=002179 !W56! id21440066 -138fpm -2.1rot 14.8dB -7.0kHz gps2x3
ICA3C6451>OGADSB,qAS,GroundRX:/103650h4821.18N/01156.06E^097/248/A=005331 !W08! id253C6451 +1408fpm FL055.79 A3:XYZ789
```

Each line contains:
- Sender callsign (e.g., ICA3C6590 = ICAO address-based aircraft)
- Destination (OGADSB for ADS-B aircraft, OGFLR for FLARM)
- Receiving station (qAS,EDMQ = ground station)
- Position (latitude/longitude in APRS format: 4818.00N = 48°18.00'N)
- Course/Speed (^164/253 = 164° at 253 knots)
- Altitude (A=007829 = 7829 feet MSL)
- Extended data: ID, climb rate (+3712fpm), flight level (FL081.58), callsign (A3:ABC123), and for FLARM: turn rate, signal strength, errors, frequency offset, GPS quality

### Example: SBS-1 BaseStation Output

```bash
dumpOGN --lat 48.3537 --lon 11.7860 --radius 100 --sbs1
```

Output:
```
MSG,8,111,11111,3C65D7,111111,2026/02/01,10:37:05.000,2026/02/01,10:37:05.000,ABC123,3465,178,84,48.346650,11.832450,1663,,,,,
MSG,8,111,11111,3C4DC4,111111,2026/02/01,10:37:05.000,2026/02/01,10:37:05.000,XYZ789,5062,244,48,48.423817,11.923667,1279,,,,,
MSG,8,111,11111,4BCE0D,111111,2026/02/01,10:37:05.000,2026/02/01,10:37:05.000,DEF456,13803,349,102,48.270817,12.381767,3071,,,,,
```

SBS-1 format fields (comma-separated):
1. MSG,8 = Message type (all data)
2. Session/Aircraft IDs (111,11111)
3. ICAO hex address (3C65D7)
4. Flight ID (111111)
5. Date/time generated and logged (2026/02/01,10:37:05.000)
6. Callsign (ABC123)
7. Altitude (feet) - 3465
8. Ground speed (knots) - 178
9. Track (degrees) - 84
10. Latitude/Longitude (decimal degrees) - 48.346650,11.832450
11. Vertical rate (feet/min) - 1663

## License

GPL v3 - See LICENSE file for details.

