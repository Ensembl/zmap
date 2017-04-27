/*  File: ZGuiFeaturePainterCompound.cpp
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

#include "ZFeaturePainterCompound.h"
#include <algorithm>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZFeaturePainterCompound>::m_name ("ZFeaturePainterCompound") ;

ZFeaturePainterCompound::ZFeaturePainterCompound()
    : ZFeaturePainter(),
      Util::InstanceCounter<ZFeaturePainterCompound>(),
      Util::ClassName<ZFeaturePainterCompound>(),
      m_components()
{
}

ZFeaturePainterCompound::~ZFeaturePainterCompound()
{
    std::set<ZFeaturePainter*>::iterator it
            = m_components.begin() ;
    for( ; it!=m_components.end() ; ++it)
    {
        if (*it)
            delete *it ;
    }
}

bool ZFeaturePainterCompound::add(ZFeaturePainter *painter)
{
    bool result = false;
    if (painter && !isPresent(painter))
    {
        m_components.insert(painter) ;
        result = true ;
    }
    return result ;
}

double ZFeaturePainterCompound::getWidth() const
{
    double result = 0.0 ;
    if (m_components.size())
    {
        std::set<ZFeaturePainter*>::const_iterator it = m_components.begin() ;
        if (*it)
            result = (*it)->getWidth() ;
    }
    return result ;
}

bool ZFeaturePainterCompound::setWidth(const double& value)
{
    if (value <= 0.0)
        return false ;
    std::set<ZFeaturePainter*>::iterator it
            = m_components.begin() ;
    for( ; it!=m_components.end() ; ++it)
    {
        if (*it)
            (*it)->setWidth(value) ;
    }
    return true ;
}

bool ZFeaturePainterCompound::remove(ZFeaturePainter *painter)
{
    bool result = false ;
    std::set<ZFeaturePainter*>::iterator it
            = std::find(m_components.begin(), m_components.end(), painter) ;
    if (it != m_components.end())
    {
        if (*it)
            delete *it ;
        m_components.erase(it) ;
        result = true ;
    }
    return result ;
}

bool ZFeaturePainterCompound::isPresent(ZFeaturePainter *painter) const
{
    bool result = false ;
    std::set<ZFeaturePainter*>::const_iterator it
            = std::find(m_components.begin(), m_components.end(), painter) ;
    if (it != m_components.end())
    {
        result = true ;
    }
    return result ;
}

void ZFeaturePainterCompound::paint(ZFeature *feature, QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    std::set<ZFeaturePainter*>::iterator it
            = m_components.begin() ;
    for( ; it!=m_components.end() ; ++it)
    {
        if (*it)
            (*it)->paint(feature, painter, option, widget) ;
    }
}

} // namespace GUI

} // namespace ZMap
