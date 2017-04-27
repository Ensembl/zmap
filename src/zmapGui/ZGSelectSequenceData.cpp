#include "ZGSelectSequenceData.h"
#include <sstream>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGSelectSequenceData>::m_name("ZGSelectSequenceData") ;

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGSelectSequenceData& data)
{
    std::stringstream str ;
    //dbg.nospace() << data.someBitOfDataOrOther() ;
    std::set<std::pair<std::string, ZGSourceType> > sources = data.getSources() ;
    str << data.name() << "(" ;
    for (auto it = sources.begin() ; it != sources.end() ; ++it )
    {
        str << it->first << "," << it->second << "," ;
    }
    str << data.getSequence() << "," << data.getConfigFile() << "," << data.getStart() << "," << data.getEnd() << ")" ;
    dbg.nospace() << QString::fromStdString(str.str()) ;
    return dbg.space() ;
}
#endif


ZGSelectSequenceData::ZGSelectSequenceData()
    : Util::InstanceCounter<ZGSelectSequenceData>(),
      Util::ClassName<ZGSelectSequenceData>(),
      m_sources(),
      m_sequence(),
      m_config_file(),
      m_start(),
      m_end()
{
}

ZGSelectSequenceData::ZGSelectSequenceData(const std::set<std::pair<std::string, ZGSourceType> > &sources,
                                           const std::string &sequence,
                                           const std::string &config_file,
                                           uint32_t start,
                                           uint32_t end)
    : Util::InstanceCounter<ZGSelectSequenceData>(),
      Util::ClassName<ZGSelectSequenceData>(),
      m_sources(sources),
      m_sequence(sequence),
      m_config_file(config_file),
      m_start(start),
      m_end(end)
{
}

ZGSelectSequenceData::ZGSelectSequenceData(const ZGSelectSequenceData& data)
    : Util::InstanceCounter<ZGSelectSequenceData>(),
      Util::ClassName<ZGSelectSequenceData>(),
      m_sources(data.m_sources),
      m_sequence(data.m_sequence),
      m_config_file(data.m_config_file),
      m_start(data.m_start),
      m_end(data.m_end)
{
}



ZGSelectSequenceData& ZGSelectSequenceData::operator=(const ZGSelectSequenceData& data)
{
    if (this != &data)
    {
        m_sources = data.m_sources ;
        m_sequence = data.m_sequence ;
        m_config_file = data.m_config_file ;
        m_start = data.m_start ;
        m_end = data.m_end ;
    }
    return *this ;
}

ZGSelectSequenceData::~ZGSelectSequenceData()
{
}

void ZGSelectSequenceData::setSources(const std::set<std::pair<std::string, ZGSourceType> > &data)
{
    m_sources = data ;
}

void ZGSelectSequenceData::setSequence(const std::string &data)
{
    m_sequence = data ;
}

void ZGSelectSequenceData::setConfigFile(const std::string &data)
{
    m_config_file = data ;
}

void ZGSelectSequenceData::setStart(uint32_t start)
{
    m_start = start ;
}

void ZGSelectSequenceData::setEnd(uint32_t end)
{
    m_end = end ;
}

} // namespace GUI

} // namespace ZMap
