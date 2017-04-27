#include "ZGuiWidgetButtonMouseSensitive.h"
#include <QMouseEvent>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetButtonMouseSensitive>::m_name("ZGuiWidgetButtonMouseSensitive") ;


ZGuiWidgetButtonMouseSensitive* ZGuiWidgetButtonMouseSensitive::newInstance(QWidget* parent)
{
    ZGuiWidgetButtonMouseSensitive* thing = Q_NULLPTR ;
    try
    {
        thing = new ZGuiWidgetButtonMouseSensitive(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

ZGuiWidgetButtonMouseSensitive::ZGuiWidgetButtonMouseSensitive(QWidget* parent)
    : QPushButton(parent),
      Util::InstanceCounter<ZGuiWidgetButtonMouseSensitive>(),
      Util::ClassName<ZGuiWidgetButtonMouseSensitive>()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetButtonMouseSensitive::ZGuiWidgetButtonMouseSensitive() ; unable to create instance ")) ;
}

ZGuiWidgetButtonMouseSensitive::~ZGuiWidgetButtonMouseSensitive()
{
}


//void ZGuiWidgetButtonMouseSensitive::mousePressEvent(QMouseEvent *event)
//{
//    if (event)
//    {
//        Qt::MouseButton button = event->button() ;
//        if (button == Qt::MiddleButton)
//        {
//            emit clickedMiddle(event) ;
//        }
//        else if (button == Qt::RightButton)
//        {
//            emit clickedRight(event) ;
//        }
//    }
//    QPushButton::mousePressEvent(event) ;
//}

void ZGuiWidgetButtonMouseSensitive::mouseReleaseEvent(QMouseEvent* event)
{
    if (event)
    {
        Qt::MouseButton button = event->button() ;
        QPoint pos = event->pos() ;
        if (button == Qt::MiddleButton && inside(pos))
        {
            emit clickedMiddle(event) ;
        }
        else if (button == Qt::RightButton && inside(pos))
        {
            emit clickedRight(event) ;
        }
    }
    QPushButton::mouseReleaseEvent(event) ;
}

bool ZGuiWidgetButtonMouseSensitive::inside(const QPoint pos) const
{
    bool result = false ;
    if (pos.x() > 0 && pos.x() < width() && pos.y() > 0 && pos.y() < height())
        result = true ;
    return result ;
}

} // namespace GUI

} // namespace ZMap
