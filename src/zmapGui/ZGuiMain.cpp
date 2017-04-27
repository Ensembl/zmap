/*  File: ZGuiMain.cpp
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

#include "ZGuiMain.h"
#include "ZGuiScene.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiViewControl.h"
#include "ZGuiMainStyle.h"
#include "ZGuiSequences.h"
#include "ZGuiFileImportDialogue.h"
#include "ZGResourceImportData.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiSequenceContextMenu.h"
#include <memory>
#include <stdexcept>
#include <sstream>
#include <QtWidgets>
#include <QSignalMapper>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiMain>::m_name("ZGuiMain") ;
const QString ZGuiMain::m_display_name("ZMap Annotation Tool"),
    ZGuiMain::m_name_base("ZGuiMain_"),
    ZGuiMain::m_context_menu_title("ZMap Sequence Context Menu");
const int ZGuiMain::m_filter_op_single(1),
    ZGuiMain::m_filter_op_all(2) ;

QString ZGuiMain::getNameFromID(ZInternalID id)
{
    QString str ;
    return m_name_base + str.setNum(id) ;
}

ZGuiMain::ZGuiMain(ZInternalID id)
    : QMainWindow(Q_NULLPTR),
      Util::InstanceCounter<ZGuiMain>(),
      Util::ClassName<ZGuiMain>(),
      m_pos(),
      m_geometry(),
      m_id(id),
      m_view_control(Q_NULLPTR),
      m_sequences(Q_NULLPTR),
      m_style(Q_NULLPTR),
      m_mapper_filter_cs(Q_NULLPTR),
      m_mapper_filter_ni(Q_NULLPTR),
      m_mapper_filter_ne(Q_NULLPTR),
      m_mapper_filter_nc(Q_NULLPTR),
      m_mapper_filter_me(Q_NULLPTR),
      m_mapper_filter_mc(Q_NULLPTR),
      m_menu_file(Q_NULLPTR),
      m_menu_export(Q_NULLPTR),
      m_menu_edit(Q_NULLPTR),
      m_menu_view(Q_NULLPTR),
      m_menu_ticket(Q_NULLPTR),
      m_menu_help(Q_NULLPTR),
      m_action_context_menu_title(Q_NULLPTR),
      m_action_file_new_sequence(Q_NULLPTR),
      m_action_file_save(Q_NULLPTR),
      m_action_file_save_as(Q_NULLPTR),
      m_action_file_import(Q_NULLPTR),
      m_action_file_print_screenshot(Q_NULLPTR),
      m_action_file_close(Q_NULLPTR),
      m_action_file_quit(Q_NULLPTR),
      m_action_export_dna(Q_NULLPTR),
      m_action_export_features(Q_NULLPTR),
      m_action_export_features_marked(Q_NULLPTR),
      m_action_export_configuration(Q_NULLPTR),
      m_action_export_styles(Q_NULLPTR),
      m_action_edit_copy(Q_NULLPTR),
      m_action_edit_ucopy(Q_NULLPTR),
      m_action_edit_paste(Q_NULLPTR),
      m_action_edit_styles(Q_NULLPTR),
      m_action_edit_preferences(Q_NULLPTR),
      m_action_edit_dev(Q_NULLPTR),
      m_action_view_details(Q_NULLPTR),
      m_action_toggle_coords(Q_NULLPTR),
      m_action_ticket_viewz(Q_NULLPTR),
      m_action_ticket_zmap(Q_NULLPTR),
      m_action_ticket_seq(Q_NULLPTR),
      m_action_ticket_ana(Q_NULLPTR),
      m_action_help_quick_start(Q_NULLPTR),
      m_action_help_keyboard(Q_NULLPTR),
      m_action_help_filtering(Q_NULLPTR),
      m_action_help_alignment(Q_NULLPTR),
      m_action_help_release_notes(Q_NULLPTR),
      m_action_help_about_zmap(Q_NULLPTR),
      m_action_blixem(Q_NULLPTR),
      m_action_track_bump(Q_NULLPTR),
      m_action_track_hide(Q_NULLPTR),
      m_action_toggle_mark(Q_NULLPTR),
      m_action_compress_tracks(Q_NULLPTR),
      m_action_track_config(Q_NULLPTR),
      m_action_show_feature(Q_NULLPTR),
      m_action_show_featureitem(Q_NULLPTR),
      m_action_show_featuresetitem(Q_NULLPTR),
      m_action_print_style(Q_NULLPTR),
      m_action_show_windowstats(Q_NULLPTR),
      m_action_filter_test(Q_NULLPTR),
      m_action_filter_un(Q_NULLPTR),
      m_action_filter_subtitle(Q_NULLPTR),
      m_action_filter1_cs(Q_NULLPTR),
      m_action_filter1_ni(Q_NULLPTR),
      m_action_filter1_ne(Q_NULLPTR),
      m_action_filter1_nc(Q_NULLPTR),
      m_action_filter1_me(Q_NULLPTR),
      m_action_filter1_mc(Q_NULLPTR),
      m_action_filter2_cs(Q_NULLPTR),
      m_action_filter2_ni(Q_NULLPTR),
      m_action_filter2_ne(Q_NULLPTR),
      m_action_filter2_nc(Q_NULLPTR),
      m_action_filter2_me(Q_NULLPTR),
      m_action_filter2_mc(Q_NULLPTR),
      m_action_search_dna(Q_NULLPTR),
      m_action_search_peptide(Q_NULLPTR),
      m_action_search_feature(Q_NULLPTR),
      m_action_search_list(Q_NULLPTR),
      m_error_message(Q_NULLPTR)
{
    // probably should never get
    // to this point without this being true, but check anyway...
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; can not create ZGuiMain without QApplication instance existing ")) ;

    if (!m_id)
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; id may not be set to zero ")) ;

    // create menu and status bar
    QMenuBar *menu_bar = Q_NULLPTR ;
    try
    {
        menu_bar = menuBar() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught exception on call to menuBar(): ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught unknown exception on call to menuBar()")) ;
    }
    if (!menu_bar)
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not create menu bar for this object ")) ;
    std::stringstream str ;
    str << menu_bar ;
    menu_bar->setObjectName(QString("QMenuBar_%1").arg(QString::fromStdString(str.str()))) ;

    // create style
    try
    {
        m_style = ZGuiMainStyle::newInstance() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught exception on attempt to create window style: ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught unknown exception on attempt to create window style")) ;
    }
    if (!m_style)
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not create window style for this object ")) ;
    m_style->setParent(this) ;
    menu_bar->setStyle(m_style) ;

    // create status bar
    QStatusBar *status_bar = Q_NULLPTR ;
    try
    {
        status_bar = statusBar() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught exception on call to statusBar(): ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught unknown exception on call to statusBar()") ) ;
    }
    if (!status_bar)
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not create status bar for this object") ) ;

    // create some signal mappers
    try
    {
        createMappers() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught exception on call to createMappers() : ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught unknown exception on call to createMappers() ;")) ;
    }

    // create action objects for the system
    try
    {
        createFileMenuActions() ;
        createExportMenuActions() ;
        createEditMenuActions() ;
        createViewMenuActions() ;
        createTicketMenuActions() ;
        createHelpMenuActions() ;
        createMiscActions() ;
        createFilterActions() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught exception on call to createActions functions: ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught unknown exception on call to createActions functions ")) ;
    }

    // create menus for the system
    try
    {
        createAllMenus() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught exception on call to createAllMenus(): ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught unknown exception on call to createAllMenus()")) ;
    }
    // create the layout for the central widget
    QVBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not create layout for this object ")) ;
    layout->setMargin(0) ;

    // create the dummy central widget
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not create dummy widget for this object")) ;

    // set the dummy_widget to be the central on for this window
    try
    {
        setCentralWidget(widget) ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught exception on attempt to set central widget: ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; caught unknown exception on attempt to set central widget")) ;
    }
    // set the layout on the dummy widget
    widget->setLayout(layout) ;

    // create the view control
    if (!(m_view_control  = ZGuiViewControl::newInstance() ))
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not create view control for this object")) ;
    layout->addWidget(m_view_control) ;

    // frame as separator
    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    layout->addWidget(frame) ;

    // create the sequences container
    if (!(m_sequences = ZGuiSequences::newInstance(m_view_control) ))
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not create sequences container for this object ")) ;
    layout->addWidget(m_sequences) ;

    // make some more connections
    if (!connect(m_sequences, SIGNAL(signal_sequenceMadeCurrent(ZGuiSequence*)), m_view_control, SLOT(slot_setCurrentSequence(ZGuiSequence*))))
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not connect sequence_made_current to appropriate receiver ")) ;
    if (!connect(m_sequences, SIGNAL(signal_contextMenuEvent(QPointF)), this, SLOT(slot_contextMenuEvent(QPointF))))
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not connect sequences to context menu event to appropriate receiver ")) ;
    if (!connect(m_sequences, SIGNAL(signal_sequenceSelectionChanged()), this, SLOT(slot_sequenceSelectionChanged())))
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not conenct sequences to selection changed event ")) ;

    // attributes and other properties of the main window
    setWindowTitle(m_display_name) ;

    // create error message object
    if (!(m_error_message = ZGuiWidgetCreator::createErrorMessage(this)))
        throw std::runtime_error(std::string("ZGuiMain::ZGuiMain() ; could not create QErrorMessage object")) ;
}

ZGuiMain::~ZGuiMain()
{
}

ZGuiMain* ZGuiMain::newInstance(ZInternalID id)
{
    ZGuiMain * gui = Q_NULLPTR  ;

    try
    {
        gui = new ZGuiMain(id) ;
    }
    catch (...)
    {
        gui = Q_NULLPTR ;
    }

    if (gui)
    {
        gui->setObjectName(getNameFromID(id)) ;
    }

    return gui ;
}

void ZGuiMain::createMappers()
{
    if (!(m_mapper_filter_cs = ZGuiWidgetCreator::createSignalMapper(this)))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to create mapper_filter_cs")) ;
    if (!connect(m_mapper_filter_cs, SIGNAL(mapped(int)), this, SLOT(slot_action_filter_cs(int))))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to connect mapper_filter_cs ")) ;
    if (!(m_mapper_filter_ni = ZGuiWidgetCreator::createSignalMapper(this)))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to create mapper_filter_ni")) ;
    if (!connect(m_mapper_filter_ni, SIGNAL(mapped(int)), this, SLOT(slot_action_filter_ni(int))))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to connect mapper_filter_ni")) ;
    if (!(m_mapper_filter_ne = ZGuiWidgetCreator::createSignalMapper(this)))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to create mapper_filter_ne")) ;
    if (!connect(m_mapper_filter_ne, SIGNAL(mapped(int)), this, SLOT(slot_action_filter_ne(int))))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to connect mapper_filter_ne")) ;
    if (!(m_mapper_filter_nc = ZGuiWidgetCreator::createSignalMapper(this)))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to create mapper_filter_nc")) ;
    if (!connect(m_mapper_filter_nc, SIGNAL(mapped(int)), this, SLOT(slot_action_filter_nc(int))))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to connect mapper_filter_nc")) ;
    if (!(m_mapper_filter_me = ZGuiWidgetCreator::createSignalMapper(this)))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to create mapper_filter_me")) ;
    if (!connect(m_mapper_filter_me, SIGNAL(mapped(int)), this, SLOT(slot_action_filter_me(int))))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to connect mapper_filter_me")) ;
    if (!(m_mapper_filter_mc = ZGuiWidgetCreator::createSignalMapper(this)))
        throw std::runtime_error(std::string("ZGuiMain:;createMappers() ; unable to create mapper_filter_mc")) ;
    if (!connect(m_mapper_filter_mc, SIGNAL(mapped(int)), this, SLOT(slot_action_filter_mc(int))))
        throw std::runtime_error(std::string("ZGuiMain::createMappers() ; unable to connect mapper_filter_mc ")) ;
}


void ZGuiMain::hideEvent(QHideEvent *event)
{
    m_pos = pos() ;
    m_geometry = saveGeometry() ;
    QMainWindow::hideEvent(event) ;
}

void ZGuiMain::showEvent(QShowEvent* event)
{
    move(m_pos) ;
    restoreGeometry(m_geometry) ;
    QMainWindow::showEvent(event) ;
}

// add a sequence to the sequences container
bool ZGuiMain::addSequence(ZInternalID id, ZGuiScene * scene)
{
    bool result = false ;
    if (!id || !m_sequences || !scene)
        return result ;
    result = m_sequences->addSequence(id, scene) ;

    return result ;
}

std::set<ZInternalID> ZGuiMain::getSequenceIDs() const
{
    std::set<ZInternalID> data ;
    if (m_sequences)
    {
        data = m_sequences->getSequenceIDs() ;
    }
    return data ;
}

bool ZGuiMain::containsSequence(ZInternalID id)
{
    if (!id || !m_sequences)
        return false ;
    return m_sequences->isPresent(id) ;
}

bool ZGuiMain::deleteSequence(ZInternalID id)
{
    if (!id || !m_sequences)
        return false ;
    return m_sequences->deleteSequence(id) ;
}

bool ZGuiMain::setSequenceOrder(const std::vector<ZInternalID> & data)
{
    bool result = false ;
    if (m_sequences)
    {
        result = m_sequences->setSequenceOrder(data) ;
    }
    return result ;
}

void ZGuiMain::setStatusLabel(const std::string& label,
                              const std::string & tooltip )
{
    if (m_view_control)
    {
        m_view_control->setStatusLabel(QString::fromStdString(label),
                                       QString::fromStdString(tooltip)) ;
    }
}

// this is the public interface so we make a break here with Qt definitions
bool ZGuiMain::orientation() const
{
    bool result = false ;
    if (m_sequences)
        result = m_sequences->orientation() == Qt::Horizontal ;
    return result ;
}

void ZGuiMain::setViewOrientation(ZInternalID sequence_id, ZGViewOrientationType orientation)
{
    // find the sequence_id in the container of sequences, and then
    // set the orientation of the current view of this; might have to
    // query the control panel for information on the current sequence
    // and view; same applies to the following two operations as well
}

void ZGuiMain::scaleBarPosition(ZInternalID sequence_id,
                                   ZGScaleRelativeType position)
{
    // find the sequence_id in the sequences container, and set the position of the
    // scale bar in the current view
}

void ZGuiMain::scaleBarShow(ZInternalID sequence_id)
{
    // find the sequence_id in the container of sequences and then
    // set the scale bar of the current view to show()
}

void ZGuiMain::scaleBarHide(ZInternalID sequence_id)
{
    // find the sequence_id in the container of sequences and then
    // set the scale bar of the current view to hide()
}

//
bool ZGuiMain::setOrientation(bool orient)
{
    if (m_sequences)
    {
        Qt::Orientation orientation = orient ? Qt::Horizontal : Qt::Vertical ;
        m_sequences->setOrientation(orientation) ;
        return true ;
    }
    else
        return false ;
}

void ZGuiMain::createAllMenus()
{
    createFileMenu() ;
    createExportMenu() ;
    createEditMenu() ;
    createViewMenu() ;
    createTicketMenu() ;
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
        menu_bar->addSeparator() ;
    createHelpMenu() ;
}

void ZGuiMain::createFileMenu()
{
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_file = menu_bar->addMenu(QString("File")) ;
        if (!m_menu_file)
            throw std::runtime_error(std::string("ZGuiMain::createFileMenu() ; failed to create menu ")) ;

        // add actions to the menu
        if (m_action_file_new_sequence)
            m_menu_file->addAction(m_action_file_new_sequence) ;
        m_menu_file->addSeparator() ;
        if (m_action_file_save)
            m_menu_file->addAction(m_action_file_save) ;
        if (m_action_file_save_as)
            m_menu_file->addAction(m_action_file_save_as) ;
        m_menu_file->addSeparator() ;
        if (m_action_file_import)
            m_menu_file->addAction(m_action_file_import) ;

        // submenu ownership is taken by parent...
        m_menu_export = m_menu_file->addMenu(QString("Export")) ;
        if (!m_menu_export)
            throw std::runtime_error(std::string("ZGuiMain::createFileMenu() ; could not add the export (sub) menu to the file menu")) ;

        m_menu_file->addSeparator() ;
        if (m_action_file_print_screenshot)
            m_menu_file->addAction(m_action_file_print_screenshot) ;
        m_menu_file->addSeparator() ;
        if (m_action_file_close)
            m_menu_file->addAction(m_action_file_close) ;
        m_menu_file->addSeparator() ;
        if (m_action_file_quit)
            m_menu_file->addAction(m_action_file_quit) ;

    }
    else
    {
        throw std::runtime_error(std::string("ZGuiMain::createFileMenu() ; could not access menu bar")) ;
    }
}

void ZGuiMain::createExportMenu()
{
    if (!m_menu_export)
        throw std::runtime_error(std::string("ZGuiMain::createExportMenu() ; menu does not exist")) ;

    // add actions and separators to the export menu
    if (m_action_export_dna)
        m_menu_export->addAction(m_action_export_dna) ;
    if (m_action_export_features)
        m_menu_export->addAction(m_action_export_features) ;
    if (m_action_export_features_marked)
        m_menu_export->addAction(m_action_export_features_marked) ;
    if (m_action_export_configuration)
        m_menu_export->addAction(m_action_export_configuration) ;
    if (m_action_export_styles)
        m_menu_export->addAction(m_action_export_styles) ;
}


void ZGuiMain::createEditMenu()
{
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_edit = menu_bar->addMenu(QString("Edit")) ;
        if (!m_menu_edit)
            throw std::runtime_error(std::string("ZGuiMain::createEditMenu() ; failed to create menu ")) ;

        if (m_action_edit_copy)
            m_menu_edit->addAction(m_action_edit_copy) ;
        if (m_action_edit_ucopy)
            m_menu_edit->addAction(m_action_edit_ucopy) ;
        if (m_action_edit_paste)
            m_menu_edit->addAction(m_action_edit_paste) ;
        m_menu_edit->addSeparator() ;
        if (m_action_edit_styles)
            m_menu_edit->addAction(m_action_edit_styles) ;
        m_menu_edit->addSeparator() ;
        if (m_action_edit_preferences)
            m_menu_edit->addAction(m_action_edit_preferences) ;
        if (m_action_edit_dev)
            m_menu_edit->addAction(m_action_edit_dev) ;

    }
    else
    {
        throw std::runtime_error(std::string("ZGuiMain::createEditMenu() ; could not access menu bar")) ;
    }
}

void ZGuiMain::createViewMenu()
{
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_view = menu_bar->addMenu(QString("View")) ;
        if (!m_menu_view)
            throw std::runtime_error(std::string("ZGuiMain::createViewMenu() ; failed to create menu ")) ;

        if (m_action_view_details)
            m_menu_view->addAction(m_action_view_details) ;
        if (m_action_toggle_coords)
            m_menu_view->addAction(m_action_toggle_coords) ;

    }
    else
    {
        throw std::runtime_error(std::string("ZGuiMain::createViewMenu() ; could not access menu bar")) ;
    }
}

void ZGuiMain::createTicketMenu()
{
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_ticket = menu_bar->addMenu(QString("Ticket")) ;
        if (!m_menu_ticket)
            throw std::runtime_error(std::string("ZGuiMain::createTicketMenu() ; failed to create menu ")) ;

        if (m_action_ticket_viewz)
            m_menu_ticket->addAction(m_action_ticket_viewz) ;
        if (m_action_ticket_zmap)
            m_menu_ticket->addAction(m_action_ticket_zmap) ;
        if (m_action_ticket_seq)
            m_menu_ticket->addAction(m_action_ticket_seq) ;
        if (m_action_ticket_ana)
            m_menu_ticket->addAction(m_action_ticket_ana) ;

    }
    else
    {
        throw std::runtime_error(std::string("ZGuiMain::createTicketMenu() ; could not access menu bar")) ;
    }
}

void ZGuiMain::createHelpMenu()
{
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_help = menu_bar->addMenu(QString("Help")) ;
        if (!m_menu_help)
            throw std::runtime_error(std::string("ZGuiMain::createHelpMenu() ; failed to create menu ")) ;

        if (m_action_help_quick_start)
            m_menu_help->addAction(m_action_help_quick_start) ;
        if (m_action_help_keyboard)
            m_menu_help->addAction(m_action_help_keyboard) ;
        if (m_action_help_filtering)
            m_menu_help->addAction(m_action_help_filtering) ;
        if (m_action_help_alignment)
            m_menu_help->addAction(m_action_help_alignment) ;
        if (m_action_help_release_notes)
            m_menu_help->addAction(m_action_help_release_notes) ;
        if (m_action_help_about_zmap)
            m_menu_help->addAction(m_action_help_about_zmap) ;

    }
    else
    {
        throw std::runtime_error(std::string("ZGuiMain::createHelpMenu() ; could not access menu bar")) ;
    }
}


void ZGuiMain::createFileMenuActions()
{
    if (!m_action_file_new_sequence && (m_action_file_new_sequence = ZGuiWidgetCreator::createAction(QString("New sequence"), this)))
    {
        m_action_file_new_sequence->setShortcut(QKeySequence::New);
        m_action_file_new_sequence->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_new_sequence->setStatusTip(QString("New sequence"));
        addAction(m_action_file_new_sequence) ;
        connect(m_action_file_new_sequence, SIGNAL(triggered()), this, SLOT(slot_action_file_new_sequence())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFileMenuAction() ; failed to create action object ")) ;

    if (!m_action_file_save && (m_action_file_save = ZGuiWidgetCreator::createAction(QString("Save"), this)))
    {
        m_action_file_save->setShortcut(QKeySequence::Save);
        m_action_file_save->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_save->setStatusTip(QString("Save data"));
        addAction(m_action_file_save) ;
        connect(m_action_file_save, SIGNAL(triggered()), this, SLOT(slot_action_file_save())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFileMenuAction() ; failed to create action object ")) ;

    if (!m_action_file_save_as && (m_action_file_save_as = ZGuiWidgetCreator::createAction(QString("Save as"), this)))
    {
        m_action_file_save_as->setShortcut(QKeySequence::SaveAs) ;
        m_action_file_save_as->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_save_as->setStatusTip(QString("Save data as ..."));
        addAction(m_action_file_save_as) ;
        connect(m_action_file_save_as, SIGNAL(triggered()), this, SLOT(slot_action_file_save_as())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFileMenuAction() ; failed to create action object ")) ;

    if (!m_action_file_import && (m_action_file_import = ZGuiWidgetCreator::createAction(QString("Import"), this)))
    {
        m_action_file_import->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
        m_action_file_import->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_import->setStatusTip(QString("Import data"));
        addAction(m_action_file_import) ;
        connect(m_action_file_import, SIGNAL(triggered()), this, SLOT(slot_action_file_import())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFileMenuAction() ; failed to create action object ")) ;

    if (!m_action_file_print_screenshot && (m_action_file_print_screenshot = ZGuiWidgetCreator::createAction(QString("Print screenshot"), this)))
    {
        m_action_file_print_screenshot->setShortcut(QKeySequence::Print) ;
        m_action_file_print_screenshot->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_print_screenshot->setStatusTip(QString("Send screenshot to printer"));
        addAction(m_action_file_print_screenshot) ;
        connect(m_action_file_print_screenshot, SIGNAL(triggered()), this, SLOT(slot_action_file_print_screenshot())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFileMenuAction() ; failed to create action object ")) ;

    if (!m_action_file_close && (m_action_file_close = ZGuiWidgetCreator::createAction(QString("Close"), this)))
    {
        m_action_file_close->setShortcut(QKeySequence::Close);
        m_action_file_close->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_close->setStatusTip(QString("Close this ZMap window"));
        addAction(m_action_file_close) ;
        connect(m_action_file_close, SIGNAL(triggered()), this, SLOT(slot_action_file_close())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFileMenuAction() ; failed to create action object ")) ;

    if (!m_action_file_quit && (m_action_file_quit = ZGuiWidgetCreator::createAction(QString("Quit"), this)))
    {
        m_action_file_quit->setShortcut(QKeySequence::Quit);
        m_action_file_quit->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_quit->setStatusTip(QString("Quit this ZMap session"));
        addAction(m_action_file_quit) ;
        connect(m_action_file_quit, SIGNAL(triggered()), this, SLOT(slot_action_file_quit())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFileMenuAction() ; failed to create action object ")) ;


}

void ZGuiMain::createExportMenuActions()
{
    if (!m_action_export_dna && (m_action_export_dna = ZGuiWidgetCreator::createAction(QString("DNA"), this)))
    {
        m_action_export_dna->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_E, Qt::CTRL+Qt::Key_D)) ;
        m_action_export_dna->setShortcutContext(Qt::WindowShortcut) ;
        m_action_export_dna->setStatusTip(QString("Export the DNA sequence stored"));
        addAction(m_action_export_dna) ;
        connect(m_action_export_dna, SIGNAL(triggered()), this, SLOT(slot_action_export_dna())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createExportMenuAction() ; failed to create action object ")) ;

    if (!m_action_export_features && (m_action_export_features = ZGuiWidgetCreator::createAction(QString("Features"), this)))
    {
        m_action_export_features->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_E, Qt::CTRL+Qt::Key_F)) ;
        m_action_export_features->setShortcutContext(Qt::WindowShortcut) ;
        m_action_export_features->setStatusTip(QString("Export all features"));
        addAction(m_action_export_features) ;
        connect(m_action_export_features, SIGNAL(triggered()), this, SLOT(slot_action_export_features())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createExportMenuAction() ; failed to create action object ")) ;

    if (!m_action_export_features_marked && (m_action_export_features_marked = ZGuiWidgetCreator::createAction(QString("Features (marked)"), this)))
    {
        m_action_export_features_marked->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_E, Qt::CTRL+Qt::Key_M)) ;
        m_action_export_features_marked->setShortcutContext(Qt::WindowShortcut) ;
        m_action_export_features_marked->setStatusTip(QString("Export all features in marked region"));
        addAction(m_action_export_features_marked) ;
        connect(m_action_export_features_marked, SIGNAL(triggered()), this, SLOT(slot_action_export_features_marked())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createExportMenuAction() ; failed to create action object ")) ;

    if (!m_action_export_configuration && (m_action_export_configuration = ZGuiWidgetCreator::createAction(QString("Configuration"), this)))
    {
        m_action_export_configuration->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_E, Qt::CTRL+Qt::Key_C)) ;
        m_action_export_configuration->setShortcutContext(Qt::WindowShortcut) ;
        m_action_export_configuration->setStatusTip(QString("Export configuration for ZMap"));
        addAction(m_action_export_configuration) ;
        connect(m_action_export_configuration, SIGNAL(triggered()), this, SLOT(slot_action_export_configuration())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createExportMenuAction() ; failed to create action object ")) ;

    if (!m_action_export_styles && (m_action_export_styles = ZGuiWidgetCreator::createAction(QString("Styles"), this)))
    {
        m_action_export_styles->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_E, Qt::CTRL+Qt::Key_S)) ;
        m_action_export_styles->setShortcutContext(Qt::WindowShortcut) ;
        m_action_export_styles->setStatusTip(QString("Export styles for ZMap"));
        addAction(m_action_export_styles) ;
        connect(m_action_export_styles, SIGNAL(triggered()), this, SLOT(slot_action_export_styles())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createExportMenuAction() ; failed to create action object ")) ;
}

void ZGuiMain::createEditMenuActions()
{
    if (!m_action_edit_copy && (m_action_edit_copy = ZGuiWidgetCreator::createAction(QString("Copy feature coords"), this)))
    {
        m_action_edit_copy->setShortcut(QKeySequence::Copy) ;
        m_action_edit_copy->setShortcutContext(Qt::WindowShortcut) ;
        m_action_edit_copy->setStatusTip(QString("Copy feature coordinates"));
        addAction(m_action_edit_copy) ;
        connect(m_action_edit_copy, SIGNAL(triggered()), this, SLOT(slot_action_edit_copy())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createEditMenuAction() ; failed to create action object ")) ;

    if (!m_action_edit_ucopy && (m_action_edit_ucopy = ZGuiWidgetCreator::createAction(QString("Ucopy feature coords"), this)))
    {
        m_action_edit_ucopy->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_U)) ;
        m_action_edit_ucopy->setShortcutContext(Qt::WindowShortcut) ;
        m_action_edit_ucopy->setStatusTip(QString("UCopy feature coordinates"));
        addAction(m_action_edit_ucopy) ;
        connect(m_action_edit_ucopy, SIGNAL(triggered()), this, SLOT(slot_action_edit_ucopy())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createEditMenuAction() ; failed to create action object ")) ;

    if (!m_action_edit_paste && (m_action_edit_paste = ZGuiWidgetCreator::createAction(QString("Paste feature coords"), this)))
    {
        m_action_edit_paste->setShortcut(QKeySequence::Paste) ;
        m_action_edit_paste->setShortcutContext(Qt::WindowShortcut) ;
        m_action_edit_paste->setStatusTip(QString("Paste feature coordinates"));
        addAction(m_action_edit_paste) ;
        connect(m_action_edit_paste, SIGNAL(triggered()), this, SLOT(slot_action_edit_paste())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createEditMenuAction() ; failed to create action object ")) ;

    if (!m_action_edit_styles && (m_action_edit_styles = ZGuiWidgetCreator::createAction(QString("Styles menu"), this)))
    {
        m_action_edit_styles->setStatusTip(QString("Open styles dialogue"));
        addAction(m_action_edit_styles) ;
        connect(m_action_edit_styles, SIGNAL(triggered()), this, SLOT(slot_action_edit_styles())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createEditMenuAction() ; failed to create action object ")) ;

    if (!m_action_edit_preferences && (m_action_edit_preferences = ZGuiWidgetCreator::createAction(QString("Preferences"), this)))
    {
        m_action_edit_preferences->setStatusTip(QString("Open preferences dialogue"));
        addAction(m_action_edit_preferences) ;
        connect(m_action_edit_preferences, SIGNAL(triggered()), this, SLOT(slot_action_edit_preferences())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createEditMenuAction() ; failed to create action object ")) ;

    if (!m_action_edit_dev && (m_action_edit_dev = ZGuiWidgetCreator::createAction(QString("Developer status"), this)))
    {
        m_action_edit_dev->setStatusTip(QString("Set user's developer status"));
        addAction(m_action_edit_dev) ;
        connect(m_action_edit_dev, SIGNAL(triggered()), this, SLOT(slot_action_edit_dev())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createEditMenuAction() ; failed to create action object ")) ;
}

void ZGuiMain::createViewMenuActions()
{
    if (!m_action_view_details && (m_action_view_details = ZGuiWidgetCreator::createAction(QString("Session details"), this)))
    {
        m_action_view_details->setStatusTip(QString("Display session details"));
        addAction(m_action_view_details) ;
        connect(m_action_view_details, SIGNAL(triggered()), this, SLOT(slot_action_view_details())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createViewMenuAction() ; failed to create action object ")) ;

    if (!m_action_toggle_coords && (m_action_toggle_coords = ZGuiWidgetCreator::createAction(QString("Toggle coords"), this)))
    {
        m_action_toggle_coords->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_T)) ;
        m_action_toggle_coords->setShortcutContext(Qt::WindowShortcut) ;
        m_action_toggle_coords->setStatusTip(QString("Toggle display coordinate system"));
        addAction(m_action_toggle_coords) ;
        connect(m_action_toggle_coords, SIGNAL(triggered()), this, SLOT(slot_action_view_toggle())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createViewMenuAction() ; failed to create action object ")) ;
}

void ZGuiMain::createTicketMenuActions()
{

    if (!m_action_ticket_viewz && (m_action_ticket_viewz = ZGuiWidgetCreator::createAction(QString("View ZMap tickets"), this)))
    {
        m_action_ticket_viewz->setStatusTip(QString("View ZMap ticket queue"));
        addAction(m_action_ticket_viewz) ;
        connect(m_action_ticket_viewz, SIGNAL(triggered()), this, SLOT(slot_action_ticket_viewz())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createTicketMenuAction() ; failed to create action object ")) ;

    if (!m_action_ticket_zmap && (m_action_ticket_zmap = ZGuiWidgetCreator::createAction(QString("Create ZMap ticket"), this)))
    {
        m_action_ticket_zmap->setStatusTip(QString("Create a ticket in the ZMap queue"));
        addAction(m_action_ticket_zmap) ;
        connect(m_action_ticket_zmap, SIGNAL(triggered()), this, SLOT(slot_action_ticket_zmap())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createTicketMenuAction() ; failed to create action object ")) ;

    if (!m_action_ticket_seq && (m_action_ticket_seq = ZGuiWidgetCreator::createAction(QString("Create Seqtools ticket"), this)))
    {
        m_action_ticket_seq->setStatusTip(QString("Create a ticket in the Seqtools queue"));
        addAction(m_action_ticket_seq) ;
        connect(m_action_ticket_seq, SIGNAL(triggered()), this, SLOT(slot_action_ticket_seq())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createTicketMenuAction() ; failed to create action object ")) ;

    if (!m_action_ticket_ana && (m_action_ticket_ana = ZGuiWidgetCreator::createAction(QString("Create Anacode ticket"), this)))
    {
        m_action_ticket_ana->setStatusTip(QString("Create a ticket in the Anacode queue"));
        addAction(m_action_ticket_ana) ;
        connect(m_action_ticket_ana, SIGNAL(triggered()), this, SLOT(slot_action_ticket_ana())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createTicketMenuAction() ; failed to create action object ")) ;

}

void ZGuiMain::createHelpMenuActions()
{

    if (!m_action_help_quick_start && (m_action_help_quick_start = ZGuiWidgetCreator::createAction(QString("Quick start"), this)))
    {
        m_action_help_quick_start->setStatusTip(QString("Quick start guide"));
        addAction(m_action_help_quick_start) ;
        connect(m_action_help_quick_start, SIGNAL(triggered()), this, SLOT(slot_action_help_quick_start())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createHelpMenuAction() ; failed to create action object ")) ;

    if (!m_action_help_keyboard && (m_action_help_keyboard = ZGuiWidgetCreator::createAction(QString("Keyboard and mouse"), this)))
    {
        m_action_help_keyboard->setStatusTip(QString("Keyboard and mouse usage"));
        addAction(m_action_help_keyboard) ;
        connect(m_action_help_keyboard, SIGNAL(triggered()), this, SLOT(slot_action_help_keyboard())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createHelpMenuAction() ; failed to create action object ")) ;

    if (!m_action_help_filtering && (m_action_help_filtering = ZGuiWidgetCreator::createAction(QString("Feature filtering"), this)))
    {
        m_action_help_filtering->setStatusTip(QString("Feature filtering"));
        addAction(m_action_help_filtering) ;
        connect(m_action_help_filtering, SIGNAL(triggered()), this, SLOT(slot_action_help_filtering())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createHelpMenuAction() ; failed to create action object ")) ;

    if (!m_action_help_alignment && (m_action_help_alignment = ZGuiWidgetCreator::createAction(QString("Alignment display"), this)))
    {
        m_action_help_alignment->setStatusTip(QString("Alignment feature display"));
        addAction(m_action_help_alignment) ;
        connect(m_action_help_alignment, SIGNAL(triggered()), this, SLOT(slot_action_help_alignment())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createHelpMenuAction() ; failed to create action object ")) ;

    if (!m_action_help_release_notes && (m_action_help_release_notes = ZGuiWidgetCreator::createAction(QString("Release notes"), this)))
    {
        m_action_help_release_notes->setStatusTip(QString("Show release notes"));
        addAction(m_action_help_release_notes) ;
        connect(m_action_help_release_notes, SIGNAL(triggered()), this, SLOT(slot_action_help_release_notes())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createHelpMenuAction() ; failed to create action object ")) ;

    if (!m_action_help_about_zmap && (m_action_help_about_zmap = ZGuiWidgetCreator::createAction(QString("About ZMap"), this)))
    {
        m_action_help_about_zmap->setStatusTip(QString("About the ZMap editor and browser"));
        addAction(m_action_help_about_zmap) ;
        connect(m_action_help_about_zmap, SIGNAL(triggered()), this, SLOT(slot_action_help_about_zmap())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createHelpMenuAction() ; failed to create action object ")) ;

}

void ZGuiMain::setMenuBarColor(const QColor& color)
{
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
    {
        QString str = QString("QMenuBar#%1 {background: %2;} ").arg(menu_bar->objectName()).arg(color.name()) ;
        menu_bar->setStyleSheet(str) ;
    }
}

// filtering actions... yawn
void ZGuiMain::createFilterActions()
{

    if (!m_action_filter_test && (m_action_filter_test = ZGuiWidgetCreator::createAction("Filter test window", this)))
    {
        m_action_filter_test->setStatusTip(QString("Open the filter test window")) ;
        addAction(m_action_filter_test) ;
        connect(m_action_filter_test, SIGNAL(triggered(bool)), this, SLOT(slot_action_filter_test())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter_subtitle && (m_action_filter_subtitle = ZGuiWidgetCreator::createAction("To show...", this)))
    {
        addAction(m_action_filter_subtitle) ;
        m_action_filter_subtitle->setDisabled(true) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter1_cs && (m_action_filter1_cs = ZGuiWidgetCreator::createAction("Common splices", this)))
    {
        addAction(m_action_filter1_cs) ;
        m_action_filter1_cs->setStatusTip("Show common splices") ;
        if (m_mapper_filter_cs)
        {
            m_mapper_filter_cs->setMapping(m_action_filter1_cs, m_filter_op_single) ;
            connect(m_action_filter1_cs, SIGNAL(triggered(bool)), m_mapper_filter_cs, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter1_ni && (m_action_filter1_ni = ZGuiWidgetCreator::createAction("Non-matching introns", this)))
    {
        addAction(m_action_filter1_ni) ;
        m_action_filter1_ni->setStatusTip("Show non-matching introns ") ;
        if (m_mapper_filter_ni)
        {
            m_mapper_filter_ni->setMapping(m_action_filter1_ni, m_filter_op_single) ;
            connect(m_action_filter1_ni, SIGNAL(triggered(bool)), m_mapper_filter_ni, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter1_ne && (m_action_filter1_ne = ZGuiWidgetCreator::createAction("Non-matching exons", this)))
    {
        addAction(m_action_filter1_ne) ;
        m_action_filter1_ne->setStatusTip("Show non-matching exons ") ;
        if (m_mapper_filter_ne)
        {
            m_mapper_filter_ne->setMapping(m_action_filter1_ne, m_filter_op_single) ;
            connect(m_action_filter1_ne, SIGNAL(triggered(bool)), m_mapper_filter_ne, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter1_nc && (m_action_filter1_nc = ZGuiWidgetCreator::createAction("Non-matching confirmed introns", this)))
    {
        addAction(m_action_filter1_nc) ;
        m_action_filter1_nc->setStatusTip("Show non-matching confirmed introns") ;
        if (m_mapper_filter_nc)
        {
            m_mapper_filter_nc->setMapping(m_action_filter1_nc, m_filter_op_single) ;
            connect(m_action_filter1_nc, SIGNAL(triggered(bool)), m_mapper_filter_nc, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter1_me && (m_action_filter1_me = ZGuiWidgetCreator::createAction("Matching exons", this)))
    {
        addAction(m_action_filter1_me) ;
        m_action_filter1_me->setStatusTip("Show matching exons") ;
        if (m_mapper_filter_me)
        {
            m_mapper_filter_me->setMapping(m_action_filter1_me, m_filter_op_single) ;
            connect(m_action_filter1_me, SIGNAL(triggered(bool)), m_mapper_filter_me, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter1_mc && (m_action_filter1_mc = ZGuiWidgetCreator::createAction("Matching CDS", this)))
    {
        addAction(m_action_filter1_mc) ;
        m_action_filter1_mc->setStatusTip("Show matching CDS") ;
        if (m_mapper_filter_mc)
        {
            m_mapper_filter_mc->setMapping(m_action_filter1_mc, m_filter_op_single) ;
            connect(m_action_filter1_mc, SIGNAL(triggered(bool)), m_mapper_filter_mc, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter_un && (m_action_filter_un = ZGuiWidgetCreator::createAction("Unfilter", this)))
    {
        addAction(m_action_filter_un) ;
        m_action_filter_un->setStatusTip("Delete filtering results ") ;
        connect(m_action_filter_un, SIGNAL(triggered(bool)), this, SLOT(slot_action_filter_un())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter2_cs && (m_action_filter2_cs = ZGuiWidgetCreator::createAction("Common splices ", this)))
    {
        addAction(m_action_filter2_cs) ;
        m_action_filter2_cs->setStatusTip("Show common splices ") ;
        if (m_mapper_filter_cs)
        {
            m_mapper_filter_cs->setMapping(m_action_filter2_cs, m_filter_op_all) ;
            connect(m_action_filter2_cs, SIGNAL(triggered(bool)), m_mapper_filter_cs, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter2_ni && (m_action_filter2_ni = ZGuiWidgetCreator::createAction("Non-matching introns", this)))
    {
        addAction(m_action_filter2_ni) ;
        m_action_filter2_ni->setStatusTip("Show non-matching introns ") ;
        if (m_mapper_filter_ni)
        {
            m_mapper_filter_ni->setMapping(m_action_filter2_ni, m_filter_op_all) ;
            connect(m_action_filter2_ni, SIGNAL(triggered(bool)), m_mapper_filter_ni, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter2_ne && (m_action_filter2_ne = ZGuiWidgetCreator::createAction("Non-matching exons", this)))
    {
        addAction(m_action_filter2_ne) ;
        m_action_filter2_ne->setStatusTip("Show non-matching exons") ;
        if (m_mapper_filter_ne)
        {
            m_mapper_filter_ne->setMapping(m_action_filter2_ne, m_filter_op_all) ;
            connect(m_action_filter2_ne, SIGNAL(triggered(bool)), m_mapper_filter_ne, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter2_nc && (m_action_filter2_nc = ZGuiWidgetCreator::createAction("Non-matching confirmed introns", this)))
    {
        addAction(m_action_filter2_nc) ;
        m_action_filter2_nc->setStatusTip("Show non-matching confirmed introns") ;
        if (m_mapper_filter_nc)
        {
            m_mapper_filter_nc->setMapping(m_action_filter2_nc, m_filter_op_all) ;
            connect(m_action_filter2_nc, SIGNAL(triggered(bool)), m_mapper_filter_nc, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter2_me && (m_action_filter2_me = ZGuiWidgetCreator::createAction("Matching exons", this)))
    {
        addAction(m_action_filter2_me) ;
        m_action_filter2_me->setStatusTip("Show matching exons") ;
        if (m_mapper_filter_me)
        {
            m_mapper_filter_me->setMapping(m_action_filter2_me, m_filter_op_all) ;
            connect(m_action_filter2_me, SIGNAL(triggered(bool)), m_mapper_filter_me, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

    if (!m_action_filter2_mc && (m_action_filter2_mc = ZGuiWidgetCreator::createAction("Matching CDS ", this)))
    {
        addAction(m_action_filter2_mc) ;
        m_action_filter2_mc->setStatusTip("Show matching CDS ") ;
        if (m_mapper_filter_mc)
        {
            m_mapper_filter_mc->setMapping(m_action_filter2_mc, m_filter_op_all) ;
            connect(m_action_filter2_mc, SIGNAL(triggered(bool)), m_mapper_filter_mc, SLOT(map())) ;
        }
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createFilterActions() ; unable to create action object ")) ;

}

// create other actions not part of the menus for this class; this should be
// called on construction only
void ZGuiMain::createMiscActions()
{
    if (!m_action_context_menu_title && (m_action_context_menu_title = ZGuiWidgetCreator::createAction(m_context_menu_title, this)))
    {
        m_action_context_menu_title->setDisabled(true) ;
        addAction(m_action_context_menu_title) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_blixem && (m_action_blixem = ZGuiWidgetCreator::createAction("Blixem", this)))
    {
        m_action_blixem->setStatusTip(QString("External call to blixem"));
        addAction(m_action_blixem) ;
        connect(m_action_blixem, SIGNAL(triggered()), this, SLOT(slot_action_blixem())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_track_bump && (m_action_track_bump = ZGuiWidgetCreator::createAction("Track bump", this)))
    {
        m_action_track_bump->setShortcut(QKeySequence(Qt::Key_B)) ;
        m_action_track_bump->setShortcutContext(Qt::WindowShortcut) ;
        m_action_track_bump->setStatusTip(QString("Bump the current track ")) ;
        m_action_track_bump->setCheckable(true) ;
        m_action_track_bump->setChecked(true) ;
        addAction(m_action_track_bump) ;
        connect(m_action_track_bump, SIGNAL(triggered(bool)), this, SLOT(slot_action_track_bump())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_track_hide && (m_action_track_hide = ZGuiWidgetCreator::createAction("Track Hide", this)))
    {
        m_action_track_hide->setStatusTip(QString("Hide the current track ")) ;
        addAction(m_action_track_hide) ;
        connect(m_action_track_hide, SIGNAL(triggered(bool)), this, SLOT(slot_action_track_hide())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_toggle_mark && (m_action_toggle_mark = ZGuiWidgetCreator::createAction("Toggle mark", this)))
    {
        m_action_toggle_mark->setShortcut(QKeySequence(Qt::Key_M)) ;
        m_action_toggle_mark->setShortcutContext(Qt::WindowShortcut) ;
        m_action_toggle_mark->setStatusTip(QString("Toggle marked region ")) ;
        m_action_toggle_mark->setCheckable(true) ;
        m_action_toggle_mark->setChecked(false) ;
        addAction(m_action_toggle_mark) ;
        connect(m_action_toggle_mark, SIGNAL(triggered(bool)), this, SLOT(slot_action_toggle_mark())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object")) ;

    if (!m_action_compress_tracks && (m_action_compress_tracks = ZGuiWidgetCreator::createAction("Compress tracks", this)))
    {
        m_action_compress_tracks->setStatusTip(QString("Compress tracks in the current view")) ;
        addAction(m_action_compress_tracks) ;
        connect(m_action_compress_tracks, SIGNAL(triggered(bool)), this, SLOT(slot_action_compress_cols())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_track_config && (m_action_track_config = ZGuiWidgetCreator::createAction("Track config", this)))
    {
        m_action_track_config->setStatusTip(QString("Edit track configuration ")) ;
        addAction(m_action_track_config) ;
        connect(m_action_track_config, SIGNAL(triggered(bool)), this, SLOT(slot_action_track_config())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_show_feature && (m_action_show_feature = ZGuiWidgetCreator::createAction("Show feature", this)))
    {
        m_action_show_feature->setStatusTip(QString("Show details of the feature")) ;
        addAction(m_action_show_feature) ;
        connect(m_action_show_feature, SIGNAL(triggered(bool)), this, SLOT(slot_action_show_feature())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_show_featureitem && (m_action_show_featureitem = ZGuiWidgetCreator::createAction("Show featureitem", this)))
    {
        m_action_show_featureitem->setStatusTip(QString("Show details of the feature item ")) ;
        addAction(m_action_show_featureitem) ;
        connect(m_action_show_featureitem, SIGNAL(triggered(bool)), this, SLOT(slot_action_show_featureitem())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_show_featuresetitem && (m_action_show_featuresetitem = ZGuiWidgetCreator::createAction("Show featuresetitem", this)))
    {
        m_action_show_featuresetitem->setStatusTip(QString("Show details of featureset item ")) ;
        addAction(m_action_show_featuresetitem) ;
        connect(m_action_show_featuresetitem, SIGNAL(triggered(bool)), this, SLOT(slot_action_show_featuresetitem())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_print_style && (m_action_print_style = ZGuiWidgetCreator::createAction("Print style ", this)))
    {
        m_action_print_style->setStatusTip(QString("Print the style of the current track or feature")) ;
        addAction(m_action_print_style) ;
        connect(m_action_print_style, SIGNAL(triggered(bool)), this, SLOT(slot_action_print_style())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_show_windowstats && (m_action_show_windowstats = ZGuiWidgetCreator::createAction("Show window stats", this)))
    {
        m_action_show_windowstats->setStatusTip(QString("Show statistics for current window ")) ;
        addAction(m_action_show_windowstats) ;
        connect(m_action_show_windowstats, SIGNAL(triggered(bool)), this, SLOT(slot_action_show_windowstats())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action object ")) ;

    if (!m_action_search_dna && (m_action_search_dna = ZGuiWidgetCreator::createAction("DNA search", this)))
    {
        m_action_search_dna->setStatusTip(QString("Search the DNA sequence currently loaded ")) ;
        addAction(m_action_search_dna) ;
        connect(m_action_search_dna, SIGNAL(triggered(bool)), this, SLOT(slot_action_search_dna())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action search_dna")) ;

    if (!m_action_search_peptide && (m_action_search_peptide = ZGuiWidgetCreator::createAction("Peptide search", this)))
    {
        m_action_search_peptide->setStatusTip("Search peptide sequence currently loaded ") ;
        addAction(m_action_search_peptide) ;
        connect(m_action_search_peptide, SIGNAL(triggered(bool)), this, SLOT(slot_action_search_peptide())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action search_peptide ")) ;

    if (!m_action_search_feature && (m_action_search_feature = ZGuiWidgetCreator::createAction("Feature search", this)))
    {
        m_action_search_feature->setStatusTip("Search features currently loaded ") ;
        addAction(m_action_search_feature) ;
        connect(m_action_search_feature, SIGNAL(triggered(bool)), this, SLOT(slot_action_search_feature())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action search_feature")) ;

    if (!m_action_search_list && (m_action_search_list = ZGuiWidgetCreator::createAction("List all track features", this)))
    {
        m_action_search_list->setStatusTip("List all features in the current track ") ;
        addAction(m_action_search_list) ;
        connect(m_action_search_list, SIGNAL(triggered(bool)), this, SLOT(slot_action_search_list())) ;
    }
    else
        throw std::runtime_error(std::string("ZGuiMain::createMiscActions() ; unable to create action search_list")) ;
}


QMenu* ZGuiMain::createContextMenu()
{
    QMenu * menu = Q_NULLPTR,
            * smenu = Q_NULLPTR,
            * ssmenu = Q_NULLPTR ;

    if ((menu = ZGuiWidgetCreator::createMenu()))
    {
        menu->addAction(m_action_context_menu_title) ;

        menu->addSeparator() ;

        if ((smenu = menu->addMenu("Developer")))
        {
            smenu->addAction(m_action_show_feature) ;
            smenu->addAction(m_action_show_featureitem) ;
            smenu->addAction(m_action_show_featuresetitem) ;
            smenu->addAction(m_action_print_style) ;
            smenu->addAction(m_action_show_windowstats) ;
            menu->addSeparator() ;
        }

        if ((smenu = menu->addMenu("Track filtering")))
        {
            smenu->addAction(m_action_filter_test) ;
            if ((ssmenu = smenu->addMenu("Filter this col")))
            {
                ssmenu->addAction(m_action_filter_subtitle) ;
                ssmenu->addAction(m_action_filter1_cs) ;
                ssmenu->addAction(m_action_filter1_ni) ;
                ssmenu->addAction(m_action_filter1_ne) ;
                ssmenu->addAction(m_action_filter1_nc) ;
                ssmenu->addAction(m_action_filter1_me) ;
                ssmenu->addAction(m_action_filter1_mc) ;
            }
            if ((ssmenu = smenu->addMenu("Filter all cols")))
            {
                ssmenu->addAction(m_action_filter_test) ;
                ssmenu->addAction(m_action_filter2_cs) ;
                ssmenu->addAction(m_action_filter2_ni) ;
                ssmenu->addAction(m_action_filter2_ne) ;
                ssmenu->addAction(m_action_filter2_nc) ;
                ssmenu->addAction(m_action_filter2_me) ;
                ssmenu->addAction(m_action_filter2_mc) ;
            }
            smenu->addAction(m_action_filter_un) ;
            menu->addSeparator() ;
        }

        menu->addAction(m_action_blixem) ;

        menu->addSeparator() ;

        menu->addAction(m_action_edit_copy) ;
        menu->addAction(m_action_edit_paste) ;
        menu->addAction(m_action_track_bump) ;
        menu->addAction(m_action_track_hide) ;
        menu->addAction(m_action_toggle_mark) ;
        menu->addAction(m_action_compress_tracks) ;

        menu->addSeparator() ;

        menu->addAction(m_action_track_config) ;

        menu->addSeparator() ;

        if ((smenu = menu->addMenu("Search")))
        {
            smenu->addAction(m_action_search_dna) ;
            smenu->addAction(m_action_search_peptide) ;
            smenu->addAction(m_action_search_feature) ;
            smenu->addAction(m_action_search_list) ;
        }

        if ((smenu = menu->addMenu("Export")))
        {
            smenu->addAction(m_action_export_features) ;
            smenu->addAction(m_action_export_features_marked) ;
        }
    }

    return menu ;
}

QMenu* ZGuiMain::createSubmenuDeveloper()
{
    QMenu *menu = Q_NULLPTR ;

    return menu ;
}

QMenu* ZGuiMain::createSubmenuExport()
{
    QMenu *menu = Q_NULLPTR ;

    return menu ;
}

QMenu* ZGuiMain::createSubmenuFiltering()
{
    QMenu *menu = Q_NULLPTR ;

    return menu ;
}

QMenu* ZGuiMain::createSubmenuSearch()
{
    QMenu *menu = Q_NULLPTR ;

    return menu ;
}

///////////////////////////////////////////////////////////////////////////////
/// File menu slots
///////////////////////////////////////////////////////////////////////////////

void ZGuiMain::slot_action_file_new_sequence()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_file_new_sequence() called for " << this ;
#endif
}

void ZGuiMain::slot_action_file_save()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_file_save() called for " << this ;
#endif
}

void ZGuiMain::slot_action_file_save_as()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_file_save_as() called for " << this ;
#endif
}

void ZGuiMain::slot_action_file_import()
{
    try
    {
        std::unique_ptr<ZGuiFileImportDialogue> file_import(ZGuiFileImportDialogue::newInstance()) ;
        if (file_import)
        {
            file_import->setUserHome(m_user_home) ;
#ifndef QT_NO_DEBUG
            qDebug() << "ZGuiMain::slot_action_file_import() ; user home " << QString::fromStdString(m_user_home) ;
#endif
            if (file_import->exec() == QDialog::Accepted)
            {
                if (file_import->isValid())
                {
                    ZGResourceImportData import_data = file_import->getResourceImportData() ;
                    emit signal_resource_import_event(import_data) ;
                }
            }
            file_import->setParent(Q_NULLPTR) ;
        }

    }
    catch (...)
    {
        if (m_error_message)
        {
            m_error_message->showMessage(QString("Error encountered; see log for details.")) ;
        }
        std::stringstream str ;
        str << name() << "::slot_action_file_import() ; exception caught in attempt to perform resource import" ;
        emit signal_log_message(str.str()) ;
    }
}


void ZGuiMain::slot_action_file_print_screenshot()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_file_print_screenshot() " ;
#endif
}

void ZGuiMain::slot_action_file_close()
{
    // could query user here
    QMainWindow::close() ;
}

void ZGuiMain::slot_action_file_quit()
{
    emit signal_received_quit_event() ;
}


void ZGuiMain::slot_contextMenuEvent(const QPointF& pos)
{
    ZGuiSequences* sequences = dynamic_cast<ZGuiSequences*>(QObject::sender()) ;

    if (sequences)
    {
        QMenu *menu = createContextMenu() ;
        if (menu)
        {
            QAction * action = menu->exec(QCursor::pos()) ;
            qDebug() << "ZGuiMain::slot_contextMenuEvent() ; " << pos << action << (action ? action->text() : QString()) ;
            delete menu ;
        }
    }
}

void ZGuiMain::slot_sequenceSelectionChanged()
{
    // this probably isn't the correct sender any more ...
    //ZGuiScene* sequence = dynamic_cast<ZGuiScene*>(QObject::sender()) ;

#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain:slot_sequenceSelectionChanged() ; " << this << QObject::sender() ;
        qDebug() << "current sequence object is: " << (m_sequences ? m_sequences->getCurrent() : Q_NULLPTR);
        // should probably manage this better... should be the sequence object itself that stores the
        // current track or current selection of feature items...
#endif
}


void ZGuiMain::slot_action_blixem()
{
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_blixem() " ;
#endif
}

void ZGuiMain::slot_action_track_bump()
{
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_track_bump() " ;
#endif
}

void ZGuiMain::slot_action_track_hide()
{

#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_track_hide() " ;
#endif

}

void ZGuiMain::slot_action_toggle_mark()
{

#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_toggle_mark() " ;
#endif
}

void ZGuiMain::slot_action_compress_cols()
{

#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_compress_cols() " ;
#endif
}

void ZGuiMain::slot_action_track_config()
{

#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_track_config() " ;
#endif
}

void ZGuiMain::slot_action_show_feature()
{
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_show_feature() " ;
#endif
}

void ZGuiMain::slot_action_show_featureitem()
{
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_show_featureitem() " ;
#endif
}

void ZGuiMain::slot_action_show_featuresetitem()
{
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_show_featuresetitem() " ;
#endif
}

void ZGuiMain::slot_action_print_style()
{
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_print_style() " ;
#endif
}

void ZGuiMain::slot_action_show_windowstats()
{
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiMain::slot_action_show_windowstats() " ;
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// Export menu slots
///////////////////////////////////////////////////////////////////////////////

void ZGuiMain::slot_action_export_dna()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_export_dna() called for " << this ;
#endif
}

void ZGuiMain::slot_action_export_features()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_export_features() called for " << this ;
#endif
}

void ZGuiMain::slot_action_export_features_marked()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_export_features_marked() called for " << this ;
#endif
}

void ZGuiMain::slot_action_export_configuration()
{

#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_export_configuration() called for " << this ;
#endif
}

void ZGuiMain::slot_action_export_styles()
{

#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_export_styles() called for " << this ;
#endif
}



///////////////////////////////////////////////////////////////////////////////
/// Edit menu slots
///////////////////////////////////////////////////////////////////////////////

void ZGuiMain::slot_action_edit_copy()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_edit_copy() called for " << this ;
#endif
}

void ZGuiMain::slot_action_edit_ucopy()
{

}

void ZGuiMain::slot_action_edit_paste()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_edit_paste() called for " << this ;
#endif
}

void ZGuiMain::slot_action_edit_styles()
{

}

void ZGuiMain::slot_action_edit_preferences()
{

}

void ZGuiMain::slot_action_edit_dev()
{

}

///////////////////////////////////////////////////////////////////////////////
/// View menu slots
///////////////////////////////////////////////////////////////////////////////

void ZGuiMain::slot_action_view_details()
{

}

void ZGuiMain::slot_action_view_toggle()
{
}


///////////////////////////////////////////////////////////////////////////////
/// Ticket menu slots
///////////////////////////////////////////////////////////////////////////////

void ZGuiMain::slot_action_ticket_viewz()
{

}

void ZGuiMain::slot_action_ticket_zmap()
{

}

void ZGuiMain::slot_action_ticket_seq()
{

}

void ZGuiMain::slot_action_ticket_ana()
{

}

///////////////////////////////////////////////////////////////////////////////
/// Help menu slots
///////////////////////////////////////////////////////////////////////////////

void ZGuiMain::slot_action_help_quick_start()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_help_quick_start()  called for " << this ;
#endif
}

void ZGuiMain::slot_action_help_keyboard()
{

}

void ZGuiMain::slot_action_help_filtering()
{

}

void ZGuiMain::slot_action_help_alignment()
{

}

void ZGuiMain::slot_action_help_release_notes()
{

}

void ZGuiMain::slot_action_help_about_zmap()
{

}

void ZGuiMain::slot_action_filter_test()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_filter_test() called " ;
#endif
}

void ZGuiMain::slot_action_filter_cs(int op)
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_filter_cs() called with " << op ;
#endif
}

void ZGuiMain::slot_action_filter_ni(int op)
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_filter_ni() called with " << op ;
#endif
}

void ZGuiMain::slot_action_filter_ne(int op)
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_filter_ne() called with " << op ;
#endif
}

void ZGuiMain::slot_action_filter_nc(int op)
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_filter_nc() called with " << op ;
#endif
}

void ZGuiMain::slot_action_filter_me(int op)
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_filter_me() called with " << op ;
#endif
}

void ZGuiMain::slot_action_filter_mc(int op)
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_filter_mc() called with " << op ;
#endif
}

void ZGuiMain::slot_action_filter_un()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiMain::slot_action_filter_un() called "  ;
#endif
}

void ZGuiMain::slot_action_search_dna()
{

}

void ZGuiMain::slot_action_search_feature()
{

}

void ZGuiMain::slot_action_search_list()
{

}

void ZGuiMain::slot_action_search_peptide()
{

}

////////////////////////////////////////////////////////////////////////////////
/// other functions
////////////////////////////////////////////////////////////////////////////////
void ZGuiMain::setUserHome(const std::string& data)
{
    m_user_home = data ;
}

} // namespace GUI

} // namespace ZMap

