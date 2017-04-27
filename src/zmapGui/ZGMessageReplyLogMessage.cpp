/*  File: ZGMessageReplyLogMessage.cpp
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

#include "ZGMessageReplyLogMessage.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageReplyLogMessage>::m_name ("ZGMessageReplyLogMessage") ;
const ZGMessageType ZGMessageReplyLogMessage::m_specific_type(ZGMessageType::ReplyLogMessage) ;

ZGMessageReplyLogMessage::ZGMessageReplyLogMessage()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageReplyLogMessage>(),
      Util::ClassName<ZGMessageReplyLogMessage>(),
      m_log_message()
{
}


ZGMessageReplyLogMessage::ZGMessageReplyLogMessage(ZInternalID message_id,
                                                    const std::string& log_message)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageReplyLogMessage>(),
      Util::ClassName<ZGMessageReplyLogMessage>(),
      m_log_message(log_message)
{
    if (!m_log_message.length())
        throw std::runtime_error(std::string("ZGMessageReplyLogMessage::ZGMessageReplyLogMessage() ; log message may not be empty")) ;
}

ZGMessageReplyLogMessage::ZGMessageReplyLogMessage(const ZGMessageReplyLogMessage& message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageReplyLogMessage>(),
      Util::ClassName<ZGMessageReplyLogMessage>(),
      m_log_message(message.m_log_message)
{
}

ZGMessageReplyLogMessage& ZGMessageReplyLogMessage::operator=(const ZGMessageReplyLogMessage& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;
        m_log_message = message.m_log_message ;
    }
    return *this ;
}

ZGMessage* ZGMessageReplyLogMessage::clone() const
{
    return new (std::nothrow) ZGMessageReplyLogMessage(*this) ;
}

ZGMessageReplyLogMessage::~ZGMessageReplyLogMessage()
{
}

} // namespace GUI

} // namespace ZMap
