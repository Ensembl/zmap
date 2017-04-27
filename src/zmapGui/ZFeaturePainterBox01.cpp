/*  File: ZFeaturePainterBox01.cpp
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

#include "ZFeaturePainterBox01.h"
#include "ZFeature.h"
#include <stdexcept>
#include <QtWidgets>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZFeaturePainterBox01>::m_name("ZFeaturePainterBox01") ;

ZFeaturePainterBox01::ZFeaturePainterBox01(const double &width, const QColor &fill_color, const QColor &highlight_color )
    : ZFeaturePainter(),
      Util::InstanceCounter<ZFeaturePainterBox01>(),
      Util::ClassName<ZFeaturePainterBox01>(),
      m_color(fill_color),
      m_highlight_color(highlight_color),
      m_selected_color(Q_NULLPTR),
      m_rect(),
      m_subpart(),
      m_width(width),
      m_n_subparts()
{
    if (width <= 0.0)
        throw std::runtime_error(std::string("ZFeaturePainter01::ZFeaturePainter01() ; width must be positive ")) ;
    m_rect.setTop(0.0) ;
    m_rect.setHeight(m_width) ;
}

ZFeaturePainterBox01::~ZFeaturePainterBox01()
{
}


void ZFeaturePainterBox01::paint(ZFeature *feature, QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{

    if (!feature || !painter || !option)
        return ;
    m_n_subparts = feature->getNumSubparts() ;
    m_selected_color = feature->isSelected() ? &m_highlight_color : &m_color ;
    for (unsigned int i=0 ; i<m_n_subparts ; ++i )
    {
        m_subpart = feature->getSubpart(i) ;
        m_rect.setLeft(m_subpart.start) ;
        m_rect.setWidth(m_subpart.end-m_subpart.start) ;
        painter->fillRect(m_rect, *m_selected_color) ;
    }
}

bool ZFeaturePainterBox01::setWidth(const double &value)
{
    if (value <= 0.0)
        return false ;
    m_width = value ;
    m_rect.setHeight(m_width) ;
    return true ;
}

} // namespace GUI

} // namespace ZMap
