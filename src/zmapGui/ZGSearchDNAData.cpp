#include "ZGSearchDNAData.h"
#include <stdexcept>
#include <algorithm>
#ifndef QT_NO_DEBUG
#include <sstream>
#endif

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGSearchDNAData>::m_name("ZGuiSearchDNAData") ;
const size_t ZGSearchDNAData::m_max_sequence_length = 32767 ;

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGSearchDNAData& data)
{
    std::stringstream str ;
    str << data.getStrand() ;
    dbg.nospace() << QString::fromStdString(data.name())
                  << "("
                  << QString::fromStdString(data.getSequence()) << ","
                  << QString::fromStdString(str.str()) << ","
                  << data.getStart() << "," << data.getEnd() << ","
                  << data.getMismatches() << "," << data.getBases() << ","
                  << data.getColorForward() << "," << data.getColorReverse() << ","
                  << data.getPreserve()
                  << ")" ;
    return dbg.space() ;
}
#endif


ZGSearchDNAData::ZGSearchDNAData()
    : Util::InstanceCounter<ZGSearchDNAData>(),
      Util::ClassName<ZGSearchDNAData>(),
      m_sequence(),
      m_strand(ZGStrandType::Invalid),
      m_start(),
      m_end(),
      m_mis(),
      m_bas(),
      m_col_forward(),
      m_col_reverse(),
      m_preserve()
{
}

ZGSearchDNAData::ZGSearchDNAData(const std::string & sequence,
                                 ZGStrandType strand,
                                 uint32_t start, uint32_t end, uint32_t mis, uint32_t bas,
                                 const QColor& col_forward,
                                 const QColor& col_reverse,
                                 bool preserve)
    : Util::InstanceCounter<ZGSearchDNAData>(),
      Util::ClassName<ZGSearchDNAData>(),
      m_sequence(sequence.substr(0, m_max_sequence_length)),
      m_strand(strand),
      m_start(start),
      m_end(end),
      m_mis(mis),
      m_bas(bas),
      m_col_forward(col_forward),
      m_col_reverse(col_reverse),
      m_preserve(preserve)
{
}

ZGSearchDNAData::ZGSearchDNAData(const ZGSearchDNAData &data)
    : Util::InstanceCounter<ZGSearchDNAData>(),
      Util::ClassName<ZGSearchDNAData>(),
      m_sequence(data.m_sequence),
      m_strand(data.m_strand),
      m_start(data.m_start),
      m_end(data.m_end),
      m_mis(data.m_mis),
      m_bas(data.m_bas),
      m_col_forward(data.m_col_forward),
      m_col_reverse(data.m_col_reverse),
      m_preserve(data.m_preserve)
{
}

ZGSearchDNAData& ZGSearchDNAData::operator=(const ZGSearchDNAData& data)
{
    if (this != &data)
    {
        m_sequence = data.m_sequence ;
        m_strand = data.m_strand ;
        m_start = data.m_start ;
        m_end = data.m_end ;
        m_mis = data.m_mis ;
        m_bas = data.m_bas ;
        m_col_forward = data.m_col_forward ;
        m_col_reverse = data.m_col_reverse ;
        m_preserve = data.m_preserve ;
    }
    return *this ;
}

ZGSearchDNAData::~ZGSearchDNAData()
{
}

void ZGSearchDNAData::setSequence(const std::string &str)
{
    m_sequence = str.substr(0, m_max_sequence_length) ;
}

void ZGSearchDNAData::setColorForward(const QColor &col)
{
    m_col_forward = col ;
}

void ZGSearchDNAData::setColorReverse(const QColor &col)
{
    m_col_reverse = col ;
}

void ZGSearchDNAData::setStrand(ZGStrandType strand)
{
    m_strand = strand ;
}

void ZGSearchDNAData::setStart(uint32_t start)
{
    m_start = start ;
}

void ZGSearchDNAData::setEnd(uint32_t end)
{
    m_end = end ;
}

void ZGSearchDNAData::setMismatches(uint32_t mis)
{
    m_mis = mis ;
}

void ZGSearchDNAData::setBases(uint32_t bas)
{
    m_bas = bas ;
}

void ZGSearchDNAData::setPreserve(bool set)
{
    m_preserve = set ;
}


} // namespace GUI

} // namespace ZMap

