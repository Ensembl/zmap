/*  File: ZGuiDialogueSearchDNA.cpp
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

#include "ZGuiDialogueSearchDNA.h"
#include "ZGuiDialogueSearchDNAStyle.h"
#include "ZGuiTextDisplayDialogue.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiWidgetButtonColour.h"
#include "ZGuiSequenceValidatorDNA.h"
#include "ZGuiWidgetComboStrand.h"
#include <stdexcept>
#include <climits>
#include <sstream>


#include <QtWidgets>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QColorDialog>

#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiDialogueSearchDNA>::m_name("ZGuiDialogueSearchDNA") ;
const QString ZGuiDialogueSearchDNA::m_display_name("ZMap DNA Search"),
ZGuiDialogueSearchDNA::m_help_text_overview("The ZMap DNA Search Window allows you to search for DNA. You can cut/paste DNA into the\n"
                                            "DNA text field or type it in. You can specify the maximum number of mismatches\n"
                                            "and the maximum number of ambiguous/unknown bases acceptable in the match."),
ZGuiDialogueSearchDNA::m_help_text_title_overview("ZMap DNA Search: Help") ;

ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA(QWidget *parent)
    : QMainWindow(parent),
      Util::InstanceCounter<ZGuiDialogueSearchDNA>(),
      Util::ClassName<ZGuiDialogueSearchDNA>(),
      m_top_layout(Q_NULLPTR),
      m_text_display_dialogue(Q_NULLPTR),
      m_menu_file(Q_NULLPTR),
      m_menu_help(Q_NULLPTR),
      m_action_file_close(Q_NULLPTR),
      m_action_help_overview(Q_NULLPTR),
      m_sequence_text(Q_NULLPTR),
      m_entry_strand(Q_NULLPTR),
      m_entry_start(Q_NULLPTR),
      m_entry_end(Q_NULLPTR),
      m_entry_mis(Q_NULLPTR),
      m_entry_bas(Q_NULLPTR),
      m_button_col_forward(Q_NULLPTR),
      m_button_col_reverse(Q_NULLPTR),
      m_button_clear(Q_NULLPTR),
      m_button_find(Q_NULLPTR),
      m_check_keep(Q_NULLPTR),
      m_col_forward(),
      m_col_reverse(),
      m_validator(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; unable to create without QApplication instance existsing")) ;

    // menu bar
    QMenuBar * menu_bar = Q_NULLPTR ;
    try
    {
        menu_bar = menuBar() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; caught exception in attempt to create menu_bar:") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; caught unknown exception in attempt to create menu_bar")) ;
    }
    if (!menu_bar)
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; unable to create menu_bar")) ;

    // menu bar style
    ZGuiDialogueSearchDNAStyle *style = Q_NULLPTR ;
    try
    {
        style = ZGuiDialogueSearchDNAStyle::newInstance() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; caught exception in attempt to create style:") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; caught unknown exception in attempt to create style")) ;
    }
    if (!style)
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; unable to create style")) ;
    style->setParent(this) ;
    menu_bar->setStyle(style) ;

    // status bar
    QStatusBar *status_bar = Q_NULLPTR ;
    try
    {
        status_bar = statusBar() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; caught exception in attempt to create status_bar: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; caught unknown exception in attempt to create status_bar")) ;
    }
    if (!status_bar)
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; unable to create status_bar")) ;

    // create actions
    try
    {
        createFileMenuActions();
        createHelpMenuActions() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; caught exception in attempt to create actions: ") +error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; caught unknown exception in attempt to create actions ")) ;
    }

    // do menus
    try
    {
        createAllMenus() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNAStyle::ZGuiDialogueSearchDNAStyle() ; caught exception in call to createAllMenus(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNAStyle::ZGuiDialogueSearchDNAStyle() ; caught unknown exception in call to createAllMenus()")) ;
    }

    // top level layout
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; unable to create top_layout ")) ;

    // do central widget stuff
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; unable to create central widget")) ;
    try
    {
        setCentralWidget(widget) ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; caught exception on attempt to set central widget:  ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; caught unknown exception on attempt to set central widget ")) ;
    }
    widget->setLayout(m_top_layout) ;

    // first box of controls...
    if (!(widget = createControls01()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; unable to create widget from createControls01() ")) ;
    m_top_layout->addWidget(widget) ;

    // second box
    if (!(widget = createControls02()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; call to createControls02() failed")) ;
    m_top_layout->addWidget(widget) ;

    // third box
    if (!(widget = createControls03()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; call to createControls03() failed ")) ;
    m_top_layout->addWidget(widget) ;

    // next set of controls
    if (!(widget = createControls04()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; call to createControls04() failed")) ;
    m_top_layout->addWidget(widget) ;

    // frame as a separator
    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() : unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // final box at the bottom for buttons
    if (!(widget = createControls05()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; call to createControls05() failed ")) ;
    m_top_layout->addWidget(widget) ;

    // ...and right at the bottom
    m_top_layout->addStretch(100) ;

    // other window settings
    setWindowTitle(m_display_name) ;
    setAttribute(Qt::WA_DeleteOnClose) ;
}


ZGuiDialogueSearchDNA::~ZGuiDialogueSearchDNA()
{
}

ZGuiDialogueSearchDNA* ZGuiDialogueSearchDNA::newInstance(QWidget *parent)
{
    ZGuiDialogueSearchDNA *search = Q_NULLPTR ;

    try
    {
        search = new ZGuiDialogueSearchDNA(parent) ;
    }
    catch (...)
    {
        search = Q_NULLPTR ;
    }

    return search ;
}

void ZGuiDialogueSearchDNA::closeEvent(QCloseEvent *)
{
    emit signal_received_close_event() ;
}

void ZGuiDialogueSearchDNA::createAllMenus()
{
    createFileMenu() ;
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
        menu_bar->addSeparator() ;
    createHelpMenu() ;
}

void ZGuiDialogueSearchDNA::createFileMenu()
{
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_file = menu_bar->addMenu(QString("&File")) ;
        if (!m_menu_file)
            throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createFileMenu() ; could not create file menu ")) ;

        if (m_action_file_close)
            m_menu_file->addAction(m_action_file_close) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createFileMenu() ; could not access menu bar ")) ;
    }
}

void ZGuiDialogueSearchDNA::createHelpMenu()
{
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_help = menu_bar->addMenu(QString("&Help")) ;
        if (!m_menu_help)
            throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createHelpMenu() ; could not create help menu")) ;

        if (m_action_help_overview)
            m_menu_help->addAction(m_action_help_overview) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createHelpMenu() ; could not access menu bar ")) ;
    }
}

void ZGuiDialogueSearchDNA::createFileMenuActions()
{
    if (!m_action_file_close && (m_action_file_close = ZGuiWidgetCreator::createAction(QString("Close"), this)))
    {
        m_action_file_close->setShortcut(QKeySequence::Close) ;
        m_action_file_close->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_close->setStatusTip(QString("Close the DNA search dialogue")) ;
        addAction(m_action_file_close) ;
        connect(m_action_file_close, SIGNAL(triggered(bool)), this, SLOT(slot_action_file_close())) ;
    }
}

void ZGuiDialogueSearchDNA::createHelpMenuActions()
{
    if (!m_action_help_overview && (m_action_help_overview = ZGuiWidgetCreator::createAction(QString("Overview"), this)))
    {
        m_action_help_overview->setStatusTip(QString("Help with this dialogue ")) ;
        addAction(m_action_help_overview) ;
        connect(m_action_help_overview, SIGNAL(triggered(bool)), this, SLOT(slot_action_help_overview())) ;
    }
}



// sequence entry controls
QWidget * ZGuiDialogueSearchDNA::createControls01()
{
    QGroupBox *box = Q_NULLPTR ;
    if (!(box = ZGuiWidgetCreator::createGroupBox(QString("Query"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::ZGuiDialogueSearchDNA() ; unable to create query group box ")) ;

    // sequence entry box
    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls01() ; unable to create layout")) ;
    box->setLayout(layout) ;
    //layout->setMargin(0) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Sequence: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls01() ; unable to create label ")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;
    label = Q_NULLPTR ;
    if (!(m_sequence_text = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls01() ; unable to create sequence text ")) ;
    m_sequence_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed) ;
    layout->addWidget(m_sequence_text) ;
    if (!(m_validator = ZGuiSequenceValidatorDNA::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls01() ; unable to create DNA validator ")) ;
    m_validator->setParent(this) ;
    m_sequence_text->setValidator(m_validator) ;

    return box ;

}

// strand/start/end controls
QWidget *ZGuiDialogueSearchDNA::createControls02()
{
    QGroupBox *box = Q_NULLPTR ;
    if (!(box = ZGuiWidgetCreator::createGroupBox(QString("Set strand/start/end"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls02() ; unable to create group box ")) ;

    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls02() ; unable to create second layout")) ;
    box->setLayout(layout) ;
    //layout->setMargin(0) ;

    QLabel* label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Strand:"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls02() ; unable to create strand label ")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;
    label = Q_NULLPTR ;

    if (!(m_entry_strand = ZGuiWidgetComboStrand::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls02() ; unable to create entry strand ")) ;
    layout->addWidget(m_entry_strand, 0, Qt::AlignLeft) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Start:"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls02() ; unable to create start label ")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;
    label = Q_NULLPTR ;

    if (!(m_entry_start = ZGuiWidgetCreator::createSpinBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls02() ; unable to create spin box ")) ;
    m_entry_start->setRange(1, INT_MAX) ;
    m_entry_start->setValue(m_entry_start->minimum());
    layout->addWidget(m_entry_start, 0, Qt::AlignLeft) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("End:"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls02() ; unable to create end label ")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;
    label = Q_NULLPTR ;

    if (!(m_entry_end = ZGuiWidgetCreator::createSpinBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls02() ; unable to create entry end ")) ;
    m_entry_end->setRange(1, INT_MAX) ;
    m_entry_end->setValue(m_entry_end->maximum()) ;
    layout->addWidget(m_entry_end, 0, Qt::AlignLeft) ;

    layout->addStretch(100) ;

    return box  ;
}

QWidget *ZGuiDialogueSearchDNA::createControls03()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createGroupBox(QString("Error rates"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls03() ; unable to create widget ")) ;

    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls03() ; unable to create layout ")) ;
    widget->setLayout(layout) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Mismatches: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls03() ; unable to create label ")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;
    label = Q_NULLPTR ;

    if (!(m_entry_mis = ZGuiWidgetCreator::createSpinBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls03() ; unable to create entry mismatches")) ;
    m_entry_mis->setRange(0, 1000) ;
    layout->addWidget(m_entry_mis, 0, Qt::AlignLeft) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("N bases: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls03() ; unable to create n-bases label ")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;
    label = Q_NULLPTR ;

    if (!(m_entry_bas = ZGuiWidgetCreator::createSpinBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls03() ; unable to create n-bases spin box ")) ;
    m_entry_bas->setRange(0, 100) ;
    layout->addWidget(m_entry_bas, 0, Qt::AlignLeft) ;

    layout->addStretch(100) ;

    return widget ;
}

QWidget *ZGuiDialogueSearchDNA::createControls04()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createGroupBox(QString("Hit properties"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls04() ; unable to create hit properties group box ")) ;

    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls04() ; unable to create layout ")) ;
    widget->setLayout(layout) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Forward color: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls04() ; unable to create forward color label ")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;
    label = Q_NULLPTR ;

    if (!(m_button_col_forward = ZGuiWidgetButtonColour::newInstance(m_col_forward)))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls04() ; unable to create forward color button ")) ;
    if (!connect(m_button_col_forward, SIGNAL(clicked(bool)), this, SLOT(slot_color_select_forward())))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls04() ; unable to connect forward color button ")) ;
    layout->addWidget(m_button_col_forward, 0, Qt::AlignLeft) ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Reverse color: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls04() ; unable to create reverse color label ")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;
    label = Q_NULLPTR ;
    if (!(m_button_col_reverse = ZGuiWidgetButtonColour::newInstance(m_col_reverse)))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls04() ; unable to create reverse color button ")) ;
    if (!connect(m_button_col_reverse, SIGNAL(clicked(bool)), this, SLOT(slot_color_select_reverse())))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls04() ; unable to connect reverse color button")) ;
    layout->addWidget(m_button_col_reverse, Qt::AlignLeft) ;

    if (!(m_check_keep = ZGuiWidgetCreator::createCheckBox(QString("Keep previous search: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls04() ; unable to create keep previous search check box")) ;
    m_check_keep->setLayoutDirection(Qt::LayoutDirection::RightToLeft ) ;
    layout->addWidget(m_check_keep, 0, Qt::AlignLeft) ;

    layout->addStretch(100) ;

    return widget ;
}

QWidget * ZGuiDialogueSearchDNA::createControls05()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls05() ; unable to create widget")) ;

    QHBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls05() ; unable to create layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    if (!(m_button_clear = ZGuiWidgetCreator::createPushButton(QString("Clear"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls05() ; unable to create button clear ")) ;
    layout->addWidget(m_button_clear, 0, Qt::AlignLeft) ;
    layout->addStretch(100) ;
    if (!(m_button_find = ZGuiWidgetCreator::createPushButton(QString("Find"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchDNA::createControls05() ; unable to create button find ")) ;
    layout->addWidget(m_button_find, 0, Qt::AlignRight) ;

    return widget ;
}

// settings from input object
bool ZGuiDialogueSearchDNA::setSearchDNAData(const ZGSearchDNAData& data)
{
    bool result = false ;

    QString sequence = QString::fromStdString(data.getSequence()).toLower() ;
    if (m_validator && m_sequence_text)
    {
        int i = 0 ;
        if (m_validator->validate(sequence, i) == QValidator::Invalid)
            return result ;
        m_sequence_text->setText(sequence) ;
    }

    // test for validity of arg here
    if (m_entry_strand)
    {
        m_entry_strand->setStrand(data.getStrand()) ;
    }

    if (m_entry_start)
    {
        m_entry_start->setValue(static_cast<int>(data.getStart())) ;
    }

    if (m_entry_end)
    {
        m_entry_end->setValue(static_cast<int>(data.getEnd())) ;
    }

    if (m_entry_mis)
    {
        m_entry_mis->setValue(static_cast<int>(data.getMismatches())) ;
    }

    if (m_entry_bas)
    {
        m_entry_bas->setValue(static_cast<int>(data.getBases())) ;
    }

    if (m_button_col_forward)
        m_button_col_forward->setColour(data.getColorForward()) ;

    if (m_button_col_reverse)
        m_button_col_reverse->setColour(data.getColorReverse()) ;

    if (m_check_keep)
        m_check_keep->setChecked(data.getPreserve()) ;

    result = true ;

    return result ;
}

// return all settings
ZGSearchDNAData ZGuiDialogueSearchDNA::getSearchDNAData() const
{
    ZGSearchDNAData data ;

    QString sequence ;
    if (m_sequence_text)
        sequence = m_sequence_text->text().toLower() ;
    data.setSequence(sequence.toStdString()) ;

    if (m_entry_strand)
    {
        data.setStrand(m_entry_strand->getStrand()) ;
    }

    if (m_entry_start)
    {
        data.setStart(static_cast<uint32_t>(m_entry_start->text().toUInt())) ;
    }

    if (m_entry_end)
    {
        data.setEnd(static_cast<uint32_t>(m_entry_end->text().toUInt())) ;
    }

    if (m_entry_mis)
    {
        data.setMismatches(static_cast<uint32_t>(m_entry_mis->text().toUInt())) ;
    }

    if (m_entry_bas)
    {
        data.setBases(static_cast<uint32_t>(m_entry_bas->text().toUInt())) ;
    }

    if (m_button_col_forward)
    {
        data.setColorForward(m_button_col_forward->getColour()) ;
    }

    if (m_button_col_reverse)
    {
        data.setColorReverse(m_button_col_reverse->getColour()) ;
    }

    if (m_check_keep)
    {
        data.setPreserve(m_check_keep->isChecked()) ;
    }

    return data ;
}



////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiDialogueSearchDNA::slot_close()
{
    emit signal_received_close_event() ;
}

void ZGuiDialogueSearchDNA::slot_color_select_forward()
{
    if (m_button_col_forward)
    {
        m_col_forward = m_button_col_forward->getColour() ;
    }
}

void ZGuiDialogueSearchDNA::slot_color_select_reverse()
{
    if (m_button_col_reverse)
    {
        m_col_reverse = m_button_col_reverse->getColour() ;
    }
}


// attached to appropriate menu action
void ZGuiDialogueSearchDNA::slot_action_file_close()
{
    emit signal_received_close_event() ;
}

// attached to appropriate menu action
void ZGuiDialogueSearchDNA::slot_action_help_overview()
{
    if (!m_text_display_dialogue)
    {
        try
        {
            m_text_display_dialogue = ZGuiTextDisplayDialogue::newInstance(this) ;
            connect(m_text_display_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_help_closed())) ;
        }
        catch (...)
        {
            m_text_display_dialogue = Q_NULLPTR ;
            return ;
        }
    }
    m_text_display_dialogue->setText(m_help_text_overview) ;
    m_text_display_dialogue->setWindowTitle(m_help_text_title_overview) ;
    m_text_display_dialogue->show() ;
}


void ZGuiDialogueSearchDNA::slot_help_closed()
{
    if (m_text_display_dialogue)
    {
        m_text_display_dialogue->close() ;
        m_text_display_dialogue = Q_NULLPTR ;
    }
}

} // namespace GUI

} // namespace ZMap
