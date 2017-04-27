#include "ZGuiWidgetButtonContextMenu.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetButtonContextMenu>::m_name("ZGuiWidgetButtonContextMenu" ) ;

ZGuiWidgetButtonContextMenu* ZGuiWidgetButtonContextMenu::newInstance(QWidget *parent)
{
    ZGuiWidgetButtonContextMenu* thing = Q_NULLPTR ;
    try
    {
        thing = new (std::nothrow) ZGuiWidgetButtonContextMenu(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

ZGuiWidgetButtonContextMenu::ZGuiWidgetButtonContextMenu(QWidget * parent )
    : QPushButton(parent),
      Util::InstanceCounter<ZGuiWidgetButtonContextMenu>(),
      Util::ClassName<ZGuiWidgetButtonContextMenu>()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetButtonContextMenu::ZGuiWidgetButtonContextMenu() ; unable to create instance ")) ;
}

ZGuiWidgetButtonContextMenu::~ZGuiWidgetButtonContextMenu()
{
}

//
// we emit a signal on finding context menu event; the receiver gets the
// position of the event from the cursor position when the slot is called
//
void ZGuiWidgetButtonContextMenu::contextMenuEvent(QContextMenuEvent *)
{
    emit signal_contextMenuEvent() ;
}

} // namespace GUI

} // namespace ZMap
