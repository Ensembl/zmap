#ifndef ZGPREFERENCESBLIXEMDATA_H
#define ZGPREFERENCESBLIXEMDATA_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <cstddef>
#include <string>
#include <cstdint>


namespace ZMap
{

namespace GUI
{

class ZGPreferencesBlixemData: public Util::InstanceCounter<ZGPreferencesBlixemData>,
        public Util::ClassName<ZGPreferencesBlixemData>
{
public:

    ZGPreferencesBlixemData();
    ZGPreferencesBlixemData(bool scope, bool features, bool temp, bool sleep, bool kill,
                            uint32_t sc, uint32_t max, uint32_t port,
                            const std::string &networkid, const std::string & config_file, const std::string &launch_script) ;
    ZGPreferencesBlixemData(const ZGPreferencesBlixemData& data) ;
    ZGPreferencesBlixemData& operator=(const ZGPreferencesBlixemData&data) ;
    ~ZGPreferencesBlixemData() ;

    void setRestrictScope(bool value) {m_restrict_scope = value ; }
    bool getRestrictScope() const {return m_restrict_scope ; }

    void setRestrictFeatures(bool value)  {m_restrict_features = value ; }
    bool getRestrictFeatures() const {return m_restrict_features ; }

    void setKeepTemporary(bool value) { m_keep_temporary = value ; }
    bool getKeepTemporary() const {return m_keep_temporary ; }

    void setSleep(bool value) { m_sleep = value ; }
    bool getSleep() const {return m_sleep ; }

    void setKill(bool value) { m_kill = value ; }
    bool getKill() const {return m_kill ; }

    void setScope(uint32_t value) {m_scope = value ; }
    uint32_t getScope() const {return m_scope ; }

    void setMax(uint32_t max) {m_max = max ; }
    uint32_t getMax() const {return m_max ; }

    void setPort(uint32_t port) {m_port = port ; }
    uint32_t getPort() const {return m_port ; }

    void setNetworkID(const std::string& data) { m_networkid = data ; }
    std::string getNetworkID() const {return m_networkid ; }

    void setConfigFile(const std::string& data) {m_config_file = data ; }
    std::string getConfigFile() const {return m_config_file ; }

    void setLaunchScript(const std::string & data) {m_launch_script = data ; }
    std::string getLaunchScript() const {return m_launch_script ; }

private:

    bool m_restrict_scope,
        m_restrict_features,
        m_keep_temporary,
        m_sleep,
        m_kill ;

    uint32_t m_scope,
        m_max,
        m_port ;

    std::string m_networkid,
        m_config_file,
        m_launch_script ;
};


#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGPreferencesBlixemData& data) ;
#endif

} // namespace GUI

} // namespace ZMap



#endif // ZGPREFERENCESBLIXEMDATA_H
