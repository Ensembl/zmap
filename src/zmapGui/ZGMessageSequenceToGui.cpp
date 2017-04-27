/*  File: ZGMessageSequenceToGui.cpp
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

#include "ZGMessageSequenceToGui.h"
#include <new>
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSequenceToGui>::m_name("ZGMessageSequenceToGui") ;
const ZGMessageType ZGMessageSequenceToGui::m_specific_type(ZGMessageType::SequenceToGui) ;
const ZGMessageSequenceToGui::Type ZGMessageSequenceToGui::m_default_operation_type(ZGMessageSequenceToGui::Type::Add) ;

ZGMessageSequenceToGui::ZGMessageSequenceToGui()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSequenceToGui>(),
      Util::ClassName<ZGMessageSequenceToGui>(),
      m_sequence_id(0),
      m_gui_id(0),
      m_operation_type(m_default_operation_type)
{
}

ZGMessageSequenceToGui::ZGMessageSequenceToGui(ZInternalID message_id,
                                           ZInternalID sequence_id,
                                                     ZInternalID gui_id,
                                               Type operation_type )
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSequenceToGui>(),
      Util::ClassName<ZGMessageSequenceToGui>(),
      m_sequence_id(sequence_id),
      m_gui_id(gui_id),
      m_operation_type(operation_type)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageSequenceToGui::ZGMessageSequenceToGui() ; sequence_id may not be 0 ")) ;
    if (!m_gui_id)
        throw std::runtime_error(std::string("ZGMessageSequenceToGui::ZGMessageSequenceToGui() ; gui_id may not be set to 0")) ;
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGMessageSequenceToGui::ZGMessageSequenceToGui() ; operation_type may not be set to Invalid")) ;
}

ZGMessageSequenceToGui::ZGMessageSequenceToGui(const ZGMessageSequenceToGui &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSequenceToGui>(),
      Util::ClassName<ZGMessageSequenceToGui>(),
      m_sequence_id(message.m_sequence_id),
      m_gui_id(message.m_gui_id),
      m_operation_type(message.m_operation_type)
{
}

ZGMessageSequenceToGui& ZGMessageSequenceToGui::operator=(const ZGMessageSequenceToGui &message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;
        m_sequence_id = message.m_sequence_id ;
        m_gui_id = message.m_gui_id ;
        m_operation_type = message.m_operation_type ;
    }
    return *this ;
}

ZGMessage* ZGMessageSequenceToGui::clone() const
{
    return new (std::nothrow) ZGMessageSequenceToGui(*this) ;
}

ZGMessageSequenceToGui::~ZGMessageSequenceToGui()
{
}

} // namespace GUI

} // namespace ZMap
