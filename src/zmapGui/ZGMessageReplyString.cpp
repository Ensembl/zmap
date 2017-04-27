/*  File: ZGMessageReplyString.cpp
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

#include "ZGMessageReplyString.h"
#include <new>
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageReplyString>::m_name ("ZGMessageReplyString") ;
const ZGMessageType ZGMessageReplyString::m_specific_type(ZGMessageType::ReplyString) ;

ZGMessageReplyString::ZGMessageReplyString()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageReplyString>(),
      Util::ClassName<ZGMessageReplyString>(),
      m_reply_to_id(0),
      m_string()
{
}


ZGMessageReplyString::ZGMessageReplyString(ZInternalID message_id,
                                           ZInternalID reply_to_id,
                                                    const std::string& data)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageReplyString>(),
      Util::ClassName<ZGMessageReplyString>(),
      m_reply_to_id(reply_to_id),
      m_string(data)
{
    if (!m_reply_to_id)
        throw std::runtime_error(std::string("ZGMessageReplyString::ZGMessageReplyString() ; reply_to_id may not be set to 0")) ;
}

ZGMessageReplyString::ZGMessageReplyString(const ZGMessageReplyString& message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageReplyString>(),
      Util::ClassName<ZGMessageReplyString>(),
      m_reply_to_id(message.m_reply_to_id),
      m_string(message.m_string)
{
}

ZGMessageReplyString& ZGMessageReplyString::operator=(const ZGMessageReplyString& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;
        m_reply_to_id = message.m_reply_to_id ;
        m_string = message.m_string ;
    }
    return *this ;
}

ZGMessage* ZGMessageReplyString::clone() const
{
    return new (std::nothrow) ZGMessageReplyString(*this) ;
}

ZGMessageReplyString::~ZGMessageReplyString()
{
}


} // namespace GUI

} // namespace ZMap

