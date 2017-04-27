/*  File: ZGPacMessageReplyStatus.cpp
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

#include "ZGPacMessageReplyStatus.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageReplyStatus>::m_name("ZGPacMessageReplyStatus") ;
const ZGPacMessageType ZGPacMessageReplyStatus::m_specific_type(ZGPacMessageType::ReplyStatus) ;

ZGPacMessageReplyStatus::ZGPacMessageReplyStatus()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageReplyStatus>(),
      Util::ClassName<ZGPacMessageReplyStatus>(),
      m_status(false)
{
}

ZGPacMessageReplyStatus::ZGPacMessageReplyStatus(ZInternalID message_id,
                                                 bool status)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageReplyStatus>(),
      Util::ClassName<ZGPacMessageReplyStatus>(),
      m_status(status)
{
}

ZGPacMessageReplyStatus::ZGPacMessageReplyStatus(const ZGPacMessageReplyStatus &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageReplyStatus>(),
      Util::ClassName<ZGPacMessageReplyStatus>(),
      m_status(message.m_status)
{
}

ZGPacMessageReplyStatus& ZGPacMessageReplyStatus::operator=(const ZGPacMessageReplyStatus& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
        m_status = message.m_status ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageReplyStatus::clone() const
{
    return new (std::nothrow) ZGPacMessageReplyStatus(*this) ;
}

ZGPacMessageReplyStatus::~ZGPacMessageReplyStatus()
{
}

} // namespace GUI

} // namespace ZMap

