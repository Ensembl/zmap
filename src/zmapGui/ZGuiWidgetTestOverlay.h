#ifndef ZGUIWIDGETTESTOVERLAY_H
#define ZGUIWIDGETTESTOVERLAY_H

#include "InstanceCounter.h"
#include <QWidget>


namespace ZMap
{

namespace GUI
{

class ZGuiWidgetTestOverlay: public QWidget,
        public Util::InstanceCounter<ZGuiWidgetTestOverlay>
{
    Q_OBJECT

public:
    ZGuiWidgetTestOverlay(QWidget * parent = Q_NULLPTR ) ;

protected:

    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE ;

private:
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIWIDGETTESTOVERLAY_H
