#ifndef ZGUIWIDGETBUTTONMOUSESENSITIVE_H
#define ZGUIWIDGETBUTTONMOUSESENSITIVE_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <cstddef>
#include <string>
#include <QApplication>
#include <QPushButton>

//
// Button that also accepts middle and right clicks.
//

namespace ZMap
{

namespace GUI
{

class ZGuiWidgetButtonMouseSensitive : public QPushButton,
        public Util::InstanceCounter<ZGuiWidgetButtonMouseSensitive>,
        public Util::ClassName<ZGuiWidgetButtonMouseSensitive>
{

    Q_OBJECT

public:
    static ZGuiWidgetButtonMouseSensitive* newInstance(QWidget* parent = Q_NULLPTR) ;

    ~ZGuiWidgetButtonMouseSensitive() ;

signals:

    void clickedMiddle(QMouseEvent *event) ;
    void clickedRight(QMouseEvent* event) ;

protected:

    //void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE ;
    void mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE ;

private:

    bool inside(const QPoint pos) const ;

    ZGuiWidgetButtonMouseSensitive(QWidget* parent= Q_NULLPTR);
    ZGuiWidgetButtonMouseSensitive(const ZGuiWidgetButtonMouseSensitive&) = delete ;
    ZGuiWidgetButtonMouseSensitive& operator=(const ZGuiWidgetButtonMouseSensitive& ) = delete ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIWIDGETBUTTONMOUSESENSITIVE_H
