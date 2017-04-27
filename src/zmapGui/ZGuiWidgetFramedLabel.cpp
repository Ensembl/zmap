#include "ZGuiWidgetFramedLabel.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetFramedLabel>::m_name("ZGuiWidgetFramedLabel") ;

ZGuiWidgetFramedLabel* ZGuiWidgetFramedLabel::newInstance(QWidget *parent)
{
    ZGuiWidgetFramedLabel * thing = Q_NULLPTR ;

    try
    {
        thing = new (std::nothrow) ZGuiWidgetFramedLabel(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

ZGuiWidgetFramedLabel::ZGuiWidgetFramedLabel(QWidget* parent)
    : QLabel(parent),
      Util::InstanceCounter<ZGuiWidgetFramedLabel>(),
      Util::ClassName<ZGuiWidgetFramedLabel>()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetFramedLabel::ZGuiWidgetFramedLabel() ; can not create instance ")) ;

    // default frame settings...
    setFrameStyle(Box | Plain) ;
    setMargin(0) ;
}

ZGuiWidgetFramedLabel::~ZGuiWidgetFramedLabel()
{
}

} // namespace GUI

} // namespace ZMap
