#include "ZGSearchFeaturesData.h"
#ifndef QT_NO_DEBUG
#include <sstream>
#endif

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGSearchFeaturesData>::m_name("ZGSearchFeaturesData") ;
const size_t ZGSearchFeaturesData:: m_max_string_input_length = 32767 ;


#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGSearchFeaturesData& data)
{
    std::string str ;
    std::set<std::string> str_set ;
    std::stringstream str_stream ;
    dbg.nospace() << QString::fromStdString(data.name()) << "(" ;
    str_set = data.getAlign() ;
    for (auto it = str_set.begin() ; it != str_set.end() ; ++it)
        dbg.nospace() << QString::fromStdString(*it) << "," ;
    str_set = data.getBlock() ;
    for (auto it = str_set.begin() ; it != str_set.end() ; ++it)
        dbg.nospace() << QString::fromStdString(*it) << "," ;
    str_set = data.getTrack() ;
    for (auto it = str_set.begin() ; it != str_set.end() ; ++it)
        dbg.nospace() << QString::fromStdString(*it) << "," ;
    str_set = data.getFeatureset() ;
    for (auto it = str_set.begin() ; it != str_set.end() ; ++it)
        dbg.nospace() << QString::fromStdString(*it) << "," ;
    str = data.getFeature() ;
    dbg.nospace() << QString::fromStdString(str) << "," ;
    str_stream << data.getStrand() << "," << data.getFrame() << "," << data.getStart() << "," << data.getEnd() << "," << data.getLocus() ;
    dbg.nospace() << QString::fromStdString(str_stream.str()) << ")" ;
    return dbg.space() ;
}
#endif


ZGSearchFeaturesData::ZGSearchFeaturesData()
    : Util::InstanceCounter<ZGSearchFeaturesData>(),
      Util::ClassName<ZGSearchFeaturesData>(),
      m_align(),
      m_block(),
      m_track(),
      m_featureset(),
      m_feature(),
      m_strand(ZGStrandType::Invalid),
      m_frame(ZGFrameType::Invalid),
      m_start(),
      m_end(),
      m_locus()
{
}

ZGSearchFeaturesData::ZGSearchFeaturesData(const std::set<std::string> &align,
                                           const std::set<std::string> &block,
                                           const std::set<std::string> &track,
                                           const std::set<std::string> &featureset,
                                           const std::string &feature,
                                           ZGStrandType strand,
                                           ZGFrameType frame,
                                           uint32_t start,
                                           uint32_t end,
                                           bool locus)
    : Util::InstanceCounter<ZGSearchFeaturesData>(),
      Util::ClassName<ZGSearchFeaturesData>(),
      m_align(),
      m_block(),
      m_track(),
      m_featureset(),
      m_feature(feature.substr(0, m_max_string_input_length)),
      m_strand(strand),
      m_frame(frame),
      m_start(start),
      m_end(end),
      m_locus(locus)
{
    if (checkLengths(align))
        m_align = align ;
    if (checkLengths(block))
        m_block = block ;
    if (checkLengths(track))
        m_track = track ;
    if (checkLengths(featureset))
        m_featureset = featureset ;
}

ZGSearchFeaturesData::ZGSearchFeaturesData(const ZGSearchFeaturesData &data)
    : Util::InstanceCounter<ZGSearchFeaturesData>(),
      Util::ClassName<ZGSearchFeaturesData>(),
      m_align(data.m_align),
      m_block(data.m_block),
      m_track(data.m_track),
      m_featureset(data.m_featureset),
      m_feature(data.m_feature),
      m_strand(data.m_strand),
      m_frame(data.m_frame),
      m_start(data.m_start),
      m_end(data.m_end),
      m_locus(data.m_locus)
{
}

ZGSearchFeaturesData& ZGSearchFeaturesData::operator=(const ZGSearchFeaturesData& data)
{
    if (this != &data)
    {
        m_align = data.m_align ;
        m_block = data.m_block ;
        m_track = data.m_track ;
        m_featureset = data.m_featureset ;
        m_feature = data.m_feature ;
        m_strand = data.m_strand ;
        m_frame = data.m_frame ;
        m_start = data.m_start ;
        m_end = data.m_end ;
        m_locus = data.m_locus ;
    }
    return *this ;
}

ZGSearchFeaturesData::~ZGSearchFeaturesData()
{
}

void ZGSearchFeaturesData::setAlign(const std::set<std::string> &data)
{
    if (checkLengths(data))
        m_align = data ;
}

void ZGSearchFeaturesData::setBlock(const std::set<std::string> &data)
{
    if (checkLengths(data))
        m_block = data ;
}

void ZGSearchFeaturesData::setTrack(const std::set<std::string> &data)
{
    if (checkLengths(data))
        m_track = data ;
}

void ZGSearchFeaturesData::setFeatureset(const std::set<std::string> &data)
{
    if (checkLengths(data))
        m_featureset = data ;
}

void ZGSearchFeaturesData::setFeature(const std::string &data)
{
    m_feature = data.substr(0, m_max_string_input_length) ;
}

void ZGSearchFeaturesData::setStrand(ZGStrandType strand)
{
    m_strand = strand ;
}

void ZGSearchFeaturesData::setFrame(ZGFrameType frame)
{
    m_frame = frame ;
}

void ZGSearchFeaturesData::setStart(uint32_t start)
{
    m_start = start ;
}

void ZGSearchFeaturesData::setEnd(uint32_t end)
{
    m_end = end ;
}

void ZGSearchFeaturesData::setLocus(bool loc)
{
    m_locus = loc ;
}

bool ZGSearchFeaturesData::checkLengths(const std::set<std::string> &data) const
{
    bool result = true ;

    for (auto it = data.begin() ; it != data.end() ; ++it)
        if (it->length() > m_max_string_input_length)
        {
            result = false ;
            break ;
        }

    return result ;
}

} // namespace GUI

} // namespace ZMap
