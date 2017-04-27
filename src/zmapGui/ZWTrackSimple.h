/*  File: ZWTrackSimple.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2016: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database user interface
 * and was written by
 *
 * Steve Miller (sm23@sanger.ac.uk)
 *
 * Description:
 *-------------------------------------------------------------------
 */

#ifndef ZWTRACKSIMPLE_H
#define ZWTRACKSIMPLE_H

#include <QApplication>
#include <QString>
#include <cstddef>
#include <string>
#include <set>
#include "ZInternalID.h"
#include "ClassName.h"
#include "InstanceCounter.h"
#include "ZWTrack.h"

namespace ZMap
{

namespace GUI
{

class ZWTrackSimple : public ZWTrack,
        public Util::InstanceCounter<ZWTrackSimple>,
        public Util::ClassName<ZWTrackSimple>
{
    Q_OBJECT

public:

    enum class Strand: unsigned char
    {
        Invalid,
        Plus,
        Minus,
        Independent
    };
    static ZWTrackSimple* newInstance(ZInternalID id) ;
    static QString getNameFromID(ZInternalID id) ;

    ~ZWTrackSimple() ;

    bool addChildItem(QGraphicsItem*) override ;
    bool hasChildItem(ZInternalID id) const override ;

    QRectF boundingRect() const Q_DECL_OVERRIDE ;
    bool setStrand(Strand strand) ;
    Strand getStrand() const {return m_strand ; }

public slots:
    void internalReorder() override ;

signals:
    void internalReorderChange() ;

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE ;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE ;

private:

    static const QColor m_default_color ;
    static const qreal m_default_length,
        m_default_width ;
    static const QString m_name_base ;

    ZWTrackSimple(ZInternalID id);
    ZWTrackSimple(const ZWTrackSimple&) = delete ;
    ZWTrackSimple& operator=(const ZWTrackSimple&) = delete ;

    bool m_isBumped ;
    Strand m_strand ;
    std::set<ZInternalID> m_children ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZWTRACKSIMPLE_H
