#ifndef ZGFEATUREDATA_H
#define ZGFEATUREDATA_H

//
// This is used for copying in and out of user dialogues rather than as
// part of the set of cache data structures.
//


#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

#include <string>
#include <cstddef>
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGFeatureBounds.h"
#include "ZGStrandType.h"
#include "ZGFrameType.h"
#include "ZGFeatureScore.h"


namespace ZMap
{

namespace GUI
{

class ZGFeatureData: public Util::InstanceCounter<ZGFeatureData>,
        public Util::ClassName<ZGFeatureData>
{
public:
    ZGFeatureData() ;
    ZGFeatureData(ZInternalID id) ;
    ZGFeatureData(ZInternalID id,
                  const std::string & name,
                  const ZGFeatureBounds &bounds,
                  ZGStrandType strand,
                  ZGFrameType frame,
                  const ZGFeatureBounds &qbounds,
                  ZGStrandType qstrand,
                  const ZGFeatureScore & score,
                  const std::string &featureset,
                  const std::string &source,
                  const std::string &style) ;
    ZGFeatureData(const ZGFeatureData& data) ;
    ZGFeatureData& operator=(const ZGFeatureData& data) ;
    ~ZGFeatureData() ;

    void setID(ZInternalID id) ;
    ZInternalID getID() const {return m_id ; }

    void setName(const std::string& name) ;
    std::string getName() const {return m_feature_name ; }

    void setBounds(const ZGFeatureBounds& bounds) ;
    ZGFeatureBounds getBounds() const {return m_bounds ; }

    void setStrand(ZGStrandType strand) ;
    ZGStrandType getStrand() const {return m_strand ; }

    void setFrame(ZGFrameType frame) ;
    ZGFrameType getFrame() const {return m_frame ; }

    void setQueryBounds(const ZGFeatureBounds& bounds) ;
    ZGFeatureBounds getQueryBounds() const {return m_query_bounds; }

    void setQueryStrand(ZGStrandType strand) ;
    ZGStrandType getQueryStrand() const {return m_query_strand ; }

    void setScore(const ZGFeatureScore &score) ;
    ZGFeatureScore getScore() const {return m_score ; }

    void setFeatureset(const std::string &featureset) ;
    std::string getFeatureset() const {return m_featureset ;}

    void setSource(const std::string &data) ;
    std::string getSource() const {return m_source ; }

    void setStyle(const std::string &data) ;
    std::string getStyle() const {return m_style ; }

private:
    ZInternalID m_id ;
    std::string m_feature_name ;
    ZGFeatureBounds m_bounds ;
    ZGStrandType m_strand ;
    ZGFrameType m_frame ;
    ZGFeatureBounds m_query_bounds ;
    ZGStrandType m_query_strand ;
    ZGFeatureScore m_score ;
    std::string m_featureset,
        m_source,
        m_style ;
};

class ZGFeatureDataCompare
{
public:
    bool operator()(const ZGFeatureData & d1, const ZGFeatureData& d2) const
    {
        return d1.getID() < d2.getID() ;
    }
};

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGFeatureData& data) ;
#endif

} // namespace GUI

} // namespace ZMap


#endif // ZGFEATUREDATA_H
