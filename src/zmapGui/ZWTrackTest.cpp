#include "ZWTrackTest.h"
#include "ZFeature.h"
#include "Utilities.h"

#include <QtWidgets>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZWTrackTest>::m_name("ZWTrackTest") ;
const QString ZWTrackTest::m_name_base("ZWTrackTest_") ;
const QColor ZWTrackTest::m_default_color(Qt::transparent) ;
const qreal ZWTrackTest::m_default_length(10000.0),
    ZWTrackTest::m_default_width(20.0) ;

QString ZWTrackTest::getNameFromID(ZInternalID id)
{
    QString str ;
    return m_name_base + str.setNum(id) ;
}

ZWTrackTest::ZWTrackTest(ZInternalID id)
    : ZWTrack(id, m_default_color),
      Util::InstanceCounter<ZWTrackTest>(),
      Util::ClassName<ZWTrackTest>(),
      m_grab1(Q_NULLPTR),
      m_grab2(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZWTrackTest::ZWTrackTest() ; call to canCreate failed ")) ;
    setPos(QPointF(0.0, 0.0)) ;

    if (!(m_grab1 = new (std::nothrow) ZItemGrab))
        throw std::runtime_error(std::string("ZWTrackTest::ZWTrackTest() ; can not create grab1 ")) ;
    if (!(m_grab2 = new (std::nothrow) ZItemGrab))
        throw std::runtime_error(std::string("ZWTrackTest::ZWTrackTest() ; can not create grab2 ")) ;

    // add to current item as children
    m_grab1->setParentItem(this) ;
    m_grab2->setParentItem(this) ;

    // set positions...
    m_grab1->setPos(QPointF(0.5*boundingRect().width(), 0.0)) ;
    m_grab2->setPos(QPointF(0.5*boundingRect().width(), boundingRect().height()-m_grab2->boundingRect().height())) ;

    // hide and suspend
    m_grab1->hide() ;
    m_grab2->hide() ;
}

ZWTrackTest::~ZWTrackTest()
{
}

ZWTrackTest* ZWTrackTest::newInstance(ZInternalID id)
{
    ZWTrackTest* test = Q_NULLPTR ;

    try
    {
        test = new ZWTrackTest(id) ;
    }
    catch (...)
    {
        test = Q_NULLPTR ;
    }

    if (test)
    {
        test->setObjectName(getNameFromID(id)) ;
    }

    return test ;
}

QRectF ZWTrackTest::boundingRect() const
{
    QRectF rect = childrenBoundingRect() ;
    if (rect.width() > 0.0 && rect.height() > 0.0)
    {
        return rect ;
    }
    else
    {
        return QRectF(QPointF(0.0, 0.0), QSizeF(m_default_length, m_default_width)) ;
    }
}

bool ZWTrackTest::addChildItem(QGraphicsItem* item)
{
    bool result = false ;
    ZFeature *feature = dynamic_cast<ZFeature*>(item) ;
    if (feature)
    {
        feature->setParentItem(this) ;
        result = true ;
    }
    return result ;
}

bool ZWTrackTest::hasChildItem(ZInternalID id) const
{
    return false ;
}

} // namespace GUI

} // namespace ZMap
