/*  File: ZGPacMessageResourceImport.cpp
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

#include "ZGPacMessageResourceImport.h"
#include <new>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGPacMessageResourceImport>::m_name("ZGPacMessageResourceImport") ;
const ZGPacMessageType ZGPacMessageResourceImport::m_specific_type(ZGPacMessageType::ResourceImport) ;

ZGPacMessageResourceImport::ZGPacMessageResourceImport()
    : ZGPacMessage(m_specific_type),
      Util::InstanceCounter<ZGPacMessageResourceImport>(),
      Util::ClassName<ZGPacMessageResourceImport>(),
      m_resource_data()
{
}


ZGPacMessageResourceImport::ZGPacMessageResourceImport(ZInternalID message_id,
                                                       const ZGResourceImportData &data)
    : ZGPacMessage(m_specific_type, message_id),
      Util::InstanceCounter<ZGPacMessageResourceImport>(),
      Util::ClassName<ZGPacMessageResourceImport>(),
      m_resource_data(data)
{
}

ZGPacMessageResourceImport::ZGPacMessageResourceImport(const ZGPacMessageResourceImport& message)
    : ZGPacMessage(message),
      Util::InstanceCounter<ZGPacMessageResourceImport>(),
      Util::ClassName<ZGPacMessageResourceImport>(),
      m_resource_data(message.m_resource_data)
{
}

ZGPacMessageResourceImport& ZGPacMessageResourceImport::operator=(const ZGPacMessageResourceImport& message)
{
    if (this != &message)
    {
        ZGPacMessage::operator=(message) ;
        m_resource_data = message.m_resource_data ;
    }
    return *this ;
}

ZGPacMessage* ZGPacMessageResourceImport::clone() const
{
    return new (std::nothrow) ZGPacMessageResourceImport(*this) ;
}

ZGPacMessageResourceImport::~ZGPacMessageResourceImport()
{
}

} // namespace GUI

} // namespace ZMap

