/*  File: ZGuiWidgetOverlayScale.h
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

#ifndef ZGUIWIDGETOVERLAYSCALE_H
#define ZGUIWIDGETOVERLAYSCALE_H

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <cstddef>
#include <string>
#include <vector>

#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGViewOrientationType.h"
#include "ZGScalePositionType.h"
#include "ZGScaleRelativeType.h"

//
// external interface should set the view orientation (that is, the direction
// of the forward strand) and the relative type (positive or negative strand).
// note that this really does mean view orientation of the parent viewport
//


namespace ZMap
{

namespace GUI
{

class ZGuiWidgetOverlayScale
        : public QWidget,
          public Util::InstanceCounter<ZGuiWidgetOverlayScale>,
        public Util::ClassName<ZGuiWidgetOverlayScale>
{

    Q_OBJECT

public:
    static ZGuiWidgetOverlayScale* newInstance(QWidget *parent = Q_NULLPTR ) ;

    ~ZGuiWidgetOverlayScale() ;

    bool setOrigin(const qreal& value) ;
    qreal getOrigin() const {return m_origin; }

    bool setMinTickSpacing(int i) ;
    int getMinTickSpacing() const {return m_min_tick_spacing ; }

    bool setWidth(int w) ;

    void setStartEnd(const qreal &start, const qreal &end) ;
    std::pair<qreal,qreal> getStartEnd() const {return std::pair<qreal,qreal>(m_scene_start, m_scene_end) ; }

    bool setViewOrientation(ZGViewOrientationType type) ;
    ZGViewOrientationType getViewOrientation() const {return m_view_orientation ; }

    bool setRelativePosition(ZGScaleRelativeType type) ;
    ZGScaleRelativeType getRelativePosition() const {return m_relative_position ; }

    void setSizeAndPosition(const QSize &parent_size) ;

signals:

public slots:

protected:

    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE ;
    void resizeEvent(QResizeEvent * event) Q_DECL_OVERRIDE ;

private:

    ZGuiWidgetOverlayScale(QWidget *parent);
    ZGuiWidgetOverlayScale(const ZGuiWidgetOverlayScale&) = delete ;
    ZGuiWidgetOverlayScale& operator=(const ZGuiWidgetOverlayScale&) = delete ;

    void computePainterTransformations_left(QPainter &painter)
    {
        painter.translate(width(), height()) ;
        painter.rotate(180.0) ;
    }
    void computePainterTransformations_up(QPainter &painter)
    {
        painter.translate(0, height()) ;
        painter.rotate(270.0) ;
    }
    void computePainterTransformations_down(QPainter &painter)
    {
        painter.translate(width(), 0) ;
        painter.rotate(90.0) ;
    }

    void computePixelsPerUnitScene_h()
    {
        m_pixels_per_unit_scene = static_cast<qreal>(width()) / m_scene_extent ;
    }
    void computePixelsPerUnitScene_v()
    {
        m_pixels_per_unit_scene = static_cast<qreal>(height()) / m_scene_extent ;
    }

    void setScalePosition() ;
    void findTickPositions() ;
    void sceneToPixelsX()
    {
        m_x = m_pixels_per_unit_scene * (m_x-m_scene_start) + m_x_corr ;
    }

    static const int m_default_width,
        m_default_resolution,
        m_default_min_tick_spacing ;
    static const size_t m_default_tick_reserve ;
    static const std::vector<int> m_intervals ;

    qreal m_x,
        m_origin,
        m_scene_start,
        m_scene_end,
        m_scene_extent,
        m_pixels_per_unit_scene ;
    int m_ix,
        m_width,
        m_width2,
        m_width4,
        m_resolution,
        m_res1,
        m_min_tick_spacing,
        m_x_corr ;
    ZGViewOrientationType m_view_orientation ;
    ZGScaleRelativeType m_relative_position ;
    ZGScalePositionType m_scale_position ;
    std::vector<int> m_tick_positions ;
    std::vector<std::pair<int, QString> > m_numbers ;
    void (ZGuiWidgetOverlayScale::*m_computePixelsPerUnitScene)() ;
    void (ZGuiWidgetOverlayScale::*m_computePainterTransformations)(QPainter&) ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIWIDGETOVERLAYSCALE_H
