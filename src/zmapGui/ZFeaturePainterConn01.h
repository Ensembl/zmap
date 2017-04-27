/*  File: ZFeaturePainterConn01.h
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

#ifndef ZFEATUREPAINTERCONN01_H
#define ZFEATUREPAINTERCONN01_H

#include "ZFeaturePainter.h"
#include "ZFeatureBounds.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <QPen>
#include <QColor>
#include <string>
#include <cstddef>


namespace ZMap
{

namespace GUI
{

// a transcript-like connector
class ZFeaturePainterConn01: public ZFeaturePainter,
        public Util::InstanceCounter<ZFeaturePainterConn01>,
        public Util::ClassName<ZFeaturePainterConn01>
{
public:

    ZFeaturePainterConn01(const double& width,
                          const QColor &color,
                          const QColor &highlight_color) ;
    ~ZFeaturePainterConn01() ;

    void paint(ZFeature* feature,
               QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override ;

    double getWidth() const override {return 2.0*m_width2 ; }
    bool setWidth(const double& value) override ;

private:
    QColor m_color, m_highlight_color;
    QPen m_pen, m_pen_old ;
    QPointF m_p1, m_p2 ;
    ZFeatureBounds m_subpart, m_previous ;
    double m_width2, m_offset, m_midpoint ;
    unsigned int m_n_subparts ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZFEATUREPAINTERCONN01_H
