/*  File: ZGMessageDeleteFeatures.cpp
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

#include "ZGMessageDeleteFeatures.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageDeleteFeatures>::m_name("ZGMessageDeleteFeatures") ;
const ZGMessageType ZGMessageDeleteFeatures::m_specific_type(ZGMessageType::DeleteFeatures) ;

ZGMessageDeleteFeatures::ZGMessageDeleteFeatures()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageDeleteFeatures>(),
      Util::ClassName<ZGMessageDeleteFeatures>(),
      m_sequence_id(0),
      m_featureset_id(0),
      m_features()
{
}

ZGMessageDeleteFeatures::ZGMessageDeleteFeatures(ZInternalID message_id,
                                                 ZInternalID sequence_id,
                                                 ZInternalID featureset_id,
                                                 const std::set<ZInternalID> & features)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageDeleteFeatures>(),
      Util::ClassName<ZGMessageDeleteFeatures>(),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id),
      m_features(features)
{

    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageDeleteFeatures::ZGMessageDeleteFeatures() ; sequence_id may not be 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGMessageDeleteFeatures::ZGMessageDeleteFeatures() ; featureset_id may not be 0 ")) ;
    if (!m_features.size())
        throw std::runtime_error(std::string("ZGMessageDeleteFeatures::ZGMessageDeleteFeatures() ; no features were given")) ;
}

ZGMessageDeleteFeatures::ZGMessageDeleteFeatures(const ZGMessageDeleteFeatures &message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageDeleteFeatures>(),
      Util::ClassName<ZGMessageDeleteFeatures>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id),
      m_features(message.m_features)
{
}

ZGMessageDeleteFeatures& ZGMessageDeleteFeatures::operator=(const ZGMessageDeleteFeatures& message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
        m_features = message.m_features ;
    }
    return *this ;
}

ZGMessage* ZGMessageDeleteFeatures::clone() const
{
    return new (std::nothrow) ZGMessageDeleteFeatures(*this) ;
}

ZGMessageDeleteFeatures::~ZGMessageDeleteFeatures()
{
}

} // namespace GUI

} // namespace ZMap
