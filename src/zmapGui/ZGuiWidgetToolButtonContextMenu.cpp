#include "ZGuiWidgetToolButtonContextMenu.h"
#include "Utilities.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetToolButtonContextMenu>::m_name("ZGuiWidgetToolButtonContextMenu") ;

ZGuiWidgetToolButtonContextMenu* ZGuiWidgetToolButtonContextMenu::newInstance(QWidget *parent)
{
    ZGuiWidgetToolButtonContextMenu * thing = Q_NULLPTR ;

    try
    {
        thing = new (std::nothrow) ZGuiWidgetToolButtonContextMenu(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

ZGuiWidgetToolButtonContextMenu::ZGuiWidgetToolButtonContextMenu(QWidget* parent )
    : QToolButton(parent),
      Util::InstanceCounter<ZGuiWidgetToolButtonContextMenu>(),
      Util::ClassName<ZGuiWidgetToolButtonContextMenu>()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetToolButtonContextMenu::ZGuiWidgetToolButtonContextMenu() ; unable to create instance ")) ;
}

ZGuiWidgetToolButtonContextMenu::~ZGuiWidgetToolButtonContextMenu()
{
}


void ZGuiWidgetToolButtonContextMenu::contextMenuEvent(QContextMenuEvent *)
{
    emit signal_contextMenuEvent() ;
}



} // namespace GUI

} // namespace ZMap
