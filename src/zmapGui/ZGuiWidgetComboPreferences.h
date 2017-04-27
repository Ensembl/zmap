#ifndef ZGUIWIDGETCOMBOPREFERENCES_H
#define ZGUIWIDGETCOMBOPREFERENCES_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGPreferencesType.h"

#include <QApplication>
#include <QComboBox>
#include <QString>
#include <cstddef>
#include <string>
#include <vector>
#include <map>

namespace ZMap
{

namespace GUI
{

typedef std::vector<ZGPreferencesType> ZGuiWidgetComboPreferencesRegister ;
typedef std::map<ZGPreferencesType, int, ZGDisplayPreferencesCompare> ZGuiWidgetComboPreferencesMap ;

class ZGuiWidgetComboPreferences : public QComboBox,
        public Util::InstanceCounter<ZGuiWidgetComboPreferences>,
        public Util::ClassName<ZGuiWidgetComboPreferences>
{

    Q_OBJECT

public:
    static ZGuiWidgetComboPreferences* newInstance(QWidget* parent = Q_NULLPTR) ;

    ~ZGuiWidgetComboPreferences() ;

    bool setPreferences(ZGPreferencesType pref) ;
    ZGPreferencesType getPreferences() const ;

private:

    explicit ZGuiWidgetComboPreferences(QWidget* parent) ;
    ZGuiWidgetComboPreferences(const ZGuiWidgetComboPreferences&) = delete ;
    ZGuiWidgetComboPreferences& operator=(const ZGuiWidgetComboPreferences& ) = delete ;

    static const ZGuiWidgetComboPreferencesRegister m_preferences_register ;
    static const int m_preferences_id ;

    ZGuiWidgetComboPreferencesMap m_preferences_map ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIWIDGETCOMBOPREFERENCES_H
