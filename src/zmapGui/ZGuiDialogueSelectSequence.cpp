/*  File: ZGuiDialogueSelectSequence.cpp
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

#include "ZGuiDialogueSelectSequence.h"
#include "ZGuiWidgetSelectSequence.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QPushButton>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiDialogueSelectSequence>::m_name("ZGuiDialogueSelectSequence") ;
const QString ZGuiDialogueSelectSequence::m_display_name("ZMap Select a Sequence ") ;

ZGuiDialogueSelectSequence::ZGuiDialogueSelectSequence(QWidget* parent)
    : QDialog(parent),
      Util::InstanceCounter<ZGuiDialogueSelectSequence>(),
      Util::ClassName<ZGuiDialogueSelectSequence>(),
      m_top_layout(Q_NULLPTR),
      m_select_sequence(Q_NULLPTR),
      m_button_close(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiDialogueSelectSequence::ZGuiDialogueSelectSequence() ; cannot create instance without QApplication existing ")) ;

    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectSequence::ZGuiDialogueSelectSequence() ; unable to create top level layout ")) ;
    setLayout(m_top_layout) ;

    if (!(m_select_sequence = ZGuiWidgetSelectSequence::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectSequence::ZGuiDialogueSelectSequence() ; unable to create ZGuiWidgetSelectSequence")) ;
    m_top_layout->addWidget(m_select_sequence) ;

    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectSequence::ZGuiDialogueSelectSequence() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    QWidget *widget = Q_NULLPTR ;
    if (!(widget = createControls01()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectSequence::ZGuiDialogueSelectSequence() ; call to createControls01() failed ")) ;
    m_top_layout->addWidget(widget) ;

    // final window settings
    setWindowTitle(m_display_name) ;
}

ZGuiDialogueSelectSequence::~ZGuiDialogueSelectSequence()
{
}

bool ZGuiDialogueSelectSequence::setSelectSequenceData(const ZGSelectSequenceData &data)
{
    bool result = false ;

    if (m_select_sequence)
    {
        result = m_select_sequence->setSelectSequenceData(data) ;
    }

    return result ;
}

ZGSelectSequenceData ZGuiDialogueSelectSequence::getSelectSequenceData() const
{
    if (m_select_sequence)
    {
        return m_select_sequence->getSelectSequenceData() ;
    }
    else
    {
        return getSelectSequenceData() ;
    }
}

ZGuiDialogueSelectSequence* ZGuiDialogueSelectSequence::newInstance(QWidget *parent )
{
    ZGuiDialogueSelectSequence *selector = Q_NULLPTR ;

    try
    {
        selector = new ZGuiDialogueSelectSequence(parent) ;
    }
    catch (...)
    {
        selector = Q_NULLPTR ;
    }

    return selector ;
}


void ZGuiDialogueSelectSequence::closeEvent(QCloseEvent *)
{
    emit signal_received_close_event() ;
}


// the usual box of buttons
QWidget *ZGuiDialogueSelectSequence::createControls01()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectSequence::createControl01() ; unable to create widget ")) ;

    QHBoxLayout * hlayout = Q_NULLPTR ;
    if (!(hlayout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectSequence::createControls01() ; unable to create hlayout ")) ;
    widget->setLayout(hlayout) ;
    hlayout->setMargin(0) ;

    if (!(m_button_close = ZGuiWidgetCreator::createPushButton(QString("Close"))))
        throw std::runtime_error(std::string("ZGuiDialogueSelectSequence::createControls01() ; unable to create close button ")) ;
    if (!connect(m_button_close, SIGNAL(clicked(bool)), this, SLOT(reject())))
        throw std::runtime_error(std::string("ZGuiDialogueSelectSequence::createControls01() ; unable to connect close button to slot ")) ;

    hlayout->addWidget(m_button_close) ;
    hlayout->addStretch(100) ;

    return widget ;
}


////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////


void ZGuiDialogueSelectSequence::slot_close()
{
    emit signal_received_close_event() ;
}

} // namespace GUI

} // namespace ZMap

