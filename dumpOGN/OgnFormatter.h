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
#include "OutputFormatter.h"

/*! \brief OGN APRS-IS raw format
 *
 *  Outputs raw APRS-IS sentences as received from the OGN network.
 *  This is useful for debugging or for forwarding to other APRS-IS clients.
 */
class OgnFormatter : public OutputFormatter
{
public:
    QString format(const Ogn::OgnMessage& message) override
    {
        // Simply return the raw sentence as-is
        return message.sentence;
    }
};
