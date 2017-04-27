#ifndef ZITEMGRAB_H
#define ZITEMGRAB_H

#include "InstanceCounter.h"
#include <QGraphicsItem>
#include <cstddef>

//
// QGraphicsItem to use as a grab.
//


namespace ZMap
{

namespace GUI
{


class ZItemGrab: public QGraphicsItem,
        public Util::InstanceCounter<ZItemGrab>
{

public:

    ZItemGrab() ;
    ZItemGrab(const qreal &width, const qreal &height, const QColor & color) ;
    ~ZItemGrab() ;

    QRectF boundingRect() const Q_DECL_OVERRIDE ;
    QPainterPath shape() const Q_DECL_OVERRIDE ;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget * ) Q_DECL_OVERRIDE ;

    QColor getColor() const {return m_color ; }
    void setColor(const QColor &color) ;
    QColor getColorHighlight() const {return m_color_highlight ; }
    void setColorHighlight(const QColor& color) ;

    bool setWidth(const qreal &) ;
    qreal width() const {return m_width ; }
    bool setWidthMin(const qreal &) ;
    qreal widthMin() const {return m_width_min ; }
    bool setHeight(const qreal &) ;
    qreal height() const {return m_height ; }
    bool setHeightMin(const qreal &) ;
    qreal heightMin() const {return m_height_min ; }

    void highlight() ;
    void lowlight() ;

private:

    static const qreal m_default_width,
        m_default_height,
        m_default_width_min,
        m_default_height_min ;
    static const QColor m_default_color ;

    ZItemGrab(const ZItemGrab& ) = delete ;
    ZItemGrab& operator=(const ZItemGrab& ) = delete ;

    qreal m_width,
        m_height,
        m_width_min,
        m_height_min ;
    QColor m_color,
        m_color_highlight,
        m_color_use ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZITEMGRAB_H
