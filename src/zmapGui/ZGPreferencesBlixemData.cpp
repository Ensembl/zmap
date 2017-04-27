#include "ZGPreferencesBlixemData.h"
#ifndef QT_NO_DEBUG
#include <sstream>
#endif

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPreferencesBlixemData>::m_name ("ZGPreferencesBlixemData") ;

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGPreferencesBlixemData& data)
{
    std::stringstream str ;
    str << data.name() << "("
        << data.getRestrictScope() << ","
        << data.getRestrictFeatures() << ","
        << data.getKeepTemporary() << ","
        << data.getSleep() << ","
        << data.getKill() << ","
        << data.getScope() << ","
        << data.getMax() << ","
        << data.getPort() << ","
        << data.getNetworkID() << ","
        << data.getConfigFile() << ","
        << data.getLaunchScript()
        << ")" ;
    dbg.nospace() << QString::fromStdString(str.str()) ;
    return dbg.space() ;
}
#endif

ZGPreferencesBlixemData::ZGPreferencesBlixemData()
    : Util::InstanceCounter<ZGPreferencesBlixemData>(),
      Util::ClassName<ZGPreferencesBlixemData>(),
      m_restrict_scope(),
      m_restrict_features(),
      m_keep_temporary(),
      m_sleep(),
      m_kill(),
      m_scope(),
      m_max(),
      m_port(),
      m_networkid(),
      m_config_file(),
      m_launch_script()
{
}


ZGPreferencesBlixemData::ZGPreferencesBlixemData(bool scope,
                                                 bool features,
                                                 bool temp,
                                                 bool sleep,
                                                 bool kill,
                                                 uint32_t sc,
                                                 uint32_t max,
                                                 uint32_t port,
                                                 const std::string &networkid,
                                                 const std::string &config_file,
                                                 const std::string &launch_script)
    : Util::InstanceCounter<ZGPreferencesBlixemData>(),
      Util::ClassName<ZGPreferencesBlixemData>(),
      m_restrict_scope(scope),
      m_restrict_features(features),
      m_keep_temporary(temp),
      m_sleep(sleep),
      m_kill(kill),
      m_scope(sc),
      m_max(max),
      m_port(port),
      m_networkid(networkid),
      m_config_file(config_file),
      m_launch_script(launch_script)
{
}

ZGPreferencesBlixemData::ZGPreferencesBlixemData(const ZGPreferencesBlixemData& data)
    : Util::InstanceCounter<ZGPreferencesBlixemData>(),
      Util::ClassName<ZGPreferencesBlixemData>(),
      m_restrict_scope(data.m_restrict_scope),
      m_restrict_features(data.m_restrict_features),
      m_keep_temporary(data.m_keep_temporary),
      m_sleep(data.m_sleep),
      m_kill(data.m_kill),
      m_scope(data.m_scope),
      m_max(data.m_max),
      m_port(data.m_port),
      m_networkid(data.m_networkid),
      m_config_file(data.m_config_file),
      m_launch_script(data.m_launch_script)
{
}

ZGPreferencesBlixemData& ZGPreferencesBlixemData::operator=(const ZGPreferencesBlixemData& data)
{
    if (this != &data)
    {
        m_restrict_scope = data.m_restrict_scope ;
        m_restrict_features = data.m_restrict_features ;
        m_keep_temporary = data.m_keep_temporary ;
        m_sleep = data.m_sleep ;
        m_kill = data.m_kill ;
        m_scope = data.m_scope ;
        m_max = data.m_max ;
        m_port = data.m_port ;
        m_networkid = data.m_networkid ;
        m_config_file = data.m_config_file ;
        m_launch_script = data.m_launch_script ;
    }
    return *this ;
}

ZGPreferencesBlixemData::~ZGPreferencesBlixemData()
{
}

} // namespace GUI

} // namespace ZMap

