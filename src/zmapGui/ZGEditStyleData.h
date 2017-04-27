#ifndef ZGEDITSTYLEDATA_H
#define ZGEDITSTYLEDATA_H

#include <string>
#include <cstddef>
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZInternalID.h"
#include "ZGStyleData.h"

//
// This is copy data in and out of the edit style dialogue, so needs two styles
// one for the parent (i.e. starting point) and one for a potential created child.
//

namespace ZMap
{

namespace GUI
{

class ZGEditStyleData: public Util::InstanceCounter<ZGEditStyleData>,
        public Util::ClassName<ZGEditStyleData>
{
public:

    ZGEditStyleData() ;
    ZGEditStyleData(const ZGStyleData& data, const ZGStyleData& child = ZGStyleData()) ;
    ZGEditStyleData(const ZGEditStyleData& data) ;
    ZGEditStyleData& operator=(const ZGEditStyleData& data) ;
    ~ZGEditStyleData() ;

    void setStyleData(const ZGStyleData& data) ;
    ZGStyleData getStyleData() const {return m_style_data ; }
    void setStyleDataChild(const ZGStyleData& data) ;
    ZGStyleData getStyleDataChild() const {return m_style_child ; }

private:

    ZGStyleData m_style_data,
        m_style_child ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGEDITSTYLEDATA_H
