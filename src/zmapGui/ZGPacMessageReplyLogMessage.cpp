/*  File: ZGPacMessageReplyLogMessage.cpp
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


#include "ZGPacMessageReplyLogMessage.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageReplyLogMessage>::m_name("ZGPacMessageReplyLogMessage") ;
const ZGPacMessageType ZGPacMessageReplyLogMessage::m_specific_type(ZGPacMessageType::ReplyLogMessage) ;

ZGPacMessageReplyLogMessage::ZGPacMessageReplyLogMessage()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageReplyLogMessage>(),
      Util::ClassName<ZGPacMessageReplyLogMessage>(),
      m_log_message()
{
}


ZGPacMessageReplyLogMessage::ZGPacMessageReplyLogMessage(ZInternalID message_id,
                                                         const std::string &log_message)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageReplyLogMessage>(),
      Util::ClassName<ZGPacMessageReplyLogMessage>(),
      m_log_message(log_message)
{
    if (!m_log_message.length())
        throw std::runtime_error(std::string("ZGPacMessageReplyLogMessage::ZGPacMessageReplyLogMessage() ; log message may not be empty")) ;
}

ZGPacMessageReplyLogMessage::ZGPacMessageReplyLogMessage(const ZGPacMessageReplyLogMessage& message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageReplyLogMessage>(),
      Util::ClassName<ZGPacMessageReplyLogMessage>(),
      m_log_message(message.m_log_message)
{
}

ZGPacMessageReplyLogMessage& ZGPacMessageReplyLogMessage::operator=(const ZGPacMessageReplyLogMessage& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
        m_log_message = message.m_log_message ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageReplyLogMessage::clone() const
{
    return new (std::nothrow) ZGPacMessageReplyLogMessage(*this) ;
}

ZGPacMessageReplyLogMessage::~ZGPacMessageReplyLogMessage()
{
}

} // namespace GUI

} // namespace ZMap

