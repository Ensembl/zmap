#ifndef ZGTRACKCONFIGUREDATA_H
#define ZGTRACKCONFIGUREDATA_H

#include <cstddef>
#include <string>
#include <set>

#include "ZInternalID.h"
#include "ZGTrackData.h"
#include "InstanceCounter.h"
#include "ClassName.h"

namespace ZMap
{

namespace GUI
{

typedef std::set<ZGTrackData, ZGTrackDataCompare> ZGTrackConfigureDataType ;
typedef std::set<ZGTrackData, ZGTrackDataCompare>::iterator ZGTrackConfigureData_iterator ;
typedef std::set<ZGTrackData, ZGTrackDataCompare>::const_iterator ZGTrackConfigureData_const_iterator ;

//
// Things to do with this class...
//

class ZGTrackConfigureData: public Util::InstanceCounter<ZGTrackConfigureData>,
        public Util::ClassName<ZGTrackConfigureData>
{

public:

    ZGTrackConfigureData();
    ZGTrackConfigureData(const ZGTrackConfigureData& data) ;
    ZGTrackConfigureData& operator=(const ZGTrackConfigureData& data) ;
    ~ZGTrackConfigureData() ;

    size_t size() const {return m_data.size() ; }

    void clear() ;
    bool isPresent(ZInternalID id ) const ;
    bool isPresent(const ZGTrackData &data) const ;

    bool addTrackData(const ZGTrackData & data) ;
    bool removeTrackData(const ZInternalID &id ) ;

    ZGTrackData getTrackData(ZInternalID id ) const ;

    std::set<ZInternalID> getIDs() const ;

    void merge(const ZGTrackConfigureData& data) ;

private:

    ZGTrackConfigureDataType m_data ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGTRACKCONFIGUREDATA_H
