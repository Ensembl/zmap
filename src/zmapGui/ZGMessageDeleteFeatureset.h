/*  File: ZGMessageDeleteFeatureset.h
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

#ifndef ZGMESSAGEDELETEFEATURESET_H
#define ZGMESSAGEDELETEFEATURESET_H

// Delete a featureset from the specified sequence in the cache. Track id does not
// need to be given for this.

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

class ZGMessageDeleteFeatureset : public ZGMessage,
        public Util::InstanceCounter<ZGMessageDeleteFeatureset>,
        public Util::ClassName<ZGMessageDeleteFeatureset>
{
public:

    ZGMessageDeleteFeatureset();
    ZGMessageDeleteFeatureset(ZInternalID message_id,
                              ZInternalID sequence_id,
                              ZInternalID featureset_id) ;
    ZGMessageDeleteFeatureset(const ZGMessageDeleteFeatureset& message) ;
    ZGMessageDeleteFeatureset& operator=(const ZGMessageDeleteFeatureset &message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageDeleteFeatureset() ;

    ZInternalID getFeaturesetID() const {return m_featureset_id ; }
    ZInternalID getSequenceID() const {return m_sequence_id ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID  m_sequence_id, m_featureset_id ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGEDELETEFEATURESET_H
