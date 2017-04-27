#ifndef ZGSEARCHFEATURESDATA_H
#define ZGSEARCHFEATURESDATA_H

#include "ZInternalID.h"
#include "ZGStrandType.h"
#include "ZGFrameType.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <set>
#include <cstdint>


#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <QColor>

//
// Used to transfer data in and out of the SearchFeatures dialogue.
//


namespace ZMap
{

namespace GUI
{

class ZGSearchFeaturesData: public Util::InstanceCounter<ZGSearchFeaturesData>,
        public Util::ClassName<ZGSearchFeaturesData>
{
public:

    ZGSearchFeaturesData();
    ZGSearchFeaturesData(const std::set<std::string> &align,
                         const std::set<std::string> &block,
                         const std::set<std::string> &track,
                         const std::set<std::string> &featureset,
                         const std::string &feature,
                         ZGStrandType strand,
                         ZGFrameType frame,
                         uint32_t start,
                         uint32_t end,
                         bool locus) ;
    ZGSearchFeaturesData(const ZGSearchFeaturesData& data) ;
    ZGSearchFeaturesData& operator=(const ZGSearchFeaturesData& data) ;
    ~ZGSearchFeaturesData() ;

    void setAlign(const std::set<std::string>& data) ;
    std::set<std::string> getAlign() const {return m_align ; }

    void setBlock(const std::set<std::string>& data) ;
    std::set<std::string> getBlock() const {return m_block ; }

    void setTrack(const std::set<std::string>& data) ;
    std::set<std::string> getTrack() const {return m_track ; }

    void setFeatureset(const std::set<std::string>& data) ;
    std::set<std::string> getFeatureset() const {return m_featureset ; }

    void setFeature(const std::string &data) ;
    std::string getFeature() const {return m_feature ; }

    void setStrand(ZGStrandType strand) ;
    ZGStrandType getStrand() const {return m_strand ; }

    void setFrame(ZGFrameType frame) ;
    ZGFrameType getFrame() const {return m_frame ; }

    void setStart(uint32_t start) ;
    uint32_t getStart() const {return m_start ; }

    void setEnd(uint32_t end) ;
    uint32_t getEnd() const {return m_end ; }

    void setLocus(bool loc) ;
    bool getLocus() const {return m_locus ; }

private:
    static const size_t m_max_string_input_length ;

    bool checkLengths(const std::set<std::string> & data) const ;

    std::set<std::string> m_align,
        m_block,
        m_track,
        m_featureset ;
    std::string m_feature ;
    ZGStrandType m_strand ;
    ZGFrameType m_frame ;
    uint32_t m_start,
        m_end ;
    bool m_locus ;
};


#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGSearchFeaturesData& data) ;
#endif

} // namespace GUI

} // namespace ZMap



#endif // ZGSEARCHFEATURESDATA_H
