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

#include <iostream>
#include <string>
#include <cstring>
#include <random>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "OgnParser.h"
#include "OgnFormatter.h"
#include "SBS1Formatter.h"

// Connect to TCP server
int connectToServer(const std::string& hostname, int port) {
    // Resolve hostname
    const struct hostent* const host = gethostbyname(hostname.c_str());
    if (host == nullptr) {
        std::cerr << "Error: Could not resolve hostname: " << hostname << std::endl;
        return -1;
    }

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0); // NOLINT(misc-const-correctness) - modified by connect()
    if (sock < 0) {
        std::cerr << "Error: Could not create socket: " << std::strerror(errno) << std::endl;
        return -1;
    }

    // Setup server address
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    std::memcpy(&server_addr.sin_addr.s_addr, host->h_addr, host->h_length);

    // Connect
    std::cerr << "Connecting to " << hostname << ":" << port << "..." << std::endl;
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Error: Could not connect: " << std::strerror(errno) << std::endl;
        close(sock);
        return -1;
    }

    std::cerr << "Connected to OGN APRS-IS server" << std::endl;
    return sock;
}

// Read line from socket (buffered)
bool readLine(int sock, std::string& line, std::string& buffer) {
    line.clear();
    
    while (true) {
        // Check if we have a complete line in buffer
        const size_t pos = buffer.find('\n');
        if (pos != std::string::npos) {
            line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            // Remove trailing \r if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return true;
        }
        
        // Read more data
        char chunk[1024];
        const ssize_t bytes = recv(sock, chunk, sizeof(chunk), 0);
        if (bytes <= 0) {
            return false; // Connection closed or error
        }
        buffer.append(chunk, bytes);
    }
}

void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " [OPTIONS]\n"
              << "\nOGN APRS-IS data converter\n"
              << "\nOptions:\n"
              << "  -h, --help              Show this help message\n"
              << "  -v, --version           Show version\n"
              << "  --sbs1                  Output in SBS-1 BaseStation format (dump1090-compatible)\n"
              << "  -s, --server HOST       OGN APRS-IS server (default: aprs.glidernet.org)\n"
              << "  -p, --port PORT         Server port (default: 14580)\n"
              << "  --lat LATITUDE          Latitude for position filter (required)\n"
              << "  --lon LONGITUDE         Longitude for position filter (required)\n"
              << "  --radius KM             Radius for position filter in km (default: 50)\n"
              << "\nExample:\n"
              << "  " << progName << " --lat 48.3537 --lon 11.7860\n";
}

int main(int argc, char *argv[])
{
    // Default values
    bool sbs1Mode = false;
    std::string server = "aprs.glidernet.org";
    int port = 14580;
    double latitude = 0.0;
    double longitude = 0.0;
    int radius = 50;
    bool hasLat = false;
    bool hasLon = false;

    // Parse command line
    static struct option long_options[] = {
        {"help",    no_argument,       nullptr, 'h'},
        {"version", no_argument,       nullptr, 'v'},
        {"sbs1",    no_argument,       nullptr, '1'},
        {"server",  required_argument, nullptr, 's'},
        {"port",    required_argument, nullptr, 'p'},
        {"lat",     required_argument, nullptr, 'a'},
        {"lon",     required_argument, nullptr, 'o'},
        {"radius",  required_argument, nullptr, 'r'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "hvs:p:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 'v':
                std::cout << "dumpOGN version 1.0" << std::endl;
                return 0;
            case '1':
                sbs1Mode = true;
                break;
            case 's':
                server = optarg;
                break;
            case 'p':
                port = std::stoi(optarg);
                break;
            case 'a':
                latitude = std::stod(optarg);
                hasLat = true;
                break;
            case 'o':
                longitude = std::stod(optarg);
                hasLon = true;
                break;
            case 'r':
                radius = std::stoi(optarg);
                break;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }

    // Validate required options
    if (!hasLat || !hasLon) {
        std::cerr << "Error: --lat and --lon options are required\n" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // Connect to server
    const int sock = connectToServer(server, port);
    if (sock < 0) {
        return 1;
    }

    // Generate random callsign
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    const std::string callSign = "DMP" + std::to_string(dis(gen));

    // Send login with filter
    const std::string loginString = Ogn::OgnParser::formatLoginString(
        callSign,
        latitude,
        longitude,
        radius,
        "dumpOGN",
        "1.0"
    );
    
    if (send(sock, loginString.c_str(), loginString.length(), 0) < 0) {
        std::cerr << "Error: Could not send login: " << std::strerror(errno) << std::endl;
        close(sock);
        return 1;
    }

    std::cerr << "Logged in as " << callSign 
              << " (filter: " << latitude << "," << longitude 
              << " radius " << radius << "km)" << std::endl;

    // Create formatter based on mode
    OgnFormatter ognFormatter;
    SBS1Formatter sbs1Formatter;
    OutputFormatter* formatter = sbs1Mode ? static_cast<OutputFormatter*>(&sbs1Formatter) 
                                          : static_cast<OutputFormatter*>(&ognFormatter);

    // Read and process messages
    std::string buffer;
    std::string line;
    while (readLine(sock, line, buffer)) {
        // Parse OGN message
        Ogn::OgnMessage message;
        message.sentence = line;
        Ogn::OgnParser::parseAprsisMessage(message);
        
        // Format using the configured formatter
        const std::string output = formatter->format(message);
        if (!output.empty()) {
            std::cout << output << std::endl;
        }
    }

    std::cerr << "Disconnected from server" << std::endl;
    close(sock);
    return 0;
}
