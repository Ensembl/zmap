/*  File: ZWTrackGeneral.h
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

#ifndef ZWTRACKGENERAL_H
#define ZWTRACKGENERAL_H

#include "ZWTrack.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <QColor>
#include <cstddef>
#include <string>


namespace ZMap
{

namespace GUI
{

class ZWTrackGeneral : public ZWTrack,
        public Util::InstanceCounter<ZWTrackGeneral>,
        public Util::ClassName<ZWTrackGeneral>
{
    Q_OBJECT

public:

    ~ZWTrackGeneral() ;

    bool addChildItem(QGraphicsItem *) override ;
    bool hasChildItem(ZInternalID id) const override ;
    QRectF boundingRect() const Q_DECL_OVERRIDE ;


public slots:
    void internalReorder() override ;

signals:
    void internalReorderChange() ;

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE ;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE ;

private:
    static const QColor m_default_color ;

    ZWTrackGeneral(ZInternalID id);
    ZWTrackGeneral(const ZWTrackGeneral&) = delete ;
    ZWTrackGeneral& operator=(const ZWTrackGeneral&) = delete ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZWTRACKGENERAL_H
