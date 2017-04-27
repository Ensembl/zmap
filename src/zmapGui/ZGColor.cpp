/*  File: ZGColor.cpp
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

#include "ZGColor.h"
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGColor>::m_name("ZGColor") ;

ZGColor::ZGColor()
    : Util::InstanceCounter<ZGColor>(),
      Util::ClassName<ZGColor>(),
      m_r(),
      m_g(),
      m_b(),
      m_a()
{
}

ZGColor::ZGColor(const ZGColor &color)
    : Util::InstanceCounter<ZGColor>(),
      Util::ClassName<ZGColor>(),
      m_r(color.m_r),
      m_g(color.m_g),
      m_b(color.m_b),
      m_a(color.m_a)
{
}

ZGColor::ZGColor(unsigned char v1, unsigned char v2, unsigned char v3, unsigned char v4)
    : Util::InstanceCounter<ZGColor>(),
      Util::ClassName<ZGColor>(),
      m_r(v1),
      m_g(v2),
      m_b(v3),
      m_a(v4)
{
}

ZGColor& ZGColor::operator=(const ZGColor& color)
{
    if (this != &color)
    {
        m_r = color.m_r ;
        m_g = color.m_g ;
        m_b = color.m_b ;
        m_a = color.m_a ;
    }
    return *this ;
}

ZGColor::~ZGColor()
{
}

QColor ZGColor::asQColor() const
{
    return QColor(m_r, m_g, m_b, m_a) ;
}

} // namespace GUI

} // namespace ZMap
