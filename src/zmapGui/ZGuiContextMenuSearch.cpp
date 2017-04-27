/*  File: ZGuiContextMenuSearch.cpp
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

#include "ZGuiContextMenuSearch.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiContextMenuSearch>::m_name("ZGuiContextMenuSearch") ;
const QString ZGuiContextMenuSearch::m_menu_name("Search or list features/sequence") ;

ZGuiContextMenuSearch::ZGuiContextMenuSearch(QWidget* parent)
    : QMenu(parent),
      Util::InstanceCounter<ZGuiContextMenuSearch>(),
      Util::ClassName<ZGuiContextMenuSearch>(),
      m_action_dna(Q_NULLPTR),
      m_action_peptide(Q_NULLPTR),
      m_action_feature(Q_NULLPTR),
      m_action_list_all(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::ZGuiContextMenuSearch() ; unable to create instance ")) ;
    try
    {
        createAllItems() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::ZGuiContextMenuSearch() ; caught exception in call to createAllItems(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::ZGuiContextMenuSearch() ; caught unknown exception in call to createAllItems()")) ;
    }

    try
    {
        connectAllItems() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::ZGuiContextMenuSearch() ; caught exception in call to connectAllItems(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::ZGuiContextMenuSearch() ; caught unknown exception in call to connectAllItems()")) ;
    }


    try
    {
        addAllItems() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::ZGuiContextMenuSearch() ; caught exception in call to addAllItems(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::ZGuiContextMenuSearch() ; caught unknown exception in call to addAllItems() ")) ;
    }
}

ZGuiContextMenuSearch::~ZGuiContextMenuSearch()
{
}

ZGuiContextMenuSearch* ZGuiContextMenuSearch::newInstance(QWidget *parent)
{
    ZGuiContextMenuSearch *search = Q_NULLPTR ;

    try
    {
        search = new ZGuiContextMenuSearch(parent) ;
    }
    catch (...)
    {
        search = Q_NULLPTR ;
    }

    return search ;
}

void ZGuiContextMenuSearch::createAllItems()
{
    if (m_action_dna ||
        m_action_peptide ||
        m_action_feature ||
        m_action_list_all )
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::createAllItems() ; one or more items already exists; may not proceed ")) ;

    m_action_dna = ZGuiWidgetCreator::createAction(QString("DNA Search "), this) ;
    m_action_peptide = ZGuiWidgetCreator::createAction(QString("Peptide Search"), this) ;
    m_action_feature = ZGuiWidgetCreator::createAction(QString("Feature Search"), this) ;
    m_action_list_all = ZGuiWidgetCreator::createAction(QString("List all track features"), this) ;
}

void ZGuiContextMenuSearch::connectAllItems()
{
    if (!m_action_dna ||
        !m_action_peptide ||
        !m_action_feature ||
        !m_action_list_all )
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::connectAllItems() ; one or more items do not exist ; may not proceed")) ;

    if (!connect(m_action_dna, SIGNAL(triggered(bool)), this, SIGNAL(signal_searchDNA())))
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::connectAllItems() ; could not connect m_action_dna")) ;
    if (!connect(m_action_peptide, SIGNAL(triggered(bool)), this, SIGNAL(signal_searchPeptide())))
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::connectAllItems() ; could not connect m_action_peptide")) ;
    if (!connect(m_action_feature, SIGNAL(triggered(bool)), this, SIGNAL(signal_searchFeature())))
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::connectAllItems() ; could not connect m_action_feature")) ;
    if (!connect(m_action_list_all, SIGNAL(triggered(bool)), this, SIGNAL(signal_listAll())))
        throw std::runtime_error(std::string("ZGuiContextMenuSearch::connectAllItmes() ; could not connect m_action_list_all ")) ;
}

void ZGuiContextMenuSearch::addAllItems()
{
    if (m_action_dna)
        addAction(m_action_dna) ;
    if (m_action_peptide)
        addAction(m_action_peptide) ;
    if (m_action_feature)
        addAction(m_action_feature) ;
    if (m_action_list_all)
        addAction(m_action_list_all) ;
}

} // namespace GUI

} // namespace ZMap

