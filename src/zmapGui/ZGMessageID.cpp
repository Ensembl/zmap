/*  File: ZGMessageID.cpp
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

#include "ZGMessageID.h"


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageID>::m_name("ZGMessageID") ;

ZGMessageID::ZGMessageID(ZInternalID state)
    : Util::InstanceCounter<ZGMessageID>(),
      Util::ClassName<ZGMessageID>(),
      m_state(state)
{
}

ZGMessageID::ZGMessageID(const ZGMessageID &message_id)
    : Util::InstanceCounter<ZGMessageID>(),
      Util::ClassName<ZGMessageID>(),
      m_state(message_id.m_state)
{
}

ZGMessageID& ZGMessageID::operator=(const ZGMessageID &message_id)
{
    if (this != &message_id)
    {
        m_state = message_id.m_state ;
    }
    return *this ;
}

ZGMessageID::~ZGMessageID()
{
}

ZInternalID ZGMessageID::new_id()
{
    return m_state++ ;
}

} // namespace GUI

} // namespace ZMap

