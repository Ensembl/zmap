/*  File: ZGPacMessageAddFeatureset.cpp
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

#include "ZGPacMessageAddFeatureset.h"
#include <new>
#include <stdexcept>

unsigned int ZGPacMessageAddFeatureset::m_instances = 0 ;
const std::string ZGPacMessageAddFeatureset::m_name("ZGPacMessageAddFeatureset") ;
const ZGPacMessageType ZGPacMessageAddFeatureset::m_specific_type (ZGPacMessageType::AddFeatureset) ;

ZGPacMessageAddFeatureset::ZGPacMessageAddFeatureset()
    : ZGPacMessage(m_specific_type),
      m_sequence_id(0),
      m_featureset_id(0)
{
    ++m_instances ;
}

ZGPacMessageAddFeatureset::ZGPacMessageAddFeatureset(ZInternalID message_id,
                                                     ZInternalID sequence_id,
                                                     ZInternalID featureset_id)
    : ZGPacMessage(m_specific_type, message_id),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageAddFeatureset::ZGPacMessageAddFeatureset() ; sequence_id may not be 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGPacMessageAddFeatureset::ZGPacMessageAddFeatureset() ; featureset_id may not be 0 ")) ;
    ++m_instances ;
}

ZGPacMessageAddFeatureset::ZGPacMessageAddFeatureset(const ZGPacMessageAddFeatureset &message)
    : ZGPacMessage(message),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id)
{
    ++m_instances ;
}

ZGPacMessageAddFeatureset& ZGPacMessageAddFeatureset::operator=(const ZGPacMessageAddFeatureset& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageAddFeatureset::clone() const
{
    return new (std::nothrow) ZGPacMessageAddFeatureset(*this) ;
}

ZGPacMessageAddFeatureset::~ZGPacMessageAddFeatureset()
{
    --m_instances ;
}
