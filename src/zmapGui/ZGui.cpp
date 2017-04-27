/*  File: ZGui.cpp
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

#include "ZGui.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QMenuBar>

#include <stdexcept>

#include "ZGuiView.h"
#include "ZGuiViewNode.h"
#include "ZGuiTopControl.h"
#include "ZGuiViewControl.h"
#include "ZGuiScene.h"
#include "ZGuiPresentation.h"
#include "ZGuiSequences.h"
#include "Utilities.h"


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGui>::m_name("ZGui") ;
const QString ZGui::m_display_name("ZMap Annotation Tool") ;

ZGui::ZGui(ZInternalID id, ZGuiPresentation *)
    : QWidget(Q_NULLPTR),
      Util::InstanceCounter<ZGui>(),
      Util::ClassName<ZGui>(),
      m_parent(nullptr),
      m_id(id),
      m_layout(Q_NULLPTR),
      m_top_control(Q_NULLPTR),
      m_view_control(Q_NULLPTR),
      m_sequences(Q_NULLPTR)
{
    // we should not be able to instantiate this class without the
    // QApplication instance existing; probably should never get
    // to this point without that being true, but check anyway...
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGui::ZGui() ; can not create ZGui without QApplication instance existing ")) ;
    setWindowTitle(m_display_name) ;

    try
    {
        m_layout = new QVBoxLayout ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGui::ZGui() ; caught exception in attempt to create QVBoxLayout: ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGui::ZGui() ; caught unknown exception in attempt to create QVBoxLayout")) ;
    }
    setLayout(m_layout) ;

    // create the top control
    try
    {
        m_top_control = ZGuiTopControl::newInstance() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGui::ZGui() ; caught exception in attempt to create ZGuiTopControl: ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGui::ZGui() ; caught unknown exception in attempt to create ZGuiTopControl")) ;
    }
    m_layout->addWidget(m_top_control) ;


    // create the view control
    try
    {
        m_view_control = ZGuiViewControl::newInstance() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGui::ZGui() ; caught exception in attempt to create ZGuiViewControl: ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGui::ZGui() ; caught unknown exception in attempt to create ZGuiViewControl")) ;
    }
    m_layout->addWidget(m_view_control) ;

    // create the sequences container
    try
    {
        m_sequences = ZGuiSequences::newInstance(m_view_control)  ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGui::ZGui() ; caught exception in attempt to create ZGuiSequences") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGui::ZGui() ; caught unknown exception in attempt to create ZGuiSequences")) ;
    }
    m_layout->addWidget(m_sequences) ;
}


ZGui::~ZGui()
{
}

ZGui* ZGui::newInstance(ZInternalID id, ZGuiPresentation* parent )
{
    ZGui * gui = Q_NULLPTR ;

    try
    {
        gui = new ZGui(id, parent) ;
    }
    catch (...)
    {
        gui = Q_NULLPTR ;
    }

    return gui ;
}


//
// if we are attached to a parent, then it should control
// the lifetime of this; otherwise it won't have been reset
//
void ZGui::closeEvent(QCloseEvent* event)
{
    if (m_parent)
    {
        //m_parent->guiClose() ;
        // event->ignore() ; we're not even going to get here...
    }
    else
    {
        event->accept() ;
    }
}


// add a sequence to the sequences container
bool ZGui::addSequence(ZInternalID id)
{
    if (!id || !m_sequences)
        return false ;
    return m_sequences->addSequence(id, Q_NULLPTR) ;
}

bool ZGui::addSequences(const std::set<ZInternalID>& dataset)
{
    if (!dataset.size() || !m_sequences)
        return false ;
    for (auto it=dataset.begin();  it!=dataset.end(); ++it)
        m_sequences->addSequence(*it, Q_NULLPTR) ;
    return true ;
}

bool ZGui::deleteSequence(ZInternalID id)
{
    if (!id || !m_sequences)
        return false ;
    return m_sequences->deleteSequence(id) ;
}

// this is the public interface so we make a break here with Qt definitions
bool ZGui::orientation() const
{
    if (m_sequences)
        return (m_sequences->orientation() == Qt::Horizontal) ;
    else
        return false ;
}

bool ZGui::setOrientation(bool orient)
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

} // namespace GUI

} // namespace ZMap
