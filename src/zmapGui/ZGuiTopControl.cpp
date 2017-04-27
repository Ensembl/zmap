/*  File: ZGuiTopControl.cpp
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

#include "ZGuiTopControl.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>
#include <QHBoxLayout>
#include <QPushButton>


namespace ZMap
{

namespace GUI
{
template <>
const std::string Util::ClassName<ZGuiTopControl>::m_name ("ZGuiTopControl") ;

ZGuiTopControl::ZGuiTopControl(QWidget *parent)
    : QWidget(parent),
      Util::InstanceCounter<ZGuiTopControl>(),
      Util::ClassName<ZGuiTopControl>(),
      m_layout(Q_NULLPTR),
      m_button1(Q_NULLPTR),
      m_button2(Q_NULLPTR),
      m_button3(Q_NULLPTR)
{
    if (!(m_layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTopControl::ZGuiTopControl() ; unable to create top level layout")) ;
    setLayout(m_layout) ;
    m_layout->setSizeConstraint(QLayout::SetFixedSize) ;

    if (!(m_button1 = ZGuiWidgetCreator::createPushButton(QString("Top control 1"))))
        throw std::runtime_error(std::string("ZGuiTopControl::ZGuiTopControl() ; unable to create top control 1")) ;
    m_layout->addWidget(m_button1) ;

    if (!(m_button2 = ZGuiWidgetCreator::createPushButton(QString("Top control 2"))))
        throw std::runtime_error(std::string("ZGuiTopControl::ZGuiTopControl() ; unable to create top control 2 ")) ;
    m_layout->addWidget(m_button2) ;

    if (!(m_button3 = ZGuiWidgetCreator::createPushButton(QString("Top control 3"))))
        throw std::runtime_error(std::string("ZGuiTopControl::ZGuiTopControl() ; unable to create top control 3 ")) ;
    m_layout->addWidget(m_button3) ;
}

ZGuiTopControl::~ZGuiTopControl()
{
}



ZGuiTopControl* ZGuiTopControl::newInstance(QWidget* parent)
{
    ZGuiTopControl* control = Q_NULLPTR ;

    try
    {
        control = new ZGuiTopControl(parent) ;
    }
    catch (...)
    {
        control = Q_NULLPTR ;
    }

    return control ;
}

} // namespace GUI

} // namespace ZMap

