/*  File: ZGuiTextDisplayDialogue.cpp
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

#include "ZGuiTextDisplayDialogue.h"
#include "ZGuiWidgetCreator.h"
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiTextDisplayDialogue>::m_name("ZGuiTextDisplayDialogue") ;
const QString ZGuiTextDisplayDialogue::m_display_name("ZMap Text Display"),
              ZGuiTextDisplayDialogue::m_placeholder_text("ZMap Annotation Tool; text display") ;

ZGuiTextDisplayDialogue::ZGuiTextDisplayDialogue(QWidget* parent)
    : QDialog(parent),
      Util::InstanceCounter<ZGuiTextDisplayDialogue>(),
      Util::ClassName<ZGuiTextDisplayDialogue>(),
      m_text_edit(Q_NULLPTR)
{
    QVBoxLayout *top_layout = Q_NULLPTR ;
    if (!(top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTextDisplayDialogue::ZGuiTextDisplayDialogue() ; unable to create top_layout")) ;
    setLayout(top_layout) ;

    if (!(m_text_edit = ZGuiWidgetCreator::createTextEdit()))
        throw std::runtime_error(std::string("ZGuiTextDisplayDialogue::ZGuiTextDisplayDialogue() ; unable to create m_text_edit ")) ;
    m_text_edit->setParent(this) ;
    m_text_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;
    m_text_edit->setReadOnly(true) ;
    top_layout->addWidget(m_text_edit) ;

    // control box
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = createControls01()))
        throw std::runtime_error(std::string("ZGuiTextDisplayDialogue::ZGuiTextDisplayDialogue() ; call to createControls01() failed ")) ;
    top_layout->addWidget(widget) ;

    // some dummy text here
    m_text_edit->setPlaceholderText(m_placeholder_text) ;
    m_text_edit->setLineWrapMode(QTextEdit::NoWrap) ;

    // other window settings
    setWindowTitle(m_display_name) ;
    //setAttribute(Qt::WA_DeleteOnClose) ;
    setModal(false) ;
    // yeah well X ignores this anyway...
    //setWindowFlags(windowFlags() & ~Qt::WindowStaysOnBottomHint) ;
}

ZGuiTextDisplayDialogue::~ZGuiTextDisplayDialogue()
{
}

ZGuiTextDisplayDialogue* ZGuiTextDisplayDialogue::newInstance(QWidget *parent)
{
    ZGuiTextDisplayDialogue * dialogue = Q_NULLPTR ;

    try
    {
        dialogue = new ZGuiTextDisplayDialogue(parent) ;
    }
    catch (...)
    {
        dialogue = Q_NULLPTR ;
    }

    return dialogue ;
}

void ZGuiTextDisplayDialogue::closeEvent(QCloseEvent*)
{
    emit signal_received_close_event() ;
}

void ZGuiTextDisplayDialogue::setText(const QString& text)
{
    m_text_edit->setText(text) ;
}

void ZGuiTextDisplayDialogue::setText(const std::string& text)
{
    m_text_edit->setText(QString::fromStdString(text)) ;
}

QWidget* ZGuiTextDisplayDialogue::createControls01()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiTextDisplayDialogue::createControls01() ; unable to create widget ")) ;


    // buttons layout
    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTextDisplayDialogue::ZGuiTextDisplayDialogue() ; unable to create button_layout")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    // button for button layout
    QPushButton *button = Q_NULLPTR ;
    if (!(button = ZGuiWidgetCreator::createPushButton(QString("Close"))))
        throw std::runtime_error(std::string("ZGuiTextDisplayDialogue::ZGuiTextDisplayDialogue() ; unable to create button")) ;
    layout->addWidget(button, 0, Qt::AlignRight) ;
    if (!connect(button, SIGNAL(clicked(bool)), this, SLOT(close())))
        throw std::runtime_error(std::string("ZGuiTextDisplayDialogue::ZGuiTextDisplayDialogue() ; unable to connect button to slot_close")) ;

    return widget ;
}


} // namespace GUI

} // namespace ZMap
