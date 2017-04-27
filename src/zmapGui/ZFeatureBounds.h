/*  File: ZFeatureBounds.h
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

#ifndef ZFEATUREBOUNDS_H
#define ZFEATUREBOUNDS_H

#include <QtGlobal>

// This bounds class is used by graphics objects only (i.e.
// the QGraphicsItem related stuff)

namespace ZMap
{

namespace GUI
{

class ZFeatureBounds
{
public:
    ZFeatureBounds()
        : start(0.0), end(0.0)
    {}
    ZFeatureBounds(const qreal &s, const qreal &e)
        : start(s), end(e)
    {}
    qreal start, end ;
};

} // namespace GUI

} // namespace ZMap


#endif //
