#include "ZGStyleData.h"
#include <stdexcept>
#ifndef QT_NO_DEBUG
#include <QDebug>
#include <sstream>
#endif


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGStyleData>::m_name("ZGStyleData") ;

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGStyleData& data)
{
    std::stringstream str ;
    std::set<std::string> featuresets = data.getFeaturesetNames() ;
    for (std::set<std::string>::const_iterator it = featuresets.begin() ; it != featuresets.end() ; ++it)
        str << *it << " " ;
    dbg.nospace() << QString::fromStdString(data.name())
                  << "(" << data.getID() << ","
                  << data.getIDParent() << ","
                  << QString::fromStdString(data.getStyleName()) << ","
                  << data.getColorFill() << ","
                  << data.getColorBorder() << ","
                  << data.getIsStranded() << ", "
                  << data.getFeaturesetNumber() << ", "
                  << QString::fromStdString(str.str())
                  << ")" ;
    return dbg.space() ;
}
#endif


ZGStyleData::ZGStyleData()
    : Util::InstanceCounter<ZGStyleData>(),
      Util::ClassName<ZGStyleData>(),
      m_id(0),
      m_id_parent(0),
      m_style_name(),
      m_color_fill(),
      m_color_border(),
      m_is_stranded(false),
      m_featureset_names()
{
}

ZGStyleData::ZGStyleData(ZInternalID id)
    : Util::InstanceCounter<ZGStyleData>(),
      Util::ClassName<ZGStyleData>(),
      m_id(id),
      m_id_parent(0),
      m_style_name(),
      m_color_fill(),
      m_color_border(),
      m_is_stranded(false),
      m_featureset_names()
{
}

ZGStyleData::ZGStyleData(ZInternalID id,
                         ZInternalID id_parent,
                         const std::string &style_name,
                         const QColor &color_fill,
                         const QColor &color_border,
                         bool stranded )
    : Util::InstanceCounter<ZGStyleData>(),
      Util::ClassName<ZGStyleData>(),
      m_id(id),
      m_id_parent(id_parent),
      m_style_name(style_name),
      m_color_fill(color_fill),
      m_color_border(color_border),
      m_is_stranded(stranded),
      m_featureset_names()
{
}

ZGStyleData::ZGStyleData(const ZGStyleData &data)
    : Util::InstanceCounter<ZGStyleData>(),
      Util::ClassName<ZGStyleData>(),
      m_id(data.m_id),
      m_id_parent(data.m_id_parent),
      m_style_name(data.m_style_name),
      m_color_fill(data.m_color_fill),
      m_color_border(data.m_color_border),
      m_is_stranded(data.m_is_stranded),
      m_featureset_names(data.m_featureset_names)
{
}

ZGStyleData& ZGStyleData::operator=(const ZGStyleData& data)
{
    if (this != &data)
    {
        m_id = data.m_id ;
        m_id_parent = data.m_id_parent ;
        m_style_name = data.m_style_name ;
        m_color_fill = data.m_color_fill ;
        m_color_border = data.m_color_border ;
        m_is_stranded = data.m_is_stranded ;
        m_featureset_names = data.m_featureset_names ;
    }
    return *this ;
}

ZGStyleData::~ZGStyleData()
{
}




bool ZGStyleData::setID(ZInternalID id)
{
    if (!id)
        return false ;
    m_id = id ;
    return true ;
}

bool ZGStyleData::setIDParent(ZInternalID id)
{
    if (!id)
        return false ;
    m_id_parent = id ;
    return true ;
}


void ZGStyleData::setStyleName(const std::string &data)
{
    m_style_name = data ;
}

void ZGStyleData::setColorFill(const QColor &col)
{
    m_color_fill = col ;
}

void ZGStyleData::setColorBorder(const QColor &col)
{
    m_color_border = col ;
}

void ZGStyleData::setFeaturesetNames(const std::set<std::string> & featureset_names)
{
    m_featureset_names = featureset_names ;
}

} // namespace GUI

} // namespace ZMap

