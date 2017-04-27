#include "ZGuiWidgetTestOverlay.h"
#include <QPaintEvent>
#include <QPainter>


namespace ZMap
{

namespace GUI
{


ZGuiWidgetTestOverlay::ZGuiWidgetTestOverlay(QWidget * parent)
    : QWidget(parent),
      Util::InstanceCounter<ZGuiWidgetTestOverlay>()
{

}

// black dog, er, box
void ZGuiWidgetTestOverlay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this) ;
    painter.drawRect(0, 0, size().width()-1, size().height()-1) ;
    painter.drawRect(2, 2, size().width()-5, size().height()-5) ;
    painter.drawRect(4, 4, size().width()-9, size().height()-9) ;
    QWidget::paintEvent(event) ;
}

} // namespace GUI

} // namespace ZMap
