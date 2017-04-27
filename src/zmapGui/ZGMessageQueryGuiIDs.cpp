/*  File: ZGMessageQueryGuiIDs.cpp
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

#include "ZGMessageQueryGuiIDs.h"
#include <new>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageQueryGuiIDs>::m_name("ZGMessageQueryGuiIDs") ;
const ZGMessageType ZGMessageQueryGuiIDs::m_specific_type(ZGMessageType::QueryGuiIDs) ;

ZGMessageQueryGuiIDs::ZGMessageQueryGuiIDs()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageQueryGuiIDs>(),
      Util::ClassName<ZGMessageQueryGuiIDs>()
{
}

ZGMessageQueryGuiIDs::ZGMessageQueryGuiIDs(ZInternalID message_id)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageQueryGuiIDs>(),
      Util::ClassName<ZGMessageQueryGuiIDs>()
{
}

ZGMessageQueryGuiIDs::ZGMessageQueryGuiIDs(const ZGMessageQueryGuiIDs& message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageQueryGuiIDs>(),
    Util::ClassName<ZGMessageQueryGuiIDs>()
{
}

ZGMessageQueryGuiIDs& ZGMessageQueryGuiIDs::operator=(const ZGMessageQueryGuiIDs& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;
    }
    return *this ;
}

ZGMessage* ZGMessageQueryGuiIDs::clone() const
{
    return new (std::nothrow) ZGMessageQueryGuiIDs(*this) ;
}

ZGMessageQueryGuiIDs::~ZGMessageQueryGuiIDs()
{
}

} // namespace GUI

} // namespace ZMap

