#include "ZGuiWidgetTest.h"
#include <QPainter>


namespace ZMap
{

namespace GUI
{

ZGuiWidgetTest::ZGuiWidgetTest(QWidget* parent)
    : QWidget(parent),
      Util::InstanceCounter<ZGuiWidgetTest>()
{

}

QSize ZGuiWidgetTest::minimumSizeHint() const
{
    return QSize(100, 20) ;
}

void ZGuiWidgetTest::paintEvent(QPaintEvent *event)
{
    QPainter painter(this) ;
    painter.drawRect(1, 1, width()-2, height()-2) ;
    painter.drawText(20, 20, QString("When I grow up I will be a real widget...")) ;
    QWidget::paintEvent(event) ;
}

} // namespace GUI

} // namespace ZMap

