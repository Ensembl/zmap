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

#include "ZGuiWidgetOverlayScale.h"
#include "Utilities.h"
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <QPaintEvent>
#include <QPainter>
#include <QStaticText>
#include <QTransform>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetOverlayScale>::m_name("ZGuiWidgetOverlayScale") ;
const int ZGuiWidgetOverlayScale::m_default_width(20),
    ZGuiWidgetOverlayScale::m_default_resolution(10),
    ZGuiWidgetOverlayScale::m_default_min_tick_spacing(10) ;
const size_t ZGuiWidgetOverlayScale::m_default_tick_reserve(500) ;
const std::vector<int> ZGuiWidgetOverlayScale::m_intervals =
{
    1,
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    10000000,
    100000000
} ;

ZGuiWidgetOverlayScale::ZGuiWidgetOverlayScale(QWidget *parent)
    : QWidget(parent),
      Util::InstanceCounter<ZGuiWidgetOverlayScale>(),
      Util::ClassName<ZGuiWidgetOverlayScale>(),
      m_x(0.0),
      m_origin(0.0),
      m_scene_start(0.0),
      m_scene_end(0.0),
      m_scene_extent(0.0),
      m_pixels_per_unit_scene(0.0),
      m_ix(0),
      m_width(m_default_width),
      m_width2(m_default_width/2),
      m_width4(m_default_width/4),
      m_resolution(m_default_resolution),
      m_res1(m_default_resolution*10),
      m_min_tick_spacing(m_default_min_tick_spacing),
      m_x_corr(1),
      m_view_orientation(ZGViewOrientationType::Right),
      m_relative_position(ZGScaleRelativeType::Positive),
      m_scale_position(ZGScalePositionType::Invalid),
      m_tick_positions(),
      m_numbers(),
      m_computePixelsPerUnitScene(nullptr),
      m_computePainterTransformations(nullptr)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetOverlayScale::ZGuiWidgetOverlayScale() ; can not create instance without QApplication existing ")) ;
    setViewOrientation(m_view_orientation) ;
    m_tick_positions.reserve(m_default_tick_reserve) ;
}

ZGuiWidgetOverlayScale::~ZGuiWidgetOverlayScale()
{
}

ZGuiWidgetOverlayScale* ZGuiWidgetOverlayScale::newInstance(QWidget *parent)
{
    ZGuiWidgetOverlayScale* thing = Q_NULLPTR ;

    try
    {
        thing = new ZGuiWidgetOverlayScale(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

//
// Drawing coordinates are defined as normal, that is:
//
//                     ------------> +x
//                     |
//                     |
//                     |
//                     V  +y
//
// Note about rotations: we rotate the coordinate system so a
// rotation of 90^o means a passive rotation of axes as follows:
//
//                      +y   <-------
//                                  |
//                                  |
//                                  |
//                                  V  +x
//
// Note that when drawing text, the coordinate is the lower left
// corner of the text (relative to the text itself, not in the
// screen coordinates).
//
void ZGuiWidgetOverlayScale::paintEvent(QPaintEvent *event)
{
    QPainter painter(this) ;

    // orientation dependent stuff...
    if (m_computePainterTransformations)
        (this->*m_computePainterTransformations)(painter) ;

    // "base" line for the ruler
    m_x = m_scene_end ;
    sceneToPixelsX();
    painter.drawLine(QPointF(1.0, 1.0), QPointF(m_x, 1.0)) ;

    // draw some ticks
    for (auto it = m_tick_positions.begin() ; it != m_tick_positions.end() ; ++it )
    {
        //m_ix = *it ;
        m_x = *it ;
        sceneToPixelsX() ;
        painter.drawLine(QPointF(m_x, 1.0), QPointF(m_x, m_width4)) ;
    }

    // draw the numbers
    for (auto it = m_numbers.begin() ; it != m_numbers.end() ; ++it)
    {
        m_x = it->first ;
        sceneToPixelsX() ;
        painter.drawLine(QPointF(m_x, 1.0), QPointF(m_x, m_width2)) ;
        painter.drawText(QPointF(m_x, m_width), it->second) ;
    }

    // this is one way to transform the orientation of the text,
    // but likely far from optimal
//    painter.rotate(90.0) ;
//    for (auto it = m_numbers.begin() ; it != m_numbers.end() ; ++it)
//    {
//        m_x = it->first ;
//        sceneToPixelsX() ;
//        painter.drawText(QPointF(1.0, -m_x), it->second) ;
//    }

    QWidget::paintEvent(event) ;
}

// the widget already has the new geometry...
void ZGuiWidgetOverlayScale::resizeEvent(QResizeEvent *event)
{
    findTickPositions() ;
    QWidget::resizeEvent(event) ;
}


// this is the overall position, defined with respect to the parent viewport
void ZGuiWidgetOverlayScale::setSizeAndPosition(const QSize& parent_size)
{
    int offset = 0 ;
    QRect position ;
    switch (m_scale_position)
    {
    case ZGScalePositionType::Top:
    {
        position = QRect(0, offset, parent_size.width(), m_width) ;
        break ;
    }
    case ZGScalePositionType::Bottom:
    {
        offset = parent_size.height() - m_width ;
        position = QRect(0, offset, parent_size.width(), m_width) ;
        break ;
    }
    case ZGScalePositionType::Left:
    {
        position = QRect(offset, 0, m_width, parent_size.height()) ;
        break ;
    }
    case ZGScalePositionType::Right:
    {
        offset = parent_size.width()-m_width ;
        position = QRect(offset, 0, m_width, parent_size.height()) ;
        break ;
    }
    default:
        break ;
    }
    setGeometry(position) ;
}


// should call setSizeAndPosition after this
bool ZGuiWidgetOverlayScale::setWidth(int w)
{
    bool result = false ;
    if (w > 0)
    {
        m_width = w ;
        m_width2 = m_width / 2 ;
        m_width4 = m_width / 4 ;
        result = true ;
    }
    return result ;
}

bool ZGuiWidgetOverlayScale::setOrigin(const qreal& value)
{
    bool result = false ;
    if (value >= 0.0)
    {
        m_origin = value ;
        result = true ;
    }
    return result ;
}

bool ZGuiWidgetOverlayScale::setMinTickSpacing(int value)
{
    bool result = false ;
    if (value > 0)
    {
        m_min_tick_spacing = value ;
        result = true ;
    }
    return result ;
}

bool ZGuiWidgetOverlayScale::setViewOrientation(ZGViewOrientationType type)
{
    bool result = false ;
    if (type != ZGViewOrientationType::Invalid)
    {
        m_view_orientation = type ;
        setScalePosition() ;
        switch (type)
        {
        case ZGViewOrientationType::Right:
        {
            m_computePainterTransformations = nullptr ;
            m_computePixelsPerUnitScene = &ZGuiWidgetOverlayScale::computePixelsPerUnitScene_h ;
            m_x_corr = 1 ;
            break ;
        }
        case ZGViewOrientationType::Left:
        {
            m_computePainterTransformations = &ZGuiWidgetOverlayScale::computePainterTransformations_left ;
            m_computePixelsPerUnitScene = &ZGuiWidgetOverlayScale::computePixelsPerUnitScene_h ;
            m_x_corr = -1 ;
            break ;
        }
        case ZGViewOrientationType::Up:
        {
            m_computePainterTransformations = &ZGuiWidgetOverlayScale::computePainterTransformations_up ;
            m_computePixelsPerUnitScene = &ZGuiWidgetOverlayScale::computePixelsPerUnitScene_v ;
            m_x_corr = -1 ;
            break ;
        }
        case ZGViewOrientationType::Down:
        {
            m_computePainterTransformations = &ZGuiWidgetOverlayScale::computePainterTransformations_down ;
            m_computePixelsPerUnitScene = &ZGuiWidgetOverlayScale::computePixelsPerUnitScene_v ;
            m_x_corr = 1 ;
            break ;
        }
        default:
            break ;
        }

        result = true ;
    }
    findTickPositions() ;
    return result ;
}

// this converts the current values of m_view_orientation and
// m_relative_position to the m_scale_position variable
void ZGuiWidgetOverlayScale::setScalePosition()
{
    m_scale_position = ZGScalePositionType::Invalid ;
    switch (m_view_orientation)
    {
    case ZGViewOrientationType::Right:
    {
        if (m_relative_position == ZGScaleRelativeType::Positive)
            m_scale_position = ZGScalePositionType::Top ;
        else if (m_relative_position == ZGScaleRelativeType::Negative)
            m_scale_position = ZGScalePositionType::Bottom ;
        break ;
    }
    case ZGViewOrientationType::Left:
    {
        if (m_relative_position == ZGScaleRelativeType::Positive)
            m_scale_position = ZGScalePositionType::Bottom ;
        else if (m_relative_position == ZGScaleRelativeType::Negative)
            m_scale_position = ZGScalePositionType::Top ;
        break ;
    }
    case ZGViewOrientationType::Up:
    {
        if (m_relative_position == ZGScaleRelativeType::Positive)
            m_scale_position = ZGScalePositionType::Left ;
        else if (m_relative_position == ZGScaleRelativeType::Negative)
            m_scale_position = ZGScalePositionType::Right ;
        break ;
    }
    case ZGViewOrientationType::Down:
    {
        if (m_relative_position == ZGScaleRelativeType::Positive)
            m_scale_position = ZGScalePositionType::Right ;
        else if (m_relative_position == ZGScaleRelativeType::Negative)
            m_scale_position = ZGScalePositionType::Left ;
        break ;
    }
    default:
        break;
    }
}

//// and this
bool ZGuiWidgetOverlayScale::setRelativePosition(ZGScaleRelativeType type)
{
    bool result = false ;
    if (type != ZGScaleRelativeType::Invalid)
    {
        m_relative_position = type ;
        setScalePosition() ;
        result = true ;
    }
    return result ;
}

void ZGuiWidgetOverlayScale::setStartEnd(const qreal &start, const qreal &end)
{
    m_scene_start = start ;
    m_scene_end = end ;
    m_scene_extent = m_scene_end - m_scene_start ;

    findTickPositions() ;
}

// firstly find resolution...
// then the tick positions
void ZGuiWidgetOverlayScale::findTickPositions()
{
    if (m_scene_extent <= 0.0 || !width() || !height() || !m_computePixelsPerUnitScene)
        return ;

    // the value m_pixels_per_unit_scene is the number of pixels per unit
    // scene coordinate... note that this depends on orientation...
    (this->*m_computePixelsPerUnitScene)() ;

    // now we compute the tick resolution (note that this is in units
    // of world coordinates)
    m_resolution = 1 ;
    for (auto it = m_intervals.begin() ; it != m_intervals.end() ; ++it )
    {
        m_resolution = *it ;
        if (m_resolution * m_pixels_per_unit_scene > m_min_tick_spacing)
        {
            break ;
        }
    }
    m_res1 = 10*m_resolution ;

    // now find all ticks in the scene...
    // the values in m_tick_positions are in scene coordinates...
    m_tick_positions.clear() ;
    m_numbers.clear() ;
    int i = m_scene_start ;
    for ( ; i < m_scene_start+m_resolution ; ++i)
    {
        if (!(i%m_resolution))
            break ;
    }
    for ( ; i< m_scene_end ; i+=m_resolution)
    {
        m_tick_positions.push_back(i) ;
        if (!(i%m_res1))
        {
            QString str ;
            str.setNum(i) ;
            m_numbers.push_back(std::pair<int, QString>(i, str)) ;
        }
    }
}

} // namespace GUI

} // namespace ZMap

