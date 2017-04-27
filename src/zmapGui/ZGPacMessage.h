/*  File: ZGPacMessage.h
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

#ifndef ZGPACMESSAGE_H
#define ZGPACMESSAGE_H

#include <cstddef>
#include <string>
#include "ZGPacMessageType.h"
#include "ZInternalID.h"

namespace ZMap
{

namespace GUI
{

// Base class internal PAC communications only
class ZGPacMessage
{
public:
    ZGPacMessage() ;
    ZGPacMessage(ZGPacMessageType type) ;
    ZGPacMessage(ZGPacMessageType type, ZInternalID id) ;
    ZGPacMessage(const ZGPacMessage &message) ;
    ZGPacMessage& operator=(const ZGPacMessage &message) ;
    virtual ZGPacMessage* clone() const = 0 ;
    virtual ~ZGPacMessage() {}

    ZGPacMessageType type() const {return m_type ;}
    ZInternalID id() const {return m_id ; }

protected:
    ZGPacMessageType m_type ;
    ZInternalID m_id ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGPACMESSAGE_H
