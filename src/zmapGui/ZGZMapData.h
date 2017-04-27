#ifndef ZGZMAPDATA_H
#define ZGZMAPDATA_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include <cstddef>
#include <string>

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

namespace ZMap
{

namespace GUI
{

class ZGZMapData: public Util::InstanceCounter<ZGZMapData>
{
public:

    std::string className() {return m_name ; }

    ZGZMapData();
    ZGZMapData(const ZGZMapData& data) ;
    ZGZMapData& operator=(const ZGZMapData& data) ;
    ~ZGZMapData() ;

    std::string name() const {return m_name ; }

    bool setID(ZInternalID id) ;
    ZInternalID getID() const {return m_zmap_id ; }

    bool setState(const std::string& data) ;
    std::string getState() const {return m_state; }

    bool setSequence(const std::string& data) ;
    std::string getSequence() const {return m_sequence ; }

private:

    static const std::string m_name ;

    ZInternalID m_zmap_id ;
    std::string m_state,
        m_sequence ;
};

class ZGZMapDataCompare
{
public:
    bool operator()(const ZGZMapData& z1, const ZGZMapData& z2)  const
    {
        return z1.getID() < z2.getID() ;
    }
};


#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGZMapData& data) ;
#endif

} // namespace GUI

} // namespace ZMap


#endif // ZGZMAPDATA_H
