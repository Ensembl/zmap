/*  File: ZGFeaturesetTrackMapping.h
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

#ifndef ZGFEATURESETTRACKMAPPING_H
#define ZGFEATURESETTRACKMAPPING_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <map>
#include <set>
#include <ostream>
#include <cstddef>

namespace ZMap
{

namespace GUI
{


class ZGFeaturesetTrackMapping: Util::InstanceCounter<ZGFeaturesetTrackMapping>,
        Util::ClassName<ZGFeaturesetTrackMapping>
{
public:
    friend std::ostream& operator<<(std::ostream& str, const ZGFeaturesetTrackMapping& mapping) ;

    ZGFeaturesetTrackMapping();
    ZGFeaturesetTrackMapping(const ZGFeaturesetTrackMapping & mapping) ;
    ZGFeaturesetTrackMapping& operator=(const ZGFeaturesetTrackMapping& mapping) ;
    ~ZGFeaturesetTrackMapping() ;
    bool isPresent(ZInternalID track_id, ZInternalID featureset_id) const ;
    bool featuresetPresent(ZInternalID featureset_id ) const ;
    bool trackPresent(ZInternalID track_id ) const ;

    std::set<ZInternalID> getFeaturesets(ZInternalID track_id) const ;
    std::set<ZInternalID> getTracks() const ;
    ZInternalID getTrack(ZInternalID featureset_id) const ;

    bool addFeaturesetToTrack(ZInternalID track_id, ZInternalID featureset_id) ;
    bool removeFeaturesetFromTrack(ZInternalID track_id, ZInternalID featureset_id) ;

    bool removeFeatureset(ZInternalID featureset_id) ;
    bool removeTrack(ZInternalID track_id) ;

    bool removeAll() ;
    bool isEmpty() const ;

private:
    std::multimap<ZInternalID, ZInternalID> m_track_to_featureset ;
    std::map<ZInternalID, ZInternalID> m_featureset_to_track ;
};

std::ostream& operator<<(std::ostream& str, const ZGFeaturesetTrackMapping& mapping) ;

} // namespace GUI

} // namespace ZMap


#endif // ZGFEATURESETTRACKMAPPING_H
