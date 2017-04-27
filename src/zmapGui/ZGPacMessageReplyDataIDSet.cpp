/*  File: ZGPacMessageReplyDataIDSet.cpp
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

#include "ZGPacMessageReplyDataIDSet.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageReplyDataIDSet>::m_name("ZGPacMessageReplyDataIDSet") ;
const ZGPacMessageType ZGPacMessageReplyDataIDSet::m_specific_type(ZGPacMessageType::ReplyDataIDSet) ;

ZGPacMessageReplyDataIDSet::ZGPacMessageReplyDataIDSet()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageReplyDataIDSet>(),
      Util::ClassName<ZGPacMessageReplyDataIDSet>(),
      m_reply_to_id(0),
      m_data()
{
}

ZGPacMessageReplyDataIDSet::ZGPacMessageReplyDataIDSet(ZInternalID message_id,
                                                 ZInternalID reply_to_id,
                                                 const std::set<ZInternalID> &data)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageReplyDataIDSet>(),
      Util::ClassName<ZGPacMessageReplyDataIDSet>(),
      m_reply_to_id(reply_to_id),
      m_data(data)
{
    if (!m_reply_to_id)
        throw std::runtime_error(std::string("ZGPacMessageReplyDataIDSet::ZGPacMessageReplyDataIDSet() ; reply_to_id may not be set to 0 ")) ;
}

ZGPacMessageReplyDataIDSet::ZGPacMessageReplyDataIDSet(const ZGPacMessageReplyDataIDSet &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageReplyDataIDSet>(),
      Util::ClassName<ZGPacMessageReplyDataIDSet>(),
      m_reply_to_id(message.m_reply_to_id),
      m_data(message.m_data)
{
}

ZGPacMessageReplyDataIDSet& ZGPacMessageReplyDataIDSet::operator=(const ZGPacMessageReplyDataIDSet& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_reply_to_id = message.m_reply_to_id ;
        m_data = message.m_data ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageReplyDataIDSet::clone() const
{
    return new (std::nothrow) ZGPacMessageReplyDataIDSet(*this) ;
}

ZGPacMessageReplyDataIDSet::~ZGPacMessageReplyDataIDSet()
{
}


} // namespace GUI

} // namespace ZMap

