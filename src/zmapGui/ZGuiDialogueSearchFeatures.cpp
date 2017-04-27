/*  File: ZGuiDialogueSearchFeatures.cpp
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

#include "ZGuiDialogueSearchFeatures.h"
#include "ZGuiDialogueSearchFeaturesStyle.h"
#include "ZGuiTextDisplayDialogue.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiWidgetComboStrand.h"
#include "ZGuiWidgetComboFrame.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>

#include <stdexcept>
#include <limits>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiDialogueSearchFeatures>::m_name("ZGuiDialogueSearchFeatures") ;
const QString ZGuiDialogueSearchFeatures::m_display_name("ZMap Feature Search"),
ZGuiDialogueSearchFeatures::m_help_text_general("The ZMap Search Window allows you to search for features using simple filtering.\n"
                                                "When initially shown the search window displays the names it has assigned to the\n"
                                                "displayed alignment, block, set and the feature you clicked on. You can either\n"
                                                "replace or augment these names with the \"*\" wild card to find sets of features,\n"
                                                "e.g. if the feature name is \"B0250\" then changing it to \"B025*\" will find all\n"
                                                "the features for the given align/block/set whose name begins with \"B025\".\n"
                                                "\n"
                                                "You can add wild cards to any of the fields except the strand and frame filters.\n"
                                                "The strand filter should be set to one of  +, -, . or *, where * means both strands\n"
                                                "and . means both strands will be shown if the feature is not strand sensitive,\n"
                                                "otherwise only the forward strand is shown.\n"
                                                "\n"
                                                "The frame filter should be set to one of  ., 0, 1, 2 or *, where * means all 3 frames\n"
                                                "and . the features are not frame sensitive so frame will be ignored."),

ZGuiDialogueSearchFeatures::m_help_text_title_general("ZMap Feature Search: Help") ;

const int ZGuiDialogueSearchFeatures::m_max_int_input_value = INT_MAX ;

ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures(QWidget *parent)
    : QMainWindow(parent),
      Util::InstanceCounter<ZGuiDialogueSearchFeatures>(),
      Util::ClassName<ZGuiDialogueSearchFeatures>(),
      m_top_layout(Q_NULLPTR),
      m_text_display_dialogue(Q_NULLPTR),
      m_menu_file(Q_NULLPTR),
      m_menu_help(Q_NULLPTR),
      m_action_file_close(Q_NULLPTR),
      m_action_help_general(Q_NULLPTR),
      m_entry_align(Q_NULLPTR),
      m_entry_block(Q_NULLPTR),
      m_entry_track(Q_NULLPTR),
      m_entry_featureset(Q_NULLPTR),
      m_entry_strand(Q_NULLPTR),
      m_entry_frame(Q_NULLPTR),
      m_entry_feature(Q_NULLPTR),
      m_entry_start(Q_NULLPTR),
      m_entry_end(Q_NULLPTR),
      m_entry_locus(Q_NULLPTR),
      m_button_search(Q_NULLPTR),
      m_button_cancel(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; unable to create without QApplication instance existing")) ;

    // menu bar
    QMenuBar* menu_bar = Q_NULLPTR ;
    try
    {
        menu_bar = menuBar() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught exception on attempt to access menu bar: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught unknown exception on attempt to access menu bar")) ;
    }
    if (!menu_bar)
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; unable to access menu bar ")) ;

    // menu bar style
    ZGuiDialogueSearchFeaturesStyle *style = Q_NULLPTR ;
    try
    {
        style = ZGuiDialogueSearchFeaturesStyle::newInstance() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught exception on attempt to create menu bar style: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught unknown exception on attempt to create menu bar style ")) ;
    }
    if (!style)
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; unable to create menu bar style ")) ;
    style->setParent(this) ;
    menu_bar->setStyle(style) ;

    // status bar
    QStatusBar *status_bar = Q_NULLPTR ;
    try
    {
        status_bar = statusBar() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught exception on attempt to create status bar: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught unknown exception on attempt to create status bar ")) ;
    }
    if (!status_bar)
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; unable to create status bar")) ;

    try
    {
        createFileMenuActions() ;
        createHelpMenuActions() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught exception in attempt to create actions: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught unknown exception in attempt create actions ")) ;
    }

    // do menus themselves...
    try
    {
        createAllMenus() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught exception on call to createAllMenus(): " ) + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught unknown exception on call to createAllMenus() ")) ;
    }

    // top level layout
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; unable to create top_layout ")) ;

    // central widget
    QWidget* widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; unable to create central widget ")) ;
    try
    {
        setCentralWidget(widget) ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught exception on attempt to set central widget: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; caught unknown exception on attempt to set central widget")) ;
    }
    widget->setLayout(m_top_layout) ;


    // first group box of controls
    if (!(widget = createControls01()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; call to createControls01() failed ")) ;
    m_top_layout->addWidget(widget) ;

    // second group box of controls
    if (!(widget = createControls02() ))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; call to createControls02() failed ")) ;
    m_top_layout->addWidget(widget) ;

    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // third box of controls
    if (!(widget = createControls03()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::ZGuiDialogueSearchFeatures() ; call to createControls03() failed ")) ;
    m_top_layout->addWidget(widget) ;

    // stretch added right at the bottom
    m_top_layout->addStretch(100) ;

    // some window settings to finish off with
    setWindowTitle(m_display_name) ;
    setAttribute(Qt::WA_DeleteOnClose) ;
}

ZGuiDialogueSearchFeatures::~ZGuiDialogueSearchFeatures()
{
}

ZGuiDialogueSearchFeatures* ZGuiDialogueSearchFeatures::newInstance(QWidget* parent)
{
    ZGuiDialogueSearchFeatures * search = Q_NULLPTR ;

    try
    {
        search = new ZGuiDialogueSearchFeatures(parent) ;
    }
    catch (...)
    {
        search = Q_NULLPTR ;
    }

    return search ;
}


void ZGuiDialogueSearchFeatures::closeEvent(QCloseEvent *)
{
    emit signal_received_close_event() ;
}

void ZGuiDialogueSearchFeatures::createAllMenus()
{
    createFileMenu() ;
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
        menu_bar->addSeparator() ;
    createHelpMenu() ;
}

void ZGuiDialogueSearchFeatures::createFileMenu()
{
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_file = menu_bar->addMenu(QString("&File")) ;
        if (!m_menu_file)
            throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createFileMenu() ; could not create file menu")) ;

        if (m_action_file_close)
            m_menu_file->addAction(m_action_file_close) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createFileMenu() ; could not access menu bar ")) ;
    }
}

void ZGuiDialogueSearchFeatures::createHelpMenu()
{
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_help = menu_bar->addMenu(QString("&Help")) ;
        if (!m_menu_help)
            throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createHelpMenu() ; could not create help menu")) ;

        if (m_action_help_general)
            m_menu_help->addAction(m_action_help_general) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createHelpMenu() ; could not access menu bar ")) ;
    }
}

void ZGuiDialogueSearchFeatures::createFileMenuActions()
{
    if (!m_action_file_close && (m_action_file_close = ZGuiWidgetCreator::createAction(QString("Close"),this)))
    {
        m_action_file_close->setShortcut(QKeySequence::Close) ;
        m_action_file_close->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_close->setStatusTip(QString("Close the feature search dialogue")) ;
        addAction(m_action_file_close) ;
        connect(m_action_file_close, SIGNAL(triggered(bool)), this, SLOT(slot_action_file_close())) ;
    }
}

void ZGuiDialogueSearchFeatures::createHelpMenuActions()
{
    if (!m_action_help_general && (m_action_help_general = ZGuiWidgetCreator::createAction(QString("General"), this)))
    {
        m_action_help_general->setStatusTip(QString("Help with the search features dialogue")) ;
        addAction(m_action_help_general) ;
        connect(m_action_help_general, SIGNAL(triggered(bool)), this, SLOT(slot_action_help_general())) ;
    }
}


// first layout of controls
QWidget *ZGuiDialogueSearchFeatures::createControls01()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createGroupBox(QString("Search specificiation"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create grid layout for dialogue ")) ;

    QGridLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create layout ")) ;
    widget->setLayout(layout) ;

    // now a series of labels and combo boxes inserted into the grid
    QLabel * label = Q_NULLPTR ;
    int row = 0 ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Align: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create align label ")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    label = Q_NULLPTR ;
    if (!(m_entry_align = ZGuiWidgetCreator::createComboBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create align entry ")) ;
    m_entry_align->setEditable(true) ;
    layout->addWidget(m_entry_align, row, 1, 1, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Block: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create block label ")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    label = Q_NULLPTR ;
    if (!(m_entry_block = ZGuiWidgetCreator::createComboBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create block entry ")) ;
    m_entry_block->setEditable(true) ;
    layout->addWidget(m_entry_block, row, 1, 1, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Track: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create track label ")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    label = Q_NULLPTR ;
    if (!(m_entry_track = ZGuiWidgetCreator::createComboBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create track combo box ")) ;
    m_entry_track->setEditable(true) ;
    layout->addWidget(m_entry_track, row, 1, 1, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Featureset: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create featureset label")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    label = Q_NULLPTR ;
    if (!(m_entry_featureset = ZGuiWidgetCreator::createComboBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create featureset combo box")) ;
    m_entry_featureset->setEditable(true) ;
    m_entry_featureset->addItem(QString("*")) ;
    layout->addWidget(m_entry_featureset, row, 1, 1, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Feature: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create feature label")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    label = Q_NULLPTR ;
    if (!(m_entry_feature = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls01() ; unable to create line edit ")) ;
    m_entry_feature->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed) ;
    m_entry_feature->setText(QString("*")) ;
    layout->addWidget(m_entry_feature, row, 1, 1, 1) ;

    layout->setColumnStretch(0, 0) ;
    layout->setColumnStretch(1, 100) ;

    return widget ;
}

// second layout of controls
QWidget* ZGuiDialogueSearchFeatures::createControls02()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createGroupBox(QString("Filters"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create Filters group box ")) ;

    QGridLayout* layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to  create layout ")) ;
    widget->setLayout(layout) ;

    // now my nice rows of controls
    QLabel * label = Q_NULLPTR ;
    int row = 0 ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Strand: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create strand label ")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    label = Q_NULLPTR ;
    if (!(m_entry_strand = ZGuiWidgetComboStrand::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create strand combo box ")) ;
    layout->addWidget(m_entry_strand, row, 1, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Frame: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create frame label ")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    label = Q_NULLPTR ;
    if (!(m_entry_frame = ZGuiWidgetComboFrame::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create frame combo box")) ;
    layout->addWidget(m_entry_frame, row, 1, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Start: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create start label ")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    label = Q_NULLPTR ;
    if (!(m_entry_start = ZGuiWidgetCreator::createSpinBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create start spin box ")) ;
    m_entry_start->setRange(1, m_max_int_input_value) ;
    layout->addWidget(m_entry_start, row, 1, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("End: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create end label ")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    label = Q_NULLPTR ;
    if (!(m_entry_end = ZGuiWidgetCreator::createSpinBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create end spinbox ")) ;
    m_entry_end->setRange(1, m_max_int_input_value) ;
    layout->addWidget(m_entry_end, row, 1, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Locus: "))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create locus label ")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    label = Q_NULLPTR ;
    if (!(m_entry_locus = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls02() ; unable to create locus check box ")) ;
    layout->addWidget(m_entry_locus, row, 1, 1, 1, Qt::AlignLeft) ;
    ++row ;

    layout->setColumnStretch(0, 0) ;
    layout->setColumnStretch(1, 100) ;

    return widget ;
}

// buttons at the bottom...
QWidget *ZGuiDialogueSearchFeatures::createControls03()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls03() ; unable to create widget ")) ;

    QHBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls03() ; unable to create layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    // create a couple of buttons
    if (!(m_button_cancel = ZGuiWidgetCreator::createPushButton(QString("Cancel"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls03() ; unable to create cancel button ")) ;
    layout->addWidget(m_button_cancel, 0, Qt::AlignLeft) ;
    connect(m_button_cancel, SIGNAL(clicked(bool)), this, SLOT(slot_close())) ;
    layout->addStretch(100) ;
    if (!(m_button_search = ZGuiWidgetCreator::createPushButton(QString("Search"))))
        throw std::runtime_error(std::string("ZGuiDialogueSearchFeatures::createControls03() ; unable to create search button ")) ;
    layout->addWidget(m_button_search, 0, Qt::AlignRight) ;

    return widget ;
}

// set data from transfer object
bool ZGuiDialogueSearchFeatures::setSearchFeaturesData(const ZGSearchFeaturesData &data)
{
    bool result = false ;

    std::set<std::string> str_set ;

    if (m_entry_align)
    {
        m_entry_align->clear() ;
        str_set = data.getAlign() ;
        for (auto it = str_set.begin() ; it != str_set.end() ; ++it )
            m_entry_align->addItem(QString::fromStdString(*it)) ;
    }

    if (m_entry_block)
    {
        m_entry_block->clear() ;
        str_set = data.getBlock() ;
        for (auto it = str_set.begin() ; it != str_set.end() ; ++it)
            m_entry_block->addItem(QString::fromStdString(*it)) ;
    }

    if (m_entry_track)
    {
        m_entry_track->clear() ;
        str_set = data.getTrack() ;
        for (auto it = str_set.begin() ; it != str_set.end() ; ++it)
            m_entry_track->addItem(QString::fromStdString(*it)) ;
    }

    if (m_entry_featureset)
    {
        m_entry_featureset->clear() ;
        str_set = data.getFeatureset() ;
        m_entry_featureset->addItem(QString("*")) ;
        for (auto it = str_set.begin() ; it != str_set.end() ; ++it)
            m_entry_featureset->addItem(QString::fromStdString(*it)) ;
    }

    if (m_entry_feature)
    {
        std::string feature = data.getFeature() ;
        if (feature.length())
            m_entry_feature->setText(QString::fromStdString(feature)) ;
        else
            m_entry_feature->setText(QString("*")) ;
    }

    if (m_entry_strand)
    {
        m_entry_strand->setStrand(data.getStrand()) ;
    }

    if (m_entry_frame)
    {
        m_entry_frame->setFrame(data.getFrame()) ;
    }

    if (m_entry_start && data.getStart())
    {
        m_entry_start->setValue(static_cast<int>(data.getStart()));
    }

    if (m_entry_end && data.getEnd())
    {
        m_entry_end->setValue(static_cast<int>(data.getEnd())) ;
    }

    if (m_entry_locus)
    {
        m_entry_locus->setChecked(data.getLocus()) ;
    }

    result = true ;
    return result ;
}

ZGSearchFeaturesData ZGuiDialogueSearchFeatures::getSearchFeaturesData() const
{
    ZGSearchFeaturesData data ;
    QString qstr ;
    std::set<std::string> str_set ;

    if (m_entry_align)
    {
        str_set.insert(m_entry_align->currentText().toStdString()) ;
        data.setAlign(str_set) ;
        str_set.clear() ;
    }

    if (m_entry_block)
    {
        str_set.insert(m_entry_block->currentText().toStdString()) ;
        data.setBlock(str_set) ;
        str_set.clear() ;
    }

    if (m_entry_track)
    {
        str_set.insert(m_entry_track->currentText().toStdString()) ;
        data.setTrack(str_set) ;
        str_set.clear() ;
    }

    if (m_entry_featureset)
    {
        str_set.insert(m_entry_featureset->currentText().toStdString()) ;
        data.setFeatureset(str_set) ;
        str_set.clear() ;
    }

    if (m_entry_feature)
    {
        data.setFeature(m_entry_feature->text().toStdString()) ;
    }

    if (m_entry_strand)
    {
        data.setStrand(m_entry_strand->getStrand()) ;
    }

    if (m_entry_frame)
    {
        data.setFrame(m_entry_frame->getFrame()) ;
    }

    if (m_entry_start)
    {
        data.setStart(static_cast<uint32_t>(m_entry_start->value())) ;
    }

    if (m_entry_end)
    {
        data.setEnd(static_cast<uint32_t>(m_entry_end->value())) ;
    }

    if (m_entry_locus)
    {
        data.setLocus(m_entry_locus->isChecked()) ;
    }

    return data ;
}




///////////////////////////////////////////////////////////////////////////////
/// Slots
///////////////////////////////////////////////////////////////////////////////

void ZGuiDialogueSearchFeatures::slot_close()
{
    emit signal_received_close_event() ;
}

void ZGuiDialogueSearchFeatures::slot_action_file_close()
{
    emit signal_received_close_event() ;
}

void ZGuiDialogueSearchFeatures::slot_action_help_general()
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
            return ;
        }
    }
    m_text_display_dialogue->setText(m_help_text_general) ;
    m_text_display_dialogue->setWindowTitle(m_help_text_title_general) ;
    m_text_display_dialogue->show() ;
}

void ZGuiDialogueSearchFeatures::slot_help_closed()
{
    if (m_text_display_dialogue)
    {
        m_text_display_dialogue->close() ;
        m_text_display_dialogue = Q_NULLPTR ;
    }
}

} // namespace GUI

} // namespace ZMap
