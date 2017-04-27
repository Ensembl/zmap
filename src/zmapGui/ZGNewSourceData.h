#ifndef ZGNEWSOURCEDATA_H
#define ZGNEWSOURCEDATA_H

#include "ZInternalID.h"
#include "ZGSourceType.h"
#include "InstanceCounter.h"
#include <string>
#include <set>
#include <cstddef>
#include <cstdint>

namespace ZMap
{

namespace GUI
{

class ZGNewSourceData: public Util::InstanceCounter<ZGNewSourceData>
{
public:

    std::string className() {return m_name ; }


    ZGNewSourceData();
    ZGNewSourceData(ZGSourceType source_type,
        const std::string& source_name,
        const std::string& path_url,
        const std::string& database,
        const std::string& prefix,
        const std::string& host,
        const std::string& username,
        const std::string& password,
        const std::set<std::string>& featuresets,
        const std::set<std::string>& biotypes,
        uint32_t port,
        bool load_dna) ;
    ZGNewSourceData(const ZGNewSourceData& data) ;
    ZGNewSourceData& operator=(const ZGNewSourceData& data) ;
    ~ZGNewSourceData() ;

    std::string name() const {return m_name ;}

    void setSourceType(ZGSourceType type) ;
    ZGSourceType getSourceType() const {return m_source_type ; }

    void setSourceName(const std::string& data) ;
    std::string getSourceName() const {return m_source_name ; }

    void setPathURL(const std::string& data) ;
    std::string getPathURL() const {return m_file_url ; }

    void setDatabase(const std::string& data) ;
    std::string getDatabase() const {return m_database ; }

    void setPrefix(const std::string& data) ;
    std::string getPrefix() const {return m_prefix ; }

    void setHost(const std::string & data) ;
    std::string getHost() const {return m_host ; }

    void setPort(uint32_t port) ;
    uint32_t getPort() const {return m_port ; }

    void setUsername(const std::string& data) ;
    std::string getUsername() const {return m_username ; }

    void setPassword(const std::string & data) ;
    std::string getPassword() const {return m_password ; }

    void setFeaturesets(const std::set<std::string>& data) ;
    std::set<std::string> getFeaturesets() const {return m_featuresets ; }

    void setBiotypes(const std::set<std::string> & data) ;
    std::set<std::string> getBiotypes() const {return m_biotypes ; }

    void setLoadDNA(bool load) ;
    bool getLoadDNA() const {return m_load_dna ; }


private:

    static const std::string m_name ;

    ZGSourceType m_source_type ;
    std::string m_source_name,
        m_file_url,
        m_database,
        m_prefix,
        m_host,
        m_username,
        m_password ;
    std::set<std::string> m_featuresets,
        m_biotypes ;
    uint32_t m_port ;
    bool m_load_dna ;
};


#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGNewSourceData& data) ;
#endif


} // namespace GUI

} // namespace ZMap

#endif // ZGNEWSOURCEDATA_H
