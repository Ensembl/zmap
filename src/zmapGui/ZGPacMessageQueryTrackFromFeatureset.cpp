/*  File: ZGPacMessageQueryTrackFromFeatureset.cpp
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

#include "ZGPacMessageQueryTrackFromFeatureset.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageQueryTrackFromFeatureset>::m_name("ZGPacMessageQueryTrackFromFeatureset") ;
const ZGPacMessageType ZGPacMessageQueryTrackFromFeatureset::m_specific_type(ZGPacMessageType::QueryTrackFromFeatureset) ;

ZGPacMessageQueryTrackFromFeatureset::ZGPacMessageQueryTrackFromFeatureset()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageQueryTrackFromFeatureset>(),
      Util::ClassName<ZGPacMessageQueryTrackFromFeatureset>(),
      m_sequence_id(0),
      m_featureset_id(0)
{
}


ZGPacMessageQueryTrackFromFeatureset::ZGPacMessageQueryTrackFromFeatureset(ZInternalID message_id,
                                                                           ZInternalID sequence_id,
                                                                           ZInternalID featureset_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageQueryTrackFromFeatureset>(),
      Util::ClassName<ZGPacMessageQueryTrackFromFeatureset>(),
      m_sequence_id(sequence_id),
      m_featureset_id(featureset_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageQueryTrackFromFeatureset::ZGPacMessageQueryTrackFromFeatureset() ; sequence_id may not be set to zero ")) ;
    if (!m_featureset_id)
        throw std::runtime_error(std::string("ZGPacMessageQueryTrackFromFeatureset::ZGPacMessageQueryTrackFromFeatureset() ; featureset_id may not be set to zero ")) ;
}

ZGPacMessageQueryTrackFromFeatureset::ZGPacMessageQueryTrackFromFeatureset(const ZGPacMessageQueryTrackFromFeatureset& message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageQueryTrackFromFeatureset>(),
      Util::ClassName<ZGPacMessageQueryTrackFromFeatureset>(),
      m_sequence_id(message.m_sequence_id),
      m_featureset_id(message.m_featureset_id)
{
}

ZGPacMessageQueryTrackFromFeatureset& ZGPacMessageQueryTrackFromFeatureset::operator=(const ZGPacMessageQueryTrackFromFeatureset& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator =(message) ;

        m_sequence_id = message.m_sequence_id ;
        m_featureset_id = message.m_featureset_id ;
    }
    return *this ;
}


ZGPacMessage* ZGPacMessageQueryTrackFromFeatureset::clone() const
{
    return new (std::nothrow) ZGPacMessageQueryTrackFromFeatureset(*this) ;
}

ZGPacMessageQueryTrackFromFeatureset::~ZGPacMessageQueryTrackFromFeatureset()
{
}

} // namespace GUI

} // namespace ZMap
