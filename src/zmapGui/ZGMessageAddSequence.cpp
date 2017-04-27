/*  File: ZGMessageAddSequence.cpp
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

#include "ZGMessageAddSequence.h"
#include <new>
#include <stdexcept>

unsigned int ZGMessageAddSequence::m_instances = 0 ;
const std::string ZGMessageAddSequence::m_name("ZGMessageAddSequence") ;
const ZGMessageType ZGMessageAddSequence::m_specific_type(ZGMessageType::AddSequence) ;

ZGMessageAddSequence::ZGMessageAddSequence()
    : ZGMessage(m_specific_type),
      m_sequence_id(0)
{
    ++m_instances ;
}

ZGMessageAddSequence::ZGMessageAddSequence(ZInternalID message_id,
                                           ZInternalID sequence_id)
    : ZGMessage(m_specific_type, message_id),
      m_sequence_id(sequence_id)
{
    if (!m_sequence_id)
        throw std::runtime_error(std::string("ZGMessageAddSequence::ZGMessageAddSequence() ; sequence_id may not be 0 ")) ;

    ++m_instances ;
}

ZGMessageAddSequence::ZGMessageAddSequence(const ZGMessageAddSequence &message)
    : ZGMessage(message),
      m_sequence_id(message.m_sequence_id)
{
    ++m_instances ;
}

ZGMessageAddSequence& ZGMessageAddSequence::operator=(const ZGMessageAddSequence &message)
{
    if (this != &message)
    {
        ZGMessage::operator=(message) ;

        m_sequence_id = message.m_sequence_id ;
    }
    return *this ;
}

ZGMessage* ZGMessageAddSequence::clone() const
{
    return new (std::nothrow) ZGMessageAddSequence(*this) ;
}

ZGMessageAddSequence::~ZGMessageAddSequence()
{
    --m_instances ;
}
