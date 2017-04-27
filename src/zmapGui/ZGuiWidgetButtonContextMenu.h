#ifndef ZGUIWIDGETBUTTONCONTEXTMENU_H
#define ZGUIWIDGETBUTTONCONTEXTMENU_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <cstddef>
#include <string>
#include <QApplication>
#include <QPushButton>

//
// Button with a context menu. Well, not exactly. What it does is
// emit a signal on receipt of a context menu event, and it
// is then up to the handler (most likely the parent of the button)
// to handle it.
//

namespace ZMap
{

namespace GUI
{


class ZGuiWidgetButtonContextMenu : public QPushButton,
        public Util::InstanceCounter<ZGuiWidgetButtonContextMenu>,
        public Util::ClassName<ZGuiWidgetButtonContextMenu>
{

    Q_OBJECT

public:
    static ZGuiWidgetButtonContextMenu* newInstance(QWidget* parent = Q_NULLPTR ) ;

    ~ZGuiWidgetButtonContextMenu() ;

signals:
    void signal_contextMenuEvent() ;

protected:

    void contextMenuEvent(QContextMenuEvent *) Q_DECL_OVERRIDE ;

private:

    ZGuiWidgetButtonContextMenu(QWidget * parent = Q_NULLPTR );
    ZGuiWidgetButtonContextMenu(const ZGuiWidgetButtonContextMenu&) = delete ;
    ZGuiWidgetButtonContextMenu& operator=(const ZGuiWidgetButtonContextMenu &) = delete ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIWIDGETBUTTONCONTEXTMENU_H
