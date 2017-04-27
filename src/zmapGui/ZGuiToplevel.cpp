/*  File: ZGuiToplevel.cpp
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

#include "ZGuiToplevel.h"
#include "ZGuiToplevelStyle.h"
#include "ZGuiTextDisplayDialogue.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiCoordinateValidator.h"
#include "ZGuiWidgetSelectSequence.h"
#include "ZGuiScene.h"
#include "ZGuiMain.h"
#include "ZGMessageHeader.h"
#include "ZGFeatureset.h"
#include "ZGStyle.h"

#include <QtWidgets>
#include <QMenuBar>
#include <QStatusBar>
#include <QMenu>
#include <QList>
#include <QStandardItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <stdexcept>
#include <sstream>

Q_DECLARE_METATYPE(ZMap::GUI::ZGZMapData)

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiToplevel>::m_name("ZGuiToplevel") ;
const QString ZGuiToplevel::m_display_name("ZMap Annotation Tool"),
ZGuiToplevel::m_help_text_title_general("ZMap Annotation Tool: Help "),
ZGuiToplevel::m_help_text_title_release("ZMap Annotation Tool: Release Notes"),
ZGuiToplevel::m_help_text_title_about("ZMap Annotation Tool: About"),
ZGuiToplevel::m_help_text_general("General text help; opens web browser... could do other stuff"),
ZGuiToplevel::m_help_text_release("Release notes; again, opens web browser..."),
ZGuiToplevel::m_help_text_about("ZMap is a multi-threaded genome viewer program\n"
                                "that can be used stand alone or be driven from\n"
                                "an external program to provide a seamless annotation\n"
                                "package. It is currently used as part of the otter\n"
                                "system and is being added to the core Wormbase\n"
                                "annotation software.\n\n"
                                "http:://www.sanger.ac.uk/resources/software/zmap") ;
const int ZGuiToplevel::m_model_column_number(3) ;
const QStringList ZGuiToplevel::m_model_column_titles =
{
    QString("ID"),
    QString("State"),
    QString("Sequence")
} ;

const int ZGuiToplevel::m_zmapdata_id(qMetaTypeId<ZGZMapData>()) ;

ZGuiToplevel::ZGuiToplevel(ZInternalID id, QWidget *parent)
    : QMainWindow(parent),
      Util::InstanceCounter<ZGuiToplevel>(),
      Util::ClassName<ZGuiToplevel>(),
      m_id(id),
      m_style(Q_NULLPTR),
      m_top_layout(Q_NULLPTR),
      m_select_sequence(Q_NULLPTR),
      m_menu_file(Q_NULLPTR),
      m_menu_help(Q_NULLPTR),
      m_action_file_close(Q_NULLPTR),
      m_action_help_general(Q_NULLPTR),
      m_action_help_release(Q_NULLPTR),
      m_action_help_about(Q_NULLPTR),
      m_zmaps_view(Q_NULLPTR),
      m_zmaps_model(Q_NULLPTR),
      m_button_close_selected(Q_NULLPTR),
      m_button_quit_all(Q_NULLPTR),
      m_user_home()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; cannot create instance with QApplication existing ")) ;

    // menu bar
    QMenuBar* menu_bar = Q_NULLPTR ;
    try
    {
        menu_bar = menuBar() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; caught exception on attempt to create menu bar: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; caught unknown exception on attempt to create menu bar ")) ;
    }
    if (!menu_bar)
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; unable to create menu bar")) ;

    // menu bar style
    if (!(m_style = ZGuiToplevelStyle::newInstance()))
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; unable to create style ")) ;
    m_style->setParent(this) ;
    menu_bar->setStyle(m_style) ;

    // status bar
    QStatusBar *status_bar = Q_NULLPTR ;
    try
    {
        status_bar = statusBar() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; caught exception on attempt to create status bar: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; caught unknown exception on attempt to create status bar ")) ;
    }
    if (!status_bar)
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; unable to create status bar ")) ;

    // create actions before menus
    try
    {
        createFileMenuActions() ;
        createHelpMenuActions() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiTopLevel::ZGuiTopLevel() ; caught exception on attempt to create actions: ")+error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiTopLevel::ZGuiTopLevel() ; caught unknown exception on attempt to create actions ")) ;
    }

    // create menus
    try
    {
        createAllMenus() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; caught exception on attempt to call createAllMenus():  ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; caught unknown exception in attempt to call createAllMenus() ")) ;
    }

    // top level layout
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; unable to create top_layout ")) ;

    // central widget
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; unable to create central widget")) ;
    try
    {
        setCentralWidget(widget) ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; caught exception on call to setCentralWidget(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; caught uknown exception on attempt to setCentralWidget() ")) ;
    }
    widget->setLayout(m_top_layout) ;

    // select sequence widget
    if (!(m_select_sequence = ZGuiWidgetSelectSequence::newInstance()))
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; unable to create select sequence object  ")) ;
    m_top_layout->addWidget(m_select_sequence) ;

    // separator
    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiTopLevel::ZGuiToplevel() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // content; manage box at the bottom
    if (!(widget = createControls01()))
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; unable to create object from call to createControls01() ")) ;
    m_top_layout->addWidget(widget) ;

    // separator
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // quit button right at the bottom
    if (!(m_button_quit_all = ZGuiWidgetCreator::createPushButton(QString("Quit ZMap"))))
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; unable to create m_button_quit_all ")) ;
    if (!connect(m_button_quit_all, SIGNAL(clicked(bool)), this, SLOT(slot_quit())))
        throw std::runtime_error(std::string("ZGuiToplevel::ZGuiToplevel() ; unable to connect m_button_quit to slot_quit() ")) ;
    m_top_layout->addWidget(m_button_quit_all, 100) ;

    // overall window properties
    setWindowTitle(m_display_name) ;
}

ZGuiToplevel::~ZGuiToplevel()
{
}

ZGuiToplevel * ZGuiToplevel::newInstance(ZInternalID id, QWidget *parent)
{
    ZGuiToplevel * gui = Q_NULLPTR ;

    try
    {
        gui = new ZGuiToplevel(id, parent) ;
    }
    catch (...)
    {
        gui = Q_NULLPTR ;
    }

    return gui ;
}

// we only want the parent to be doing this ...
void ZGuiToplevel::closeEvent(QCloseEvent*event)
{
    emit signal_received_close_event() ;
    QMainWindow::closeEvent(event) ;
}

void ZGuiToplevel::hideEvent(QHideEvent *event)
{
    m_pos = pos() ;
    m_geometry = saveGeometry() ;
    QMainWindow::hideEvent(event) ;
}

void ZGuiToplevel::showEvent(QShowEvent* event)
{
    move(m_pos) ;
    restoreGeometry(m_geometry) ;
    QMainWindow::showEvent(event) ;
}


void ZGuiToplevel::createAllMenus()
{
    createFileMenu() ;
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
         menu_bar->addSeparator() ;
    createHelpMenu() ;
}

void ZGuiToplevel::createFileMenu()
{
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_file = menu_bar->addMenu(QString("File")) ;
        if (!m_menu_file)
            throw std::runtime_error(std::string("ZGuiToplevel::createFileMenu() ; failed to create menu ")) ;

        // add actions to menu...
        if (m_action_file_close)
            m_menu_file->addAction(m_action_file_close) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiToplevel::createFileMenu() ; unable to access menu_bar")) ;
    }
}

void ZGuiToplevel::createHelpMenu()
{
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_help = menu_bar->addMenu(QString("Help")) ;
        if (!m_menu_help)
            throw std::runtime_error(std::string("ZGuiToplevel::createHelpMenu() ; failed to create menu ")) ;


        // add actions to menu
        if (m_action_help_general)
            m_menu_help->addAction(m_action_help_general) ;
        if (m_action_help_release)
            m_menu_help->addAction(m_action_help_release) ;
        if (m_action_help_about)
            m_menu_help->addAction(m_action_help_about) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiToplevel::createHelpMenu() ; unable to access menu_bar")) ;
    }
}

void ZGuiToplevel::createFileMenuActions()
{
    if (!m_action_file_close && (m_action_file_close = ZGuiWidgetCreator::createAction(QString("Close"), this)))
    {
        m_action_file_close->setShortcut(QKeySequence::Close) ;
        m_action_file_close->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_close->setStatusTip(QString("Close ZMap and disconnect from all sources")) ;
        addAction(m_action_file_close) ;
        // because of the relationship of this item with its parent (non-qt based) we have
        connect(m_action_file_close, SIGNAL(triggered(bool)), this, SIGNAL(signal_received_close_event())) ;
        //connect(m_action_file_close, SIGNAL(triggered(bool)), this, SLOT(close())) ;
    }
}

void ZGuiToplevel::createHelpMenuActions()
{
    if (!m_action_help_general && (m_action_help_general = ZGuiWidgetCreator::createAction(QString("General"), this)))
    {
        m_action_help_general->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_H)) ;
        m_action_help_general->setShortcutContext(Qt::WindowShortcut) ;
        m_action_help_general->setStatusTip(QString("General help with ZMap ")) ;
        addAction(m_action_help_general) ;
        connect(m_action_help_general, SIGNAL(triggered(bool)), this, SLOT(slot_action_help_general())) ;
    }

    if (!m_action_help_release && (m_action_help_release = ZGuiWidgetCreator::createAction(QString("Release notes"), this)))
    {
        m_action_help_release->setStatusTip(QString("Display the release notes for ZMap")) ;
        addAction(m_action_help_release) ;
        connect(m_action_help_release, SIGNAL(triggered(bool)), this, SLOT(slot_action_help_release())) ;
    }

    if (!m_action_help_about && (m_action_help_about = ZGuiWidgetCreator::createAction(QString("About"), this)))
    {
        m_action_help_about->setStatusTip(QString("About the ZMap genomic browser ")) ;
        addAction(m_action_help_about) ;
        connect(m_action_help_about, SIGNAL(triggered(bool)), this, SLOT(slot_action_help_about())) ;
    }
}


// zmaps management box in the lower part
QWidget *ZGuiToplevel::createControls01()
{
    QVBoxLayout *vlayout = Q_NULLPTR ;
    if (!(vlayout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiToplevel::createControls01() ; unable to create vlayout")) ;

    QGroupBox *box = Q_NULLPTR ;
    if (!(box = ZGuiWidgetCreator::createGroupBox(QString("Manage ZMaps"))))
        throw std::runtime_error(std::string("ZGuiToplevel::createControls01() ; unable to create group box ")) ;
    box->setLayout(vlayout)  ;

    if (!(m_zmaps_model = ZGuiWidgetCreator::createStandardItemModel()))
        throw std::runtime_error(std::string("ZGuiToplevel::createControls01() ; unable to create zmaps standard item model ")) ;
    m_zmaps_model->setParent(this) ;
    m_zmaps_model->setColumnCount(m_model_column_number) ;
    m_zmaps_model->setHorizontalHeaderLabels(m_model_column_titles) ;

    if (!(m_zmaps_view = ZGuiWidgetCreator::createTableView()) )
        throw std::runtime_error(std::string("ZGuiToplevel::createControls01() ; unable to create zmaps table view ")) ;
    m_zmaps_view->setModel(m_zmaps_model) ;
    m_zmaps_view->resizeColumnsToContents() ;
    m_zmaps_view->resizeRowsToContents();
    m_zmaps_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;
    m_zmaps_view->setSizeAdjustPolicy(QTableView::SizeAdjustPolicy::AdjustToContents);
    vlayout->addWidget(m_zmaps_view) ;

    QHBoxLayout *hlayout = Q_NULLPTR ;
    if (!(m_button_close_selected = ZGuiWidgetCreator::createPushButton(QString("Close selected"))))
        throw std::runtime_error(std::string("ZGuiToplevel::createControls01() ; unable to create close selected button ")) ;
    if (!(hlayout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiToplevel::createControls01() ; unable to create hlayout ")) ;
    m_button_close_selected->setToolTip(QString("Close the selected ZMap View ")) ;
    hlayout->addWidget(m_button_close_selected, 0, Qt::AlignLeft) ;
    hlayout->addStretch(100) ;
    vlayout->addLayout(hlayout) ;

    return box ;
}

//

std::shared_ptr<ZGMessage> ZGuiToplevel::query(const std::shared_ptr<ZGMessage>& message)
{
    std::shared_ptr<ZGMessage> answer ;
    if (!message)
        return answer ;
    ZGMessageType message_type = message->type() ;
    ZInternalID incoming_message_id = message->id() ;

    switch (message_type)
    {

    case ZGMessageType::QueryGuiIDs:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQueryGuiIDs> specific_message =
                std::dynamic_pointer_cast<ZGMessageQueryGuiIDs, ZGMessage>(message) ;
        if (specific_message)
        {
            dataset = getGuiMainIDs() ;
            answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        }
        break ;
    }

    case ZGMessageType::QuerySequenceIDsAll:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQuerySequenceIDsAll> specific_message =
                std::dynamic_pointer_cast<ZGMessageQuerySequenceIDsAll, ZGMessage>(message) ;
        if (specific_message)
        {
            dataset = getSequenceIDs() ;
        }
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QuerySequenceIDs:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQuerySequenceIDs> specific_message =
                std::dynamic_pointer_cast<ZGMessageQuerySequenceIDs, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID gui_id = specific_message->getGuiID() ;
            dataset = getSequenceIDs(gui_id) ;
        }
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QueryFeaturesetIDs:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQueryFeaturesetIDs> specific_message =
                std::dynamic_pointer_cast<ZGMessageQueryFeaturesetIDs, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID() ;
            dataset = getFeaturesetIDs(sequence_id) ;
        }
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QueryTrackIDs:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQueryTrackIDs> specific_message =
                std::dynamic_pointer_cast<ZGMessageQueryTrackIDs, ZGMessage> (message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID() ;
            dataset = getTrackIDs(sequence_id) ;
        }
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QueryFeatureIDs:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQueryFeatureIDs> specific_message =
                std::dynamic_pointer_cast<ZGMessageQueryFeatureIDs, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    featureset_id = specific_message->getFeaturesetID() ;
            dataset = getFeatureIDs(sequence_id, featureset_id) ;
        }
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QueryFeaturesetsFromTrack:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQueryFeaturesetsFromTrack> specific_message =
                std::dynamic_pointer_cast<ZGMessageQueryFeaturesetsFromTrack, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    track_id = specific_message->getTrackID() ;
            dataset = getFeaturesetFromTrack(sequence_id, track_id) ;
        }
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QueryTrackFromFeatureset:
    {
        ZInternalID result = 0 ;
        std::shared_ptr<ZGMessageQueryTrackFromFeatureset> specific_message =
                std::dynamic_pointer_cast<ZGMessageQueryTrackFromFeatureset, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    featureset_id = specific_message->getFeaturesetID() ;
            result = getTrackFromFeatureset(sequence_id, featureset_id) ;
        }
        answer = std::make_shared<ZGMessageReplyDataID>(incoming_message_id, incoming_message_id, result) ;
        break ;
    }

    case ZGMessageType::QueryUserHome:
    {
        std::shared_ptr<ZGMessageQueryUserHome> specific_message =
                std::dynamic_pointer_cast<ZGMessageQueryUserHome, ZGMessage>(message) ;
        if (specific_message)
        {
            answer = std::make_shared<ZGMessageReplyString>(incoming_message_id, incoming_message_id, m_user_home) ;
        }
        break ;
    }

    default:
        break ;
    }

    return answer ;

}

///////////////////////////////////////////////////////////////////////////////
/// \brief ZGuiToplevel::receive
/// \param message
///////////////////////////////////////////////////////////////////////////////

void ZGuiToplevel::receive(const std::shared_ptr<ZGMessage>& message)
{
    if (!message)
        return ;

    ZInternalID message_id = message->id() ;
    ZGMessageType message_type = message->type() ;

    switch (message_type)
    {

    case ZGMessageType::GuiOperation:
    {
        std::shared_ptr<ZGMessageGuiOperation> specific_message =
                std::dynamic_pointer_cast<ZGMessageGuiOperation, ZGMessage>(message) ;
        processGuiOperation(specific_message) ;
        break ;
    }

    case ZGMessageType::SequenceOperation:
    {
        std::shared_ptr<ZGMessageSequenceOperation> specific_message =
                std::dynamic_pointer_cast<ZGMessageSequenceOperation, ZGMessage>(message) ;
        processSequenceOperation(specific_message) ;
        break ;
    }

    case ZGMessageType::SequenceToGui:
    {
        std::shared_ptr<ZGMessageSequenceToGui> specific_message =
                std::dynamic_pointer_cast<ZGMessageSequenceToGui, ZGMessage>(message) ;
        processSequenceToGui(specific_message) ;
        break ;
    }

    case ZGMessageType::TrackOperation:
    {
        std::shared_ptr<ZGMessageTrackOperation> specific_message =
                std::dynamic_pointer_cast<ZGMessageTrackOperation, ZGMessage>(message) ;
        processTrackOperation(specific_message) ;
        break ;
    }

    case ZGMessageType::SeparatorOperation:
    {
        std::shared_ptr<ZGMessageSeparatorOperation> specific_message =
                std::dynamic_pointer_cast<ZGMessageSeparatorOperation, ZGMessage>(message) ;
        processSeparatorOperation(specific_message) ;
        break ;
    }

    case ZGMessageType::FeaturesetOperation:
    {
        std::shared_ptr<ZGMessageFeaturesetOperation> specific_message =
                std::dynamic_pointer_cast<ZGMessageFeaturesetOperation, ZGMessage>(message) ;
        processFeaturesetOperation(specific_message) ;
        break ;
    }

    case ZGMessageType::FeaturesetToTrack:
    {
        std::shared_ptr<ZGMessageFeaturesetToTrack> specific_message =
                std::dynamic_pointer_cast<ZGMessageFeaturesetToTrack, ZGMessage>(message) ;
        processFeaturesetToTrack(specific_message) ;
        break ;
    }

    default:
    {
        processOperation(message) ;
        break ;
    }

    }
}

// all other operations not handled elsewhere
void ZGuiToplevel::processOperation(const std::shared_ptr<ZGMessage>& message)
{
    if (!message)
        return ;
    ZInternalID message_id = message->id() ;
    ZGMessageType message_type = message->type() ;

    switch (message_type)
    {

    case ZGMessageType::SetSequenceBGColor:
    {
        std::shared_ptr<ZGMessageSetSequenceBGColor> specific_message =
                std::dynamic_pointer_cast<ZGMessageSetSequenceBGColor, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID() ;
            ZGColor color = specific_message->getColor() ;
            ZGuiScene* sequence = getSequence(sequence_id) ;
            if (sequence)
            {
                sequence->setBackgroundColor(color.asQColor()) ;
            }
        }
        break ;
    }

    case ZGMessageType::SetSeparatorColor:
    {
        std::shared_ptr<ZGMessageSetSeparatorColor> specific_message =
                std::dynamic_pointer_cast<ZGMessageSetSeparatorColor, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID() ;
            ZGColor color = specific_message->getColor() ;
            ZGuiScene* sequence = getSequence(sequence_id) ;
            if (sequence)
            {
                sequence->setSeparatorColor(color.asQColor()) ;
            }
        }
        break ;
    }

    case ZGMessageType::SetTrackBGColor:
    {
        std::shared_ptr<ZGMessageSetTrackBGColor> specific_message =
                std::dynamic_pointer_cast<ZGMessageSetTrackBGColor, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    track_id = specific_message->getTrackID() ;
            ZGColor color = specific_message->getColor() ;
            ZGuiScene* sequence = getSequence(sequence_id) ;
            if (sequence)
            {
                sequence->setTrackColor(track_id, color.asQColor()) ;
            }
        }
        break ;
    }

    case ZGMessageType::AddFeatures:
    {
        std::shared_ptr<ZGMessageAddFeatures> specific_message =
                std::dynamic_pointer_cast<ZGMessageAddFeatures, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID() ;
            ZGuiScene * sequence = getSequence(sequence_id) ;
            std::shared_ptr<ZGFeatureset> featureset = specific_message->getFeatureset() ;
            if (sequence && sequence->containsFeatureset(featureset->getID()))
            {
                sequence->addFeatures(featureset) ;
            }
        }
        break ;
    }

    case ZGMessageType::DeleteFeatures:
    {
        std::shared_ptr<ZGMessageDeleteFeatures> specific_message =
                std::dynamic_pointer_cast<ZGMessageDeleteFeatures, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    featureset_id = specific_message->getFeaturesetID() ;
            std::set<ZInternalID> features = specific_message->getFeatures() ;
            ZGuiScene* sequence = getSequence(sequence_id) ;
            if (sequence && sequence->containsFeatureset(featureset_id))
            {
                sequence->deleteFeatures(featureset_id, features) ;
            }
        }
        break ;
    }

    case ZGMessageType::DeleteAllFeatures:
    {
        std::shared_ptr<ZGMessageDeleteAllFeatures> specific_message =
                std::dynamic_pointer_cast<ZGMessageDeleteAllFeatures, ZGMessage> (message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    featureset_id = specific_message->getFeaturesetID() ;
            ZGuiScene* sequence = getSequence(sequence_id) ;
            if (sequence && sequence->containsFeatureset(featureset_id))
            {
                sequence->deleteAllFeatures(featureset_id) ;
            }
        }
        break ;
    }

    case ZGMessageType::SetFeaturesetStyle:
    {
        std::shared_ptr<ZGMessageSetFeaturesetStyle> specific_message =
                std::dynamic_pointer_cast<ZGMessageSetFeaturesetStyle, ZGMessage> (message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    featureset_id = specific_message->getFeaturesetID() ;
            ZGuiScene* sequence = getSequence(sequence_id) ;
            std::shared_ptr<ZGStyle> style_data = specific_message->getStyle() ;
            if (sequence && sequence->containsFeatureset(featureset_id) && style_data)
            {
                sequence->setFeaturesetStyle(featureset_id, style_data) ;
            }
        }
        break ;
    }

    case ZGMessageType::SetViewOrientation:
    {
        std::shared_ptr<ZGMessageSetViewOrientation> specific_message =
                std::dynamic_pointer_cast<ZGMessageSetViewOrientation,ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    gui_id = specific_message->getGuiID() ;
            ZGuiScene* sequence = getSequence(sequence_id) ;
            ZGuiMain *gui = getGuiMain(gui_id) ;
            if (sequence && gui && gui->containsSequence(sequence_id))
            {
                gui->setViewOrientation(sequence_id, specific_message->getOrientation()) ;
            }
        }
        break ;
    }

    case ZGMessageType::ScaleBarOperation:
    {
        std::shared_ptr<ZGMessageScaleBarOperation> specific_message =
                std::dynamic_pointer_cast<ZGMessageScaleBarOperation,ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    gui_id = specific_message->getGuiID() ;
            ZGuiScene* sequence = getSequence(sequence_id) ;
            ZGuiMain* gui = getGuiMain(gui_id) ;
            if (sequence && gui && gui->containsSequence(sequence_id))
            {
                ZGMessageScaleBarOperation::Type type = specific_message->getOperationType() ;
                if (type == ZGMessageScaleBarOperation::Type::Show)
                    gui->scaleBarShow(sequence_id) ;
                else if (type == ZGMessageScaleBarOperation::Type::Hide)
                    gui->scaleBarHide(sequence_id) ;
            }
        }
        break ;
    }

    case ZGMessageType::ScaleBarPosition:
    {
        std::shared_ptr<ZGMessageScaleBarPosition> specific_message =
                std::dynamic_pointer_cast<ZGMessageScaleBarPosition,ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    gui_id = specific_message->getGuiID() ;
            ZGuiScene* sequence = getSequence(sequence_id) ;
            ZGuiMain* gui = getGuiMain(gui_id) ;
            if (sequence && gui && gui->containsSequence(sequence_id))
            {
                ZGScaleRelativeType position = specific_message->getScalePosition() ;
                gui->scaleBarPosition(sequence_id, position) ;
            }
        }
        break ;
    }

    case ZGMessageType::SetSequenceOrder:
    {
        std::shared_ptr<ZGMessageSetSequenceOrder> specific_message =
                std::dynamic_pointer_cast<ZGMessageSetSequenceOrder, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID gui_id = specific_message->getGuiID() ;
            std::vector<ZInternalID> data = specific_message->getSequenceIDs() ;
            ZGuiMain* gui = getGuiMain(gui_id) ;
            if (gui)
            {
                bool result = gui->setSequenceOrder(data) ;
            }
        }
        break ;
    }

    case ZGMessageType::SetStatusLabel:
    {
        std::shared_ptr<ZGMessageSetStatusLabel> specific_message =
                std::dynamic_pointer_cast<ZGMessageSetStatusLabel, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID() ;
            std::string label = specific_message->getLabel(),
                    tooltip = specific_message->getTooltip() ;
            setStatusLabel(sequence_id, label, tooltip) ;
        }
        break ;
    }

    case ZGMessageType::SetGuiMenuBarColor:
    {
        std::shared_ptr<ZGMessageSetGuiMenuBarColor> specific_message =
                std::dynamic_pointer_cast<ZGMessageSetGuiMenuBarColor, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID gui_id = specific_message->getGuiID() ;
            ZGColor color = specific_message->getColor() ;
            ZGuiMain* gui = getGuiMain(gui_id) ;
            if (gui)
            {
                gui->setMenuBarColor(color.asQColor()) ;
            }
        }
        break ;
    }

    case ZGMessageType::SetUserHome:
    {
        std::shared_ptr<ZGMessageSetUserHome> specific_message =
                std::dynamic_pointer_cast<ZGMessageSetUserHome, ZGMessage>(message) ;
        m_user_home = specific_message->getUserHome() ;
        break ;
    }

    default:
        break ;
    }
}

//
// Operations on the strand separator
//
void ZGuiToplevel::processSeparatorOperation(const std::shared_ptr<ZGMessageSeparatorOperation> & message)
{
    if (!message)
        return ;

    ZInternalID sequence_id = message->getSequenceID() ;
    ZGMessageSeparatorOperation::Type type = message->getOperationType() ;
    ZGuiScene* sequence = getSequence(sequence_id) ;
    if (!sequence || sequence->getID() != sequence_id)
        return ;

    switch (type)
    {

    case ZGMessageSeparatorOperation::Type::Create:
    {
        if (!sequence->containsStrandSeparator())
        {
            sequence->addStrandSeparator() ;
        }
        break ;
    }

    case ZGMessageSeparatorOperation::Type::Delete:
    {
        if (sequence->containsStrandSeparator())
        {
            sequence->deleteStrandSeparator() ;
        }
        break ;
    }

    case ZGMessageSeparatorOperation::Type::Show:
    {
        if (sequence->containsStrandSeparator())
        {
            sequence->showStrandSeparator() ;
        }
        break ;
    }

    case ZGMessageSeparatorOperation::Type::Hide:
    {
        if (sequence->containsStrandSeparator())
        {
            sequence->hideStrandSeparator() ;
        }
        break ;
    }

    default:
        break ;
    }
}

//
// Operations on the ZGuiMain instances
//
void ZGuiToplevel::processGuiOperation(const std::shared_ptr<ZGMessageGuiOperation> &message)
{
    if (!message)
        return ;

    ZInternalID message_id = message->id(),
            gui_id = message->getGuiID() ;
    ZGMessageGuiOperation::Type operation_type = message->getOperationType() ;

    switch (operation_type)
    {

    case ZGMessageGuiOperation::Type::Create:
    {
        ZGuiMain *gui = getGuiMain(gui_id) ;
        if (!gui)
        {
            try
            {
                gui = ZGuiMain::newInstance(gui_id) ;
            }
            catch (std::exception & error)
            {
                std::stringstream str ;
                str << std::string("ZGuiToplevel::processGuiOperation() ; caught exception on attempt to create instance of ZGuiMain with id: ")
                    << gui_id << ", " << error.what() ;
                emit signal_log_message(str.str()) ;
            }
            catch (...)
            {
                std::stringstream str ;
                str << std::string("ZGuiToplevel::processGuiOperation() ; caught unknown exception on attempt to create instance of ZGuiMain with id: ")
                    << gui_id ;
                emit signal_log_message(str.str()) ;
            }
            if (gui)
            {
                gui->setParent(this, Qt::Window) ;
                gui->setAttribute(Qt::WA_DeleteOnClose) ;
                connect(gui, SIGNAL(signal_log_message(std::string)), this, SLOT(slot_log_message(std::string))) ;
                connect(gui, SIGNAL(signal_received_quit_event()), this, SLOT(slot_quit())) ;
            }
        }
        break ;
    }

    case ZGMessageGuiOperation::Type::Delete:
    {
        ZGuiMain *gui = getGuiMain(gui_id) ;
        if (gui)
        {
            gui->setParent(Q_NULLPTR) ;
            delete gui ;
        }
        break ;
    }

    case ZGMessageGuiOperation::Type::Hide:
    {
        ZGuiMain *gui = getGuiMain(gui_id) ;
        if (gui)
        {
            gui->hide() ;
        }
        break ;
    }

    case ZGMessageGuiOperation::Type::Show:
    {
        ZGuiMain *gui = getGuiMain(gui_id) ;
        if (gui)
        {
            gui->show() ;
        }
        break ;
    }

    case ZGMessageGuiOperation::Type::Switch:
    {
        ZGuiMain *gui = getGuiMain(gui_id) ;
        if (gui)
        {
            gui->setOrientation(!gui->orientation()) ;
        }
        break ;
    }

    default:
        break ;
    }

#ifndef QT_NO_DEBUG
    QList<ZGuiMain*> list = findChildren<ZGuiMain*>(QString(), Qt::FindDirectChildrenOnly) ;
    qDebug() << "ZGuiToplevel::processGuiOperation(), list.size(): " << list.size() << ZGuiMain::instances() ;
#endif

}

//
// Operations to create/delete sequence instances (ZGuiScene) as children
// of the current instance.
//
void ZGuiToplevel::processSequenceOperation(const std::shared_ptr<ZGMessageSequenceOperation> &message)
{
    if (!message)
        return ;

    ZInternalID message_id = message->id(),
            sequence_id = message->getSequenceID() ;
    ZGMessageSequenceOperation::Type operation_type = message->getOperationType() ;

    switch (operation_type)
    {

    case ZGMessageSequenceOperation::Type::Create:
    {
        ZGuiScene* scene = getSequence(sequence_id) ;
        if (!scene)
        {
            try
            {
                scene = ZGuiScene::newInstance(sequence_id, true) ;
            }
            catch (std::exception& error)
            {
                std::stringstream str ;
                str << std::string("ZGuiToplevel::processSequenceOperation() ; caught exception on attempt to create instance of ZGuiScene with id: ")
                    << sequence_id << ", " << error.what() ;
                emit signal_log_message(str.str()) ;
            }
            catch (...)
            {
                std::stringstream str ;
                str << std::string("ZGuiToplevel::processSequenceOperation() ; caught unknown exception on attempt to create instance of ZGuiScene with id: ")
                    << sequence_id ;
                emit signal_log_message(str.str()) ;
            }
            if (scene)
            {
                scene->setParent(this) ;
                scene->setItemIndexMethod(QGraphicsScene::NoIndex) ;
            }
        }
        break ;
    }
    case ZGMessageSequenceOperation::Type::Delete:
    {
        ZGuiScene *scene = getSequence(sequence_id) ;
        if (scene)
        {
            // delete from gui instances
            QList<ZGuiMain*> list = findChildren<ZGuiMain*>(QString(),
                                                            Qt::FindDirectChildrenOnly) ;
            for (auto it = list.begin(); it != list.end() ; ++it)
            {
                ZGuiMain *gui = *it ;
                if (gui && gui->containsSequence(sequence_id))
                    gui->deleteSequence(sequence_id) ;
            }
            scene->setParent(Q_NULLPTR) ;
            delete scene ;
        }
        break ;
    }
    default:
        break ;

    }

#ifndef QT_NO_DEBUG
    QList<ZGuiScene*> list = findChildren<ZGuiScene*>(QString(), Qt::FindDirectChildrenOnly) ;
    qDebug() << "ZGuiToplevel::processSequenceOperation(), list.size(): " << list.size() << ZGuiScene::instances() ;
#endif
}

// sequence to gui mapping
void ZGuiToplevel::processSequenceToGui(const std::shared_ptr<ZGMessageSequenceToGui>& message)
{
    if (!message)
        return ;

    ZInternalID message_id = message->id(),
            sequence_id = message->getSequenceID(),
            gui_id = message->getGuiID() ;
    ZGMessageSequenceToGui::Type operation_type = message->getOperationType() ;

    switch (operation_type)
    {

    case ZGMessageSequenceToGui::Type::Add:
    {
        ZGuiScene* sequence = getSequence(sequence_id) ;
        if (sequence && sequence->getID() == sequence_id)
        {
            ZGuiMain* gui = getGuiMain(gui_id) ;
            if (gui)
            {
                // scene for this sequence to be passed in here...
                gui->addSequence(sequence_id, sequence) ;
            }
        }
        break ;
    }

    case ZGMessageSequenceToGui::Type::Remove:
    {
        ZGuiMain *gui = getGuiMain(gui_id) ;
        if (gui)
        {
            gui->deleteSequence(sequence_id) ;
        }
        break ;
    }

    default:
        break ;
    }
}

// featureset operation; create and delete
void ZGuiToplevel::processFeaturesetOperation(const std::shared_ptr<ZGMessageFeaturesetOperation>& message)
{
    if (!message)
        return ;

    ZInternalID message_id = message->id(),
            sequence_id = message->getSequenceID(),
            featureset_id = message->getFeaturesetID() ;

    // firstly, check that we have the sequence stored
    ZGuiScene* sequence = getSequence(sequence_id) ;
    if (!sequence || sequence->getID() != sequence_id)
        return ;

    ZGMessageFeaturesetOperation::Type type = message->getOperationType() ;

    switch (type)
    {

    case ZGMessageFeaturesetOperation::Type::Create:
    {
        sequence->addFeatureset(featureset_id) ;
        break ;
    }

    case ZGMessageFeaturesetOperation::Type::Delete:
    {
        sequence->deleteFeatureset(featureset_id) ;
    }

    default:
        break ;
    }
}

// featureset to track mapping operations
void ZGuiToplevel::processFeaturesetToTrack(const std::shared_ptr<ZGMessageFeaturesetToTrack> & message)
{
    if (!message)
        return ;

    ZGMessageFeaturesetToTrack::Type type = message->getOperationType() ;
    ZInternalID sequence_id = message->getSequenceID(),
            featureset_id = message->getFeaturesetID(),
            track_id = message->getTrackID() ;

    ZGuiScene* sequence = getSequence(sequence_id) ;
    if (!sequence || sequence->getID() != sequence_id)
        return ;

    switch (type)
    {

    case ZGMessageFeaturesetToTrack::Type::Add:
    {
        sequence->addFeaturesetToTrack(track_id, featureset_id) ;
        break ;
    }

    case ZGMessageFeaturesetToTrack::Type::Remove:
    {
        sequence->removeFeaturesetFromTrack(track_id, featureset_id) ;
        break ;
    }

    default:
        break ;
    }
}

// track operation; create, delete, show and hide
void ZGuiToplevel::processTrackOperation(const std::shared_ptr<ZGMessageTrackOperation>& message)
{
    if (!message)
        return ;

    ZInternalID message_id = message->id(),
            sequence_id = message->getSequenceID(),
            track_id = message->getTrackID() ;
    bool sensitive = message->getStrandSensitive() ;

    // firstly, check that we have the sequence stored
    ZGuiScene* sequence = getSequence(sequence_id) ;
    if (!sequence || sequence->getID() != sequence_id)
        return ;

    ZGMessageTrackOperation::Type type = message->getOperationType() ;

    switch (type)
    {

    case ZGMessageTrackOperation::Type::Create:
    {
        sequence->addTrack(track_id, sensitive) ;
        break ;
    }

    case ZGMessageTrackOperation::Type::Delete:
    {
        sequence->deleteTrack(track_id) ;
        break ;
    }

    case ZGMessageTrackOperation::Type::Show:
    {
        sequence->showTrack(track_id) ;
        break ;
    }

    case ZGMessageTrackOperation::Type::Hide:
    {
        sequence->hideTrack(track_id) ;
        break ;
    }

    default:
        break ;
    }
}


std::set<ZInternalID> ZGuiToplevel::getFeatureIDs(ZInternalID sequence_id, ZInternalID featureset_id) const
{
    std::set<ZInternalID> dataset ;
    ZGuiScene* sequence = getSequence(sequence_id) ;
    if (sequence && sequence->containsFeatureset(featureset_id))
    {
        dataset = sequence->getFeatures(featureset_id) ;
    }
    return dataset ;
}

////////////////////////////////////////////////////////////////////////////////
/// Featureset management (note processFeaturesetOperation() above)
////////////////////////////////////////////////////////////////////////////////

std::set<ZInternalID> ZGuiToplevel::getFeaturesetIDs(ZInternalID sequence_id) const
{
    std::set<ZInternalID> dataset ;
    ZGuiScene* sequence = getSequence(sequence_id) ;
    if (sequence && sequence->getID() == sequence_id)
    {
        dataset = sequence->getFeaturesets() ;
    }

    return dataset ;
}

////////////////////////////////////////////////////////////////////////////////
/// Track management
////////////////////////////////////////////////////////////////////////////////

std::set<ZInternalID> ZGuiToplevel::getTrackIDs(ZInternalID sequence_id) const
{
    std::set<ZInternalID> dataset ;
    ZGuiScene* sequence = getSequence(sequence_id)  ;
    if (sequence && sequence->getID() == sequence_id)
    {
        dataset = sequence->getTracks() ;
    }
    return dataset ;
}

////////////////////////////////////////////////////////////////////////////////
/// Scene/sequence managment
////////////////////////////////////////////////////////////////////////////////

std::set<ZInternalID> ZGuiToplevel::getSequenceIDs() const
{
    std::set<ZInternalID> data ;
    QList<ZGuiScene*> list = findChildren<ZGuiScene*>(QString(),
                                                      Qt::FindDirectChildrenOnly) ;
    for (auto it = list.begin() ; it != list.end() ; ++it )
    {
        ZGuiScene *scene = *it ;
        if (scene)
        {
            data.insert(scene->getID()) ;
        }
    }
    return data ;
}

std::set<ZInternalID> ZGuiToplevel::getSequenceIDs(ZInternalID gui_id) const
{
    std::set<ZInternalID> data ;
    ZGuiMain* gui_main = findChild<ZGuiMain*>(ZGuiMain::getNameFromID(gui_id),
                                              Qt::FindDirectChildrenOnly) ;
    if (gui_main)
    {
        data = gui_main->getSequenceIDs() ;
    }
    return data ;
}

ZGuiScene* ZGuiToplevel::getSequence(ZInternalID id) const
{
    return findChild<ZGuiScene*>(ZGuiScene::getNameFromID(id),
                                 Qt::FindDirectChildrenOnly) ;
}

bool ZGuiToplevel::containsSequence(ZInternalID sequence_id) const
{
    return static_cast<bool>(getSequence(sequence_id)) ;
}


std::set<ZInternalID> ZGuiToplevel::getFeaturesetFromTrack(ZInternalID sequence_id, ZInternalID track_id) const
{
    std::set<ZInternalID> dataset ;
    ZGuiScene* sequence = getSequence(sequence_id) ;
    if (sequence)
    {
        dataset = sequence->getFeaturesetsFromTrack(track_id) ;
    }
    return dataset ;
}

ZInternalID ZGuiToplevel::getTrackFromFeatureset(ZInternalID sequence_id, ZInternalID featureset_id ) const
{
    ZInternalID id =  0 ;
    ZGuiScene * sequence = getSequence(sequence_id) ;
    if (sequence)
    {
        id = sequence->getTrackFromFeatureset(featureset_id) ;
    }
    return id ;
}

void ZGuiToplevel::setStatusLabel(ZInternalID id,
                                  const std::string &label,
                                  const std::string &tooltip )
{
    QList<ZGuiMain*> list = findChildren<ZGuiMain*>(QString(),
                                                    Qt::FindDirectChildrenOnly) ;
    for (auto it = list.begin() ; it != list.end() ; ++it)
    {
        ZGuiMain* gui = *it ;
        if (gui && gui->containsSequence(id))
        {
            gui->setStatusLabel(label, tooltip) ;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/// Input and output of data
////////////////////////////////////////////////////////////////////////////////

bool ZGuiToplevel::setSelectSequenceData(const ZGSelectSequenceData& data)
{
    bool result = false ;

    if (m_select_sequence)
    {
        result = m_select_sequence->setSelectSequenceData(data) ;
    }

    return result ;
}

ZGSelectSequenceData ZGuiToplevel::getSelectSequenceData() const
{
    ZGSelectSequenceData data ;

    if (m_select_sequence)
        data = m_select_sequence->getSelectSequenceData() ;

    return data ;
}


////////////////////////////////////////////////////////////////////////////////
/// GuiMain management
////////////////////////////////////////////////////////////////////////////////


std::set<ZInternalID> ZGuiToplevel::getGuiMainIDs() const
{
    std::set<ZInternalID> data ;
    QList<ZGuiMain*> list = findChildren<ZGuiMain*>(QString(),
                                                    Qt::FindDirectChildrenOnly) ;
    for (auto it = list.begin() ; it != list.end() ; ++it)
    {
        ZGuiMain *gui = *it ;
        if (gui)
        {
            data.insert(gui->getID()) ;
        }
    }

    return data ;
}

ZGuiMain* ZGuiToplevel::getGuiMain(ZInternalID id) const
{
    return findChild<ZGuiMain*>(ZGuiMain::getNameFromID(id),
                                 Qt::FindDirectChildrenOnly) ;
}

bool ZGuiToplevel::containsGuiMain(ZInternalID id) const
{
    return static_cast<bool>(getGuiMain(id)) ;
}



void ZGuiToplevel::helpDisplayCreate(HelpType type)
{
    ZGuiTextDisplayDialogue *display = ZGuiTextDisplayDialogue::newInstance() ;
    display->setParent(this, Qt::Window) ;
    display->setAttribute(Qt::WA_DeleteOnClose) ;

    if (type == HelpType::General)
    {
        display->setText(m_help_text_general) ;
        display->setWindowTitle(m_help_text_title_general) ;
    }
    else if (type == HelpType::Release)
    {
        display->setText(m_help_text_release) ;
        display->setWindowTitle(m_help_text_title_release) ;
    }
    else if (type == HelpType::About)
    {
        display->setText(m_help_text_about) ;
        display->setWindowTitle(m_help_text_title_about) ;
    }
    display->show() ;
}



////////////////////////////////////////////////////////////////////////////////
/// Action slots; attached to menu actions
////////////////////////////////////////////////////////////////////////////////

void ZGuiToplevel::slot_action_help_general()
{
    helpDisplayCreate(HelpType::General) ;
}

void ZGuiToplevel::slot_action_help_release()
{
    helpDisplayCreate(HelpType::Release) ;
}

void ZGuiToplevel::slot_action_help_about()
{
    helpDisplayCreate(HelpType::About) ;
}


////////////////////////////////////////////////////////////////////////////////
/// Slots for other purposes
////////////////////////////////////////////////////////////////////////////////

void ZGuiToplevel::slot_log_message(const std::string& message)
{
    emit signal_log_message(message) ;
}

void ZGuiToplevel::slot_quit()
{
    emit signal_received_quit_event();
}

} // namespace GUI

} // namespace ZMap
