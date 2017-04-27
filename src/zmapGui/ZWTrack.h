/*  File: ZWTrack.h
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

#ifndef ZWTRACK_H
#define ZWTRACK_H

#include <QGraphicsItem>
#include <QGraphicsObject>
#include "ZInternalID.h"
#include "ZFeaturePainter.h"
#include <string>
#include <QGraphicsObject>
#include <QColor>


namespace ZMap
{

namespace GUI
{

//
// Need an internal ordering policy object for all of this.
//

class ZWTrack : public QGraphicsObject
{
public:
    ZWTrack(ZInternalID id, const QColor &color);
    virtual ~ZWTrack() {}

    ZInternalID getID() const {return m_id ; }

    virtual bool addChildItem(QGraphicsItem* ) = 0 ;
    virtual bool hasChildItem(ZInternalID id) const = 0 ;

    QPainterPath shape() const Q_DECL_OVERRIDE ;

    void setColor(const QColor& color) ;
    QColor getColor() const {return m_color ; }

public slots:
    virtual void internalReorder() = 0 ;

protected:
    static const QColor m_default_highlight_color ;
    static const int m_qgi_id ;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE ;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE ;

    ZInternalID m_id ;
    QColor m_color ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZWTRACK_H
