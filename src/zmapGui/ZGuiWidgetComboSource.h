#ifndef ZGUIWIDGETCOMBOSOURCE_H
#define ZGUIWIDGETCOMBOSOURCE_H

#include <QApplication>
#include <QComboBox>
#include <QString>
#include <cstddef>
#include <string>
#include <map>
#include <vector>

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGSourceType.h"

namespace ZMap
{

namespace GUI
{

typedef std::map<ZGSourceType, int, ZGSourceTypeCompare> ZGuiWidgetComboSourceMap ;
typedef std::vector<ZGSourceType> ZGuiWidgetComboSourceRegister ;

class ZGuiWidgetComboSource : public QComboBox,
        public Util::InstanceCounter<ZGuiWidgetComboSource>,
        public Util::ClassName<ZGuiWidgetComboSource>
{

    Q_OBJECT

public:
    static ZGuiWidgetComboSource* newInstance(QWidget *parent = Q_NULLPTR) ;

    ~ZGuiWidgetComboSource() ;

    bool setSource(ZGSourceType type) ;
    ZGSourceType getSourceType() const ;

private:
    static const ZGuiWidgetComboSourceRegister m_source_register ;
    static const int m_source_id ;

    explicit ZGuiWidgetComboSource(QWidget* parent = Q_NULLPTR) ;
    ZGuiWidgetComboSource(const ZGuiWidgetComboSource& data) = delete ;
    ZGuiWidgetComboSource& operator=(const ZGuiWidgetComboSource&) = delete ;

    ZGuiWidgetComboSourceMap m_source_map ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIWIDGETCOMBOSOURCE_H
