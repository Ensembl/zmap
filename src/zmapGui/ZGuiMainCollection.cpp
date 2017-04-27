#include "ZGuiMainCollection.h"
#include "ZGuiMain.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiMainCollection>::m_name("ZGuiMainCollection") ;

ZGuiMainCollection::ZGuiMainCollection(ZInternalID id, QWidget *parent)
    : QWidget(parent),
      Util::InstanceCounter<ZGuiMainCollection>(),
      Util::ClassName<ZGuiMainCollection>(),
      m_id(id)
{
    if (!m_id)
        throw std::runtime_error(std::string("ZGuiMainCollection::ZGuiMainCollection() ; id may not be set to 0")) ;
}

ZGuiMainCollection::~ZGuiMainCollection()
{
}

// return all ids in the collection
std::set<ZInternalID> ZGuiMainCollection::getIDs() const
{
    std::set<ZInternalID> data ;
    QList<ZGuiMain*> list = findChildren<ZGuiMain*>(QString(),
                                             Qt::FindDirectChildrenOnly) ;
    for (auto it = list.begin() ; it != list.end() ; ++it)
        data.insert((*it)->getID()) ;
    return data ;
}

// return pointer to child with given id
ZGuiMain* ZGuiMainCollection::get(ZInternalID id) const
{
    return findChild<ZGuiMain*>(ZGuiMain::getNameFromID(id),
                                Qt::FindDirectChildrenOnly) ;
}

// delete child with particular id
bool ZGuiMainCollection::remove(ZInternalID id)
{
    bool result = false ;
    ZGuiMain* gui = findChild<ZGuiMain*>(ZGuiMain::getNameFromID(id),
                                         Qt::FindDirectChildrenOnly) ;
    if (gui)
    {
        gui->setParent(Q_NULLPTR) ;
        delete gui ;
        result = true ;
    }
    return result ;
}

// delete all children
void ZGuiMainCollection::clear()
{
    QObjectList list = children() ;
    for (auto it = list.begin() ; it != list.end() ; ++it)
    {
        (*it)->setParent(Q_NULLPTR) ;
        delete *it ;
    }
}


// add a child with a given id
bool ZGuiMainCollection::add(ZInternalID id)
{
    bool result = false ;
    if (isPresent(id))
        return result ;
    ZGuiMain *gui = ZGuiMain::newInstance(id) ;
    if (gui)
    {
        gui->setParent(this, Qt::Window) ;
        result = true ;
    }
    return result ;
}

// does the container contain an instance with the specified id
bool ZGuiMainCollection::isPresent(ZInternalID id) const
{
    ZGuiMain * gui = findChild<ZGuiMain*>(ZGuiMain::getNameFromID(id),
                                      Qt::FindDirectChildrenOnly) ;
    return static_cast<bool>(gui) ;
}


ZGuiMainCollection* ZGuiMainCollection::newInstance(ZInternalID id, QWidget* parent)
{
    ZGuiMainCollection* thing = Q_NULLPTR ;
    try
    {
        thing = new ZGuiMainCollection(id, parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

} // namespace GUI

} // namespace ZMap
