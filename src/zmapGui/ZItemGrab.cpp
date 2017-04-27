#include "ZItemGrab.h"

#include <QPainter>

namespace ZMap
{

namespace GUI
{

const qreal ZItemGrab::m_default_height(10.0),
    ZItemGrab::m_default_width(10.0),
    ZItemGrab::m_default_height_min(1.0),
    ZItemGrab::m_default_width_min(2.0) ;
const QColor ZItemGrab::m_default_color(255, 0, 0, 127) ;

ZItemGrab::ZItemGrab()
    : QGraphicsItem(),
      Util::InstanceCounter<ZItemGrab>(),
      m_width(m_default_width),
      m_height(m_default_height),
      m_width_min(m_default_width_min),
      m_height_min(m_default_height_min),
      m_color(m_default_color),
      m_color_highlight(Qt::white),
      m_color_use(m_color)
{
}

ZItemGrab::ZItemGrab(const qreal &width, const qreal &height, const QColor & color)
    : QGraphicsItem(),
      Util::InstanceCounter<ZItemGrab>(),
      m_width(width),
      m_height(height),
      m_width_min(m_default_width_min),
      m_height_min(m_default_height_min),
      m_color(color),
      m_color_highlight(Qt::white),
      m_color_use(m_color)
{
    if (m_width < m_width_min)
        throw std::runtime_error(std::string("ZItemGrab::ZItemGrab() ; cannot create instance with width less than minimum ")) ;
    if (m_height < m_height_min)
        throw std::runtime_error(std::string("ZItemGrab::ZItemGrab() ; cannot create instance with height less than minimum ")) ;
}

ZItemGrab::~ZItemGrab()
{
}

QRectF ZItemGrab::boundingRect() const
{
    return QRectF(QPointF(0.0, 0.0), QSizeF(m_width, m_height)) ;
}

QPainterPath ZItemGrab::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void ZItemGrab::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!painter)
        return ;
    painter->fillRect(boundingRect(), m_color_use);
}

void ZItemGrab::setColor(const QColor &color)
{
    m_color = color ;
}

void ZItemGrab::setColorHighlight(const QColor& color)
{
    m_color_highlight = color ;
}

bool ZItemGrab::setWidth(const qreal & value)
{
    if (value < m_width_min || value == m_width )
        return false ;
    m_width = value ;
    prepareGeometryChange() ;
    return true ;
}

bool ZItemGrab::setHeight(const qreal & value)
{
    if (value < m_height_min || value == m_height)
        return false ;
    m_height = value ;
    prepareGeometryChange() ;
    return true ;
}

bool ZItemGrab::setWidthMin(const qreal &value)
{
    if (value <= 0.0 || value == m_width_min )
        return false ;
    m_width_min = value ;
    if (m_width_min > m_width)
    {
        m_width = m_width_min ;
        prepareGeometryChange() ;
    }
    return true ;
}

bool ZItemGrab::setHeightMin(const qreal &value)
{
    if (value <= 0.0 || value == m_height_min )
        return false ;
    m_height_min = value ;
    if (m_height_min > m_height)
    {
        m_height = m_height_min ;
        prepareGeometryChange() ;
    }
    return true ;
}

void ZItemGrab::highlight()
{
    m_color_use = m_color_highlight ;
    update() ;
}

void ZItemGrab::lowlight()
{
    m_color_use = m_color ;
    update() ;
}

} // namespace GUI

} // namespace ZMap

