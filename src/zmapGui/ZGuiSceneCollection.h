#ifndef ZGUISCENECOLLECTION_H
#define ZGUISCENECOLLECTION_H

#include <QObject>
#include <cstddef>
#include <string>
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <set>


namespace ZMap
{

namespace GUI
{

class ZGuiScene ;

//
// This is a collection of ZGuiScene objects which are QGraphicsScene, which in turn
// inherits from QObject. Operations such as finding of particular children are managed
// using the QObject::findChild[ren] functions; these work using the name returned
// by QObject::objectName().
//

class ZGuiSceneCollection: public QObject,
        public Util::InstanceCounter<ZGuiSceneCollection>,
        public Util::ClassName<ZGuiSceneCollection>
{

    Q_OBJECT

public:
    static ZGuiSceneCollection* newInstance(ZInternalID id, QObject *parent = Q_NULLPTR) ;

    ~ZGuiSceneCollection() ;
    ZInternalID getID() const {return m_id ; }

    std::set<ZInternalID> getIDs() const ;
    size_t size() const {return children().size() ; }
    bool isPresent(ZInternalID id) const ;
    bool add(ZInternalID id) ;
    bool remove(ZInternalID id) ;
    ZGuiScene* get(ZInternalID id) const ;
    void clear() ;

private:
    ZGuiSceneCollection(ZInternalID id, QObject *parent = Q_NULLPTR) ;
    ZGuiSceneCollection(const ZGuiSceneCollection& ) = delete ;
    ZGuiSceneCollection& operator=(const ZGuiSceneCollection&) = delete ;

    ZInternalID m_id ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUISCENECOLLECTION_H
