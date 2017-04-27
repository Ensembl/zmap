/*  File: ZFeaturePainterConn02.cpp
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

#include "ZFeaturePainterConn02.h"
#include "ZFeature.h"
#include <stdexcept>
#include <QtWidgets>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZFeaturePainterConn02>::m_name ("ZFeaturePainterConn02" ) ;

ZFeaturePainterConn02::ZFeaturePainterConn02(const double &width,
                                             const QColor &c1,
                                             const QColor &c2)
    : ZFeaturePainter(),
      Util::InstanceCounter<ZFeaturePainterConn02>(),
      Util::ClassName<ZFeaturePainterConn02>(),
      m_color(c1),
      m_highlight_color(c2),
      m_pen(),
      m_pen_old(),
      m_p1(),
      m_p2(),
      m_subpart(),
      m_previous(),
      m_width2(0.5*width),
      m_n_subparts()
{
    if (m_width2 <= 0.0)
        throw std::runtime_error(std::string("ZFeaturePainterConn02::ZFeaturePainterConn02() ; width must be positive ")) ;
    m_pen.setWidthF(0.0) ;
    m_pen.setCosmetic(true) ;
    m_p1.setY(m_width2) ;
    m_p2.setY(m_width2) ;
}

ZFeaturePainterConn02::~ZFeaturePainterConn02()
{
}

void ZFeaturePainterConn02::paint(ZFeature *feature, QPainter *painter,
                                         const QStyleOptionGraphicsItem *option,
                                         QWidget *widget)
{
    Q_UNUSED(widget) ;
    if (!feature || !painter || !option)
        return ;
    m_n_subparts = feature->getNumSubparts() ;
    if (m_n_subparts > 1)
    {
        m_pen_old = painter->pen() ;
        m_pen.setColor(feature->isSelected() ? m_highlight_color : m_color ) ;
        painter->setPen(m_pen) ;
        m_previous = feature->getSubpart(0) ;
        for (unsigned int i=1; i<m_n_subparts; ++i )
        {
            m_subpart = feature->getSubpart(i) ;
            m_p1.setX(m_previous.end) ;
            m_p2.setX(m_subpart.start) ;
            painter->drawLine(m_p1, m_p2) ;
            m_previous = m_subpart ;
        }
        painter->setPen(m_pen_old) ;
    }
}



bool ZFeaturePainterConn02::setWidth(const double &value)
{
    if (value <= 0.0)
        return false ;
    m_width2 = 0.5 * value ;
    m_p1.setY(m_width2) ;
    m_p2.setY(m_width2) ;
    return true ;
}

} // namespace GUI

} // namespace ZMap

