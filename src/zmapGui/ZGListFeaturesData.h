#ifndef ZGLISTFEATURESDATA_H
#define ZGLISTFEATURESDATA_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGFeatureData.h"

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

#include <cstddef>
#include <utility>
#include <string>
#include <set>

//
// To provide data for copying in and out of the ListFeatures dialogue.
//
//

namespace ZMap
{

namespace GUI
{

typedef std::set<ZGFeatureData, ZGFeatureDataCompare> ZGListFeaturesDataType ;

class ZGListFeaturesData: public Util::InstanceCounter<ZGListFeaturesData>,
        public Util::ClassName<ZGListFeaturesData>
{
public:
    ZGListFeaturesData() ;
    ZGListFeaturesData(const ZGListFeaturesData& data) ;
    ZGListFeaturesData& operator=(const ZGListFeaturesData& data) ;
    ~ZGListFeaturesData() ;
    size_t size() {return m_data.size() ; }
    void clear() {m_data.clear() ; }

    bool isPresent(ZInternalID id) ;
    bool isPresent(const ZGFeatureData& data) ;

    bool addFeatureData(const ZGFeatureData& data) ;
    bool removeFeatureData(ZInternalID id) ;

    ZGFeatureData getFeatureData(ZInternalID id) const ;
    std::set<ZInternalID> getIDs() const ;
    void merge(const ZGListFeaturesData& data) ;


private:
    ZGListFeaturesDataType m_data ;
};


#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGListFeaturesData& data) ;
#endif

} // namespace GUI

} // namespace ZMap


#endif // ZGLISTFEATURESDATA_H
