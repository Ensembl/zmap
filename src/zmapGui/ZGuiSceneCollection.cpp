#include "ZGuiSceneCollection.h"
#include "ZGuiScene.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiSceneCollection>::m_name("ZGuiSceneCollection") ;

ZGuiSceneCollection::ZGuiSceneCollection(ZInternalID id, QObject* parent)
    : QObject(parent),
      Util::InstanceCounter<ZGuiSceneCollection>(),
      Util::ClassName<ZGuiSceneCollection>(),
      m_id(id)
{
    if (!m_id)
        throw std::runtime_error(std::string("ZGuiSceneCollection::ZGuiSceneCollection() ; id may not be set to zero")) ;
}

ZGuiSceneCollection::~ZGuiSceneCollection()
{
}

// return the ids of all objects in the collection
std::set<ZInternalID> ZGuiSceneCollection::getIDs() const
{
    std::set<ZInternalID> data ;
    QList<ZGuiScene*> list = findChildren<ZGuiScene*>(QString(),
                                          Qt::FindDirectChildrenOnly) ;
    for (auto it = list.begin() ; it != list.end() ; ++it)
        data.insert((*it)->getID()) ;
    return data ;
}

ZGuiScene * ZGuiSceneCollection::get(ZInternalID id) const
{
    return findChild<ZGuiScene*>(ZGuiScene::getNameFromID(id),
                                 Qt::FindDirectChildrenOnly) ;
}

bool ZGuiSceneCollection::remove(ZInternalID id)
{
    bool result = false ;
    ZGuiScene* scene = findChild<ZGuiScene*>(ZGuiScene::getNameFromID(id),
                                             Qt::FindDirectChildrenOnly) ;
    if (scene)
    {
        scene->setParent(Q_NULLPTR) ;
        delete scene ;
        result = true ;
    }
    return result ;
}

// delete all children
void ZGuiSceneCollection::clear()
{
    QObjectList list = children() ;
    for (auto it = list.begin() ; it != list.end() ; ++it)
    {
        QObject *o = *it ;
        if (o)
        {
            o->setParent(Q_NULLPTR) ;
            delete o ;
        }
    }
}

bool ZGuiSceneCollection::add(ZInternalID id)
{
    bool result = false ;
    if (isPresent(id))
        return result ;
    ZGuiScene *scene = ZGuiScene::newInstance(id) ;
    if (scene)
    {
        scene->setParent(this) ;
        result = true ;
    }
    return result ;
}

// does the container store a scene with a particular id?
bool ZGuiSceneCollection::isPresent(ZInternalID id) const
{
    ZGuiScene * scene = findChild<ZGuiScene*>(ZGuiScene::getNameFromID(id),
                                      Qt::FindDirectChildrenOnly) ;
    return static_cast<bool>(scene) ;
}

ZGuiSceneCollection* ZGuiSceneCollection::newInstance(ZInternalID id, QObject *parent)
{
    ZGuiSceneCollection* thing = Q_NULLPTR ;
    try
    {
        thing = new ZGuiSceneCollection(id, parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

} // namespace GUI

} // namespace ZMap
