/*  File: ZGuiContextMenuFiltering.cpp
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


#include "ZGuiContextMenuFiltering.h"
#include "ZGuiTrackFilterDialogue.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>
#include <memory>
#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiContextMenuFiltering>::m_name("ZGuiContextMenuFilter") ;
const QString ZGuiContextMenuFiltering::m_menu_name("Track Filtering") ;

ZGuiContextMenuFiltering::ZGuiContextMenuFiltering(QWidget* parent)
    : QMenu(parent),
      Util::InstanceCounter<ZGuiContextMenuFiltering>(),
      Util::ClassName<ZGuiContextMenuFiltering>(),
      m_action_filter_test(Q_NULLPTR),
      m_action1_title(Q_NULLPTR),
      m_action1_cs(Q_NULLPTR),
      m_action1_ni(Q_NULLPTR),
      m_action1_ne(Q_NULLPTR),
      m_action1_nc(Q_NULLPTR),
      m_action1_me(Q_NULLPTR),
      m_action1_mc(Q_NULLPTR),
      m_action1_un(Q_NULLPTR),
      m_action2_title(Q_NULLPTR),
      m_action2_test(Q_NULLPTR),
      m_action2_cs(Q_NULLPTR),
      m_action2_ni(Q_NULLPTR),
      m_action2_ne(Q_NULLPTR),
      m_action2_nc(Q_NULLPTR),
      m_action2_me(Q_NULLPTR),
      m_action2_mc(Q_NULLPTR),
      m_action2_un(Q_NULLPTR),
      m_submenu1(Q_NULLPTR),
      m_submenu2(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::ZGuiContextMenuFiltering() ; unable to create instance ")) ;
    try
    {
        createAllItems() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::ZGuiContextMenuFiltering() ; caught exception in call to createAllItems(): ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::ZGuiContextMenuFiltering() ; caught unknown exception in call to createAllItems() ")) ;
    }

    try
    {
        connectAllItems() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::ZGuiContextMenuFiltering() ; caught exception in call to connectAllItems(): ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::ZGuiContextMenuFiltering() ; caught unknown exception in call to connectAllItems() ")) ;
    }

    try
    {
        addAllItems() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::ZGuiContextMenuFiltering() ; caught exception in call to addAllItems(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::ZGuiContextMenuFiltering() ; caught unknown exception in call to addAllItems()")) ;
    }
}

ZGuiContextMenuFiltering::~ZGuiContextMenuFiltering()
{
}


ZGuiContextMenuFiltering* ZGuiContextMenuFiltering::newInstance(QWidget* parent )
{
    ZGuiContextMenuFiltering *filtering = Q_NULLPTR;

    try
    {
        filtering = new ZGuiContextMenuFiltering(parent) ;
    }
    catch (...)
    {
        filtering = Q_NULLPTR ;
    }

    return filtering ;
}


void ZGuiContextMenuFiltering::createAllItems()
{
    if (m_action_filter_test ||
        m_submenu1 ||
        m_submenu2)
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::createAllItems() ; one or more items already exists; may not proceed")) ;

    m_action_filter_test = ZGuiWidgetCreator::createAction(QString("Filter test window"), this) ;
    if (!m_action_filter_test)
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::createAllItems() ; could not create m_action_filter_test ")) ;
    addAction(m_action_filter_test) ;

    m_submenu1 = addMenu(QString("Filter this col"));
    m_submenu2 = addMenu(QString("Filter all cols"));

    if (m_submenu1)
    {
        m_action1_title = ZGuiWidgetCreator::createAction(QString("To show..."), m_submenu1) ;
        m_action1_title->setDisabled(true) ;
        m_action1_cs = ZGuiWidgetCreator::createAction(QString("Common splices"), m_submenu1) ;
        m_action1_ni = ZGuiWidgetCreator::createAction(QString("Non-matching introns"), m_submenu1) ;
        m_action1_ne = ZGuiWidgetCreator::createAction(QString("Non-matching exons"), m_submenu1) ;
        m_action1_nc = ZGuiWidgetCreator::createAction(QString("Non-matching confirmed introns"), m_submenu1) ;
        m_action1_me = ZGuiWidgetCreator::createAction(QString("Matching exons"), m_submenu1) ;
        m_action1_mc = ZGuiWidgetCreator::createAction(QString("Matching CDS"), m_submenu1) ;
        m_action1_un = ZGuiWidgetCreator::createAction(QString("Unfilter"), m_submenu1) ;
    }

    if (m_submenu2)
    {
        m_action2_title = ZGuiWidgetCreator::createAction(QString("To show..."), m_submenu2) ;
        m_action2_title->setDisabled(true) ;
        m_action2_test = ZGuiWidgetCreator::createAction(QString("Test window"), m_submenu2) ;
        m_action2_cs = ZGuiWidgetCreator::createAction(QString("Common splices"), m_submenu2) ;
        m_action2_ni = ZGuiWidgetCreator::createAction(QString("Non-matching introns"), m_submenu2) ;
        m_action2_ne = ZGuiWidgetCreator::createAction(QString("Non-matching exons"), m_submenu2) ;
        m_action2_nc = ZGuiWidgetCreator::createAction(QString("Non-matching confirmed introns"), m_submenu2) ;
        m_action2_me = ZGuiWidgetCreator::createAction(QString("Matching exons"), m_submenu2) ;
        m_action2_mc = ZGuiWidgetCreator::createAction(QString("Matching CDS"), m_submenu2) ;
        m_action2_un = ZGuiWidgetCreator::createAction(QString("Unfilter"), m_submenu2) ;
    }
}

void ZGuiContextMenuFiltering::connectAllItems()
{

    if (!connect(m_action_filter_test, SIGNAL(triggered(bool)), this, SIGNAL(signal_filterTest())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action_filter_test ")) ;

    if (!connect(m_action1_cs, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter1_cs())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action1_cs")) ;
    if (!connect(m_action1_ni, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter1_ni())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action1_ni")) ;
    if (!connect(m_action1_ne, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter1_ne())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action1_ne")) ;
    if (!connect(m_action1_nc, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter1_nc())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action1_nc")) ;
    if (!connect(m_action1_me, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter1_me())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action1_me")) ;
    if (!connect(m_action1_mc, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter1_mc())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action1_mc")) ;
    if (!connect(m_action1_un, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter1_un())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action1_un")) ;

    if (!connect(m_action2_cs, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter2_cs())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action2_cs")) ;
    if (!connect(m_action2_ni, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter2_ni())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action2_ni")) ;
    if (!connect(m_action2_ne, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter2_ne())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action2_ne")) ;
    if (!connect(m_action2_nc, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter2_nc())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action2_nc")) ;
    if (!connect(m_action2_me, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter2_me())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action2_me")) ;
    if (!connect(m_action2_mc, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter2_mc())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action2_mc")) ;
    if (!connect(m_action2_un, SIGNAL(triggered(bool)), this, SIGNAL(signal_filter2_un())))
        throw std::runtime_error(std::string("ZGuiContextMenuFiltering::connectAllItems() ; could not connect m_action2_un")) ;
}

void ZGuiContextMenuFiltering::addAllItems()
{

    if (m_submenu1)
    {
        if (m_action1_title)
            m_submenu1->addAction(m_action1_title) ;
        if (m_action1_cs)
            m_submenu1->addAction(m_action1_cs) ;
        if (m_action1_ni)
            m_submenu1->addAction(m_action1_ni) ;
        if (m_action1_ne)
            m_submenu1->addAction(m_action1_ne) ;
        if (m_action1_nc)
            m_submenu1->addAction(m_action1_nc) ;
        if (m_action1_me)
            m_submenu1->addAction(m_action1_me) ;
        if (m_action1_mc)
            m_submenu1->addAction(m_action1_mc) ;
        if (m_action1_un)
        {
            m_submenu1->addAction(m_action1_un) ;
            m_action1_un->setCheckable(true) ;
            m_action1_un->setChecked(true) ;
        }
    }
    if (m_submenu2)
    {
        if (m_action2_title)
            m_submenu2->addAction(m_action2_title) ;
        if (m_action_filter_test)
            m_submenu2->addAction(m_action_filter_test) ;
        if (m_action2_cs)
            m_submenu2->addAction(m_action2_cs) ;
        if (m_action2_ni)
            m_submenu2->addAction(m_action2_ni) ;
        if (m_action2_ne)
            m_submenu2->addAction(m_action2_ne) ;
        if (m_action2_nc)
            m_submenu2->addAction(m_action2_nc) ;
        if (m_action2_me)
            m_submenu2->addAction(m_action2_me) ;
        if (m_action2_mc)
            m_submenu2->addAction(m_action2_mc) ;
        if (m_action2_un)
        {
            m_submenu2->addAction(m_action2_un) ;
            m_action2_un->setCheckable(true) ;
            m_action2_un->setChecked(true) ;
        }
    }
}

} // namespace GUI

} // namespace ZMap
