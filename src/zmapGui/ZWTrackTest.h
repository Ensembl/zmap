#ifndef ZWTRACKTEST_H
#define ZWTRACKTEST_H

#include <QApplication>
#include <QString>
#include <QPainter>
#include <cstddef>
#include <string>
#include <set>
#include "ZInternalID.h"
#include "ClassName.h"
#include "InstanceCounter.h"
#include "ZWTrack.h"
#include "ZItemGrab.h"

//
// OK, so how to proceed with this:
//
// (1) We show() the grabs on entry and hide() again on exit()
// (2) Also possibly need to deal with hover events for this
//     (test whether pointer pos is within the item on startup)
// (3) Define a coordinate range at top and bottom in which the
//     grab may be activated
// (4) Define mouse click event handler
// (5) Define mouse drag event handler
// (6) Define mouse release event handler
// (7) Difference in positions is the coordinate amount to change
// (8) Change the size of the item when done...
//
//


namespace ZMap
{

namespace GUI
{


class ZWTrackTest : public ZWTrack,
        public Util::InstanceCounter<ZWTrackTest>,
        public Util::ClassName<ZWTrackTest>
{

    Q_OBJECT

public:
    static ZWTrackTest* newInstance(ZInternalID id) ;
    static QString getNameFromID(ZInternalID id) ;

    ~ZWTrackTest() ;
    bool addChildItem(QGraphicsItem * ) override ;
    bool hasChildItem(ZInternalID id) const override ;

    QRectF boundingRect() const Q_DECL_OVERRIDE ;

public slots:
    void internalReorder() override {}

private:

    static const QColor m_default_color ;
    static const qreal m_default_length,
        m_default_width ;
    static const QString m_name_base ;

    ZWTrackTest(ZInternalID id) ;
    ZWTrackTest(const ZWTrackTest&) = delete ;
    ZWTrackTest& operator=(const ZWTrackTest&) = delete ;

    QGraphicsItem *m_grab1,
        *m_grab2 ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZWTRACKTEST_H
