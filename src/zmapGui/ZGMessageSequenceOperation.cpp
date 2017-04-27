/*  File: ZGMessageSequenceOperation.cpp
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

#include "ZGMessageSequenceOperation.h"
#include <stdexcept>
#include <new>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageSequenceOperation>::m_name ("ZGMessageSequenceOperation" ) ;
const ZGMessageType ZGMessageSequenceOperation::m_specific_type(ZGMessageType::SequenceOperation) ;

ZGMessageSequenceOperation::ZGMessageSequenceOperation()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageSequenceOperation>(),
      m_sequence_id(0),
      m_operation_type(Type::Invalid)
{
}

ZGMessageSequenceOperation::ZGMessageSequenceOperation(ZInternalID message_id,
                                                       ZInternalID sequence_id,
                                                       Type operation_type)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageSequenceOperation>(),
      Util::ClassName<ZGMessageSequenceOperation>(),
      m_sequence_id(sequence_id),
      m_operation_type(operation_type)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageSequenceOperation::ZGMessageSequenceOperation() ; sequence_id may not be set to 0 ")) ;
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGMessageSequenceOperation::ZGMessageSequenceOperation() ; operation_type may not be set to Invalid")) ;
}

ZGMessageSequenceOperation::ZGMessageSequenceOperation(const ZGMessageSequenceOperation &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageSequenceOperation>(),
      Util::ClassName<ZGMessageSequenceOperation>(),
      m_sequence_id(message.m_sequence_id),
      m_operation_type(message.m_operation_type)
{
}

ZGMessageSequenceOperation& ZGMessageSequenceOperation::operator=(const ZGMessageSequenceOperation& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;
        m_sequence_id = message.m_sequence_id ;
        m_operation_type = message.m_operation_type ;
    }
    return *this ;
}

ZGMessage* ZGMessageSequenceOperation::clone() const
{
    return new (std::nothrow) ZGMessageSequenceOperation(*this) ;
}


ZGMessageSequenceOperation::~ZGMessageSequenceOperation()
{
}



} // namespace GUI

} // namespace ZMap

