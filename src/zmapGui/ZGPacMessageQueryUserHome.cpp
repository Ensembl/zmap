/*  File: ZGPacMessageQueryUserHome.cpp
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

#include "ZGPacMessageQueryUserHome.h"
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageQueryUserHome>::m_name("ZGPacMessageQueryUserHome") ;
const ZGPacMessageType ZGPacMessageQueryUserHome::m_specific_type(ZGPacMessageType::QueryUserHome) ;

ZGPacMessageQueryUserHome::ZGPacMessageQueryUserHome()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageQueryUserHome>(),
      Util::ClassName<ZGPacMessageQueryUserHome>()
{
}

ZGPacMessageQueryUserHome::ZGPacMessageQueryUserHome(ZInternalID message_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageQueryUserHome>(),
      Util::ClassName<ZGPacMessageQueryUserHome>()
{
}

ZGPacMessageQueryUserHome::ZGPacMessageQueryUserHome(const ZGPacMessageQueryUserHome &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageQueryUserHome>(),
      Util::ClassName<ZGPacMessageQueryUserHome>()
{
}

ZGPacMessageQueryUserHome& ZGPacMessageQueryUserHome::operator=(const ZGPacMessageQueryUserHome& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator =(message) ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageQueryUserHome::clone() const
{
    return new (std::nothrow) ZGPacMessageQueryUserHome(*this) ;
}

ZGPacMessageQueryUserHome::~ZGPacMessageQueryUserHome()
{
}


} // namespace GUI

} // namespace ZMap

