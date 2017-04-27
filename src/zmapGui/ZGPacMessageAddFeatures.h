/*  File: ZGPacMessageAddFeatures.h
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

#ifndef ZGPACMESSAGEADDFEATURES_H
#define ZGPACMESSAGEADDFEATURES_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <memory>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

class ZGFeatureset ;

class ZGPacMessageAddFeatures : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageAddFeatures>,
        public Util::ClassName<ZGPacMessageAddFeatures>
{
public:

    ZGPacMessageAddFeatures();
    ZGPacMessageAddFeatures(ZInternalID message_id,
                            ZInternalID sequence_id,
                            const std::shared_ptr<ZGFeatureset> & featureset_data) ;
    ZGPacMessageAddFeatures(const ZGPacMessageAddFeatures& message) ;
    ZGPacMessageAddFeatures& operator=(const ZGPacMessageAddFeatures& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageAddFeatures() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    std::shared_ptr<ZGFeatureset> getFeatureset() const {return m_featureset_data ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id ;
    std::shared_ptr<ZGFeatureset> m_featureset_data ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGPACMESSAGEADDFEATURES_H
