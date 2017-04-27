/*  File: ZGMessageDeleteSequence.cpp
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

#include "ZGMessageDeleteSequence.h"
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGMessageDeleteSequence>::m_name("ZGMessageDeleteSequence") ;
const ZGMessageType ZGMessageDeleteSequence::m_specific_type(ZGMessageType::Invalid) ;

ZGMessageDeleteSequence::ZGMessageDeleteSequence()
    : ZGMessage(m_specific_type),
      Util::InstanceCounter<ZGMessageDeleteSequence>(),
      Util::ClassName<ZGMessageDeleteSequence>(),
      m_sequence_id(0)
{
}

ZGMessageDeleteSequence::ZGMessageDeleteSequence(ZInternalID message_id, ZInternalID sequence_id)
    : ZGMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGMessageDeleteSequence>(),
      Util::ClassName<ZGMessageDeleteSequence>(),
      m_sequence_id(sequence_id)
{

    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageDeleteSequence::ZGMessageDeleteSequence() ; sequence_id may not be 0 ")) ;
}

ZGMessageDeleteSequence::ZGMessageDeleteSequence(const ZGMessageDeleteSequence& message)
    : ZGMessage(message),
      Util::InstanceCounter<ZGMessageDeleteSequence>(),
      Util::ClassName<ZGMessageDeleteSequence>(),
      m_sequence_id(message.m_sequence_id)
{
}

ZGMessageDeleteSequence& ZGMessageDeleteSequence::operator=(const ZGMessageDeleteSequence &message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;
        m_sequence_id = message.m_sequence_id ;
    }
    return *this ;
}

ZGMessage* ZGMessageDeleteSequence::clone() const
{
    return new (std::nothrow) ZGMessageDeleteSequence(*this) ;
}

ZGMessageDeleteSequence::~ZGMessageDeleteSequence()
{
}


} // namespace GUI

} // namespace ZMap

