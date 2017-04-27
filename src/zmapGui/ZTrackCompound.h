/*  File: ZTrackCompound.h
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

#ifndef ZTRACKCOMPOUND_H
#define ZTRACKCOMPOUND_H

#include <QColor>
#include "ZTrack.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>

namespace ZMap
{

namespace GUI
{

// contains tracks only
class ZTrackCompound : public ZTrack,
        public Util::InstanceCounter<ZTrackCompound>,
        public Util::ClassName<ZTrackCompound>
{
public:

    ZTrackCompound(const QColor & color);
    ~ZTrackCompound() ;

    bool addChildItem(QGraphicsItem*) override ;
    void internalReorder() override ;

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) Q_DECL_OVERRIDE ;

private:
};

} // namespace GUI

} // namespace ZMap

#endif // ZTRACKCOMPOUND_H
