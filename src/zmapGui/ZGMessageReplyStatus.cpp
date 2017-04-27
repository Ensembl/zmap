/*  File: ZGMessageReplyStatus.cpp
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

#include "ZGMessageReplyStatus.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageReplyStatus>::m_name("ZGMessageReply") ;
const ZGMessageType ZGMessageReplyStatus::m_specific_type(ZGMessageType::ReplyStatus) ;

ZGMessageReplyStatus::ZGMessageReplyStatus()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageReplyStatus>(),
      Util::ClassName<ZGMessageReplyStatus>(),
      m_reply_to_id(0)
{
}

ZGMessageReplyStatus::ZGMessageReplyStatus(ZInternalID message_id,
                               ZInternalID reply_to_id,
                               bool status)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageReplyStatus>(),
      Util::ClassName<ZGMessageReplyStatus>(),
      m_reply_to_id(reply_to_id),
      m_status(status)
{
    if (!m_reply_to_id)
        throw std::runtime_error(std::string("ZGMessageReply::ZGMessageReply() ; reply_to_id may not be set to 0 ")) ;

}

ZGMessageReplyStatus::ZGMessageReplyStatus(const ZGMessageReplyStatus &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageReplyStatus>(),
      Util::ClassName<ZGMessageReplyStatus>(),
      m_reply_to_id(message.m_reply_to_id),
      m_status(message.m_status)
{
}

ZGMessageReplyStatus& ZGMessageReplyStatus::operator=(const ZGMessageReplyStatus& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;
        m_reply_to_id = message.m_reply_to_id ;
        m_status = message.m_status ;
    }
    return *this ;
}

ZGMessage* ZGMessageReplyStatus::clone() const
{
    return new (std::nothrow) ZGMessageReplyStatus(*this) ;
}

ZGMessageReplyStatus::~ZGMessageReplyStatus()
{
}

} // namespace GUI

} // namespace ZMap

