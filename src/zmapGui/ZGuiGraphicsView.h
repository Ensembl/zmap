/*  File: ZGuiGraphicsView.h
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

#ifndef ZGUIGRAPHICSVIEW_H
#define ZGUIGRAPHICSVIEW_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGViewOrientationType.h"
#include "ZGScalePositionType.h"
#include "ZGScaleRelativeType.h"
#include <QApplication>
#include <QGraphicsView>
#include <QTransform>
#include <QPointF>
#include <QPainter>
#include <QPen>
#include <QCursor>
#include <cstddef>
#include <string>
#include <utility>

namespace ZMap
{

namespace GUI
{

class ZGuiScene ;
class ZGuiWidgetOverlayScale ;

class ZGuiGraphicsView : public QGraphicsView,
        public Util::InstanceCounter<ZGuiGraphicsView>,
        public Util::ClassName<ZGuiGraphicsView>
{
    Q_OBJECT

public:
    static ZGuiGraphicsView* newInstance(QWidget *parent = Q_NULLPTR) ;

    ~ZGuiGraphicsView() ;

    ZGuiScene* getGraphicsScene() const ;

    qreal getZoom() const ;
    void setZoom(const qreal& value) ;
    void setCurrent(bool) ;
    void setScaleRelative(ZGScaleRelativeType position) ;
    void setViewOrientation(ZGViewOrientationType orientation) ;
    ZGViewOrientationType getViewOrientation() const {return m_view_orientation ; }
    bool isHorizontal() const ;
    bool isVertical() const ;
    void scaleBarShow() ;
    void scaleBarHide() ;
    void zoomToRange(const qreal &start, const qreal &end) ;

    bool getAntialiasing() const {return m_is_aa ; }
    void setAntialiasing(bool flag) ;
    void setDragColor(const QColor& color) ;
    QColor getDragColor() const {return m_drag_color; }
    void setCursorColor(const QColor& color) ;
    QColor getCursorColor() const {return m_cursor_color ; }
    void setFeatureOutlineColor(const QColor& color) ;
    QColor getFeatureOutlintColor() const { return m_feature_outline ;  }

public slots:
    void slot_hsplit() ;
    void slot_vsplit() ;
    void slot_unsplit() ;
    void slot_zoomin_x2() ;
    void slot_zoomout_x2() ;
    void slot_zoom_level(const qreal & value) ;
    void slot_scale_change() ;

signals:
    void signal_contextMenuEvent(QPointF) ;
    void signal_zoom_changed(const qreal &value ) ;
    void signal_requestMadeCurrent() ;
    void signal_itemTakenFocus() ;
    void signal_itemLostFocus() ;
    void signal_split(const QString& data) ;
    void signal_unsplit() ;

protected:
    void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE ;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE ;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE ;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE ;
    void enterEvent(QEvent *event) Q_DECL_OVERRIDE ;
    void leaveEvent(QEvent* event) Q_DECL_OVERRIDE ;

    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE ;
    //void mouseDoubleClickEvent(QMouseEvent* event) Q_DECL_OVERRIDE ;
    void focusInEvent(QFocusEvent* event) Q_DECL_OVERRIDE ;
    //void focusOutEvent(QFocusEvent *event) Q_DECL_OVERRIDE ;
    void resizeEvent(QResizeEvent *event)  Q_DECL_OVERRIDE  ;

    void contextMenuEvent(QContextMenuEvent* event) Q_DECL_OVERRIDE ;

private:

    enum class DraggingMode
    {
        None,
        Zooming,
        SelectionRect,
        Ruler,
        Feature
    };

    explicit ZGuiGraphicsView(QWidget *parent = Q_NULLPTR );
    ZGuiGraphicsView(const ZGuiGraphicsView& view) = delete ;
    ZGuiGraphicsView& operator=(const ZGuiGraphicsView& view ) = delete ;

    static const QString m_name_base,
        m_style_sheet_current,
        m_style_sheet_normal  ;
    static const int m_pixmap_size,
        m_pixmap_hot,
        m_pixmap_hot_gap,
        m_tooltip_offset,
        m_tooltip_padding ;

    void setBordersStyle() ;
    void setCursorPixmap() ;
    QPointF viewCentre() const ;
    void computeAndApplyMatrix() ;
    bool outside(const QPoint & point) const ;
    std::pair<qreal, qreal> getStartEnd(const QSize& size_data) const ;

    ZGuiWidgetOverlayScale *m_scale_bar ;
    ZGScaleRelativeType m_scale_relative ;
    ZGViewOrientationType m_view_orientation ;
    qreal m_scale_sequence,
        m_scale_other ;
    bool m_is_current,
        m_is_aa ;
    DraggingMode m_drag_mode ;
    QPoint m_move_start,
        m_move_old,
        m_move_end ;
    QCursor m_cursor,
        m_cursor_temp ;
    QColor m_cursor_color,
        m_drag_color,
        m_tooltip_color,
        m_feature_outline,
        m_feature_fill ;
    QPointF m_rect_pos ;
    QRectF m_feature_rect ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIGRAPHICSVIEW_H
