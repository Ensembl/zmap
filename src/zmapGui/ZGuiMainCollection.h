#ifndef ZGUIMAINCOLLECTION_H
#define ZGUIMAINCOLLECTION_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include <QWidget>

#include <cstddef>
#include <string>
#include "ZInternalID.h"
#include <set>


namespace ZMap
{

namespace GUI
{

class ZGuiMain ;

class ZGuiMainCollection : public QWidget,
        public Util::InstanceCounter<ZGuiMainCollection>,
        public Util::ClassName<ZGuiMainCollection>
{

    Q_OBJECT

public:
    static ZGuiMainCollection* newInstance(ZInternalID id, QWidget* parent = Q_NULLPTR) ;

    ~ZGuiMainCollection() ;

    ZInternalID getID() const {return m_id ; }

    std::set<ZInternalID> getIDs() const ;
    size_t size() const {return children().size() ; }
    bool isPresent(ZInternalID id) const ;
    bool add(ZInternalID id) ;
    bool remove(ZInternalID id) ;
    ZGuiMain* get(ZInternalID id) const ;
    void clear() ;

private:
    ZGuiMainCollection(ZInternalID id, QWidget *parent = Q_NULLPTR) ;
    ZGuiMainCollection(const ZGuiMainCollection&) = delete ;
    ZGuiMainCollection& operator=(const ZGuiMainCollection&) = delete ;

    ZInternalID m_id ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIMAINCOLLECTION_H
