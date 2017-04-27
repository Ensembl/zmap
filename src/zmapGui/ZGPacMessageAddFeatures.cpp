/*  File: ZGPacMessageAddFeatures.cpp
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

#include "ZGPacMessageAddFeatures.h"
#include "ZGFeatureset.h"
#include <new>
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageAddFeatures>::m_name("ZGPacMessageAddFeatures") ;
const ZGPacMessageType ZGPacMessageAddFeatures::m_specific_type(ZGPacMessageType::AddFeatures) ;

ZGPacMessageAddFeatures::ZGPacMessageAddFeatures()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageAddFeatures>(),
      Util::ClassName<ZGPacMessageAddFeatures>(),
      m_sequence_id(0),
      m_featureset_data()
{
}

ZGPacMessageAddFeatures::ZGPacMessageAddFeatures(ZInternalID message_id,
                                                 ZInternalID sequence_id,
                                                 const std::shared_ptr<ZGFeatureset>& featureset_data)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageAddFeatures>(),
      Util::ClassName<ZGPacMessageAddFeatures>(),
      m_sequence_id(sequence_id),
      m_featureset_data(featureset_data)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageAddFeatures::ZGPacMessageAddFeatures() ; sequence_id may not be 0 ")) ;
    if (!m_featureset_data)
        throw std::runtime_error(std::string("ZGPacMessageAddFeatures::ZGPacMessageAddFeatures() ; featureset_data does not refer to object ")) ;
}

ZGPacMessageAddFeatures::ZGPacMessageAddFeatures(const ZGPacMessageAddFeatures &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageAddFeatures>(),
      Util::ClassName<ZGPacMessageAddFeatures>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_data(message.m_featureset_data)
{
}

ZGPacMessageAddFeatures& ZGPacMessageAddFeatures::operator=(const ZGPacMessageAddFeatures& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
        m_sequence_id = message.m_sequence_id ;
        m_featureset_data = message.m_featureset_data ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageAddFeatures::clone() const
{
    return new (std::nothrow) ZGPacMessageAddFeatures(*this) ;
}

ZGPacMessageAddFeatures::~ZGPacMessageAddFeatures()
{
}

} // namespace GUI

} // namespace ZMap
