/*  File: ZGuiPresentation.cpp
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

#include "ZGuiPresentation.h"
#include "ZDebug.h"
#include "ZGuiControl.h"
#include "ZGMessage.h"
#include "ZGMessageHeader.h"
#include "ZGuiMain.h"
#include "ZGuiToplevel.h"
#include <sstream>
#include <QDebug>


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiPresentation>::m_name("ZGuiPresentation") ;
const ZInternalID ZGuiPresentation::m_id_init(1) ;
const ZInternalID ZGuiPresentation::m_toplevel_default_id(1) ;

ZGuiPresentation::ZGuiPresentation(ZGPacControl *control)
    : QObject(Q_NULLPTR),
      ZGPacComponent(control),
      Util::InstanceCounter<ZGuiPresentation>(),
      Util::ClassName<ZGuiPresentation>(),
      m_id_generator(m_id_init),
      m_toplevel(Q_NULLPTR)
{
}


ZGuiPresentation::~ZGuiPresentation()
{
    delete m_toplevel ;
}

ZGuiPresentation * ZGuiPresentation::newInstance(ZGPacControl* control)
{
    ZGuiPresentation * presentation = Q_NULLPTR ;

    try
    {
        presentation = new ZGuiPresentation(control) ;
    }
    catch (...)
    {
        presentation = Q_NULLPTR ;
    }

    return presentation ;
}

std::shared_ptr<ZGMessage> ZGuiPresentation::query(const std::shared_ptr<ZGMessage>& message)
{
    std::shared_ptr<ZGMessage> answer ;

    if (message && m_toplevel)
    {
        answer = m_toplevel->query(message) ;
    }

    return answer ;
}


void ZGuiPresentation::receive(const std::shared_ptr<ZGMessage>&  message)
{
    if (!message)
        return ;
    ZGMessageType message_type = message->type() ;

    if (message_type == ZGMessageType::ToplevelOperation)
    {

        std::shared_ptr<ZGMessageToplevelOperation> specific_message =
                std::dynamic_pointer_cast<ZGMessageToplevelOperation, ZGMessage>(message) ;
        if (specific_message)
        {
            processToplevelOperation(specific_message) ;
        }
    }
    else
    {
        if (m_toplevel)
        {
            m_toplevel->receive(message) ;
        }
    }
}

void ZGuiPresentation::processToplevelOperation(const std::shared_ptr<ZGMessageToplevelOperation>& message)
{
    if (!message)
        return ;

    ZGMessageToplevelOperation::Type operation_type = message->getOperationType() ;

    switch (operation_type)
    {

    case ZGMessageToplevelOperation::Type::Create:
    {
        if (!m_toplevel && Util::canCreateQtItem())
        {
            try
            {
                m_toplevel = ZGuiToplevel::newInstance(m_toplevel_default_id) ;
                // Nurse, the screens!
                // m_toplevel->setAttribute(Qt::WA_DeleteOnClose) ;
            }
            catch (std::exception& error)
            {
                slot_log_message(std::string("ZGuiPresentation::processToplevelOperation() ; attempt to create ZGuiToplevel failed; caught excepion: ") + error.what()) ;
            }
            catch (...)
            {
                slot_log_message(std::string("ZGuiPresentation::processToplevelOperation() ; attempt to create ZGuiToplevel failed: caught unknown exception")) ;
            }
            if (!m_toplevel)
            {
                slot_log_message(std::string("ZGuiPresentation::processToplevelOperation() ; attempt to create ZGuiToplevel failed.")) ;
            }
            else
            {
                if (!connect_toplevel())
                {
                    // send an error again....
                }
                else
                {
                    // show? keep seperate for the moment..
                }
            }
        }
        break ;
    }

    case ZGMessageToplevelOperation::Type::Delete:
    {
        if (m_toplevel)
        {
            delete m_toplevel ;
            m_toplevel = Q_NULLPTR ;
        }
        break ;
    }

    case ZGMessageToplevelOperation::Type::Show:
    {
        if (m_toplevel)
        {
            m_toplevel->show() ;
        }
        break ;
    }

    case ZGMessageToplevelOperation::Type::Hide:
    {
        if (m_toplevel)
        {
            m_toplevel->hide() ;
        }
        break ;
    }

    default:
        break ;

    }
}


bool ZGuiPresentation::connect_toplevel()
{
    bool result = false ;

    if (!m_toplevel)
        return result ;

    m_toplevel->disconnect() ;

    if (!(result = connect(m_toplevel, SIGNAL(signal_received_close_event()), this, SLOT(slot_toplevel_closed()))))
        return result ;

    if (!(result = connect(m_toplevel, SIGNAL(signal_received_quit_event()), this, SLOT(slot_toplevel_quit()))))
        return result ;

    if (!(result = connect(m_toplevel, SIGNAL(signal_log_message(std::string)), this, SLOT(slot_log_message(std::string)))))
        return result ;

    result = true ;

    return result ;
}


////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

// received a close signal from the toplevel object
void ZGuiPresentation::slot_toplevel_closed()
{
    if (m_toplevel)
    {
        delete m_toplevel ;
        m_toplevel = Q_NULLPTR ;
    }
}

// quit the application completely
void ZGuiPresentation::slot_toplevel_quit()
{
#ifndef QT_NO_DEBUG
    qDebug() << "slot_toplevel_quit() called " ;
#endif
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageQuit>(message_id)) ;
    if (m_control)
    {
        m_control->changed(this, message) ;
    }
}


// emit a log message
void ZGuiPresentation::slot_log_message(const std::string& str)
{
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageReplyLogMessage>(message_id, str)) ;
    if (m_control)
    {
        m_control->changed(this, message) ;
    }
}

} // namespace GUI

} // namespace ZMap

