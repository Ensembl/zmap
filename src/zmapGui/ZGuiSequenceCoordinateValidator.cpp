/*  File: ZGuiSequenceCoordinateValidator.cpp
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

#include "ZGuiSequenceCoordinateValidator.h"

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiSequenceCoordinateValidator>::m_name("ZGuiSequenceCoordinateValidator") ;
const uint32_t ZGuiSequenceCoordinateValidator::m_min(1) ;

ZGuiSequenceCoordinateValidator::ZGuiSequenceCoordinateValidator()
    : QValidator(Q_NULLPTR),
      Util::InstanceCounter<ZGuiSequenceCoordinateValidator>(),
      Util::ClassName<ZGuiSequenceCoordinateValidator>()
{
}

ZGuiSequenceCoordinateValidator::~ZGuiSequenceCoordinateValidator()
{
}

QValidator::State ZGuiSequenceCoordinateValidator::validate(QString &data, int &) const
{
    State state = Invalid ;
    uint32_t value = 0 ;
    bool ok = false ;

    if (!data.length())
        state = Intermediate ;
    else if (((value = data.toUInt(&ok)) >= m_min) && ok )
        state = Acceptable ;

    return state ;
}

ZGuiSequenceCoordinateValidator *ZGuiSequenceCoordinateValidator::newInstance()
{
    ZGuiSequenceCoordinateValidator * validator = Q_NULLPTR ;

    try
    {
        validator = new ZGuiSequenceCoordinateValidator ;
    }
    catch (...)
    {
        validator = Q_NULLPTR ;
    }

    return validator ;
}

} // namespace GUI

} // namespace ZMap

