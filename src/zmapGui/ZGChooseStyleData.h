#ifndef ZGCHOOSESTYLEDATA_H
#define ZGCHOOSESTYLEDATA_H

#include <string>
#include <set>
#include <cstddef>
#include "ZInternalID.h"
#include "ZGStyleData.h"
#include "InstanceCounter.h"
#include "ClassName.h"

//
// This class is basically a container of ZGStyleData items, used for communicating with
// the ChooseStyle dialogue.
//

namespace ZMap
{

namespace GUI
{

typedef std::set<ZGStyleData, ZGStyleDataCompare> ZGChooseStyleDataType ;
typedef std::set<ZGStyleData, ZGStyleDataCompare>::iterator ZGChooseStyleData_iterator ;
typedef std::set<ZGStyleData, ZGStyleDataCompare>::const_iterator ZGChooseStyleData_const_iterator ;

class ZGChooseStyleData: public Util::InstanceCounter<ZGChooseStyleData>,
        public Util::ClassName<ZGChooseStyleData>
{
public:

    ZGChooseStyleData() ;
    ZGChooseStyleData(const ZGChooseStyleData& data) ;
    ZGChooseStyleData& operator=(const ZGChooseStyleData& data) ;
    ~ZGChooseStyleData() ;

    size_t size() const {return m_data.size() ; }

    void clear() ;

    bool isPresent(ZInternalID id) const ;
    bool isPresent(const ZGStyleData& data) const ;

    bool addStyleData(const ZGStyleData& data) ;
    bool removeStyleData(const ZInternalID& id) ;

    ZGStyleData getStyleData(ZInternalID id) const ;
    std::set<ZInternalID> getIDs() const ;
    void merge(const ZGChooseStyleData& data) ;

private:
    ZGChooseStyleDataType m_data ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGCHOOSESTYLEDATA_H
