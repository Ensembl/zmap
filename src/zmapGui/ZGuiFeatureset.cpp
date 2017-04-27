#include "ZGuiFeatureset.h"
#include "ZFeature.h"


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiFeatureset>::m_name("ZGuiFeatureset") ;

ZGuiFeatureset* ZGuiFeatureset::newInstance(ZInternalID id)
{
    ZGuiFeatureset* featureset = nullptr ;

    try
    {
        featureset = new ZGuiFeatureset(id) ;
    }
    catch (...)
    {
        featureset = nullptr ;
    }

    return featureset ;
}


ZGuiFeatureset::ZGuiFeatureset(ZInternalID id)
    : Util::InstanceCounter<ZGuiFeatureset>(),
      Util::ClassName<ZGuiFeatureset>(),
      m_id(id),
      m_feature_painter(nullptr),
      m_feature_map()
{
    if (!m_id)
        throw std::runtime_error(std::string("ZGuiFeatureset::ZGuiFeatureset() ; may not set the id to 0 ")) ;
}


ZGuiFeatureset::~ZGuiFeatureset()
{
    delete m_feature_painter ;
}

// give up ownership of the current feature painter
ZFeaturePainter* ZGuiFeatureset::detatchFeaturePainter()
{
    ZFeaturePainter * painter = m_feature_painter ;
    m_feature_painter = nullptr ;
    return painter ;
}

//
// set the feature painter associated with this featureset,
// and also make sure that the painter is set for all of the
// features currently stored
//
void ZGuiFeatureset::setFeaturePainter(ZFeaturePainter * feature_painter)
{
    if (m_feature_painter)
    {
        delete m_feature_painter ;
    }
    m_feature_painter = feature_painter ;
    for (auto it = m_feature_map.begin() ; it != m_feature_map.end() ; ++it )
    {
        if (it->second)
        {
            it->second->setFeaturePainter(m_feature_painter) ;
        }
    }
}


bool ZGuiFeatureset::contains(ZInternalID id) const
{
    return m_feature_map.find(id) != m_feature_map.end() ;
}

//
// Returns the pointer to the graphics object associated with
// the given id, if there is one present.
//
ZFeature* ZGuiFeatureset::getFeature(ZInternalID id) const
{
    ZFeature *feature = nullptr ;
    auto it = m_feature_map.find(id) ;
    if (it != m_feature_map.end())
        feature = it->second ;
    return feature ;
}

//
// Return the feature ids stored in this featureset.
//
std::set<ZInternalID> ZGuiFeatureset::getFeatureIDs() const
{
    std::set<ZInternalID> dataset ;
    for (auto it = m_feature_map.begin() ; it != m_feature_map.end() ; ++it)
    {
        dataset.insert(it->first) ;
    }
    return dataset ;
}

//
// Inserts a feature into the internal map if the id is not already present.
// Current painter that the featureset stores is set for the feature that's added.
//
bool ZGuiFeatureset::insertFeature(ZFeature *feature)
{
    if (!feature || m_feature_map.find(feature->getID()) != m_feature_map.end())
        return false ;
    feature->setFeaturePainter(m_feature_painter) ;
    m_feature_map.insert(std::pair<ZInternalID, ZFeature*>(feature->getID(), feature)) ;
    return true ;
}

//
// Removes a feature from the map and returns pointer to the corresponding
// graphics object (which you would then need to remove from the scene itself).
//
ZFeature* ZGuiFeatureset::removeFeature(ZInternalID id)
{
    ZFeature *feature = nullptr ;
    auto it = m_feature_map.find(id) ;
    if (it != m_feature_map.end())
    {
        feature = it->second ;
    }
    m_feature_map.erase(it) ;
    return feature ;
}


//
// Remove all features from the map, and return a container of the pointers
// to graphics items.
//
std::set<ZFeature*> ZGuiFeatureset::removeFeatures()
{
    std::set<ZFeature*> features ;
    for (auto it = m_feature_map.begin() ; it != m_feature_map.end() ; ++it)
    {
        features.insert(it->second) ;
    }
    m_feature_map.clear() ;
    return features ;
}

} // namespace GUI

} // namespace ZMap
