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

#pragma once

#include <QString>
#include "TrafficDataSource_OgnParser.h"

/*! \brief Base class for output formatters
 *
 *  This abstract class defines the interface for converting OGN messages
 *  to different output formats (SBS-1, GDL90, Flarm, etc.)
 */
class OutputFormatter
{
public:
    virtual ~OutputFormatter() = default;
    
    /*! \brief Format an OGN message for output
     *  \param message The parsed OGN message
     *  \return Formatted string for output, or empty string if message should be skipped
     */
    virtual QString format(const Traffic::Ogn::OgnMessage& message) = 0;
};
