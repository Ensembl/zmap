/*  File: ZGPacMessageAddTrack.cpp
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

#include "ZGPacMessageAddTrack.h"
#include <new>
#include <stdexcept>

unsigned int ZGPacMessageAddTrack::m_instances = 0 ;
const std::string ZGPacMessageAddTrack::m_name("ZGPacMessageAddTrack") ;
const ZGPacMessageType ZGPacMessageAddTrack::m_specific_type(ZGPacMessageType::AddTrack) ;

ZGPacMessageAddTrack::ZGPacMessageAddTrack()
    : ZGPacMessage(m_specific_type),
      m_sequence_id(0),
      m_track_id(0)
{
    ++m_instances ;
}

ZGPacMessageAddTrack::ZGPacMessageAddTrack(ZInternalID message_id,
                                           ZInternalID sequence_id,
                                           ZInternalID track_id)
    : ZGPacMessage(m_specific_type, message_id),
      m_sequence_id(sequence_id),
      m_track_id(track_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageAddTrack::ZGPacMessageAddTrack() ; sequence_id may not be 0 ")) ;
    if (!m_track_id)
        throw std::runtime_error(std::string("ZGPacMessageAddTrack::ZGPacMessageAddTrack() ; track_id may not be 0 ")) ;
    ++m_instances ;
}

ZGPacMessageAddTrack::ZGPacMessageAddTrack(const ZGPacMessageAddTrack &message)
    : ZGPacMessage(message),
      m_sequence_id(message.m_sequence_id),
      m_track_id(message.m_track_id)
{
    ++m_instances ;
}

ZGPacMessageAddTrack& ZGPacMessageAddTrack::operator =(const ZGPacMessageAddTrack& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator =(message) ;
        m_sequence_id = message.m_sequence_id ;
        m_track_id = message.m_track_id ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageAddTrack::clone() const
{
    return new (std::nothrow) ZGPacMessageAddTrack(*this) ;
}

ZGPacMessageAddTrack::~ZGPacMessageAddTrack()
{
    --m_instances ;
}
