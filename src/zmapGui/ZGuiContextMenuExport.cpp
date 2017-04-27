/*  File: ZGuiContextMenuExport.cpp
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

#include "ZGuiContextMenuExport.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiContextMenuExport>::m_name("ZGuiContextMenuExport") ;
const QString ZGuiContextMenuExport::m_menu_name("Export features") ;

ZGuiContextMenuExport::ZGuiContextMenuExport(QWidget* parent)
    : QMenu(parent),
      Util::InstanceCounter<ZGuiContextMenuExport>(),
      Util::ClassName<ZGuiContextMenuExport>(),
      m_action_features(Q_NULLPTR),
      m_action_features_marked(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiContextMenuExport::ZGuiContextMenuExport() ; unable to create instance ")) ;
    try
    {
        createAllItems() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuExport::ZGuiContextMenuExport() ; caught exception in call to createAllItems(): ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuExport::ZGuiContextMenuExport() ; caught unknown exception in call to createAllItems()")) ;
    }

    try
    {
        connectAllItems() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuExport::ZGuiContextMenuExport() ; caught exception in call to connectAllItems(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuExport::ZGuiContextMenuExport() ; caught unknown exception in call to connectAllItems()")) ;
    }

    try
    {
        addAllItems() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuExport::ZGuiContextMenuExport() ; caught exception in call to addAllItems(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuExport::ZGuiContextMenuExport() ; caught unknown exception in call to addAllItems() ")) ;
    }

}

ZGuiContextMenuExport::~ZGuiContextMenuExport()
{
}

ZGuiContextMenuExport* ZGuiContextMenuExport::newInstance(QWidget* parent)
{
    ZGuiContextMenuExport* menu = Q_NULLPTR ;

    try
    {
        menu = new ZGuiContextMenuExport(parent) ;
    }
    catch (...)
    {
        menu = Q_NULLPTR ;
    }

    return menu ;
}

void ZGuiContextMenuExport::createAllItems()
{
    if (m_action_features ||
        m_action_features_marked)
        throw std::runtime_error(std::string("ZGuiContextMenuExport::createAllItems() ; one or more action items alreay exists; may not proceed")) ;

    m_action_features = ZGuiWidgetCreator::createAction(QString("Track Features"), this) ;
    m_action_features_marked = ZGuiWidgetCreator::createAction(QString("Track Features (marked)"), this) ;
}

void ZGuiContextMenuExport::connectAllItems()
{
    if (!m_action_features ||
        !m_action_features_marked)
        throw std::runtime_error(std::string("ZGuiContextMenuExport::connectAllItems() ; one or more action items does not exist ; may not proceed ")) ;

    if (!connect(m_action_features, SIGNAL(triggered(bool)), this, SIGNAL(signal_exportFeatures())))
        throw std::runtime_error(std::string("ZGuiContextMenuExport::connectAllItems() ; could not connect m_action_features")) ;
    if (!connect(m_action_features_marked, SIGNAL(triggered(bool)), this, SIGNAL(signal_exportFeaturesMarked())))
        throw std::runtime_error(std::string("ZGuiContextMenuExport::connectAllItems() ; could not connect m_action_features_marked ")) ;
}

void ZGuiContextMenuExport::addAllItems()
{
    if (m_action_features)
        addAction(m_action_features) ;
    if (m_action_features_marked)
        addAction(m_action_features_marked) ;
}



} // namespace GUI

} // namespace ZMap


