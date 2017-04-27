/*  File: ZGMessageFeaturesetOperation.cpp
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

#include "ZGMessageFeaturesetOperation.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageFeaturesetOperation>::m_name("ZGMessageFeaturesetOperation") ;
const ZGMessageType ZGMessageFeaturesetOperation::m_specific_type(ZGMessageType::FeaturesetOperation) ;

ZGMessageFeaturesetOperation::ZGMessageFeaturesetOperation()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageFeaturesetOperation>(),
      Util::ClassName<ZGMessageFeaturesetOperation>(),
      m_sequence_id(0),
      m_featureset_id(0),
      m_operation_type(Type::Invalid)
{
}

ZGMessageFeaturesetOperation::ZGMessageFeaturesetOperation(ZInternalID message_id,
                                               ZInternalID sequence_id,
                                               ZInternalID featureset_id,
                                                           Type operation_type)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageFeaturesetOperation>(),
      Util::ClassName<ZGMessageFeaturesetOperation>(),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id),
      m_operation_type(operation_type)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageFeaturesetOperation::ZGMessageFeaturesetOperation() ; sequence_id may not be 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGMessageFeaturesetOperation::ZGMessageFeaturesetOperation() ; featureset_id may not be 0 ")) ;
    if (m_operation_type == Type::Invalid)
        throw std::runtime_error(std::string("ZGMessageFeaturesetOperation::ZGMessageFeaturesetOperation() ; operation_type may not be set to be Invalid ")) ;
}

ZGMessageFeaturesetOperation::ZGMessageFeaturesetOperation(const ZGMessageFeaturesetOperation &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageFeaturesetOperation>(),
      Util::ClassName<ZGMessageFeaturesetOperation>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id),
      m_operation_type(message.m_operation_type)
{
}

ZGMessageFeaturesetOperation& ZGMessageFeaturesetOperation::operator=(const ZGMessageFeaturesetOperation &message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
        m_operation_type = message.m_operation_type ;
    }
    return *this ;
}

ZGMessage* ZGMessageFeaturesetOperation::clone() const{
    return new (std::nothrow) ZGMessageFeaturesetOperation(*this) ;
}

ZGMessageFeaturesetOperation::~ZGMessageFeaturesetOperation()
{
}

} // namespace GUI

} // namespace ZMap

