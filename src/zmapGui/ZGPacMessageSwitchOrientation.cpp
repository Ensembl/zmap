/*  File: ZGPacMessageSwitchOrientation.cpp
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2016: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database user interface
 * and was written by
 *
 * Steve Miller (sm23@sanger.ac.uk)
 *
 * Description:
 *-------------------------------------------------------------------
 */

#include "ZGPacMessageSwitchOrientation.h"
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageSwitchOrientation>::m_name ("ZGPacMessageSwitchOrientation") ;
const ZGPacMessageType ZGPacMessageSwitchOrientation::m_specific_type (ZGPacMessageType::SwitchOrientation) ;

ZGPacMessageSwitchOrientation::ZGPacMessageSwitchOrientation()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageSwitchOrientation>(),
      Util::ClassName<ZGPacMessageSwitchOrientation>()
{
}

ZGPacMessageSwitchOrientation::ZGPacMessageSwitchOrientation(ZInternalID message_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageSwitchOrientation>(),
      Util::ClassName<ZGPacMessageSwitchOrientation>()
{
}

ZGPacMessageSwitchOrientation::ZGPacMessageSwitchOrientation(const ZGPacMessageSwitchOrientation& message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageSwitchOrientation>(),
      Util::ClassName<ZGPacMessageSwitchOrientation>()
{
}

ZGPacMessageSwitchOrientation &ZGPacMessageSwitchOrientation::operator=(const ZGPacMessageSwitchOrientation& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageSwitchOrientation::clone() const
{
    return new (std::nothrow) ZGPacMessageSwitchOrientation(*this) ;
}

ZGPacMessageSwitchOrientation::~ZGPacMessageSwitchOrientation()
{
}

} // namespace GUI

} // namespace ZMap

