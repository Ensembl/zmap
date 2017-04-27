/*  File: ZGPacMessageReplyDataID.cpp
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

#include "ZGPacMessageReplyDataID.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template<>
const std::string Util::ClassName<ZGPacMessageReplyDataID>::m_name("ZGPacMessageReplyDataID") ;
const ZGPacMessageType ZGPacMessageReplyDataID::m_specific_type(ZGPacMessageType::ReplyDataID) ;

ZGPacMessageReplyDataID::ZGPacMessageReplyDataID()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageReplyDataID>(),
      Util::ClassName<ZGPacMessageReplyDataID>(),
      m_reply_to_id(0),
      m_data(0)
{
}

ZGPacMessageReplyDataID::ZGPacMessageReplyDataID(ZInternalID message_id, ZInternalID reply_to_id, ZInternalID data)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageReplyDataID>(),
      Util::ClassName<ZGPacMessageReplyDataID>(),
      m_reply_to_id(reply_to_id),
      m_data(data)
{
    if (!m_reply_to_id)
        throw std::runtime_error(std::string("ZGPacMessageReplyDataID::ZGPacMessageReplyDataID() ; m_reply_to_id may not be set to 0 ")) ;
}

ZGPacMessageReplyDataID::ZGPacMessageReplyDataID(const ZGPacMessageReplyDataID& message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageReplyDataID>(),
      Util::ClassName<ZGPacMessageReplyDataID>(),
      m_reply_to_id(message.m_reply_to_id),
      m_data(message.m_data)
{
}

ZGPacMessageReplyDataID& ZGPacMessageReplyDataID::operator =(const ZGPacMessageReplyDataID& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;

        m_reply_to_id = message.m_reply_to_id ;
        m_data = message.m_data ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageReplyDataID::clone() const
{
    return new (std::nothrow) ZGPacMessageReplyDataID(*this) ;
}

ZGPacMessageReplyDataID::~ZGPacMessageReplyDataID()
{
}

} // namespace GUI

} // namespace ZMap

