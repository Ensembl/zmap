#include "ZGFeatureData.h"
#include <string>
#include <sstream>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGFeatureData>::m_name ("ZGFeatureData") ;

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGFeatureData& data)
{
    std::stringstream str ;
    str << "(" << data.getID() << ","
        << data.getName() << ","
        << data.getBounds() << ","
        << data.getStrand() << ","
        << data.getFrame() << ","
        << data.getQueryBounds() << ","
        << data.getQueryStrand() << ","
        << data.getScore() << ","
        << data.getFeatureset() << ","
        << data.getSource() << ","
        << data.getStyle() << ")";
    dbg.nospace() << QString::fromStdString(data.name())
                  << QString::fromStdString(str.str()) ;
    return dbg.space() ;
}
#endif



ZGFeatureData::ZGFeatureData()
    : Util::InstanceCounter<ZGFeatureData>(),
      Util::ClassName<ZGFeatureData>(),
      m_id(),
      m_feature_name(),
      m_bounds(),
      m_strand(ZGStrandType::Invalid),
      m_frame(ZGFrameType::Invalid),
      m_query_bounds(),
      m_query_strand(ZGStrandType::Invalid),
      m_score(),
      m_featureset(),
      m_source(),
      m_style()
{
}

ZGFeatureData::ZGFeatureData(ZInternalID id)
    : Util::InstanceCounter<ZGFeatureData>(),
      Util::ClassName<ZGFeatureData>(),
      m_id(id),
      m_feature_name(),
      m_bounds(),
      m_strand(ZGStrandType::Invalid),
      m_frame(ZGFrameType::Invalid),
      m_query_bounds(),
      m_query_strand(ZGStrandType::Invalid),
      m_score(),
      m_featureset(),
      m_source(),
      m_style()
{
}

ZGFeatureData::ZGFeatureData(ZInternalID id,
                             const std::string &name,
                             const ZGFeatureBounds &bounds,
                             ZGStrandType strand,
                             ZGFrameType frame,
                             const ZGFeatureBounds &qbounds,
                             ZGStrandType qstrand,
                             const ZGFeatureScore &score,
                             const std::string &featureset,
                             const std::string &source,
                             const std::string &style)
    : Util::InstanceCounter<ZGFeatureData>(),
      Util::ClassName<ZGFeatureData>(),
      m_id(id),
      m_feature_name(name),
      m_bounds(bounds),
      m_strand(strand),
      m_frame(frame),
      m_query_bounds(qbounds),
      m_query_strand(qstrand),
      m_score(score),
      m_featureset(featureset),
      m_source(source),
      m_style(style)
{
}

ZGFeatureData::ZGFeatureData(const ZGFeatureData &data)
    : Util::InstanceCounter<ZGFeatureData>(),
      Util::ClassName<ZGFeatureData>(),
      m_id(data.m_id),
      m_feature_name(data.m_feature_name),
      m_bounds(data.m_bounds),
      m_strand(data.m_strand),
      m_frame(data.m_frame),
      m_query_bounds(data.m_query_bounds),
      m_query_strand(data.m_query_strand),
      m_score(data.m_score),
      m_featureset(data.m_featureset),
      m_source(data.m_source),
      m_style(data.m_style)
{
}

ZGFeatureData& ZGFeatureData::operator=(const ZGFeatureData& data)
{
    if (this != &data)
    {
        m_id = data.m_id ;
        m_feature_name = data.m_feature_name ;
        m_bounds = data.m_bounds ;
        m_strand = data.m_strand ;
        m_frame = data.m_frame ;
        m_query_bounds = data.m_query_bounds ;
        m_query_strand = data.m_query_strand ;
        m_score = data.m_score ;
        m_featureset = data.m_featureset ;
        m_source = data.m_source ;
        m_style = data.m_style ;
    }
    return *this ;
}

ZGFeatureData::~ZGFeatureData()
{
}

} // namespace GUI

} // namespace ZMap

