/*  File: ZGPacMessageDeleteSequence.cpp
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


#include "ZGPacMessageDeleteSequence.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageDeleteSequence>::m_name("ZGPacMessageDeleteSequence") ;
const ZGPacMessageType ZGPacMessageDeleteSequence::m_specific_type(ZGPacMessageType::Invalid) ;

ZGPacMessageDeleteSequence::ZGPacMessageDeleteSequence()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageDeleteSequence>(),
      Util::ClassName<ZGPacMessageDeleteSequence>(),
      m_sequence_id(0)
{
}

ZGPacMessageDeleteSequence::ZGPacMessageDeleteSequence(ZInternalID message_id,
                                                       ZInternalID sequence_id)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageDeleteSequence>(),
      Util::ClassName<ZGPacMessageDeleteSequence>(),
      m_sequence_id(sequence_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGPacMessageDeleteSequence::ZGPacMessageDeleteSequence() ; sequence_id may not be 0 ")) ;
}

ZGPacMessageDeleteSequence::ZGPacMessageDeleteSequence(const ZGPacMessageDeleteSequence &message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageDeleteSequence>(),
      Util::ClassName<ZGPacMessageDeleteSequence>(),
      m_sequence_id(message.m_sequence_id)
{
}

ZGPacMessageDeleteSequence& ZGPacMessageDeleteSequence::operator=(const ZGPacMessageDeleteSequence& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
        m_sequence_id = message.m_sequence_id ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageDeleteSequence::clone() const
{
    return new (std::nothrow) ZGPacMessageDeleteSequence(*this) ;
}

ZGPacMessageDeleteSequence::~ZGPacMessageDeleteSequence()
{
}

} // namespace GUI

} // namespace ZMap
