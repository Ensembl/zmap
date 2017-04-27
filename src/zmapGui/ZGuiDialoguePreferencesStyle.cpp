#include "ZGuiDialoguePreferencesStyle.h"


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiDialoguePreferencesStyle>::m_name("ZGuiDialoguePreferencesStyle") ;

ZGuiDialoguePreferencesStyle::ZGuiDialoguePreferencesStyle()
    : QProxyStyle(),
      Util::InstanceCounter<ZGuiDialoguePreferencesStyle>(),
      Util::ClassName<ZGuiDialoguePreferencesStyle>()
{
}

ZGuiDialoguePreferencesStyle::~ZGuiDialoguePreferencesStyle()
{
}

ZGuiDialoguePreferencesStyle* ZGuiDialoguePreferencesStyle::newInstance()
{
    ZGuiDialoguePreferencesStyle* thing = Q_NULLPTR ;

    try
    {
        thing = new ZGuiDialoguePreferencesStyle;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

int ZGuiDialoguePreferencesStyle::styleHint( StyleHint hint,
               const QStyleOption * option,
               const QWidget * widget,
               QStyleHintReturn * returnData) const
{
    if ( hint == SH_DrawMenuBarSeparator )
        return 1 ;
    else
        return QProxyStyle::styleHint(hint, option, widget, returnData);
}

} // namespace GUI

} // namespace ZMap

