#ifndef ZITEMTEST_H
#define ZITEMTEST_H

#include "InstanceCounter.h"
#include <cstddef>
#include <string>
#include <QGraphicsItem>
#include <QPen>
//
// Item to test resizing via grabs.
//


namespace ZMap
{

namespace GUI
{

class ZItemGrab ;

class ZItemTest : public QGraphicsItem,
        public Util::InstanceCounter<ZItemTest>
{

public:

    enum class Boundary: unsigned char
    {
        Invalid,
        Top,
        Bottom,
        Left,
        Right
    } ;

    ZItemTest() ;
    ZItemTest(const qreal &width, const qreal &height, const QColor & color) ;
    ~ZItemTest() ;

    QRectF boundingRect() const Q_DECL_OVERRIDE ;
    QPainterPath shape() const Q_DECL_OVERRIDE ;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget* ) Q_DECL_OVERRIDE ;

    QColor getColor() const {return m_color ; }
    void setColor(const QColor& color) ;

    bool setWidth(const qreal &value) ;
    qreal width() const {return m_width ; }

    bool setHeight(const qreal &value) ;
    qreal height() const {return m_height ; }



protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) Q_DECL_OVERRIDE ;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) Q_DECL_OVERRIDE ;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) Q_DECL_OVERRIDE ;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE ;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE ;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE ;

private:

    static const qreal m_default_width,
        m_default_height,
        m_min_height,
        m_min_width,
        m_default_boundary ;
    static const QColor m_default_color ;

    ZItemTest(const ZItemTest& ) = delete ;
    ZItemTest& operator=(const ZItemTest&) = delete ;

    void set_grab_positions() ;
    void set_grab_hide() ;
    void set_grab_show() ;
    bool in_boundary_region(const QPointF & ) ;
    bool resize(const QPointF& event_pos) ;

    qreal m_width,
        m_height,
        m_boundary ;
    QColor m_color ;
    ZItemGrab *m_top,
        *m_bottom,
        *m_left,
        *m_right ;
    Boundary m_boundary_type ;
    QPointF m_move_start ;
    QPen m_border_pen ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZITEMTEST_H
