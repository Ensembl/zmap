/*  File: ZGuiWidgetButtonColour.cpp
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

#include "ZGuiWidgetButtonColour.h"
#include <QColorDialog>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetButtonColour>::m_name("ZGuiWidgetButtonColour") ;
const QString ZGuiWidgetButtonColour::m_style_string("background-color:%1") ;

ZGuiWidgetButtonColour::ZGuiWidgetButtonColour(const QColor& colour,
                                               QWidget *parent)
    : QPushButton(parent),
      Util::InstanceCounter<ZGuiWidgetButtonColour>(),
      Util::ClassName<ZGuiWidgetButtonColour>(),
      m_colour(colour)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetButtonColour::ZGuiWidgetButtonColour() ; unable to create instance with out QApplication existing")) ;

    if (!connect(this, SIGNAL(clicked(bool)), this, SLOT(slot_clicked())))
        throw std::runtime_error(std::string("ZGuiWidgetButtonColour::ZGuiWidgetButtonColour() ; unable to connect this button to appropriate slot ")) ;

    setColour(m_colour) ;
}

ZGuiWidgetButtonColour::~ZGuiWidgetButtonColour()
{
}

ZGuiWidgetButtonColour* ZGuiWidgetButtonColour::newInstance(const QColor& col,
                                                       QWidget* parent)
{
    ZGuiWidgetButtonColour * button = Q_NULLPTR ;

    try
    {
        button = new ZGuiWidgetButtonColour(col, parent) ;
    }
    catch (...)
    {
        button = Q_NULLPTR ;
    }

    return button ;
}

void ZGuiWidgetButtonColour::setColour(const QColor& col)
{
    m_colour = col ;
    setStyleSheet(m_style_string.arg(m_colour.name())) ;
}

////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiWidgetButtonColour::slot_set_button_colour(const QColor & colour)
{
    setColour(colour) ;
}

// note that use of the native widget has been disabled here
void ZGuiWidgetButtonColour::slot_clicked()
{
    QColorDialog dialog ;
    dialog.setOption(QColorDialog::DontUseNativeDialog) ;
    dialog.setCurrentColor(m_colour) ;
    QColor old_colour = m_colour ;
    connect(&dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(slot_set_button_colour(QColor))) ;
    if (dialog.exec() == QDialog::Accepted)
    {
        setColour(dialog.currentColor()) ;
    }
    else
    {
        setColour(old_colour) ;
    }
}

} // namespace GUI

} // namespace ZMap

