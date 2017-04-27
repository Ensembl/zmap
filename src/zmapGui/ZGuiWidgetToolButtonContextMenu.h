#ifndef ZGUIWIDGETTOOLBUTTONCONTEXTMENU_H
#define ZGUIWIDGETTOOLBUTTONCONTEXTMENU_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <QApplication>
#include <QToolButton>

//
// Tool button with context menu event. See ZGuiWidgetButtonContextMenu.
//

namespace ZMap
{

namespace GUI
{

class ZGuiWidgetToolButtonContextMenu : public QToolButton,
        public Util::InstanceCounter<ZGuiWidgetToolButtonContextMenu>,
        public Util::ClassName<ZGuiWidgetToolButtonContextMenu>
{

    Q_OBJECT

public:
    static ZGuiWidgetToolButtonContextMenu * newInstance( QWidget* parent = Q_NULLPTR) ;
    ~ZGuiWidgetToolButtonContextMenu() ;

signals:
    void signal_contextMenuEvent() ;

protected:
    void contextMenuEvent(QContextMenuEvent *) Q_DECL_OVERRIDE ;

private:

    ZGuiWidgetToolButtonContextMenu( QWidget* parent = Q_NULLPTR);
    ZGuiWidgetToolButtonContextMenu(const ZGuiWidgetToolButtonContextMenu&) = delete ;
    ZGuiWidgetToolButtonContextMenu& operator=(const ZGuiWidgetToolButtonContextMenu&) = delete ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIWIDGETTOOLBUTTONCONTEXTMENU_H
