/*  File: ZGMessageReplyDataID.h
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

#ifndef ZGMESSAGEREPLYDATAID_H
#define ZGMESSAGEREPLYDATAID_H

#include "ZGMessage.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZInternalID.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

// returns a single ID as the result of a query;
class ZGMessageReplyDataID : public ZGMessage,
        public Util::InstanceCounter<ZGMessageReplyDataID>,
        public Util::ClassName<ZGMessageReplyDataID>
{
public:

    ZGMessageReplyDataID();
    ZGMessageReplyDataID(ZInternalID message_id,
                         ZInternalID reply_to_id,
                         ZInternalID data) ;
    ZGMessageReplyDataID(const ZGMessageReplyDataID& message) ;
    ZGMessageReplyDataID& operator=(const ZGMessageReplyDataID& message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageReplyDataID() ;

    ZInternalID getReplyToID() const {return m_reply_to_id ; }
    ZInternalID getData() const {return m_data ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_reply_to_id, m_data ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGEREPLYDATAID_H
