/*  File: ZGuiSequenceContextMenu.h
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

#ifndef ZGUISEQUENCECONTEXTMENU_H
#define ZGUISEQUENCECONTEXTMENU_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <QApplication>
#include <QMenu>
#include <cstddef>
#include <string>

//
// Top level context menu. Submenus are separate classes.
//
// One possible way in which this can be used is to set the caller to be
// the ZGuiSequences container; then the menu actions could be connected
// to specific named slots in the parent. Hence the testing of the parent
// in the constructor.
//
// If some other approach is taken, one would have to cook up some
// rather more complex way of fielding the communications, so for the
// moment I'm leaving it like this.
//


namespace ZMap
{

namespace GUI
{

class ZGuiSequenceContextMenu : public QMenu,
        public Util::InstanceCounter<ZGuiSequenceContextMenu>,
        public Util::ClassName<ZGuiSequenceContextMenu>
{
    Q_OBJECT

public:
    static ZGuiSequenceContextMenu * newInstance(QWidget *parent = Q_NULLPTR) ;

    ~ZGuiSequenceContextMenu() ;
    void setMenuTitle(const QString& title) { m_menu_title = title ; }
    QString getMenuTitle() const {return m_menu_title ; }

private:

    ZGuiSequenceContextMenu(QWidget* parent) ;
    ZGuiSequenceContextMenu(const ZGuiSequenceContextMenu&) = delete ;
    ZGuiSequenceContextMenu& operator=(const ZGuiSequenceContextMenu& ) = delete ;

    static const QString m_default_menu_title ;

    void createAllItems() ;
    void connectAllItems() ;
    void addAllItems() ;

    QString m_menu_title ;

    QAction *m_action_title,
        *m_action_b1,
        *m_action_copy_coords,
        *m_action_paste_coords,
        *m_action_track_bump,
        *m_action_track_hide,
        *m_action_toggle_mark,
        *m_action_compress_cols,
        *m_action_track_config
    ;

    QMenu *m_submenu_developer,
        *m_submenu_filtering,
        *m_submenu_search,
        *m_submenu_export ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGUISEQUENCECONTEXTMENU_H
