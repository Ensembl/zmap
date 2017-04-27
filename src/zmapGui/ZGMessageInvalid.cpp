/*  File: ZGMessageInvalid.cpp
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

#include "ZGMessageInvalid.h"
#include <new>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageInvalid>::m_name("ZGMessageInvalid") ;

ZGMessageInvalid::ZGMessageInvalid()
    : ZGMessage(),
      Util::InstanceCounter<ZGMessageInvalid>(),
      Util::ClassName<ZGMessageInvalid>()
{
}

ZGMessageInvalid::ZGMessageInvalid(const ZGMessageInvalid &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageInvalid>(),
    Util::ClassName<ZGMessageInvalid>()
{
}

ZGMessageInvalid& ZGMessageInvalid::operator =(const ZGMessageInvalid& message)
{
    if (this != &message)
    {
        ZGMessage::operator =(message) ;
    }
    return *this ;
}

ZGMessage* ZGMessageInvalid::clone() const
{
    return new (std::nothrow) ZGMessageInvalid(*this) ;
}

ZGMessageInvalid::~ZGMessageInvalid()
{
}

} // namespace GUI

} // namespace ZMap
