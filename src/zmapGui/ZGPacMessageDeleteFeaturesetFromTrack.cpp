/*  File: ZGPacMessageDeleteFeaturesetFromTrack.cpp
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

#include "ZGPacMessageDeleteFeaturesetFromTrack.h"

#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageDeleteFeaturesetFromTrack>::m_name("ZGPacMessageDeleteFeaturesetFromTrack") ;
const ZGPacMessageType ZGPacMessageDeleteFeaturesetFromTrack::m_specific_type (ZGPacMessageType::DeleteFeaturesetFromTrack) ;

ZGPacMessageDeleteFeaturesetFromTrack::ZGPacMessageDeleteFeaturesetFromTrack()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageDeleteFeaturesetFromTrack>(),
      Util::ClassName<ZGPacMessageDeleteFeaturesetFromTrack>(),
      m_sequence_id(0),
      m_featureset_id(0),
      m_track_id(0)
{
}

ZGPacMessageDeleteFeaturesetFromTrack::ZGPacMessageDeleteFeaturesetFromTrack(ZInternalID message_id,
                                                     ZInternalID sequence_id,
                                                     ZInternalID featureset_id,
                                                     ZInternalID track_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageDeleteFeaturesetFromTrack>(),
      Util::ClassName<ZGPacMessageDeleteFeaturesetFromTrack>(),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id),
      m_track_id(track_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageDeleteFeaturesetFromTrack::ZGPacMessageDeleteFeaturesetFromTrack() ; sequence_id may not be 0 ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGPacMessageDeleteFeaturesetFromTrack::ZGPacMessageDeleteFeaturesetFromTrack() ; featureset_id may not be 0 ")) ;
    if (!m_track_id)
        throw std::runtime_error(std::string("ZGPacMessageDeleteFeaturesetFromTrack::ZGPacMessageDeleteFeaturesetFromTrack() ; track_id may not be 0 ")) ;
}

ZGPacMessageDeleteFeaturesetFromTrack::ZGPacMessageDeleteFeaturesetFromTrack(const ZGPacMessageDeleteFeaturesetFromTrack &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageDeleteFeaturesetFromTrack>(),
      Util::ClassName<ZGPacMessageDeleteFeaturesetFromTrack>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id),
      m_track_id(message.m_track_id)
{
}

ZGPacMessageDeleteFeaturesetFromTrack& ZGPacMessageDeleteFeaturesetFromTrack::operator=(const ZGPacMessageDeleteFeaturesetFromTrack& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
        m_track_id = message.m_track_id ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageDeleteFeaturesetFromTrack::clone() const
{
    return new (std::nothrow) ZGPacMessageDeleteFeaturesetFromTrack(*this) ;
}

ZGPacMessageDeleteFeaturesetFromTrack::~ZGPacMessageDeleteFeaturesetFromTrack()
{
}

} // namespace GUI

} // namespace ZMap

