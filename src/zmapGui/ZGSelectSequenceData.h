#ifndef ZGSELECTSEQUENCEDATA_H
#define ZGSELECTSEQUENCEDATA_H

#include "ZInternalID.h"
#include "ZGSourceType.h"
#include "InstanceCounter.h"
#include "ClassName.h"

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

#include <cstddef>
#include <string>
#include <cstdint>
#include <utility>
#include <set>

//
// Class for  copying data in and out of the SelectSequence dialogue.
//
//


namespace ZMap
{

namespace GUI
{

class ZGSelectSequenceData: public Util::InstanceCounter<ZGSelectSequenceData>,
        public Util::ClassName<ZGSelectSequenceData>
{
public:

    ZGSelectSequenceData();
    ZGSelectSequenceData(const std::set<std::pair<std::string, ZGSourceType> > & sources,
                         const std::string& sequence,
                         const std::string& config_file,
                         uint32_t start, uint32_t end ) ;
    ZGSelectSequenceData(const ZGSelectSequenceData& data) ;
    ZGSelectSequenceData& operator=(const ZGSelectSequenceData& data) ;
    ~ZGSelectSequenceData() ;

    void setSources(const std::set<std::pair<std::string, ZGSourceType> > & data) ;
    std::set<std::pair<std::string, ZGSourceType> > getSources() const {return m_sources ; }

    void setSequence(const std::string & data) ;
    std::string getSequence() const {return m_sequence; }

    void setConfigFile(const std::string& data) ;
    std::string getConfigFile() const {return m_config_file ; }

    void setStart(uint32_t start) ;
    uint32_t getStart() const {return m_start ; }

    void setEnd(uint32_t end) ;
    uint32_t getEnd() const {return m_end ; }

private:

    // also need a collection of sources, so for the moment this is a bit of a hack...
    // at some point, this should be replaced with a class to describe a source and
    // associated data, but ...
    std::set<std::pair<std::string, ZGSourceType> >  m_sources ;
    std::string m_sequence,
        m_config_file ;
    uint32_t m_start,
        m_end ;
} ;



#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGSelectSequenceData& data) ;
#endif

} // namespace GUI

} // namespace ZMap


#endif // ZGSELECTSEQUENCEDATA_H
