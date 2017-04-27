#include "ZItemTest.h"
#include "ZItemGrab.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <new>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif


namespace ZMap
{

namespace GUI
{

const qreal ZItemTest::m_default_height(100.0),
    ZItemTest::m_default_width(250.0),
    ZItemTest::m_min_height(25.0),
    ZItemTest::m_min_width(25.0),
    ZItemTest::m_default_boundary(10.0) ;
const QColor ZItemTest::m_default_color(0, 255, 0) ;

ZItemTest::ZItemTest()
    : QGraphicsItem(),
      Util::InstanceCounter<ZItemTest>(),
      m_width(m_default_width),
      m_height(m_default_height),
      m_boundary(m_default_boundary),
      m_color(m_default_color),
      m_top(Q_NULLPTR),
      m_bottom(Q_NULLPTR),
      m_left(Q_NULLPTR),
      m_right(Q_NULLPTR),
      m_boundary_type(Boundary::Invalid),
      m_move_start(0.0, 0.0),
      m_border_pen()
{
    if (!(m_top = new (std::nothrow) ZItemGrab))
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; could not create top grab ")) ;
    if (!(m_bottom = new (std::nothrow) ZItemGrab))
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; could not create bottom grab")) ;
    if (!(m_left = new (std::nothrow) ZItemGrab))
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; could not create left grab")) ;
    if (!(m_right = new (std::nothrow) ZItemGrab))
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; could not create right grab")) ;
    if (m_boundary >= m_min_height)
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; boundary must be less than min height ")) ;
    if (m_boundary >= m_min_width)
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; boundary must be less than min width ")) ;

    m_top->setParentItem(this) ;
    m_bottom->setParentItem(this) ;
    m_left->setParentItem(this) ;
    m_right->setParentItem(this) ;
    set_grab_positions() ;
    set_grab_hide() ;
    setAcceptHoverEvents(true) ;

    setFlags(ItemIsSelectable) ;

    m_border_pen.setCosmetic(true) ;
}

ZItemTest::ZItemTest(const qreal &width, const qreal &height, const QColor &color)
    : QGraphicsItem(),
      Util::InstanceCounter<ZItemTest>(),
      m_width(width),
      m_height(height),
      m_color(color),
      m_top(Q_NULLPTR),
      m_bottom(Q_NULLPTR),
      m_left(Q_NULLPTR),
      m_right(Q_NULLPTR)
{
    if (width < m_min_width)
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; may not set width less than min ")) ;
    if (height < m_min_height)
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; may not set height less than min ")) ;
    if (!(m_top = new (std::nothrow) ZItemGrab))
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; could not create top grab ")) ;
    if (!(m_bottom = new (std::nothrow) ZItemGrab))
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; could not create bottom grab")) ;
    if (!(m_left = new (std::nothrow) ZItemGrab))
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; could not create left grab")) ;
    if (!(m_right = new (std::nothrow) ZItemGrab))
        throw std::runtime_error(std::string("ZItemTest::ZItemTest() ; could not create right grab")) ;
    set_grab_positions() ;
}

ZItemTest::~ZItemTest()
{
}


QRectF ZItemTest::boundingRect() const
{
    return QRectF(QPointF(0.0, 0.0), QSizeF(m_width, m_height)) ;
}

QPainterPath ZItemTest::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void ZItemTest::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!painter)
        return ;
    if (isSelected())
    {
        painter->fillRect(boundingRect(), m_color.dark()) ;
    }
    else
    {
        painter->fillRect(boundingRect(), m_color);
    }

    // draw a border...
    QPen old_pen = painter->pen() ;
    painter->setPen(m_border_pen) ;
    painter->drawRect(boundingRect()) ;
    painter->setPen(old_pen) ;
}

void ZItemTest::set_grab_positions()
{
    if (m_top)
    {
        m_top->setPos(0.5*m_width, 0.0) ;
    }
    if (m_bottom)
    {
        m_bottom->setPos(0.5*m_width, m_height-m_bottom->height()) ;
    }
    if (m_left)
    {
        m_left->setPos(0.0, 0.5*m_height) ;
    }
    if (m_right)
    {
        m_right->setPos(m_width-m_right->width(), 0.5*m_height) ;
    }
}

void ZItemTest::set_grab_hide()
{
    if (m_top)
    {
        m_top->hide() ;
    }
    if (m_bottom)
    {
        m_bottom->hide() ;
    }
    if (m_left)
    {
        m_left->hide() ;
    }
    if (m_right)
    {
        m_right->hide() ;
    }
}

void ZItemTest::set_grab_show()
{

    if (m_top)
    {
        m_top->show() ;
    }
    if (m_bottom)
    {
        m_bottom->show() ;
    }
    if (m_left)
    {
        m_left->show() ;
    }
    if (m_right)
    {
        m_right->show() ;
    }
}

void ZItemTest::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event)
    {
        if (!isSelected())
            setSelected(true) ;
        QPointF point = event->pos() ;
        if (in_boundary_region(point))
        {
            m_move_start = point ;
        }
    }
}

void ZItemTest::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event)
    {
        if (m_boundary_type != Boundary::Invalid)
        {
            resize(event->pos()) ;
        }
    }
}

void ZItemTest::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event)
    {
        if (m_boundary_type != Boundary::Invalid)
        {
            resize(event->pos()) ;
        }
    }
}

void  ZItemTest::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    set_grab_show() ;
    QGraphicsItem::hoverEnterEvent(event) ;
}

void ZItemTest::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (in_boundary_region(event->pos()))
    {
        switch (m_boundary_type)
        {
        case Boundary::Top:
        {
            m_top->highlight() ;
            m_bottom->lowlight() ;
            m_left->lowlight() ;
            m_right->lowlight() ;
            break ;
        }
        case Boundary::Bottom:
        {
            m_top->lowlight() ;
            m_bottom->highlight() ;
            m_left->lowlight() ;
            m_right->lowlight() ;
            break ;
        }
        case Boundary::Left:
        {
            m_top->lowlight() ;
            m_bottom->lowlight() ;
            m_left->highlight() ;
            m_right->lowlight() ;
            break ;
        }
        case Boundary::Right:
        {
            m_top->lowlight() ;
            m_bottom->lowlight() ;
            m_left->lowlight() ;
            m_right->highlight() ;

            break ;
        }
        default:
            break ;
        }
    }
    else
    {
        m_top->lowlight() ;
        m_bottom->lowlight() ;
        m_left->lowlight() ;
        m_right->lowlight() ;
    }
}

void ZItemTest::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    set_grab_hide() ;
    QGraphicsItem::hoverLeaveEvent(event) ;
}

bool ZItemTest::in_boundary_region(const QPointF & point)
{
    bool result = false ;
    qreal x = point.x(),
            y = point.y() ;
    m_boundary_type = Boundary::Invalid ;
    if (y < m_boundary)
    {
        m_boundary_type = Boundary::Top ;
        result = true ;
    }
    else if (y > m_height-m_boundary)
    {
        m_boundary_type = Boundary::Bottom ;
        result = true ;
    }
    else if (x < m_boundary)
    {
        m_boundary_type = Boundary::Left ;
        result = true ;
    }
    else if (x > m_width-m_boundary)
    {
        m_boundary_type = Boundary::Right ;
        result = true ;
    }
    return result ;
}

bool ZItemTest::resize(const QPointF &event_pos)
{
    bool result = false ;
    qreal delta = 0.0,
            new_value = 0.0 ;
    QPointF old_item_pos = pos() ;
    QPointF new_item_pos = old_item_pos ;

    switch (m_boundary_type)
    {
    case Boundary::Top:
    {
        delta = event_pos.y() - m_move_start.y() ;
        new_value = m_height - delta ;
        if (new_value >= m_min_height)
        {
            new_item_pos.setY(old_item_pos.y() + delta) ;
            setPos(new_item_pos) ;
            m_height = new_value ;
            result = true ;
        }
        break ;
    }
    case Boundary::Bottom:
    {
        delta = event_pos.y() - m_move_start.y() ;
        new_value = m_height + delta ;
        if (new_value >= m_min_height)
        {
            m_height = new_value ;
            result = true ;
        }
        break ;
    }
    case Boundary::Left:
    {
        delta = event_pos.x() - m_move_start.x() ;
        new_value = m_width - delta ;
        if (new_value >= m_min_width)
        {
            new_item_pos.setX(old_item_pos.x() + delta) ;
            setPos(new_item_pos) ;
            m_width = new_value ;
            result = true ;
        }
        break ;
    }
    case Boundary::Right:
    {
        delta = event_pos.x() - m_move_start.x() ;
        new_value = m_width + delta ;
        if (new_value >= m_min_width)
        {
            m_width = new_value ;
            result = true ;
        }
        break ;
    }

    default:
        break ;
    }

    if (result)
    {
        set_grab_positions();
        prepareGeometryChange();
        m_move_start = event_pos + (old_item_pos-new_item_pos) ;
    }

    return result ;
}

void ZItemTest::setColor(const QColor& color)
{
    m_color = color ;
}

bool ZItemTest::setWidth(const qreal &value)
{
    bool result = false ;
    if (value >= m_min_width && value != m_width)
    {
        m_width = value ;
        set_grab_positions() ;
        prepareGeometryChange() ;
        result = true ;
    }
    return result ;
}

bool ZItemTest::setHeight(const qreal &value)
{
    bool result = false ;
    if (value >= m_min_height && value != m_height)
    {
        m_height = value ;
        set_grab_positions() ;
        prepareGeometryChange() ;
        result = true ;
    }
    return result ;
}

} // namespace GUI

} // namespace ZMap
