/*  File: ZGuiFileChooser.cpp
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

#include "ZGuiFileChooser.h"

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiFileChooser>::m_name ("ZGuiFileChooser") ;

// note that using the native dialog on ubuntu linux sometimes
// causes a segfault if no filename is specified and we then emit accept.
// never got to the bottom of why that is, but that's why use of the
// native widget below is disabled
ZGuiFileChooser::ZGuiFileChooser(QWidget *parent,
                                 const QString& caption,
                                 const QString& directory)
    : Util::InstanceCounter<ZGuiFileChooser>(),
      Util::ClassName<ZGuiFileChooser>(),
      m_dialog(parent, caption, directory)
{
    m_dialog.setOption(QFileDialog::DontUseNativeDialog) ;
    m_dialog.setFileMode(QFileDialog::ExistingFile) ;
    m_dialog.setViewMode(QFileDialog::Detail) ;
}

ZGuiFileChooser::~ZGuiFileChooser()
{
}

ZGuiFileChooser* ZGuiFileChooser::newInstance(QWidget* parent,
                                              const QString& caption,
                                              const QString& directory )
{
    ZGuiFileChooser *chooser = nullptr ;

    try
    {
        chooser = new ZGuiFileChooser(parent, caption, directory) ;
    }
    catch (...)
    {
        chooser = nullptr ;
    }

    return chooser ;
}


int ZGuiFileChooser::exec()
{
    return m_dialog.exec() ;
}


void ZGuiFileChooser::setOption(QFileDialog::Option option, bool on)
{
    m_dialog.setOption(option, on) ;
}

} // namespace GUI

} // namespace ZMap

