/*  File: ZGuiSequenceContextMenu.cpp
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

#include "ZGuiSequenceContextMenu.h"
#include "ZGuiContextMenuDeveloper.h"
#include "ZGuiContextMenuFiltering.h"
#include "ZGuiContextMenuSearch.h"
#include "ZGuiContextMenuExport.h"
#include "ZGuiSequences.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiSequenceContextMenu>::m_name("ZGuiSequenceContextMenu") ;
const QString ZGuiSequenceContextMenu::m_default_menu_title("ZMap Sequence Context Menu") ;

ZGuiSequenceContextMenu::ZGuiSequenceContextMenu(QWidget* parent)
    : QMenu(parent),
      Util::InstanceCounter<ZGuiSequenceContextMenu>(),
      Util::ClassName<ZGuiSequenceContextMenu>(),
      m_menu_title(m_default_menu_title),
      m_action_title(Q_NULLPTR),
      m_action_b1(Q_NULLPTR),
      m_action_copy_coords(Q_NULLPTR),
      m_action_paste_coords(Q_NULLPTR),
      m_action_track_bump(Q_NULLPTR),
      m_action_track_hide(Q_NULLPTR),
      m_action_toggle_mark(Q_NULLPTR),
      m_action_compress_cols(Q_NULLPTR),
      m_action_track_config(Q_NULLPTR),
      m_submenu_developer(Q_NULLPTR),
      m_submenu_filtering(Q_NULLPTR),
      m_submenu_search(Q_NULLPTR),
      m_submenu_export(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::ZGuiSequenceContextMenu() ; unable to create instance ")) ;

    if (!parent)
    {
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::ZGuiSequenceContextMenu() ; parent may not be set to NULL.")) ;
    }

    try
    {
        createAllItems() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::ZGuiSequenceContextMenu() ; caught exception in attempt to create context menu actions: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::ZGuiSequenceContextMenu() ; caught unknown exception in attempt to create context menu actions")) ;
    }

    try
    {
        connectAllItems() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::ZGuiSequenceContextMenu() ; caught exception in call to connectAllItems(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::ZGuiSequenceContextMenu() ; caught unknown exception in call to connectAllItems()")) ;
    }


    try
    {
        addAllItems() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::ZGuiSequenceContextMenu() ; caught exception in attempt to add context menu actions: ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::ZGuiSequenceContextMenu() ; caught unknown exception in attempt to add context menu actions")) ;
    }
}

ZGuiSequenceContextMenu::~ZGuiSequenceContextMenu()
{
}

ZGuiSequenceContextMenu * ZGuiSequenceContextMenu::newInstance(QWidget* parent)
{
    ZGuiSequenceContextMenu * menu = Q_NULLPTR ;

    try
    {
        menu = new ZGuiSequenceContextMenu(parent) ;
    }
    catch (...)
    {
        menu = Q_NULLPTR ;
    }

    return menu ;
}

void ZGuiSequenceContextMenu::createAllItems()
{
    if (m_action_title ||
        m_action_b1)
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::createAllActions() ; one or more actions have already been created; may not proceed ")) ;

    m_action_title = ZGuiWidgetCreator::createAction(m_menu_title, this) ;
    m_action_title->setDisabled(true) ;

    m_action_b1 = ZGuiWidgetCreator::createAction(QString("Blixem action"), this) ;
    m_action_copy_coords = ZGuiWidgetCreator::createAction(QString("Copy chr coords"), this) ;
    m_action_paste_coords = ZGuiWidgetCreator::createAction(QString("Paste chr coords"), this) ;
    m_action_track_bump = ZGuiWidgetCreator::createAction(QString("Track Bump"), this) ;
    m_action_track_hide = ZGuiWidgetCreator::createAction(QString("Track Hide"), this) ;
    m_action_toggle_mark = ZGuiWidgetCreator::createAction(QString("Toggle Mark"), this) ;
    m_action_compress_cols = ZGuiWidgetCreator::createAction(QString("Compress Tracks"), this) ;
    m_action_track_config = ZGuiWidgetCreator::createAction(QString("Track Configuration"), this) ;

    m_submenu_developer = ZGuiContextMenuDeveloper::newInstance(this) ;
    m_submenu_filtering = ZGuiContextMenuFiltering::newInstance(this) ;
    m_submenu_search = ZGuiContextMenuSearch::newInstance(this) ;
    m_submenu_export = ZGuiContextMenuExport::newInstance(this) ;

}

void ZGuiSequenceContextMenu::connectAllItems()
{
    if (!connect(m_action_b1, SIGNAL(triggered(bool)), this, SIGNAL(signal_blixemAction())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect m_action_b1 to receiver")) ;
    if (!connect(m_action_copy_coords, SIGNAL(triggered(bool)), this, SIGNAL(signal_copyCoords())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect m_action_copy_coords to receiver ")) ;
    if (!connect(m_action_paste_coords, SIGNAL(triggered(bool)), this, SIGNAL(signal_pasteCoords())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect m_action_paste_coords to receiver ")) ;
    if (!connect(m_action_track_bump, SIGNAL(triggered(bool)), this, SIGNAL(signal_trackBump())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect m_action_track_bump to receiver ")) ;
    if (!connect(m_action_track_hide, SIGNAL(triggered(bool)), this, SIGNAL(signal_trackHide())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect m_action_track_hide to receiver ")) ;
    if (!connect(m_action_toggle_mark, SIGNAL(triggered(bool)), this, SIGNAL(signal_toggleMark())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect m_action_toggle_mark to receiver ")) ;
    if (!connect(m_action_compress_cols, SIGNAL(triggered(bool)), this, SIGNAL(signal_compressCols())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect m_action_compress_cols to receiver ")) ;
    if (!connect(m_action_track_config, SIGNAL(triggered(bool)), this, SIGNAL(signal_trackConfig())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect m_action_track_config to receiver ")) ;

    if (!connect(m_submenu_developer, SIGNAL(signal_showFeature()), this, SIGNAL(signal_showFeature())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect submenu_developer to receiver showFeature")) ;
    if (!connect(m_submenu_developer, SIGNAL(signal_showFeatureItem()), this, SIGNAL(signal_showFeatureItem())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect submenu_developer to receiver showFeatureItem")) ;
    if (!connect(m_submenu_developer, SIGNAL(signal_showFeaturesetItem()), this, SIGNAL(signal_showFeaturesetItem())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect submenu_developer to receiver showFeatresetItem")) ;
    if (!connect(m_submenu_developer, SIGNAL(signal_printStyle()), this, SIGNAL(signal_printStyle())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect submenu_developer to receiver printStyle")) ;
    if (!connect(m_submenu_developer, SIGNAL(signal_printCanvas()), this, SIGNAL(signal_printCanvas())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect submenu_developer to receiver printCanvas ")) ;
    if (!connect(m_submenu_developer, SIGNAL(signal_showWindowStats()), this, SIGNAL(signal_showWindowStats())))
        throw std::runtime_error(std::string("ZGuiSequenceContextMenu::connectAllItems() ; unable to connect submenu_developer to receiver showWindowStats ")) ;


}

void ZGuiSequenceContextMenu::addAllItems()
{
    // top entry, just a title
    if (m_action_title)
        addAction(m_action_title) ;

    // ...
    addSeparator() ;
    if (m_submenu_developer)
    {
        QAction *action = addMenu(m_submenu_developer) ;
        if (action)
            action->setText(ZGuiContextMenuDeveloper::menuName()) ;
    }

    // ...
    addSeparator() ;
    if (m_submenu_filtering)
    {
        QAction *action = addMenu(m_submenu_filtering) ;
        if (action)
            action->setText(ZGuiContextMenuFiltering::menuName()) ;
    }

    // ... blixem options...
    addSeparator() ;
    if (m_action_b1)
        addAction(m_action_b1) ;

    // display options
    addSeparator() ;
    if (m_action_copy_coords)
        addAction(m_action_copy_coords) ;
    if (m_action_paste_coords)
        addAction(m_action_paste_coords) ;
    if (m_action_track_bump)
    {
        m_action_track_bump->setCheckable(true) ;
        m_action_track_bump->setChecked(true) ; // should really depend on the track properties
        addAction(m_action_track_bump) ;
    }
    if (m_action_track_hide)
        addAction(m_action_track_hide) ;
    if (m_action_toggle_mark)
        addAction(m_action_toggle_mark) ;
    if (m_action_compress_cols)
        addAction(m_action_compress_cols) ;

    //
    addSeparator() ;
    if (m_action_track_config)
        addAction(m_action_track_config) ;


    // now the two final submenus...
    addSeparator() ;
    if (m_submenu_search)
    {
        QAction* action = addMenu(m_submenu_search) ;
        if (action)
            action->setText(ZGuiContextMenuSearch::menuName()) ;
    }
    if (m_submenu_export)
    {
        QAction *action = addMenu(m_submenu_export) ;
        if (action)
            action->setText(ZGuiContextMenuExport::menuName()) ;
    }

}

} // namespace GUI

} // namespace ZMap
