/*  File: ZGuiMainStyle.cpp
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

#include "ZGuiMainStyle.h"


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiMainStyle>::m_name("ZGuiMainStyle") ;

ZGuiMainStyle::ZGuiMainStyle()
    : QProxyStyle(),
      Util::InstanceCounter<ZGuiMainStyle>(),
      Util::ClassName<ZGuiMainStyle>()
{
}

ZGuiMainStyle::~ZGuiMainStyle()
{
}

ZGuiMainStyle* ZGuiMainStyle::newInstance()
{
    ZGuiMainStyle *style = Q_NULLPTR ;

    try
    {
        style = new ZGuiMainStyle ;
    }
    catch (...)
    {
        style = Q_NULLPTR ;
    }

    return style ;
}


// return the modified style hint
int ZGuiMainStyle::styleHint( StyleHint hint,
               const QStyleOption * option,
               const QWidget * widget,
               QStyleHintReturn * returnData) const
{
    if ( hint == SH_DrawMenuBarSeparator )
        return 1 ;
    else
        return QProxyStyle::styleHint(hint, option, widget, returnData);
}

} // namespace GUI

} // namespace ZMap

