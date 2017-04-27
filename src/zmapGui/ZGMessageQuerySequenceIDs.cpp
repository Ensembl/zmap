/*  File: ZGMessageQuerySequenceIDs.cpp
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

#include "ZGMessageQuerySequenceIDs.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageQuerySequenceIDs>::m_name("ZGMessageQuerySequenceIDs") ;
const ZGMessageType ZGMessageQuerySequenceIDs::m_specific_type(ZGMessageType::QuerySequenceIDs) ;

ZGMessageQuerySequenceIDs::ZGMessageQuerySequenceIDs()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageQuerySequenceIDs>(),
      Util::ClassName<ZGMessageQuerySequenceIDs>(),
      m_gui_id(0)
{
}

ZGMessageQuerySequenceIDs::ZGMessageQuerySequenceIDs(ZInternalID message_id,
                                                     ZInternalID gui_id)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageQuerySequenceIDs>(),
      Util::ClassName<ZGMessageQuerySequenceIDs>(),
      m_gui_id(gui_id)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGMessageQuerySequenceIDs::ZGMessageQuerySequenceIDs() ; may not set gui_id to 0")) ;
}

ZGMessageQuerySequenceIDs::ZGMessageQuerySequenceIDs(const ZGMessageQuerySequenceIDs &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageQuerySequenceIDs>(),
      Util::ClassName<ZGMessageQuerySequenceIDs>(),
      m_gui_id(message.m_gui_id)
{
}

ZGMessageQuerySequenceIDs& ZGMessageQuerySequenceIDs::operator=(const ZGMessageQuerySequenceIDs& message)
{
    if (this != &message)
    {
        ZGMessage::operator =(message) ;
        m_gui_id = message.m_gui_id ;
    }
    return *this ;
}

ZGMessage* ZGMessageQuerySequenceIDs::clone() const
{
    return new (std::nothrow) ZGMessageQuerySequenceIDs(*this) ;
}

ZGMessageQuerySequenceIDs::~ZGMessageQuerySequenceIDs()
{
}

} // namespace GUI

} // namespace ZMap

