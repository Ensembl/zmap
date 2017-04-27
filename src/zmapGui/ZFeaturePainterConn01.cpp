/*  File: ZFeaturePainterConn01.cpp
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

#include "ZFeaturePainterConn01.h"
#include "ZFeature.h"

#include <stdexcept>
#include <QtWidgets>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZFeaturePainterConn01>::m_name ("ZFeaturePainterConn01") ;

ZFeaturePainterConn01::ZFeaturePainterConn01(const double & width,
                                             const QColor &color,
                                             const QColor &highlight_color)
    : ZFeaturePainter(),
      Util::InstanceCounter<ZFeaturePainterConn01>(),
      Util::ClassName<ZFeaturePainterConn01>(),
      m_color(color),
      m_highlight_color(highlight_color),
      m_pen(),
      m_pen_old(),
      m_p1(),
      m_p2(),
      m_subpart(),
      m_previous(),
      m_width2(0.5*width),
      m_offset(0.1*width),
      m_midpoint(),
      m_n_subparts()
{
    if (width <= 0.0)
        throw std::runtime_error(std::string("ZFeaturePainterConn01::ZFeaturePainterConn01() ; width must be positive ")) ;
    m_pen.setWidthF(0.0) ;
    m_pen.setCosmetic(true) ;
}

ZFeaturePainterConn01::~ZFeaturePainterConn01()
{
}


void ZFeaturePainterConn01::paint(ZFeature *feature, QPainter *painter,
                                  const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget) ;
    if (!feature || !painter ||  !option )
        return ;
    m_n_subparts = feature->getNumSubparts() ;

    m_pen_old = painter->pen() ;
    m_pen.setColor(feature->isSelected() ? m_highlight_color : m_color ) ;
    painter->setPen(m_pen) ;

    m_previous = feature->getSubpart(0) ;
    for (unsigned int i=1; i<m_n_subparts ; ++i)
    {
        m_subpart = feature->getSubpart(i) ;
        m_midpoint = 0.5*(m_subpart.start + m_previous.end) ;

        m_p1.setX(m_previous.end) ;
        m_p1.setY(m_width2) ;
        m_p2.setX(m_midpoint) ;
        m_p2.setY(m_offset) ;
        painter->drawLine(m_p1, m_p2) ;

        m_p1.setX(m_midpoint) ;
        m_p1.setY(m_offset) ;
        m_p2.setX(m_subpart.start) ;
        m_p2.setY(m_width2) ;
        painter->drawLine(m_p1, m_p2) ;

        m_previous = m_subpart ;
    }
    painter->setPen(m_pen_old) ;

}


bool ZFeaturePainterConn01::setWidth(const double &value)
{
    if (value <= 0.0)
        return false ;
    m_width2 = 0.5*value ;
    m_offset = 0.1*value ;
    return true ;
}

} // namespace GUI

} // namespace ZMap
