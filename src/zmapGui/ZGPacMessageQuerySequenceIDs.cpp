/*  File: ZGPacMessageQuerySequenceIDs.cpp
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

#include "ZGPacMessageQuerySequenceIDs.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageQuerySequenceIDs>::m_name("ZGPacMessageQuerySequenceIDs") ;
const ZGPacMessageType ZGPacMessageQuerySequenceIDs::m_specific_type(ZGPacMessageType::QuerySequenceIDs) ;

ZGPacMessageQuerySequenceIDs::ZGPacMessageQuerySequenceIDs()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageQuerySequenceIDs>(),
      Util::ClassName<ZGPacMessageQuerySequenceIDs>(),
      m_gui_id(0)
{
}

ZGPacMessageQuerySequenceIDs::ZGPacMessageQuerySequenceIDs(ZInternalID message_id,
                                                           ZInternalID gui_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageQuerySequenceIDs>(),
      Util::ClassName<ZGPacMessageQuerySequenceIDs>(),
      m_gui_id(gui_id)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGPacMessageQuerySequenceIDs::ZGPacMessageQuerySequenceIDs() ; may not set gui_id to 0 ")) ;
}

ZGPacMessageQuerySequenceIDs::ZGPacMessageQuerySequenceIDs(const ZGPacMessageQuerySequenceIDs &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageQuerySequenceIDs>(),
      Util::ClassName<ZGPacMessageQuerySequenceIDs>(),
      m_gui_id(message.m_gui_id)
{
}

ZGPacMessageQuerySequenceIDs& ZGPacMessageQuerySequenceIDs::operator=(const ZGPacMessageQuerySequenceIDs& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator =(message) ;

        m_gui_id = message.m_gui_id ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageQuerySequenceIDs::clone() const
{
    return new (std::nothrow) ZGPacMessageQuerySequenceIDs(*this) ;
}

ZGPacMessageQuerySequenceIDs::~ZGPacMessageQuerySequenceIDs()
{
}

} // namespace GUI

} // namespace ZMap


