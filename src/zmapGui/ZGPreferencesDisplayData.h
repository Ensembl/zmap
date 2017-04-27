#ifndef ZGPREFERENCESDISPLAYDATA_H
#define ZGPREFERENCESDISPLAYDATA_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

class ZGPreferencesDisplayData: public Util::InstanceCounter<ZGPreferencesDisplayData>,
        public Util::ClassName<ZGPreferencesDisplayData>
{
public:

    ZGPreferencesDisplayData();
    ZGPreferencesDisplayData(bool shrink, bool high, bool annotation) ;
    ZGPreferencesDisplayData(const ZGPreferencesDisplayData& data) ;
    ZGPreferencesDisplayData& operator=(const ZGPreferencesDisplayData& data) ;
    ~ZGPreferencesDisplayData() ;

    void setShrinkableWindow(bool value) ;
    bool getShrinkableWindow() const {return m_shrinkable ; }

    void setHighlightFiltered(bool value) ;
    bool getHighlightFiltered() const {return m_highlight ; }

    void setEnableAnnotation(bool value) ;
    bool getEnableAnnotation() const {return m_annotation ; }

private:

    bool m_shrinkable,
        m_highlight,
        m_annotation ;
};


#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGPreferencesDisplayData& data) ;
#endif

} // namespace GUI

} // namespace ZMap


#endif // ZGPREFERENCESDISPLAYDATA_H
