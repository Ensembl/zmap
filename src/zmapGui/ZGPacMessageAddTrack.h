/*  File: ZGPacMessageAddTrack.h
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

#ifndef ZGPACMESSAGEADDTRACK_H
#define ZGPACMESSAGEADDTRACK_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include <string>

class ZGPacMessageAddTrack : public ZGPacMessage
{
public:
    static unsigned int instances() {return m_instances ; }
    static std::string className() {return m_name ; }

    ZGPacMessageAddTrack();
    ZGPacMessageAddTrack(ZInternalID message_id,
                         ZInternalID sequence_id,
                         ZInternalID track_id) ;
    ZGPacMessageAddTrack(const ZGPacMessageAddTrack& message) ;
    ZGPacMessageAddTrack& operator=(const ZGPacMessageAddTrack& message) ;
    ZGPacMessage* clone() const ;
    std::string name() const {return m_name ;}
    ~ZGPacMessageAddTrack() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getTrackID() const {return m_track_id ; }

private:
    static unsigned int m_instances ;
    static const std::string m_name ;
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id, m_track_id ;
};

#endif // ZGPACMESSAGEADDTRACK_H
