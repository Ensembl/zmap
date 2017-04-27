/*  File: ZFeaturePainter.h
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

#ifndef ZFEATUREPAINTER_H
#define ZFEATUREPAINTER_H

#include <QtGlobal>
#include <string>

QT_BEGIN_NAMESPACE
class QPainter ;
class QStyleOptionGraphicsItem ;
class QWidget ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

class ZFeature ;

class ZFeaturePainter
{
public:


    ZFeaturePainter() ;
    virtual ~ZFeaturePainter() ;
    virtual bool isCompound() const {return false ; }
    virtual bool add(ZFeaturePainter* ) {return false ; }
    virtual bool remove(ZFeaturePainter* ) {return false ; }
    virtual void paint(ZFeature *feature, QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget) = 0  ;
    virtual double getWidth() const {return 0.0 ; }
    virtual bool setWidth(const double &) {return false ; }
};

} // namespace GUI

} // namespace ZMap

#endif // ZFEATUREPAINTER_H
