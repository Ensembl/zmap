#ifndef ZGUIFEATURESET_H
#define ZGUIFEATURESET_H

//
// This is used to store the featureset_id and its contained feature_id data.
// We also maintain a mapping between the feature_id and the associated
// ZFeature * (QGraphicsItem).
//
// In addition, each of these featuresets store a pointer to a ZFeaturePainter
// which can be set (or removed completely) at run time.
//
//

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <map>
#include <set>


namespace ZMap
{

namespace GUI
{

class ZFeature ;
class ZFeaturePainter ;

//
// This container does not control the lifetimes of the ZFeature*
// that it refers to; it exists to do lookup of graphics objects
// from their id.
//
// Note that it does however control the lifetime of the feature painter
// object that's passed in.
//


class ZGuiFeatureset: public Util::InstanceCounter<ZGuiFeatureset>,
        public Util::ClassName<ZGuiFeatureset>
{
public:
    static ZGuiFeatureset* newInstance(ZInternalID id ) ;

    ZGuiFeatureset(ZInternalID id) ;
    ~ZGuiFeatureset() ;

    ZInternalID getID() const {return m_id ; }

    void setFeaturePainter(ZFeaturePainter * painter) ;
    ZFeaturePainter * detatchFeaturePainter() ;
    ZFeaturePainter * getFeaturePainter() const {return m_feature_painter ; }

    bool contains(ZInternalID id) const ;
    ZFeature* getFeature(ZInternalID id) const ;
    std::set<ZInternalID> getFeatureIDs() const ;
    bool insertFeature(ZFeature *feature) ;
    ZFeature * removeFeature(ZInternalID id) ;
    std::set<ZFeature*> removeFeatures() ;

private:
    ZGuiFeatureset(const ZGuiFeatureset&) = delete ;
    ZGuiFeatureset& operator=(const ZGuiFeatureset& ) = delete ;

    ZInternalID m_id ;
    ZFeaturePainter *m_feature_painter ;
    std::map<ZInternalID, ZFeature*> m_feature_map ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIFEATURESET_H
