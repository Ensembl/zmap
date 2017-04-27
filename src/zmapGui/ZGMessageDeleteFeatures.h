/*  File: ZGMessageDeleteFeatures.h
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

#ifndef ZGMESSAGEDELETEFEATURES_H
#define ZGMESSAGEDELETEFEATURES_H

// Delete features from a sequence and featureset.
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGMessage.h"
#include <string>
#include <set>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

class ZGMessageDeleteFeatures : public ZGMessage,
        public Util::InstanceCounter<ZGMessageDeleteFeatures>,
        public Util::ClassName<ZGMessageDeleteFeatures>
{
public:

    ZGMessageDeleteFeatures();
    ZGMessageDeleteFeatures(ZInternalID message_id,
                            ZInternalID sequence_id,
                            ZInternalID featureset_id,
                            const std::set<ZInternalID> & features) ;
    ZGMessageDeleteFeatures(const ZGMessageDeleteFeatures &message) ;
    ZGMessageDeleteFeatures& operator=(const ZGMessageDeleteFeatures& message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageDeleteFeatures() ;

    ZInternalID getFeaturesetID() const {return m_featureset_id ; }
    ZInternalID getSequenceID() const {return m_sequence_id ; }
    const std::set<ZInternalID>& getFeatures() const {return m_features ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id,
                m_featureset_id ;
    std::set<ZInternalID> m_features ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGMESSAGEDELETEFEATURES_H
