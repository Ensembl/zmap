#include "ZGuiWidgetComboPreferences.h"
#include <stdexcept>
#include <sstream>

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

Q_DECLARE_METATYPE(ZMap::GUI::ZGPreferencesType)

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetComboPreferences>::m_name("ZGuiWidgetComboPreferences") ;
const std::vector<ZGPreferencesType> ZGuiWidgetComboPreferences::m_preferences_register
{
    ZGPreferencesType::Blixem,
    ZGPreferencesType::Display
} ;
const int ZGuiWidgetComboPreferences::m_preferences_id(qMetaTypeId<ZGPreferencesType>()) ;

ZGuiWidgetComboPreferences::ZGuiWidgetComboPreferences(QWidget* parent )
    : QComboBox(parent),
      Util::InstanceCounter<ZGuiWidgetComboPreferences>(),
      Util::ClassName<ZGuiWidgetComboPreferences>(),
      m_preferences_map()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetComboPreferences::ZGuiWidgetComboPreferences() ; call to canCreate() failed ")) ;

    QVariant var ;
    ZGPreferencesType preferences ;
    std::stringstream str ;

    int index = 0 ;
    for (auto it = m_preferences_register.begin() ; it != m_preferences_register.end() ; ++it, str.str(""))
    {
        preferences = *it ;
        var.setValue(preferences) ;
        str << preferences ;
        insertItem(index, QString::fromStdString(str.str()), var) ;
        m_preferences_map.insert(std::pair<ZGPreferencesType, int>(preferences, index)) ;
    }
}

ZGuiWidgetComboPreferences::~ZGuiWidgetComboPreferences()
{
}


bool ZGuiWidgetComboPreferences::setPreferences(ZGPreferencesType pref)
{
    bool result = false ;
    auto it = m_preferences_map.find(pref) ;
    if (it != m_preferences_map.end())
    {
        setCurrentIndex(it->second) ;
        result = true ;
    }
    return result ;
}

ZGPreferencesType ZGuiWidgetComboPreferences::getPreferences() const
{
    ZGPreferencesType preferences = ZGPreferencesType::Invalid ;
    QVariant var = currentData() ;
    if (var.canConvert<ZGPreferencesType>() && var.convert(m_preferences_id))
        preferences = var.value<ZGPreferencesType>() ;
    return preferences ;
}

ZGuiWidgetComboPreferences* ZGuiWidgetComboPreferences::newInstance(QWidget* parent)
{
    ZGuiWidgetComboPreferences* thing = Q_NULLPTR ;

    try
    {
        thing = new ZGuiWidgetComboPreferences(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

} // namespace GUI

} // namespace ZMap

