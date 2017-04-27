/*  File: ZGuiGraphicsView.cpp
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

#include "ZGuiGraphicsView.h"
#include "ZGuiScene.h"
#include "ZGuiWidgetOverlayScale.h"
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScrollBar>
#include <QMouseEvent>
#include <QRect>
#include <QBitmap>
#include <QPixmap>
#include <QGraphicsItem>
#include <sstream>
#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiGraphicsView>::m_name ("ZGuiGraphicsView") ;
const QString ZGuiGraphicsView::m_name_base("ZGuiGraphicsView_"),
    ZGuiGraphicsView::m_style_sheet_current("QFrame {border: 2px solid black;}"),
    ZGuiGraphicsView::m_style_sheet_normal("QFrame {}") ;
const int ZGuiGraphicsView::m_pixmap_size(21),
    ZGuiGraphicsView::m_pixmap_hot(10),
    ZGuiGraphicsView::m_pixmap_hot_gap(2),
    ZGuiGraphicsView::m_tooltip_offset(5),
    ZGuiGraphicsView::m_tooltip_padding(2);

ZGuiGraphicsView::ZGuiGraphicsView(QWidget* parent)
    : QGraphicsView(parent),
      Util::InstanceCounter<ZGuiGraphicsView>(),
      Util::ClassName<ZGuiGraphicsView>(),
      m_scale_bar(Q_NULLPTR),
      m_scale_relative(ZGScaleRelativeType::Positive),
      m_view_orientation(ZGViewOrientationType::Right),
      m_scale_sequence(1.0),
      m_scale_other(1.0),
      m_is_current(false),
      m_is_aa(false),
      m_drag_mode(DraggingMode::None),
      m_move_start(),
      m_move_old(),
      m_move_end(),
      m_cursor(),
      m_cursor_temp(),
      m_cursor_color(Qt::black),
      m_drag_color(Qt::gray),
      m_tooltip_color(255,255,255,100),
      m_feature_outline(Qt::red),
      m_feature_fill(255,255,255,100),
      m_rect_pos(),
      m_feature_rect()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiGraphicsView::ZGuiGraphicsView() ; can not create instance without QApplication existing")) ;

    // this works and puts stuff in the correct place...
    if (!(m_scale_bar = ZGuiWidgetOverlayScale::newInstance(this)))
        throw std::runtime_error(std::string("ZGuiGraphicsView::ZGuiGraphicsView() ; can not create instance of scale bar...")) ;
    m_scale_bar->setAttribute(Qt::WA_TransparentForMouseEvents) ;
    m_scale_bar->show() ;
    m_scale_bar->setViewOrientation(m_view_orientation) ;
    m_scale_bar->setRelativePosition(m_scale_relative) ;

    QScrollBar *bar = horizontalScrollBar() ;
    if (bar)
    {
        connect(bar, SIGNAL(valueChanged(int)), this, SLOT(slot_scale_change())) ;
    }
    if ((bar = verticalScrollBar()))
    {
        connect(bar, SIGNAL(valueChanged(int)), this, SLOT(slot_scale_change())) ;
    }

    // some general settings that we should pretty much always use
    setResizeAnchor(NoAnchor) ;
    setRenderHint(QPainter::Antialiasing, false) ;
    setDragMode(QGraphicsView::NoDrag) ;
    setOptimizationFlag(QGraphicsView::DontSavePainterState) ;
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate) ;
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse) ;

    //setFrameDrawing() ;
    //setBordersStyle() ;
    setCursorPixmap() ;

    computeAndApplyMatrix() ;
}

ZGuiGraphicsView::~ZGuiGraphicsView()
{
}

ZGuiGraphicsView* ZGuiGraphicsView::newInstance(QWidget* parent)
{
    ZGuiGraphicsView *view = Q_NULLPTR ;

    try
    {
        view = new ZGuiGraphicsView(parent) ;
    }
    catch (...)
    {
        view = Q_NULLPTR ;
    }

    if (view)
    {
        std::stringstream str ;
        str << view ;
        view->setObjectName(m_name_base + QString::fromStdString(str.str())) ;
    }

    return view ;
}

void ZGuiGraphicsView::setDragColor(const QColor &color)
{
    if (m_drag_color != color)
    {
        m_drag_color = color ;
    }
}

void ZGuiGraphicsView::setCursorColor(const QColor &color)
{
    if (m_cursor_color != color)
    {
        m_cursor_color = color ;
    }
}

void ZGuiGraphicsView::setFeatureOutlineColor(const QColor &color)
{
    if (m_feature_outline != color)
    {
        m_feature_outline = color ;
    }
}

bool ZGuiGraphicsView::isHorizontal() const
{
    return (m_view_orientation == ZGViewOrientationType::Left || m_view_orientation == ZGViewOrientationType::Right) ;
}

bool ZGuiGraphicsView::isVertical() const
{
    return (m_view_orientation == ZGViewOrientationType::Down || m_view_orientation == ZGViewOrientationType::Up) ;
}

void ZGuiGraphicsView::setCursorPixmap()
{
    QPixmap pixmap(m_pixmap_size, m_pixmap_size) ;
    pixmap.fill(Qt::transparent) ;
    QBitmap bitmap(QSize(m_pixmap_size, m_pixmap_size)) ;
    bitmap.clear() ;
    pixmap.setMask(bitmap) ;
    QPainter painter(&pixmap);
    QPen pen ;
    pen.setColor(m_cursor_color) ;
    painter.setPen(pen) ;

    painter.drawLine(QPoint(m_pixmap_hot, 0), QPoint(m_pixmap_hot, m_pixmap_hot-m_pixmap_hot_gap)) ;
    painter.drawLine(QPoint(m_pixmap_hot, m_pixmap_hot+m_pixmap_hot_gap), QPoint(m_pixmap_hot, m_pixmap_size)) ;

    painter.drawLine(QPoint(0, m_pixmap_hot), QPoint(m_pixmap_hot-m_pixmap_hot_gap, m_pixmap_hot)) ;
    painter.drawLine(QPoint(m_pixmap_hot+m_pixmap_hot_gap, m_pixmap_hot), QPoint(m_pixmap_size, m_pixmap_hot)) ;

    m_cursor = QCursor(pixmap, m_pixmap_hot, m_pixmap_hot) ;
}


void ZGuiGraphicsView::computeAndApplyMatrix()
{
    if (std::isfinite(m_scale_sequence) && m_scale_sequence > 0.0 && std::isfinite(m_scale_other) && m_scale_other > 0.0)
    {
        QTransform transform ;
        switch (m_view_orientation)
        {
        case ZGViewOrientationType::Right:
        {
            //rotate(0.0) ;
            break ;
        }
        case ZGViewOrientationType::Left:
        {
            transform.rotate(180.0) ;
            break ;
        }
        case ZGViewOrientationType::Down:
        {
            transform.rotate(90.0)  ;
            break ;
        }
        case ZGViewOrientationType::Up:
        {
            transform.rotate(270.0) ;
            break ;
        }
        default:
            break ;
        }
        transform.scale(m_scale_sequence, m_scale_other) ;
        setTransform(transform) ;
        if (m_scale_bar)
        {
            m_scale_bar->setViewOrientation(m_view_orientation) ;
        }
        slot_scale_change();
    }

#ifndef QT_NO_DEBUG
    QPolygonF p = mapToScene(QRect(0, 0, 1, 1)) ;
    qDebug() << "ZGuiGraphicsView::computeAndApplyMatrix() ; " << m_scale_sequence << m_scale_other << 1.0/p.boundingRect().width() << 1.0/p.boundingRect().height() ;
#endif
}

// this is zooming in sequence coordinate only
void ZGuiGraphicsView::zoomToRange(const qreal &start, const qreal &end)
{
    if (!scene() || !viewport() || !std::isfinite(start) || !std::isfinite(end) || start >= end )
        return ;

    qreal d = 0.0,
            delta = end - start ;
    if (isHorizontal())
    {
        d = viewport()->width() ;
    }
    else if (isVertical())
    {
        d = viewport()->height() ;
    }
    m_scale_sequence = d / delta ;

    if (std::isfinite(m_scale_sequence) && m_scale_sequence > 0.0)
    {
        QPointF point = viewCentre() ;
        point.setX(start+0.5*delta);
        computeAndApplyMatrix() ;
        centerOn(point) ;
        emit signal_zoom_changed(1.0/m_scale_sequence) ;
    }
}

QPointF ZGuiGraphicsView::viewCentre() const
{
    QPointF point ;
    if (viewport())
    {
        point = mapToScene(0.5*static_cast<qreal>(viewport()->width()),
                           0.5*static_cast<qreal>(viewport()->height())) ;
    }
    return point ;
}


void ZGuiGraphicsView::setCurrent(bool flag)
{
    m_is_current = flag ;
    setBordersStyle() ;
}

void ZGuiGraphicsView::setBordersStyle()
{
    if (m_is_current)
    {
        setStyleSheet(m_style_sheet_current) ;
    }
    else
    {
        setStyleSheet(m_style_sheet_normal) ;
    }
}

void ZGuiGraphicsView::enterEvent(QEvent *)
{
    m_cursor_temp = QCursor(cursor()) ;
    setCursor(m_cursor) ;
}

void ZGuiGraphicsView::leaveEvent(QEvent *)
{
    m_drag_mode = DraggingMode::None ;
    setCursor(m_cursor_temp) ;
}

void ZGuiGraphicsView::contextMenuEvent(QContextMenuEvent *event)
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiGraphicsView::contextMenuEvent() : " << this << mapToScene(event->pos()) ;
#endif
    emit signal_contextMenuEvent(mapToScene(event->pos())) ;
    //QGraphicsView::contextMenuEvent(event) ;
}

// when this is called the item already has the new size
void ZGuiGraphicsView::resizeEvent(QResizeEvent *event)
{
//    std::pair<qreal,qreal> size_data = getStartEnd(event->size()) ;
//    m_scale_bar->setStartEnd(size_data.first, size_data.second) ;
//    m_scale_bar->setSizeAndPosition(event->size()) ;
    slot_scale_change() ;
    QGraphicsView::resizeEvent(event) ;
}


void ZGuiGraphicsView::mousePressEvent(QMouseEvent *event)
{
    //
    // This implements the following behaviour:
    // Left click:
    //                  <no-modifier>      select one item                          (default behaviour)
    //                  <control>          add or remove item from selection        (default behaviour)
    //                  <shift>            rubber band area/group selection
    //                  <control-shift>    initiate dragging a feature
    // Middle click:
    //                  <no-modifier>      zooming
    //                  <shift>            ruler
    // Right click is used for contextMenu events and is sent to the handler for those (But not
    // sent off to the scene. This is because the scene automatically clears the selection when
    // receiving this event, but I want to preserve it).
    //
    Qt::KeyboardModifiers modifiers = event->modifiers() ;
    m_move_end = m_move_old = m_move_start = event->pos() ;

    switch (event->button())
    {

    case Qt::LeftButton:
    {
//        if (modifiers.testFlag(Qt::NoModifier))
//        {
//            // default behaviour
//        }
//        else if (modifiers.testFlag(Qt::ControlModifier) && !modifiers.testFlag(Qt::ShiftModifier))
//        {
//            // default behaviour
//        }
        if (!modifiers.testFlag(Qt::ControlModifier) && modifiers.testFlag(Qt::ShiftModifier))
        {
            m_drag_mode = DraggingMode::SelectionRect ;
        }
        else if (modifiers.testFlag(Qt::ControlModifier) && modifiers.testFlag(Qt::ShiftModifier))
        {
            QGraphicsItem *item = scene()->itemAt(mapToScene(event->pos()), transform()) ;
            if (item && item->isEnabled() && item->flags().testFlag(QGraphicsItem::ItemIsSelectable))
            {
                m_feature_rect = mapFromScene(item->sceneBoundingRect()).boundingRect() ;
                m_feature_rect.setWidth(m_feature_rect.width()-1) ;
                m_feature_rect.setHeight(m_feature_rect.height()-1) ;
                m_drag_mode = DraggingMode::Feature ;
            }
        }
        else
        {
            QGraphicsView::mousePressEvent(event) ;
        }

        break ;
    }

    case Qt::MiddleButton:
    {

        if (modifiers.testFlag(Qt::NoModifier))
        {
            m_drag_mode = DraggingMode::Zooming ;
        }
        else if (!modifiers.testFlag(Qt::ControlModifier) && modifiers.testFlag(Qt::ShiftModifier))
        {
            m_drag_mode = DraggingMode::Ruler ;
        }

        break ;
    }


    default:
    {
        QGraphicsView::mousePressEvent(event) ;
        break ;
    }

    }

    // update here triggers a redraw
    update() ;
    emit signal_requestMadeCurrent();
}

void ZGuiGraphicsView::paintEvent(QPaintEvent* event)
{
    QGraphicsView::paintEvent(event) ;
    switch (m_drag_mode)
    {

    case DraggingMode::Zooming:
    {
        if (m_move_start != m_move_end && viewport())
        {
            QPainter painter(viewport()) ;
            QPen pen ;
            pen.setColor(m_drag_color) ;
            painter.setPen(pen) ;
            if (isHorizontal())
            {
                painter.drawLine(QPointF(m_move_start.x(), 0), QPointF(m_move_start.x(), viewport()->height())) ;
                painter.drawLine(QPointF(m_move_end.x(), 0), QPointF(m_move_end.x(), viewport()->height())) ;
            }
            else if (isVertical())
            {
                painter.drawLine(QPointF(0, m_move_start.y()), QPointF(viewport()->width(), m_move_start.y())) ;
                painter.drawLine(QPointF(0, m_move_end.y()), QPointF(viewport()->width(), m_move_end.y())) ;
            }
        }
        break ;
    }

    case DraggingMode::SelectionRect:
    {
        if (m_move_start != m_move_end && viewport())
        {
            QRectF rect(m_move_start, m_move_end) ;
            QPainter painter(viewport()) ;
            QPen pen ;
            pen.setColor(m_drag_color) ;
            painter.setPen(pen) ;
            painter.drawRect(rect) ;
        }
        break ;
    }

    case DraggingMode::Ruler:
    {
        if (viewport())
        {
            QPainter painter(viewport()) ;
            QFontMetrics metric(painter.font()) ;
            QPen pen ;
            pen.setColor(m_drag_color) ;
            painter.setPen(pen) ;
            QString string ;
            int height = metric.height(),
                    width = 0 ;

            if (isHorizontal())
            {
                painter.drawLine(QPointF(m_move_end.x(), 0), QPointF(m_move_end.x(), viewport()->height())) ;
                QPointF point = mapToScene(m_move_end.x(), 0) ;
                string.setNum(std::floor(point.x())) ;
                width = metric.width(string) ;
            }
            else if (isVertical())
            {
                painter.drawLine(QPointF(0, m_move_end.y()), QPointF(viewport()->width(), m_move_end.y())) ;
                QPointF point = mapToScene(0, m_move_end.y()) ;
                string.setNum(std::floor(point.x())) ;
                width = metric.width(string) ;
            }

            QPointF pos(m_move_end.x()+m_tooltip_offset, m_move_end.y()-m_tooltip_offset) ;
            QRectF rect(pos.x()-m_tooltip_padding, pos.y()-height, width+m_tooltip_padding, height+m_tooltip_padding) ;
            painter.fillRect(rect, m_tooltip_color) ;
            pen.setColor(m_cursor_color) ;
            painter.setPen(pen) ;
            painter.drawRect(rect) ;
            painter.drawText(pos, string) ;

        }
        break ;
    }

    case DraggingMode::Feature:
    {
        QPainter painter(viewport()) ;
        //painter.setRenderHint(QPainter::Antialiasing);
        QPen pen ;
        pen.setColor(m_feature_outline) ;
        painter.setPen(pen) ;
        m_feature_rect.translate(m_move_end.x()-m_move_old.x(), m_move_end.y()-m_move_old.y()) ;
        painter.fillRect(m_feature_rect, m_feature_fill) ;
        painter.drawRect(m_feature_rect) ;
        break ;
    }

    default:
        break ;

    }
}

// is a position outside of the current widget
bool ZGuiGraphicsView::outside(const QPoint &pos) const
{
    if (viewport())
    {
        return pos.x() < 0 || pos.x() >= viewport()->width() || pos.y() < 0 || pos.y() >= viewport()->height() ;
    }
    return true ;
}

void ZGuiGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos() ;
    if (outside(pos))
    {
        m_drag_mode = DraggingMode::None ;
        setCursor(m_cursor_temp) ;
        update() ;
    }
    else
    {
        setCursor(m_cursor) ;
    }
    if (m_drag_mode != DraggingMode::None)
    {
        m_move_old = m_move_end ;
        m_move_end = event->pos() ;
        update() ;
    }
    else
    {
        QGraphicsView::mouseMoveEvent(event) ;
    }
}

void ZGuiGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    m_move_end = event->pos() ;
    switch (m_drag_mode)
    {

    case DraggingMode::Zooming:
    {
        // get coordinates and do some fidding about
        if (m_move_end != m_move_start)
        {
            QPointF start = mapToScene(m_move_start),
                    end = mapToScene(m_move_end) ;
            if (start.x() < end.x())
                zoomToRange(start.x(), end.x()) ;
            else
                zoomToRange(end.x(), start.x()) ;
        }

        // finish up this process
        m_move_end = m_move_start = QPoint() ;
        m_drag_mode = DraggingMode::None ;
        update() ;

        break ;
    }

    case DraggingMode::SelectionRect:
    {
        // do selection business with the area given
        QPoint topLeft, bottomRight ;
        topLeft.setX(std::min(m_move_start.x(), m_move_end.x())) ;
        topLeft.setY(std::min(m_move_start.y(), m_move_end.y())) ;
        bottomRight.setX(std::max(m_move_start.x(), m_move_end.x())) ;
        bottomRight.setY(std::max(m_move_start.y(), m_move_end.y())) ;
        QRect rect(topLeft, bottomRight) ;
        QPainterPath path ;
        path.addRect(mapToScene(rect).boundingRect()) ;
        if (scene())
        {
            scene()->setSelectionArea(path) ;
        }

        // finish up this process
        m_move_end = m_move_start = QPoint() ;
        m_drag_mode = DraggingMode::None ;
        update() ;

        break ;
    }

    case DraggingMode::Ruler:
    {
        // nothing to do here

        // finish up
        m_move_end = m_move_start = QPoint() ;
        m_drag_mode = DraggingMode::None ;
        update() ;

        break ;
    }

    case DraggingMode::Feature:
    {
        // what else to do? this is the end of a feature drag, so
        // we need to query where it went as well...

        // finish up
        m_move_end = m_move_start = QPoint() ;
        m_drag_mode = DraggingMode::None ;
        update() ;

        break ;
    }

    default:
    {
        m_move_end = m_move_start = QPoint() ;
        m_drag_mode = DraggingMode::None ;
        QGraphicsView::mouseReleaseEvent(event) ;
        break ;
    }

    }
}

//
// wow, like archaeology... can probably get rid of this now...
//
void ZGuiGraphicsView::keyPressEvent(QKeyEvent* event)
{
    if (event)
    {
        QString text = event->text().toLower() ;
        if ( (text == QString("h")) || (text == QString("v")) )
        {
            emit signal_split(text) ;
        }
        else if (text == QString("u") )
        {
            emit signal_unsplit() ;
        }
    }
}


//void ZGuiGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
//{
//    QGraphicsView::mouseDoubleClickEvent(event) ;
//}

//
// Bear in mind that when a view receives a mouse click, this
// signal will be sent twice... however, I am not sure if this
// one is necessary, since we are operating the focus on split/unsplit
// events in the right way. Nonetheless, it is a little awkward that
// these two apparently unrelated series of events might have to
// be in sync for other reasons. Not completely satisfactory.
//
void ZGuiGraphicsView::focusInEvent(QFocusEvent* event)
{
//#ifndef QT_NO_DEBUG
//    qDebug() << "ZGuiGraphicsView::focusInEvent() " << this ;
//#endif
    //m_is_current = true ;
    emit signal_requestMadeCurrent() ;
    QGraphicsView::focusInEvent(event) ;
}

//void ZGuiGraphicsView::focusOutEvent(QFocusEvent* event)
//{
//#ifndef QT_NO_DEBUG
//    qDebug() << "ZGuiGraphicsView::focusOutEvent() " << this ;
//#endif
    //emit signal_itemLostFocus() ;
//    QGraphicsView::focusOutEvent(event) ;
//}

ZGuiScene * ZGuiGraphicsView::getGraphicsScene() const
{
    return dynamic_cast<ZGuiScene*> (scene()) ;
}


void ZGuiGraphicsView::setScaleRelative(ZGScaleRelativeType position)
{
    if (!m_scale_bar && position != ZGScaleRelativeType::Invalid)
        return ;
    m_scale_relative = position ;
    if (m_scale_bar && viewport())
    {
        m_scale_bar->setRelativePosition(m_scale_relative) ;
        //m_scale_bar->setSizeAndPosition(viewport()->size()) ;
        slot_scale_change() ;
    }
}

void ZGuiGraphicsView::setViewOrientation(ZGViewOrientationType orientation)
{
    m_view_orientation = orientation ;
    computeAndApplyMatrix();
}

//
// this returns the start and end of the viewport (through size_data)
// in scene coordinates
//
std::pair<qreal, qreal> ZGuiGraphicsView::getStartEnd(const QSize& size_data) const
{
    std::pair<qreal, qreal> result(0.0, 0.0) ;

    switch (m_view_orientation)
    {
    case ZGViewOrientationType::Right:
    {
        // x coordinates
        result.first = mapToScene(0, 0).x() ;
        result.second = mapToScene(size_data.width(), 0).x() ;
        break ;
    }
    case ZGViewOrientationType::Left:
    {
        // x coordinates
        result.first = mapToScene(size_data.width(), 0).x() ;
        result.second = mapToScene(0, 0).x() ;
        break ;
    }
    case ZGViewOrientationType::Up:
    {
        // y coordinates
        result.first = mapToScene(0, size_data.height()).x() ;
        result.second =  mapToScene(0, 0).x() ;
        break ;
    }
    case ZGViewOrientationType::Down:
    {
        // y coordinates
        result.first = mapToScene(0, 0).x() ;
        result.second = mapToScene(0, size_data.height()).x()  ;
        break ;
    }
    default:
        break ;
    }

    return result ;
}

void ZGuiGraphicsView::scaleBarShow()
{
    if (m_scale_bar)
        m_scale_bar->show() ;
}

void ZGuiGraphicsView::scaleBarHide()
{
    if (m_scale_bar)
        m_scale_bar->hide() ;
}

qreal ZGuiGraphicsView::getZoom() const
{
    qreal x = m_scale_sequence ;
    if (x != 0.0)
    {
        return 1.0/x ;
    }
    else
    {
        return 1.0 ;
    }
}

void ZGuiGraphicsView::setAntialiasing(bool flag)
{
    if (m_is_aa != flag)
    {
        m_is_aa = flag ;

        setRenderHint(QPainter::Antialiasing, m_is_aa);
    }
}

void ZGuiGraphicsView::setZoom(const qreal& value)
{
    if (std::isfinite(value) && value > 0.0)
    {
        qreal v = 1.0 / value ;
        if (v != m_scale_sequence)
        {
            computeAndApplyMatrix() ;
            slot_scale_change() ;
            emit signal_zoom_changed(v) ;
        }
    }

}

////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////


void ZGuiGraphicsView::slot_hsplit()
{
    emit signal_split(QString("h")) ;
}

void ZGuiGraphicsView::slot_vsplit()
{
    emit signal_split(QString("v")) ;
}

void ZGuiGraphicsView::slot_unsplit()
{
    emit signal_unsplit() ;
}


//
// scale factors here are defined as the number of pixels
// per world coordinate, so zooming in x2 means doubling the
// number of pixels to represent one world coordinate unit,
// i.e. it be twice a big on the display; however, this is
// not so intuitive so the receiver is sent the value in
// units of bp per pixel
//
void ZGuiGraphicsView::slot_zoomin_x2()
{
    m_scale_sequence *= 2.0 ;

    computeAndApplyMatrix();

    slot_scale_change() ;
    emit signal_zoom_changed(1.0/m_scale_sequence) ;
}

void ZGuiGraphicsView::slot_zoomout_x2()
{
    m_scale_sequence *= 0.5 ;

    computeAndApplyMatrix();

    slot_scale_change() ;
    emit signal_zoom_changed(1.0/m_scale_sequence) ;
}

// argument is bp (world coordinates) per pixel...
void ZGuiGraphicsView::slot_zoom_level(const qreal& value)
{
    setZoom(value) ;
}

void ZGuiGraphicsView::slot_scale_change()
{
    if (m_scale_bar && viewport())
    {
        QSize size = viewport()->size() ;
        std::pair<qreal,qreal> size_data = getStartEnd(size) ;
        m_scale_bar->setStartEnd(size_data.first, size_data.second) ;
        m_scale_bar->setSizeAndPosition(size) ;
    }
}


} // namespace GUI

} // namespace ZMap

