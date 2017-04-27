/*  File: ZGuiContextMenuSearch.h
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

#ifndef ZGUICONTEXTMENUSEARCH_H
#define ZGUICONTEXTMENUSEARCH_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <QApplication>
#include <QMenu>
#include <cstddef>
#include <string>

//
// Submenu of context menu; search options
//

namespace ZMap
{

namespace GUI
{
class ZGuiContextMenuSearch : public QMenu,
        public Util::InstanceCounter<ZGuiContextMenuSearch>,
        public Util::ClassName<ZGuiContextMenuSearch>
{

    Q_OBJECT

public:
    static QString menuName() {return m_menu_name ; }
    static ZGuiContextMenuSearch* newInstance(QWidget* parent = Q_NULLPTR) ;

    ~ZGuiContextMenuSearch() ;

signals:
    void signal_searchDNA() ;
    void signal_searchPeptide() ;
    void signal_searchFeature() ;
    void signal_listAll() ;

private:

    ZGuiContextMenuSearch(QWidget* parent=Q_NULLPTR) ;
    ZGuiContextMenuSearch(const ZGuiContextMenuSearch&) = delete ;
    ZGuiContextMenuSearch& operator=(const ZGuiContextMenuSearch&) = delete ;

    static const QString m_menu_name ;

    void createAllItems() ;
    void connectAllItems() ;
    void addAllItems() ;

    QAction *m_action_dna,
        *m_action_peptide,
        *m_action_feature,
        *m_action_list_all ;

};


} // namespace GUI

} // namespace ZMap


#endif // ZGUICONTEXTMENUSEARCH_H
