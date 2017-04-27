/*  File: ZGMessageTrackOperation.h
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

#ifndef ZGMESSAGETRACKOPERATION_H
#define ZGMESSAGETRACKOPERATION_H

#include "ZGMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

// Create or delete a track in a sequence.

class ZGMessageTrackOperation : public ZGMessage,
        public Util::InstanceCounter<ZGMessageTrackOperation>,
        public Util::ClassName<ZGMessageTrackOperation>
{
public:

    enum class Type: unsigned char
    {
        Invalid,
        Create,
        Delete,
        Show,
        Hide
    } ;

    ZGMessageTrackOperation();
    ZGMessageTrackOperation(ZInternalID message_id,
                      ZInternalID sequence_id,
                      ZInternalID track_id,
                            bool sensitive,
                            Type operation_type ) ;
    ZGMessageTrackOperation(const ZGMessageTrackOperation& message) ;
    ZGMessageTrackOperation& operator=(const ZGMessageTrackOperation& message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageTrackOperation() ;

    ZInternalID getTrackID() const {return m_track_id ; }
    ZInternalID getSequenceID() const {return m_sequence_id ; }
    bool getStrandSensitive() const {return m_strand_sensitive ; }
    Type getOperationType() const {return m_operation_type ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id, m_track_id ;
    bool m_strand_sensitive ;
    Type m_operation_type ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGETRACKOPERATION_H
