/*  File: ZFeaturePainterBox01.h
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

#ifndef ZFEATUREPAINTERBOX01_H
#define ZFEATUREPAINTERBOX01_H

#include "ZFeaturePainter.h"
#include "ZFeatureBounds.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <QColor>
#include <QRectF>
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

class ZFeature ;

class ZFeaturePainterBox01: public ZFeaturePainter,
        public Util::InstanceCounter<ZFeaturePainterBox01>,
        public Util::ClassName<ZFeaturePainterBox01>
{
public:

    ZFeaturePainterBox01(const double & width,
                         const QColor &fill_color,
                         const QColor &highlight_color ) ;
    ~ZFeaturePainterBox01() ;

    void paint(ZFeature *feature,
               QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override ;

    double getWidth() const override {return m_width ; }
    bool setWidth(const double& value) override ;

private:

    QColor m_color, m_highlight_color, *m_selected_color ;
    QRectF m_rect ;
    ZFeatureBounds m_subpart ;
    double m_width ;
    unsigned int m_n_subparts ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZFEATUREPAINTERBOX01_H
