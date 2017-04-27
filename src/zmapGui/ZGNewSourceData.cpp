
#ifndef QT_NO_DEBUG
#include <QDebug>
#include <sstream>
#endif

#include "ZGNewSourceData.h"
#include <stdexcept>
#include <set>


namespace ZMap
{

namespace GUI
{


#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGNewSourceData& data)
{
    std::stringstream str ;
    str << "("
        << data.getSourceType() << "," << data.getSourceName() << "," << data.getPathURL() << "," << data.getDatabase() << ","
        << data.getPrefix() << "," << data.getHost() << "," << data.getUsername() << "," << data.getPassword() << "," ;
    std::set<std::string> collection = data.getFeaturesets() ;
    if (collection.size())
    {
        for (auto it = collection.begin() ; it != collection.end() ; ++it)
            str << *it << "," ;
    }
    collection = data.getBiotypes() ;
    if (collection.size())
    {
        for (auto it = collection.begin() ; it != collection.end() ; ++it)
            str << *it << "," ;
    }
    str << data.getPort() << "," << data.getLoadDNA() << ")" ;
    dbg.nospace() << QString::fromStdString(data.name())
                  << QString::fromStdString(str.str()) ;
    return dbg.space() ;
}
#endif

const std::string ZGNewSourceData::m_name("ZGNewSourceData") ;


ZGNewSourceData::ZGNewSourceData()
    : Util::InstanceCounter<ZGNewSourceData>(),
      m_source_type(ZGSourceType::Invalid),
      m_source_name(),
      m_file_url(),
      m_database(),
      m_prefix(),
      m_host(),
      m_username(),
      m_password(),
      m_featuresets(),
      m_biotypes(),
      m_port(),
      m_load_dna()
{
}

ZGNewSourceData::ZGNewSourceData(ZGSourceType source_type,
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
                                 bool load_dna)

    : Util::InstanceCounter<ZGNewSourceData>(),
      m_source_type(source_type),
      m_source_name(source_name),
      m_file_url(path_url),
      m_database(database),
      m_prefix(prefix),
      m_host(host),
      m_username(username),
      m_password(password),
      m_featuresets(featuresets),
      m_biotypes(biotypes),
      m_port(port),
      m_load_dna(load_dna)
{
}

ZGNewSourceData::ZGNewSourceData(const ZGNewSourceData& data)

    : Util::InstanceCounter<ZGNewSourceData>(),
      m_source_type(data.m_source_type),
      m_source_name(data.m_source_name),
      m_file_url(data.m_file_url),
      m_database(data.m_database),
      m_prefix(data.m_prefix),
      m_host(data.m_host),
      m_username(data.m_username),
      m_password(data.m_password),
      m_featuresets(data.m_featuresets),
      m_biotypes(data.m_biotypes),
      m_port(data.m_port),
      m_load_dna(data.m_load_dna)
{
}

ZGNewSourceData& ZGNewSourceData::operator=(const ZGNewSourceData& data)
{
    if (this != &data)
    {
        m_source_type = data.m_source_type ;
        m_source_name = data.m_source_name ;
        m_file_url = data.m_file_url ;
        m_database = data.m_database ;
        m_prefix = data.m_prefix ;
        m_host = data.m_host ;
        m_username = data.m_username ;
        m_password = data.m_password ;
        m_featuresets = data.m_featuresets ;
        m_biotypes = data.m_biotypes ;
        m_port = data.m_port ;
        m_load_dna = data.m_load_dna ;
    }
    return *this ;
}

ZGNewSourceData::~ZGNewSourceData()
{
}

void ZGNewSourceData::setSourceType(ZGSourceType type)
{
    m_source_type = type ;
}

void ZGNewSourceData::setSourceName(const std::string &data)
{
    m_source_name = data ;
}

void ZGNewSourceData::setPathURL(const std::string &data)
{
    m_file_url = data ;
}

void ZGNewSourceData::setDatabase(const std::string& data)
{
    m_database = data ;
}

void ZGNewSourceData::setPrefix(const std::string& data)
{
    m_prefix = data ;
}

void ZGNewSourceData::setHost(const std::string& data)
{
    m_host = data ;
}

void ZGNewSourceData::setPort(uint32_t port)
{
    m_port = port ;
}

void ZGNewSourceData::setUsername(const std::string& data)
{
    m_username = data ;
}

void ZGNewSourceData::setPassword(const std::string& data)
{
    m_password = data ;
}

void ZGNewSourceData::setFeaturesets(const std::set<std::string> &data)
{
    m_featuresets = data ;
}

void ZGNewSourceData::setBiotypes(const std::set<std::string> & data )
{
    m_biotypes = data ;
}

void ZGNewSourceData::setLoadDNA(bool load)
{
    m_load_dna = load ;
}

} // namespace GUI

} // namespace ZMap
