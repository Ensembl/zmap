#include "ZWTrackSeparator.h"
#include "ZFeature.h"
#include "Utilities.h"
#include <QtWidgets>


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZWTrackSeparator>::m_name ("ZWTrackSeparator") ;
const QString ZWTrackSeparator::m_name_base("ZWTrackSeparator_") ;
const QColor ZWTrackSeparator::m_default_color(QColor(255, 255, 0)) ;
const qreal ZWTrackSeparator::m_default_length(10000.0),
    ZWTrackSeparator::m_default_width(10.0) ;
const ZInternalID ZWTrackSeparator::m_default_id(0) ;

QString ZWTrackSeparator::getNameFromID(ZInternalID id)
{
    QString str ;
    return m_name_base + str.setNum(id) ;
}

// id of this thing is always set to m_default_id, 0 above.
ZWTrackSeparator::ZWTrackSeparator()
    : ZWTrack(m_default_id, m_default_color),
      Util::InstanceCounter<ZWTrackSeparator>(),
      Util::ClassName<ZWTrackSeparator>(),
      m_children()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZWTrackSeparator::ZWTrackSeparator() ; can not create without instance of qApp ")) ;
    if (m_id != m_default_id)
        throw std::runtime_error(std::string("ZWTrackSeparator::ZWTrackSeparator() ; can not create instance with id different to default" )) ;
    setPos(QPointF(0.0, 0.0)) ;
    setLength(m_default_length) ;
    setWidth(m_default_width) ;
}

ZWTrackSeparator::~ZWTrackSeparator()
{
}

ZWTrackSeparator* ZWTrackSeparator::newInstance()
{
    ZWTrackSeparator *track = Q_NULLPTR ;

    try
    {
        track = new ZWTrackSeparator ;
    }
    catch (...)
    {
        track = Q_NULLPTR ;
    }

    if (track)
    {
        track->setObjectName(getNameFromID(track->getID())) ;
    }

    return track ;
}

QRectF ZWTrackSeparator::boundingRect() const
{
    QRectF rect(QPointF(0.0, 0.0), QSizeF(m_length, m_width)) ;
    return rect ;
}


bool ZWTrackSeparator::addChildItem(QGraphicsItem * item)
{
    bool result = false ;
    ZFeature* feature = dynamic_cast<ZFeature*>(item) ;
    if (feature && !hasChildItem(feature->getID()))
    {
        feature->setParentItem(this) ;
        result = true ;
    }
    return result ;
}

bool ZWTrackSeparator::hasChildItem(ZInternalID id ) const
{
    return m_children.find(id) != m_children.end() ;
}

bool ZWTrackSeparator::setLength(const qreal& value)
{
    bool result = false ;
    if (value > 0.0)
    {
        m_length = value ;
        update() ;
        result = true ;
    }
    return result ;
}

bool ZWTrackSeparator::setWidth(const qreal & value)
{
    bool result = false ;
    if (value > 0.0)
    {
        m_width = value ;
        update() ;
        result = true ;
    }
    return result ;
}

QVariant ZWTrackSeparator::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemChildAddedChange)
    {
        QGraphicsItem *item = Q_NULLPTR ;
        if (value.canConvert<QGraphicsItem*>())
        {
            item = value.value<QGraphicsItem*>() ;
            ZFeature* feature = dynamic_cast<ZFeature*>(item) ;
            if (feature)
            {
                m_children.insert(feature->getID()) ;
            }
        }
    }
    if (change == ItemChildRemovedChange)
    {
        QGraphicsItem *item = Q_NULLPTR ;
        if (value.canConvert<QGraphicsItem*>())
        {
            item = value.value<QGraphicsItem*>() ;
            ZFeature *feature = dynamic_cast<ZFeature*>(item) ;
            if (feature)
            {
                m_children.erase(feature->getID()) ;
            }
        }
    }
    return QGraphicsItem::itemChange(change, value) ;
}


////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZWTrackSeparator::internalReorder()
{
    emit internalReorderChange() ;
}

} // namespace GUI

} // namespace ZMap

