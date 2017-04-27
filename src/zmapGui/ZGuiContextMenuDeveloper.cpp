/*  File: ZGuiContextMenuDeveloper.cpp
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

#include "ZGuiContextMenuDeveloper.h"
#include "ZGuiWidgetCreator.h"
#include "Utilities.h"
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiContextMenuDeveloper>::m_name("ZGuiContextMenuDeveloper") ;
const QString ZGuiContextMenuDeveloper::m_menu_name("Developer") ;

ZGuiContextMenuDeveloper::ZGuiContextMenuDeveloper(QWidget* parent)
    : QMenu(parent),
      Util::InstanceCounter<ZGuiContextMenuDeveloper>(),
      Util::ClassName<ZGuiContextMenuDeveloper>(),
      m_action_show_feature(Q_NULLPTR),
      m_action_show_feature_item(Q_NULLPTR),
      m_action_show_featureset_item(Q_NULLPTR),
      m_action_print_style(Q_NULLPTR),
      m_action_print_canvas(Q_NULLPTR),
      m_action_show_window_stats(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::ZGuiContextMenuDeveloper() ; unable to create instance ")) ;
    try
    {
        createAllItems() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::ZGuiContextMenuDeveloper() ; caught exception in call to createAllItems(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::ZGuiContextMenuDeveloper() ; caught unknown exception in call to createAllItems()")) ;
    }

    try
    {
        connectAllItems() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::ZGuiContextMenuDeveloper() ; caught exception in call to connectAllItems(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::ZGuiContextMenuDeveloper() ; caught unknown exception in call to connectAllItems() ")) ;
    }

    try
    {
        addAllItems() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::ZGuiContextMenuDeveloper() ; caught exception in call to addAllItems(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::ZGuiContextMenuDeveloper() ; caught unknown exception in call to createAllItems()")) ;
    }
}

ZGuiContextMenuDeveloper::~ZGuiContextMenuDeveloper()
{
}

ZGuiContextMenuDeveloper* ZGuiContextMenuDeveloper::newInstance(QWidget *parent )
{
    ZGuiContextMenuDeveloper *menu = Q_NULLPTR ;

    try
    {
        menu = new ZGuiContextMenuDeveloper(parent) ;
    }
    catch (...)
    {
        menu = Q_NULLPTR ;
    }

    return menu ;
}

void ZGuiContextMenuDeveloper::createAllItems()
{
    if (m_action_show_feature ||
        m_action_show_feature_item ||
        m_action_show_featureset_item ||
        m_action_print_style ||
        m_action_print_canvas ||
        m_action_show_window_stats)
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::createAllItems() ; one or more actions have been created; may not proceed")) ;

    m_action_show_feature = ZGuiWidgetCreator::createAction(QString("Show Feature"), this) ;
    m_action_show_feature_item = ZGuiWidgetCreator::createAction(QString("Show FeatureItem, Feature"), this) ;
    m_action_show_featureset_item = ZGuiWidgetCreator::createAction(QString("Show FeaturesetItem, FeatureItem, Feature"), this) ;
    m_action_print_style = ZGuiWidgetCreator::createAction(QString("Print Style"), this) ;
    m_action_print_canvas = ZGuiWidgetCreator::createAction(QString("Print Canvas"), this) ;
    m_action_show_window_stats = ZGuiWidgetCreator::createAction(QString("Show window stats"), this) ;
}

void ZGuiContextMenuDeveloper::connectAllItems()
{
    if (!m_action_show_feature ||
        !m_action_show_feature_item ||
        !m_action_show_featureset_item ||
        !m_action_print_style ||
        !m_action_print_canvas ||
        !m_action_show_window_stats)
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::connectAllActions() ; one or more actions has not been created; may not proceed")) ;

    if (!connect(m_action_show_feature, SIGNAL(triggered(bool)), this, SIGNAL(signal_showFeature())))
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::connectAllItems(): could not connect m_action_show_feature")) ;
    if (!connect(m_action_show_feature_item, SIGNAL(triggered(bool)), this, SIGNAL(signal_showFeatureItem())))
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::connectAllItems(): could not connect m_action_show_feature_item")) ;
    if (!connect(m_action_show_featureset_item, SIGNAL(triggered(bool)), this, SIGNAL(signal_showFeaturesetItem())))
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::connectAllItems(): could not connect m_action_show_featureset_item ")) ;
    if (!connect(m_action_print_style, SIGNAL(triggered(bool)), this, SIGNAL(signal_printStyle())))
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::connectAllItems(): could not connect m_action_print_style")) ;
    if (!connect(m_action_print_canvas, SIGNAL(triggered(bool)), SIGNAL(signal_printCanvas())))
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::connectAllItems(): could not connect m_action_print_canvas")) ;
    if (!connect(m_action_show_window_stats, SIGNAL(triggered(bool)), this, SIGNAL(signal_showWindowStats())))
        throw std::runtime_error(std::string("ZGuiContextMenuDeveloper::connectAllItems(): could not connect m_action_show_window_stats")) ;

}

void ZGuiContextMenuDeveloper::addAllItems()
{
    if (m_action_show_feature)
        addAction(m_action_show_feature) ;
    if (m_action_show_feature_item)
        addAction(m_action_show_feature_item) ;
    if (m_action_show_featureset_item)
        addAction(m_action_show_featureset_item) ;
    if (m_action_print_style)
        addAction(m_action_print_style) ;
    if (m_action_print_canvas)
        addAction(m_action_print_style) ;
    if (m_action_show_window_stats)
        addAction(m_action_show_window_stats) ;
}

} // namespace GUI

} // namespace ZMap
