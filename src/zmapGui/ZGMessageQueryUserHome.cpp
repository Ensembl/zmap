/*  File: ZGMessageQueryUserHome.cpp
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

#include "ZGMessageQueryUserHome.h"
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageQueryUserHome>::m_name("ZGMessageQueryUserHome") ;
const ZGMessageType ZGMessageQueryUserHome::m_specific_type(ZGMessageType::QueryUserHome) ;

ZGMessageQueryUserHome::ZGMessageQueryUserHome()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageQueryUserHome>(),
      Util::ClassName<ZGMessageQueryUserHome>()
{
}

ZGMessageQueryUserHome::ZGMessageQueryUserHome(ZInternalID message_id)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageQueryUserHome>(),
      Util::ClassName<ZGMessageQueryUserHome>()
{
}

ZGMessageQueryUserHome::ZGMessageQueryUserHome(const ZGMessageQueryUserHome &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageQueryUserHome>(),
      Util::ClassName<ZGMessageQueryUserHome>()
{
}

ZGMessageQueryUserHome& ZGMessageQueryUserHome::operator=(const ZGMessageQueryUserHome& message)
{
    if (this != &message)
    {
        ZGMessage::operator =(message) ;
    }
    return *this ;
}

ZGMessage* ZGMessageQueryUserHome::clone() const
{
    return new (std::nothrow) ZGMessageQueryUserHome(*this) ;
}

ZGMessageQueryUserHome::~ZGMessageQueryUserHome()
{
}

} // namespace GUI

} // namespace ZMap
