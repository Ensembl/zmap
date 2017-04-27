/*  File: ZGuiCoordinateValidator.cpp
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

#include "ZGuiCoordinateValidator.h"

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiCoordinateValidator>::m_name("ZGuiCoordinateValidator") ;

const uint32_t ZGuiCoordinateValidator::m_min_default(1),
    ZGuiCoordinateValidator::m_max_default(UINT32_MAX) ;

ZGuiCoordinateValidator::ZGuiCoordinateValidator()
    : QValidator(Q_NULLPTR),
      Util::InstanceCounter<ZGuiCoordinateValidator>(),
      Util::ClassName<ZGuiCoordinateValidator>(),
      m_min(m_min_default),
      m_max(m_max_default)
{
}

ZGuiCoordinateValidator::~ZGuiCoordinateValidator()
{
}


QValidator::State ZGuiCoordinateValidator::validate(QString& data, int &) const
{
    State state = Invalid ;
    uint32_t value = 0 ;
    bool ok = false ;

    if (!data.length())
        state = Intermediate ;
    else
    {
        value = data.toUInt(&ok) ;
        if (ok)
        {
            if (value > m_max)
                state = Invalid ;
            else if (value < m_min)
                state = Intermediate ;
            else
                state = Acceptable ;
        }
    }

    return state ;
}

ZGuiCoordinateValidator * ZGuiCoordinateValidator::newInstance()
{
    ZGuiCoordinateValidator * validator = Q_NULLPTR ;

    try
    {
        validator = new ZGuiCoordinateValidator ;
    }
    catch (...)
    {
        validator = Q_NULLPTR ;
    }

    return validator ;
}


bool ZGuiCoordinateValidator::setRange(uint32_t min, uint32_t max)
{
    bool result = false ;

    if (min <= max)
    {
        m_min = min ;
        m_max = max ;
        result = true ;
    }

    return result ;
}


} // namespace GUI

} // namespace ZMap
