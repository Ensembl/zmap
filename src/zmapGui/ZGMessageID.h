/*  File: ZGMessageID.h
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

#ifndef ZGMESSAGEID_H
#define ZGMESSAGEID_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

// generates a message ID for use with external messages

class ZGMessageID: public Util::InstanceCounter<ZGMessageID>,
        public Util::ClassName<ZGMessageID>
{
public:

    ZGMessageID(ZInternalID state = 1) ;
    ZGMessageID(const ZGMessageID &message_id) ;
    ZGMessageID& operator=(const ZGMessageID &message_id) ;
    ~ZGMessageID() ;

    ZInternalID new_id() ;

private:

    ZInternalID m_state ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGEID_H
