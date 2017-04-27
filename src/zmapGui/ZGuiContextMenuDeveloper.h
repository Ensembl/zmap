/*  File: ZGuiContextMenuDeveloper.h
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

#ifndef ZGUICONTEXTMENUDEVELOPER_H
#define ZGUICONTEXTMENUDEVELOPER_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include <QApplication>
#include <QMenu>
#include <cstddef>
#include <string>

//
// Submenu of context menu; developer options.
//


namespace ZMap
{

namespace GUI
{

class ZGuiContextMenuDeveloper : public QMenu,
        public Util::InstanceCounter<ZGuiContextMenuDeveloper>,
        public Util::ClassName<ZGuiContextMenuDeveloper>
{
    Q_OBJECT

public:
    static QString menuName() {return m_menu_name ; }
    static ZGuiContextMenuDeveloper* newInstance(QWidget* parent = Q_NULLPTR) ;

    ~ZGuiContextMenuDeveloper() ;

signals:
    void signal_showFeature() ;
    void signal_showFeatureItem() ;
    void signal_showFeaturesetItem() ;
    void signal_printStyle() ;
    void signal_printCanvas() ;
    void signal_showWindowStats() ;


private:

    ZGuiContextMenuDeveloper(QWidget* parent=Q_NULLPTR) ;
    ZGuiContextMenuDeveloper(const ZGuiContextMenuDeveloper&) = delete ;
    ZGuiContextMenuDeveloper& operator=(const ZGuiContextMenuDeveloper&) = delete ;

    static const QString m_menu_name ;

    void createAllItems() ;
    void connectAllItems() ;
    void addAllItems() ;

    QAction *m_action_show_feature,
        *m_action_show_feature_item,
        *m_action_show_featureset_item,
        *m_action_print_style,
        *m_action_print_canvas,
        *m_action_show_window_stats ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUICONTEXTMENUDEVELOPER_H
