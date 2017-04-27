/*  File: ZGPacMessage.cpp
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

#include "ZGPacMessage.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

ZGPacMessage::ZGPacMessage()
    : m_type(ZGPacMessageType::Invalid),
      m_id(0)
{

}

ZGPacMessage::ZGPacMessage(ZGPacMessageType type)
    : m_type(type),
      m_id(0)
{
    if (m_type == ZGPacMessageType::Invalid)
        throw std::runtime_error(std::string("ZGPacMessage::ZGPacMessage() ; instance may not be created with invalid type ")) ;
}

ZGPacMessage::ZGPacMessage(ZGPacMessageType type, ZInternalID id)
    : m_type(type),
      m_id(id)
{
    if (m_type == ZGPacMessageType::Invalid)
        throw std::runtime_error(std::string("ZGPacMessage::ZGPacMessage() ; instance may not be created with invalid type ")) ;
    if (!m_id)
        throw std::runtime_error(std::string("ZGPacMessage::ZGPacMessage() ; id may not be 0 ")) ;
}

ZGPacMessage::ZGPacMessage(const ZGPacMessage &message)
    : m_type(message.m_type),
      m_id(message.m_id)
{

}

ZGPacMessage& ZGPacMessage::operator=(const ZGPacMessage& message)
{
    if (this != &message)
    {
        m_type = message.m_type ;
        m_id = message.m_id ;
    }
    return *this ;
}


} // namespace GUI

} // namespace ZMap

