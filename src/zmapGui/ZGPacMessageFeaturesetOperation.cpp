/*  File: ZGPacMessageFeaturesetOperation.cpp
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

#include "ZGPacMessageFeaturesetOperation.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageFeaturesetOperation>::m_name("ZGPacMessageFeaturesetOperation") ;
const ZGPacMessageType ZGPacMessageFeaturesetOperation::m_specific_type (ZGPacMessageType::FeaturesetOperation) ;

ZGPacMessageFeaturesetOperation::ZGPacMessageFeaturesetOperation()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageFeaturesetOperation>(),
      Util::ClassName<ZGPacMessageFeaturesetOperation>(),
      m_sequence_id(0),
      m_featureset_id(0),
      m_operation_type(Type::Invalid)
{
}

ZGPacMessageFeaturesetOperation::ZGPacMessageFeaturesetOperation(ZInternalID message_id,
                                                     ZInternalID sequence_id,
                                                     ZInternalID featureset_id,
                                                                 Type operation_type )
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageFeaturesetOperation>(),
      Util::ClassName<ZGPacMessageFeaturesetOperation>(),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id),
      m_operation_type(operation_type)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageFeaturesetOperation::ZGPacMessageFeaturesetOperation() ; sequence_id may not be 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGPacMessageFeaturesetOperation::ZGPacMessageFeaturesetOperation() ; featureset_id may not be 0 ")) ;
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGPacMessageFeaturesetOperation::ZGPacMessageFeaturesetOperation() ; operation_type may not be Invalid ")) ;

}

ZGPacMessageFeaturesetOperation::ZGPacMessageFeaturesetOperation(const ZGPacMessageFeaturesetOperation &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageFeaturesetOperation>(),
      Util::ClassName<ZGPacMessageFeaturesetOperation>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id),
      m_operation_type(message.m_operation_type)
{
}

ZGPacMessageFeaturesetOperation& ZGPacMessageFeaturesetOperation::operator=(const ZGPacMessageFeaturesetOperation& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
        m_operation_type = message.m_operation_type ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageFeaturesetOperation::clone() const
{
    return new (std::nothrow) ZGPacMessageFeaturesetOperation(*this) ;
}

ZGPacMessageFeaturesetOperation::~ZGPacMessageFeaturesetOperation()
{
}

} // namespace GUI

} // namespace ZMap

