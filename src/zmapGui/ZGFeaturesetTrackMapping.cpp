/*  File: ZGFeaturesetTrackMapping.cpp
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

#include "ZGFeaturesetTrackMapping.h"


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGFeaturesetTrackMapping>::m_name("ZGFeaturesetTrackMapping") ;

ZGFeaturesetTrackMapping::ZGFeaturesetTrackMapping()
    : Util::InstanceCounter<ZGFeaturesetTrackMapping>(),
      Util::ClassName<ZGFeaturesetTrackMapping>(),
      m_track_to_featureset(),
      m_featureset_to_track()
{
}

ZGFeaturesetTrackMapping::ZGFeaturesetTrackMapping(const ZGFeaturesetTrackMapping &mapping)
    : Util::InstanceCounter<ZGFeaturesetTrackMapping>(),
      Util::ClassName<ZGFeaturesetTrackMapping>(),
      m_track_to_featureset(mapping.m_track_to_featureset),
      m_featureset_to_track(mapping.m_featureset_to_track)
{
}

ZGFeaturesetTrackMapping& ZGFeaturesetTrackMapping::operator=(const ZGFeaturesetTrackMapping& mapping)
{
    if (this != &mapping)
    {
        m_track_to_featureset = mapping.m_track_to_featureset ;
        m_featureset_to_track = mapping.m_featureset_to_track ;
    }
    return *this ;
}

ZGFeaturesetTrackMapping::~ZGFeaturesetTrackMapping()
{
}

//
// both of the internal containers must have the specified pair for it to be considered
// valid
//
bool ZGFeaturesetTrackMapping::isPresent(ZInternalID track_id, ZInternalID featureset_id) const
{
    bool check1 = false, check2 = false ;
    if (!track_id || !featureset_id)
        return false ;

    // first check m_track_to_featureset
    auto pit = m_track_to_featureset.equal_range(track_id) ;
    if (pit.first != m_track_to_featureset.end())
    {
        for (auto it=pit.first ; it!=pit.second ; ++it)
        {
            if ((check1 = (it->second == featureset_id)))
                break ;
        }
    }

    // then check m_featureset_to_track
    if (check1)
    {
        auto it = m_featureset_to_track.find(featureset_id) ;
        check2 = (it != m_featureset_to_track.end()) && (it->second == track_id) ;
    }

    // both of these must contain the pair that was specified
    return check1 && check2 ;
}

//
// do we contain the featureset_id
//
bool ZGFeaturesetTrackMapping::featuresetPresent(ZInternalID featureset_id ) const
{
    return (m_featureset_to_track.find(featureset_id) != m_featureset_to_track.end()) ;
}

//
// do we contain the track_id
//
bool ZGFeaturesetTrackMapping::trackPresent(ZInternalID track_id ) const
{
    return (m_track_to_featureset.equal_range(track_id).first != m_track_to_featureset.end()) ;
}

//
// add one particular entry; note that a featureset may only be present in one track, so cannot
// be entered twice
//
bool ZGFeaturesetTrackMapping::addFeaturesetToTrack(ZInternalID track_id, ZInternalID featureset_id)
{
    bool result = false ;
    if (featuresetPresent(featureset_id) || isPresent(track_id, featureset_id))
        return result ;
    m_track_to_featureset.insert(std::pair<ZInternalID,ZInternalID>(track_id, featureset_id)) ;
    m_featureset_to_track.insert(std::pair<ZInternalID,ZInternalID>(featureset_id, track_id)) ;
    result = true ;
    return result ;
}

//
// remove one particular entry
//
bool ZGFeaturesetTrackMapping::removeFeaturesetFromTrack(ZInternalID track_id, ZInternalID featureset_id)
{
    bool result = false ;
    if (!isPresent(track_id, featureset_id))
        return result ;

    // first container
    auto pit = m_track_to_featureset.equal_range(track_id) ;
    auto sit = pit.first ;
    for ( ; sit!=pit.second ; ++sit)
        if (sit->second == featureset_id)
        {
            break ;
        }
    if ( sit != pit.second ) // i.e. object found
        m_track_to_featureset.erase(sit) ;

    // second container
    auto it = m_featureset_to_track.find(featureset_id) ;
    m_featureset_to_track.erase(it) ;

    result = true ;

    return result ;
}

// remove all entries associated with this id
bool ZGFeaturesetTrackMapping::removeFeatureset(ZInternalID featureset_id)
{
    bool result = false ;
    if (!featuresetPresent(featureset_id))
        return result ;

    //
    auto sit = m_featureset_to_track.find(featureset_id) ;
    ZInternalID track_id = 0 ;

    if (sit != m_featureset_to_track.end())
    {
        track_id = sit->second ;
        m_featureset_to_track.erase(sit) ;
    }

    //
    auto pit = m_track_to_featureset.equal_range(track_id) ;
    if (pit.first != m_track_to_featureset.end())
    {
        auto it = pit.first ;
        for ( ; it!=pit.second ; ++it)
            if (it->second == featureset_id)
                break ;
        if (it != pit.second)
            m_track_to_featureset.erase(it) ;
    }

    result = true ;
    return result ;
}

// remove all entries associated with this id
bool ZGFeaturesetTrackMapping::removeTrack(ZInternalID track_id)
{
    bool result = false ;
    if (!trackPresent(track_id))
        return result ;

    std::set<ZInternalID> featureset_ids ;
    auto pit = m_track_to_featureset.equal_range(track_id) ;
    auto it = pit.first ;
    for ( ; it!=pit.second ; ++it)
        featureset_ids.insert(it->second) ;
    m_track_to_featureset.erase(pit.first, pit.second) ;
    auto sit = featureset_ids.begin() ;
    for ( ; sit!=featureset_ids.end() ; ++sit)
    {
        auto mit = m_featureset_to_track.find(*sit) ;
        m_featureset_to_track.erase(mit) ;
    }
    result = true ;
    return result ;
}


// clear eveything in the containers
bool ZGFeaturesetTrackMapping::removeAll()
{
    m_featureset_to_track.clear() ;
    m_track_to_featureset.clear() ;

    return true ;
}

bool ZGFeaturesetTrackMapping::isEmpty() const
{
    return !static_cast<bool>(m_featureset_to_track.size()) ;
}

// fetch all featuresets associated with the given track
std::set<ZInternalID> ZGFeaturesetTrackMapping::getFeaturesets(ZInternalID track_id) const
{
    std::set<ZInternalID> result ;
    auto pit = m_track_to_featureset.equal_range(track_id) ;
    for (auto it=pit.first; it!=pit.second; ++it)
        result.insert(it->second) ;
    return result ;
}


// fetch the track associated with the given featureset
ZInternalID ZGFeaturesetTrackMapping::getTrack(ZInternalID featureset_id) const
{
    ZInternalID result = 0 ;
    auto it = m_featureset_to_track.find(featureset_id) ;
    if (it != m_featureset_to_track.end())
        result = it->second ;
    return result ;
}

std::ostream& operator<<(std::ostream& str, const ZGFeaturesetTrackMapping& mapping)
{
    if (mapping.m_featureset_to_track.size())
    {
        str << "F2T: " ;
        auto it = mapping.m_featureset_to_track.begin() ;
        for ( ; it!=mapping.m_featureset_to_track.end() ; ++it)
            str << "(" << it->first << ", " << it->second << ") " ;
    }
    if (mapping.m_track_to_featureset.size())
    {
        str << "T2F: " ;
        auto it = mapping.m_track_to_featureset.begin() ;
        for ( ; it!=mapping.m_track_to_featureset.end() ; ++it)
            str << "(" << it->first << ", " << it->second << ") " ;
    }
    return str ;
}

} // namespace GUI

} // namespace ZMap
