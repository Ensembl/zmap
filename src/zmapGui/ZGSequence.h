/*  File: ZGSequence.h
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

#ifndef ZGSEQUENCE_H
#define ZGSEQUENCE_H

#include "ZInternalID.h"
#include "ZGTrackCollection.h"
#include "ZGFeaturesetCollection.h"
#include "ZGTrack.h"
#include "ZGFeatureset.h"
#include "ZGFeaturesetTrackMapping.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <ostream>


namespace ZMap
{

namespace GUI
{


// GUI cache class; these are the classes used to store cache data in
// the abstraction

class ZGSequence: public Util::InstanceCounter<ZGSequence>,
        public Util::ClassName<ZGSequence>
{
public:

    friend std::ostream& operator<<(std::ostream& str, const ZGSequence& sequence) ;

    ZGSequence();
    ZGSequence(ZInternalID id) ;
    ZGSequence(const ZGSequence& sequence) ;
    ZGSequence& operator=(const ZGSequence& sequence) ;
    ~ZGSequence() ;

    ZInternalID getID() const {return m_id ; }

    bool containsTrack(ZInternalID id) const ;
    bool containsFeatureset(ZInternalID id) const ;

    bool addTrack(const ZGTrack& track) ;
    bool addFeatureset(const ZGFeatureset &featureset) ;

    const ZGTrack& track(ZInternalID id) const ;
    const ZGFeatureset& featureset(ZInternalID id) const ;

    const ZGTrackCollection& trackCollection() const ;
    ZGTrackCollection& trackCollection() ;
    const ZGFeaturesetCollection& featuresetCollection() const ;
    ZGFeaturesetCollection& featuresetCollection() ;
    ZGFeaturesetTrackMapping featuresetTrackMapping() const ;

    bool removeTrack(ZInternalID id) ;
    bool removeFeatureset(ZInternalID id) ;
    bool removeFeatures(ZInternalID featureset_id, const std::set<ZInternalID> & dataset) ;

    bool addFeaturesetToTrack(ZInternalID track_id, ZInternalID featureset_id) ;
    bool removeFeaturesetFromTrack(ZInternalID track_id, ZInternalID featureset_id) ;

    size_t nTrack() const ;
    size_t nFeatureset() const ;

    std::set<ZInternalID> getTrackIDs() const ;
    std::set<ZInternalID> getFeaturesetIDs() const ;
    std::set<ZInternalID> getFeaturesetIDs(ZInternalID track_id) const ;
    ZInternalID getTrackID(ZInternalID featureset_id) const ;

private:

    ZInternalID m_id ;

    ZGTrackCollection m_track_collection ;
    ZGFeaturesetCollection m_featureset_collection ;
    ZGFeaturesetTrackMapping m_featureset_track_mapping ;
};

std::ostream& operator<<(std::ostream& str, const ZGSequence& sequence) ;

} // namespace GUI

} // namespace ZMap

#endif // ZGSEQUENCE_H
