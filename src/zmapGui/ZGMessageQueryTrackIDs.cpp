/*  File: ZGMessageQueryTrackIDs.cpp
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

#include "ZGMessageQueryTrackIDs.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageQueryTrackIDs>::m_name("ZGMessageQueryTrackIDs") ;
const ZGMessageType ZGMessageQueryTrackIDs::m_specific_type(ZGMessageType::QueryTrackIDs) ;

ZGMessageQueryTrackIDs::ZGMessageQueryTrackIDs()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageQueryTrackIDs>(),
      Util::ClassName<ZGMessageQueryTrackIDs>(),
      m_sequence_id(0)
{
}

ZGMessageQueryTrackIDs::ZGMessageQueryTrackIDs(ZInternalID message_id, ZInternalID sequence_id)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageQueryTrackIDs>(),
      Util::ClassName<ZGMessageQueryTrackIDs>(),
      m_sequence_id(sequence_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageQueryTrackIDs::ZGMessageQueryTrackIDs() ; sequence_id may not be set to 0")) ;
}


ZGMessageQueryTrackIDs::ZGMessageQueryTrackIDs(const ZGMessageQueryTrackIDs &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageQueryTrackIDs>(),
      Util::ClassName<ZGMessageQueryTrackIDs>(),
      m_sequence_id(message.m_sequence_id)
{
}

ZGMessageQueryTrackIDs& ZGMessageQueryTrackIDs::operator=(const ZGMessageQueryTrackIDs& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
    }
    return *this ;
}

ZGMessage* ZGMessageQueryTrackIDs::clone() const
{
    return new (std::nothrow) ZGMessageQueryTrackIDs(*this ) ;
}

ZGMessageQueryTrackIDs::~ZGMessageQueryTrackIDs()
{
}

} // namespace GUI

} // namespace ZMap

