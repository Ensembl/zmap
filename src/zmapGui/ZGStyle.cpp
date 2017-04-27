/*  File: ZGStyle.cpp
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

#include "ZGStyle.h"
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGStyle>::m_name("ZGStyle") ;

ZGStyle::ZGStyle()
    : Util::InstanceCounter<ZGStyle>(),
      Util::ClassName<ZGStyle>(),
      m_id(0),
      m_width(0.0),
      m_colorfill(),
      m_coloroutline(),
      m_colorhighlight(),
      m_colorcds(),
      m_feature_type(ZGFeatureType::Invalid)
{
}

ZGStyle::ZGStyle(ZInternalID id)
    : Util::InstanceCounter<ZGStyle>(),
      Util::ClassName<ZGStyle>(),
      m_id(id),
      m_width(),
      m_colorfill(),
      m_coloroutline(),
      m_colorhighlight(),
      m_colorcds(),
      m_feature_type(ZGFeatureType::Invalid)
{
    if (!m_id)
        throw std::runtime_error(std::string("ZGStyle::ZGStyle() ; id may not be 0 ")) ;
}

ZGStyle::ZGStyle(ZInternalID id,
                 const double& width,
                 const ZGColor &c1,
                 const ZGColor &c2,
                 const ZGColor &c3,
                 const ZGColor &c4,
                 ZGFeatureType feature_type)
    : Util::InstanceCounter<ZGStyle>(),
      Util::ClassName<ZGStyle>(),
      m_id(id),
      m_width(width),
      m_colorfill(c1),
      m_coloroutline(c2),
      m_colorhighlight(c3),
      m_colorcds(c4),
      m_feature_type(feature_type)
{
    if (!m_id)
        throw std::runtime_error(std::string("ZGStyle::ZGStyle() ; id may not be 0 ")) ;
}

ZGStyle::ZGStyle(const ZGStyle& style)
    : Util::InstanceCounter<ZGStyle>(),
      Util::ClassName<ZGStyle>(),
      m_id(style.m_id),
      m_width(style.m_width),
      m_colorfill(style.m_colorfill),
      m_coloroutline(style.m_coloroutline),
      m_colorhighlight(style.m_colorhighlight),
      m_colorcds(style.m_colorcds),
      m_feature_type(style.m_feature_type)
{
}

ZGStyle& ZGStyle::operator=(const ZGStyle &style)
{
    if (this != &style)
    {
        m_id = style.m_id ;
        m_width = style.m_width ;
        m_colorfill = style.m_colorfill ;
        m_coloroutline = style.m_coloroutline ;
        m_colorhighlight = style.m_colorhighlight ;
        m_colorcds = style.m_colorcds ;
        m_feature_type = style.m_feature_type ;
    }
    return *this ;
}

ZGStyle::~ZGStyle()
{
}

} // namespace GUI

} // namespace ZMap
