/*  File: ZGPacMessageGuiOperation.cpp
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

#include "ZGPacMessageGuiOperation.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageGuiOperation>::m_name("ZGPacMessageGuiOperation") ;
const ZGPacMessageType ZGPacMessageGuiOperation::m_specific_type(ZGPacMessageType::GuiOperation) ;

ZGPacMessageGuiOperation::ZGPacMessageGuiOperation()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageGuiOperation>(),
      Util::ClassName<ZGPacMessageGuiOperation>(),
      m_gui_id(0),
      m_operation_type(Type::Invalid)
{
}

ZGPacMessageGuiOperation::ZGPacMessageGuiOperation(ZInternalID message_id,
                                                   ZInternalID gui_id,
                                                   Type operation_type)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageGuiOperation>(),
      Util::ClassName<ZGPacMessageGuiOperation>(),
      m_gui_id(gui_id),
      m_operation_type(operation_type)
{
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGPacMessageGuiOperation::ZGPacMessageGuiOperation() ; gui_id may not be 0 ")) ;
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGPacMessageGuiOperation::ZGPacMessageGuiOperation() ; operation_type may not be Invalid")) ;

}

ZGPacMessageGuiOperation::ZGPacMessageGuiOperation(const ZGPacMessageGuiOperation &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageGuiOperation>(),
      Util::ClassName<ZGPacMessageGuiOperation>(),
      m_gui_id(message.m_gui_id),
      m_operation_type(message.m_operation_type)
{
}

ZGPacMessageGuiOperation& ZGPacMessageGuiOperation::operator=(const ZGPacMessageGuiOperation& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
        m_gui_id = message.m_gui_id ;
        m_operation_type = message.m_operation_type ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageGuiOperation::clone() const
{
    return new (std::nothrow) ZGPacMessageGuiOperation(*this) ;
}

ZGPacMessageGuiOperation::~ZGPacMessageGuiOperation()
{
}

} // namespace GUI

} // namespace ZMap

