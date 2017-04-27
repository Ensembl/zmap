#ifndef ZGUIWIDGETCOMBOSTRAND_H
#define ZGUIWIDGETCOMBOSTRAND_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGStrandType.h"

#include <QApplication>
#include <QComboBox>
#include <QString>
#include <cstddef>
#include <string>
#include <map>
#include <vector>

namespace ZMap
{

namespace GUI
{

typedef std::vector<ZGStrandType> ZGuiWidgetComboStrandRegister ;
typedef std::map<ZGStrandType,int,ZGStrandCompare> ZGuiWidgetComboStrandMap ;

class ZGuiWidgetComboStrand : public QComboBox,
        public Util::InstanceCounter<ZGuiWidgetComboStrand>,
        public Util::ClassName<ZGuiWidgetComboStrand>
{   

    Q_OBJECT

public:
    static ZGuiWidgetComboStrand* newInstance(QWidget *parent = Q_NULLPTR) ;

    ~ZGuiWidgetComboStrand() ;

    bool setStrand(ZGStrandType strand) ;
    ZGStrandType getStrand() const ;

private:

    explicit ZGuiWidgetComboStrand(QWidget *parent = Q_NULLPTR) ;
    ZGuiWidgetComboStrand(const ZGuiWidgetComboStrand&) = delete ;
    ZGuiWidgetComboStrand& operator=(const ZGuiWidgetComboStrand&) = delete ;

    static const ZGuiWidgetComboStrandRegister m_strand_register ;
    static const int m_strand_id ;

    ZGuiWidgetComboStrandMap m_strand_map ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIWIDGETCOMBOSTRAND_H
