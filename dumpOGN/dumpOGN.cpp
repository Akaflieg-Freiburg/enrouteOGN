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

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTcpSocket>
#include <QTextStream>
#include <QTimer>
#include <QRandomGenerator>
#include <QDebug>
#include <QDateTime>
#include <iostream>
#include "OgnFormatter.h"
#include "SBS1Formatter.h"

class OgnDumper : public QObject
{
    Q_OBJECT

public:
    explicit OgnDumper(OutputFormatter* formatter, const QString& server, quint16 port, 
                       double latitude, double longitude, int radius, QObject* parent = nullptr)
        : QObject(parent)
        , m_formatter(formatter)
        , m_server(server)
        , m_port(port)
        , m_latitude(latitude)
        , m_longitude(longitude)
        , m_radius(radius)
    {
        m_textStream.setEncoding(QStringConverter::Latin1);
        
        connect(&m_socket, &QTcpSocket::connected, this, &OgnDumper::onConnected);
        connect(&m_socket, &QTcpSocket::disconnected, this, &OgnDumper::onDisconnected);
        connect(&m_socket, &QTcpSocket::errorOccurred, this, &OgnDumper::onError);
        connect(&m_socket, &QTcpSocket::readyRead, this, &OgnDumper::onReadyRead);
    }

    void start()
    {
        std::cerr << "Connecting to " << m_server.toStdString() 
                  << ":" << m_port << "..." << std::endl;
        m_socket.connectToHost(m_server, m_port);
    }

private slots:
    void onConnected()
    {
        std::cerr << "Connected to OGN APRS-IS server" << std::endl;
        
        // Generate random callsign
        QString callSign = QString("DMP%1").arg(QRandomGenerator::global()->bounded(100000, 999999));
        
        // Calculate simple password (sum of callsign characters mod 10000)
        int sum = 0;
        for (int i = 0; i < callSign.length() && i < 6; ++i) {
            sum += callSign.at(i).unicode();
        }
        
        // Send login with filter
        m_textStream.setDevice(&m_socket);
        m_textStream << QString("user %1 pass %2 vers dumpOGN 1.0 filter r/%3/%4/%5 t/o\n")
                            .arg(callSign)
                            .arg(sum % 10000)
                            .arg(m_latitude, 0, 'f', 4)
                            .arg(m_longitude, 0, 'f', 4)
                            .arg(m_radius);
        m_textStream.flush();
        
        std::cerr << "Logged in as " << callSign.toStdString() 
                  << " (filter: " << m_latitude << "," << m_longitude 
                  << " radius " << m_radius << "km)" << std::endl;
    }

    void onDisconnected()
    {
        std::cerr << "Disconnected from server" << std::endl;
        QCoreApplication::exit(1);
    }

    void onError(QAbstractSocket::SocketError error)
    {
        std::cerr << "Socket error: " << m_socket.errorString().toStdString() << std::endl;
        QCoreApplication::exit(1);
    }

    void onReadyRead()
    {
        QString line;
        while (m_textStream.readLineInto(&line)) {
            // Parse OGN message
            Traffic::Ogn::OgnMessage message;
            message.sentence = line;
            Traffic::Ogn::TrafficDataSource_OgnParser::parseAprsisMessage(message);
            
            // Format using the configured formatter
            QString output = m_formatter->format(message);
            if (!output.isEmpty()) {
                std::cout << output.toStdString() << std::endl;
            }
        }
    }

private:
    OutputFormatter* m_formatter;
    QString m_server;
    quint16 m_port;
    double m_latitude;
    double m_longitude;
    int m_radius;
    QTcpSocket m_socket;
    QTextStream m_textStream;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("dumpOGN");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("OGN APRS-IS data converter");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption sbs1Option(QStringList() << "sbs1",
                                   "Output in SBS-1 BaseStation format (dump1090-compatible)");
    parser.addOption(sbs1Option);

    QCommandLineOption serverOption(QStringList() << "s" << "server",
                                     "OGN APRS-IS server (default: aprs.glidernet.org)",
                                     "hostname", "aprs.glidernet.org");
    parser.addOption(serverOption);

    QCommandLineOption portOption(QStringList() << "p" << "port",
                                   "Server port (default: 14580)",
                                   "port", "14580");
    parser.addOption(portOption);

    QCommandLineOption latOption("lat",
                                  "Latitude for position filter (required)",
                                  "latitude");
    parser.addOption(latOption);

    QCommandLineOption lonOption("lon",
                                  "Longitude for position filter (required)",
                                  "longitude");
    parser.addOption(lonOption);

    QCommandLineOption radiusOption("radius",
                                     "Radius for position filter in km (default: 50)",
                                     "km", "50");
    parser.addOption(radiusOption);

    parser.process(app);

    // Check if lat/lon are provided
    if (!parser.isSet(latOption) || !parser.isSet(lonOption)) {
        std::cerr << "Error: --lat and --lon options are required" << std::endl << std::endl;
        std::cerr << "Example: --lat 48.3537 --lon 11.7860" << std::endl << std::endl;
        parser.showHelp(1);
    }

    bool sbs1Mode = parser.isSet(sbs1Option);
    QString server = parser.value(serverOption);
    quint16 port = parser.value(portOption).toUShort();
    double latitude = parser.value(latOption).toDouble();
    double longitude = parser.value(lonOption).toDouble();
    int radius = parser.value(radiusOption).toInt();

    // Create formatter based on mode (OGN is default)
    OgnFormatter ognFormatter;
    SBS1Formatter sbs1Formatter;
    OutputFormatter* formatter = sbs1Mode ? static_cast<OutputFormatter*>(&sbs1Formatter) 
                                          : static_cast<OutputFormatter*>(&ognFormatter);
    
    OgnDumper dumper(formatter, server, port, latitude, longitude, radius);
    dumper.start();

    return app.exec();
}

#include "dumpOGN.moc"
