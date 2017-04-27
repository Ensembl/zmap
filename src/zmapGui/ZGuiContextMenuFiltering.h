/*  File: ZGuiContextMenuFiltering.h
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

#ifndef ZGUICONTEXTMENUFILTERING_H
#define ZGUICONTEXTMENUFILTERING_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <QApplication>
#include <QMenu>
#include <cstddef>
#include <string>

//
// Submenu of context menu; filtering options.
//

namespace ZMap
{

namespace GUI
{

class ZGuiContextMenuFiltering : public QMenu,
        public Util::InstanceCounter<ZGuiContextMenuFiltering>,
        public Util::ClassName<ZGuiContextMenuFiltering>
{
    Q_OBJECT

public:
    static QString menuName() {return m_menu_name ; }
    static ZGuiContextMenuFiltering* newInstance(QWidget* parent=Q_NULLPTR) ;

    ~ZGuiContextMenuFiltering() ;

signals:
    void signal_filterTest() ;
    void signal_filter1_cs() ;
    void signal_filter1_ni() ;
    void signal_filter1_ne() ;
    void signal_filter1_nc() ;
    void signal_filter1_me() ;
    void signal_filter1_mc() ;
    void signal_filter1_un() ;
    void signal_filter2_cs() ;
    void signal_filter2_ni() ;
    void signal_filter2_ne() ;
    void signal_filter2_nc() ;
    void signal_filter2_me() ;
    void signal_filter2_mc() ;
    void signal_filter2_un() ;

private:

    ZGuiContextMenuFiltering(QWidget* parent=Q_NULLPTR) ;
    ZGuiContextMenuFiltering(const ZGuiContextMenuFiltering&) = delete ;
    ZGuiContextMenuFiltering& operator=(const ZGuiContextMenuFiltering&) = delete ;

    static const QString m_menu_name ;

    void createAllItems() ;
    void connectAllItems() ;
    void addAllItems() ;

    QAction *m_action_filter_test,
        *m_action1_title,
        *m_action1_cs,
        *m_action1_ni,
        *m_action1_ne,
        *m_action1_nc,
        *m_action1_me,
        *m_action1_mc,
        *m_action1_un,
        *m_action2_title,
        *m_action2_test,
        *m_action2_cs,
        *m_action2_ni,
        *m_action2_ne,
        *m_action2_nc,
        *m_action2_me,
        *m_action2_mc,
        *m_action2_un ;

    QMenu *m_submenu1,
        *m_submenu2 ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUICONTEXTMENUFILTERING_H
