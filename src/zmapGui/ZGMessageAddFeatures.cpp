/*  File: ZGMessageAddFeatures.cpp
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

#include "ZGMessageAddFeatures.h"
#include "ZGFeatureset.h"
#include <stdexcept>
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageAddFeatures>::m_name("ZGMessageAddFeatures") ;
const ZGMessageType ZGMessageAddFeatures::m_specific_type(ZGMessageType::AddFeatures) ;

ZGMessageAddFeatures::ZGMessageAddFeatures()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageAddFeatures>(),
      Util::ClassName<ZGMessageAddFeatures>(),
      m_sequence_id(0),
      m_featureset_data()
{
}

ZGMessageAddFeatures::ZGMessageAddFeatures(ZInternalID message_id,
                                           ZInternalID sequence_id,
                                           const std::shared_ptr<ZGFeatureset>& featureset_data)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageAddFeatures>(),
      Util::ClassName<ZGMessageAddFeatures>(),
      m_sequence_id(sequence_id),
      m_featureset_data(featureset_data)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageAddFeatures::ZGMessageAddFeatures() ; sequence_id may not be 0 ")) ;
    if (!m_featureset_data)
        throw std::runtime_error(std::string("ZGMessageAddFeatures::ZGMessageAddFeatures() ; featureset_data pointer is not valid ")) ;
    if (!m_featureset_data || !m_featureset_data->size())
        throw std::runtime_error(std::string("ZGMessageAddFeatures::ZGMessageAddFeatures() ; featureset_data has 0 size")) ;
}

ZGMessageAddFeatures::ZGMessageAddFeatures(const ZGMessageAddFeatures &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageAddFeatures>(),
      Util::ClassName<ZGMessageAddFeatures>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_data(message.m_featureset_data)
{
}

ZGMessageAddFeatures& ZGMessageAddFeatures::operator=(const ZGMessageAddFeatures &message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_featureset_data = message.m_featureset_data ;
    }
    return *this ;
}

ZGMessage* ZGMessageAddFeatures::clone() const
{
    return new (std::nothrow) ZGMessageAddFeatures(*this) ;
}

ZGMessageAddFeatures::~ZGMessageAddFeatures()
{
}

} // namespace GUI

} // namespace ZMap
