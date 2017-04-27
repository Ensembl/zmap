#ifndef ZGUIWIDGETTEST_H
#define ZGUIWIDGETTEST_H

#include "InstanceCounter.h"
#include <QWidget>
#include <QString>


namespace ZMap
{

namespace GUI
{

class ZGuiWidgetTest: public QWidget,
        public Util::InstanceCounter<ZGuiWidgetTest>
{

public:
    explicit ZGuiWidgetTest(QWidget* parent = Q_NULLPTR) ;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:

    ZGuiWidgetTest(const ZGuiWidgetTest& ) = delete ;
    ZGuiWidgetTest& operator=(const ZGuiWidgetTest& ) = delete ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIWIDGETTEST_H
