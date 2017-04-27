/*  File: ZGuiControl.cpp
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

#include <stdexcept>
#include <new>

#include "ZDebug.h"
#include "ZGuiControl.h"
#include "ZGuiAbstraction.h"
#include "ZGuiPresentation.h"
#include "ZGMessageHeader.h"
#include "ZGMessageHeader.h"

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiControl>::m_name("ZGuiControl") ;
const ZInternalID ZGuiControl::m_id_init = 1 ;

ZGuiControl::ZGuiControl()
    : ZGPacControl(),
      Util::InstanceCounter<ZGuiControl>(),
      Util::ClassName<ZGuiControl>(),
      m_id_generator(ZGuiControl::m_id_init),
      m_presentation()
{
    try
    {
        create() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiControl::ZGuiControl() ; exception caught on call to create(): ") + error.what());
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiControl::ZGuiControl() ; unknown exception caught on call to create()")) ;
    }
}

ZGuiControl::~ZGuiControl()
{
}

ZGuiControl *ZGuiControl::newInstance()
{
    ZGuiControl *control= nullptr ;

    try
    {
        control = new ZGuiControl ;
    }
    catch (...)
    {
        control = nullptr ;
    }

    return control ;
}

// instantiate our components
void ZGuiControl::create()
{
    try
    {
        if (!static_cast<bool>(m_presentation))
            m_presentation = std::unique_ptr<ZGPacComponent>(ZGuiPresentation::newInstance(this)) ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiControl::create() ; exception caught on attempt to instantiate ZGuiPresentation: ") + error.what());
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiControl::create() ; unknown exception caught on attempt to instantiate ZGuiPresentation")) ;
    }

}

// a child component has done something and sent an internal message
void ZGuiControl::changed(const ZGPacComponent * const component,
                          const std::shared_ptr<ZGMessage> &message)
{
    if (message->type() == ZGMessageType::Quit)
    {
        send(message) ;
    }
    else if (message->type() == ZGMessageType::ReplyLogMessage)
    {
        send(message) ;
    }
}



// this is for external control messages only
void ZGuiControl::receive(const ZGPacControl * const sender,
                          std::shared_ptr<ZGMessage> message)
{
    if (!message || this == sender)
        return ;

    if (m_presentation)
    {
        m_presentation->receive(message) ;
    }

}

std::shared_ptr<ZGMessage> ZGuiControl::query(const ZGPacControl * const sender,
                                              const std::shared_ptr<ZGMessage>& message )
{
    std::shared_ptr<ZGMessage> answer(std::make_shared<ZGMessageInvalid>());
    if (!message || this == sender)
        return answer ;
    if (m_presentation)
    {
        answer = m_presentation->query(message) ;
    }

    return answer ;
}

} // namespace GUI

} // namespace ZMap
