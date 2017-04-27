#include "ZGPreferencesDisplayData.h"
#ifndef QT_NO_DEBUG
#include <sstream>
#endif

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPreferencesDisplayData>::m_name ("ZGPreferencesDisplayData") ;

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGPreferencesDisplayData& data)
{
    std::stringstream str ;
    str << data.name() << "("
        << data.getShrinkableWindow() << ","
        << data.getHighlightFiltered() << ","
        << data.getEnableAnnotation()
        << ")" ;
    dbg.nospace() << QString::fromStdString(str.str()) ;
    return dbg.space() ;
}
#endif


ZGPreferencesDisplayData::ZGPreferencesDisplayData()
    : Util::InstanceCounter<ZGPreferencesDisplayData>(),
      Util::ClassName<ZGPreferencesDisplayData>(),
      m_shrinkable(),
        m_highlight(),
        m_annotation()
{

}

ZGPreferencesDisplayData::ZGPreferencesDisplayData(bool shrink, bool high, bool annotation)
    : Util::InstanceCounter<ZGPreferencesDisplayData>(),
      Util::ClassName<ZGPreferencesDisplayData>(),
      m_shrinkable(shrink),
      m_highlight(high),
      m_annotation(annotation)
{
}


ZGPreferencesDisplayData::ZGPreferencesDisplayData(const ZGPreferencesDisplayData &data)
    : Util::InstanceCounter<ZGPreferencesDisplayData>(),
      Util::ClassName<ZGPreferencesDisplayData>(),
      m_shrinkable(data.m_shrinkable),
      m_highlight(data.m_highlight),
      m_annotation(data.m_annotation)
{

}

ZGPreferencesDisplayData& ZGPreferencesDisplayData::operator=(const ZGPreferencesDisplayData& data)
{
    if (this != &data)
    {
        m_shrinkable = data.m_shrinkable ;
        m_highlight = data.m_highlight ;
        m_annotation = data.m_annotation ;
     }
    return *this ;
}

ZGPreferencesDisplayData::~ZGPreferencesDisplayData()
{
}



void ZGPreferencesDisplayData::setShrinkableWindow(bool value)
{
    m_shrinkable = value ;
}

void ZGPreferencesDisplayData::setHighlightFiltered(bool value)
{
    m_highlight = value ;
}

void ZGPreferencesDisplayData::setEnableAnnotation(bool value)
{
    m_annotation  = value ;
}

} // namespace GUI

} // namespace ZMap
