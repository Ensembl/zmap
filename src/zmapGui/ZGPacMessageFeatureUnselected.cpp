/*  File: ZGPacMessageFeatureUnselected.h
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

#include "ZGPacMessageFeatureUnselected.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageFeatureUnselected>::m_name("ZGPacMessageFeatureUnselected") ;
const ZGPacMessageType ZGPacMessageFeatureUnselected::m_specific_type(ZGPacMessageType::FeatureUnselected) ;

ZGPacMessageFeatureUnselected::ZGPacMessageFeatureUnselected()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageFeatureUnselected>(),
      Util::ClassName<ZGPacMessageFeatureUnselected>(),
      m_sequence_id(0),
      m_featureset_id(0),
      m_feature_id(0)
{
}

ZGPacMessageFeatureUnselected::ZGPacMessageFeatureUnselected(ZInternalID message_id,
                                                             ZInternalID sequence_id,
                                                             ZInternalID featureset_id,
                                                             ZInternalID feature_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageFeatureUnselected>(),
      Util::ClassName<ZGPacMessageFeatureUnselected>(),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id),
      m_feature_id(feature_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageFeatureUnselected::ZGPacMessageFeatureUnselected() ; sequence_id may not be 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGPacMessageFeatureUnselected::ZGPacMessageFeatureUnselected() ; featureset_id may not be 0 ")) ;
    if (!m_feature_id)
        throw std::runtime_error(std::string("ZGPacMessageFeatureUnselected::ZGPacMessageFeatureUnselected() ; feature_id may not be 0 ")) ;
}

ZGPacMessageFeatureUnselected::ZGPacMessageFeatureUnselected(const ZGPacMessageFeatureUnselected &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageFeatureUnselected>(),
      Util::ClassName<ZGPacMessageFeatureUnselected>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id),
      m_feature_id(message.m_feature_id)
{
}

ZGPacMessageFeatureUnselected& ZGPacMessageFeatureUnselected::operator =(const ZGPacMessageFeatureUnselected& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator =(message) ;
        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
        m_feature_id = message.m_feature_id ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageFeatureUnselected::clone() const
{
    return new (std::nothrow) ZGPacMessageFeatureUnselected(*this ) ;
}

ZGPacMessageFeatureUnselected::~ZGPacMessageFeatureUnselected()
{
}

} // namespace GUI

} // namespace ZMap
