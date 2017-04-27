/*  File: ZGSequence.cpp
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

#include "ZGSequence.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGSequence>::m_name("ZGSequence") ;

ZGSequence::ZGSequence()
    : Util::InstanceCounter<ZGSequence>(),
      Util::ClassName<ZGSequence>(),
      m_id(0),
      m_track_collection(),
      m_featureset_collection(),
      m_featureset_track_mapping()

{
}

ZGSequence::ZGSequence(ZInternalID id)
    : Util::InstanceCounter<ZGSequence>(),
      Util::ClassName<ZGSequence>(),
      m_id(id),
      m_track_collection(),
      m_featureset_collection(),
      m_featureset_track_mapping()
{
    if (!m_id)
        throw std::runtime_error(std::string("ZGSequence::ZGSequence(); id may not be 0 ")) ;

}

ZGSequence::ZGSequence(const ZGSequence& sequence)
    : Util::InstanceCounter<ZGSequence>(),
      Util::ClassName<ZGSequence>(),
      m_id(sequence.m_id),
      m_track_collection(sequence.m_track_collection),
      m_featureset_collection(sequence.m_featureset_collection),
      m_featureset_track_mapping(sequence.m_featureset_track_mapping)
{
}

ZGSequence& ZGSequence::operator=(const ZGSequence &sequence)
{
    if (this != &sequence)
    {
        m_id = sequence.m_id ;
        m_track_collection = sequence.m_track_collection ;
        m_featureset_collection = sequence.m_featureset_collection ;
        m_featureset_track_mapping = sequence.m_featureset_track_mapping ;
    }
    return *this ;
}

ZGSequence::~ZGSequence()
{
}

bool ZGSequence::containsTrack(ZInternalID id) const
{
    return m_track_collection.contains(id) ;
}

bool ZGSequence::addTrack(const ZGTrack& track)
{
    return m_track_collection.add(track) ;
}

bool ZGSequence::removeTrack(ZInternalID track_id)
{
    m_featureset_track_mapping.removeTrack(track_id) ;
    return m_track_collection.remove(track_id) ;
}

bool ZGSequence::containsFeatureset(ZInternalID id) const
{
    return m_featureset_collection.contains(id) ;
}


bool ZGSequence::addFeatureset(const ZGFeatureset &featureset)
{
    return m_featureset_collection.add(featureset) ;
}

bool ZGSequence::removeFeatureset(ZInternalID featureset_id)
{
    m_featureset_track_mapping.removeFeatureset(featureset_id) ;
    return m_featureset_collection.remove(featureset_id) ;
}

bool ZGSequence::removeFeatures(ZInternalID featureset_id, const std::set<ZInternalID> & dataset)
{
    if (!containsFeatureset(featureset_id) || !dataset.size())
        return false ;
    return m_featureset_collection[featureset_id].removeFeatures(dataset) ;
}

// return all of the track ids in the container
std::set<ZInternalID> ZGSequence::getTrackIDs() const
{
    return m_track_collection.getIDs() ;
}

// return all of the featureset ids in the container
std::set<ZInternalID> ZGSequence::getFeaturesetIDs() const
{
    return m_featureset_collection.getIDs() ;
}

// return all of the featureset ids associated with given track id
std::set<ZInternalID> ZGSequence::getFeaturesetIDs(ZInternalID track_id) const
{
    return m_featureset_track_mapping.getFeaturesets(track_id) ;
}

// return the track id associated with the given featureset id
ZInternalID ZGSequence::getTrackID(ZInternalID featureset_id) const
{
    return m_featureset_track_mapping.getTrack(featureset_id) ;
}

//
// add a mapping between a featureset and a track
//
bool ZGSequence::addFeaturesetToTrack(ZInternalID track_id, ZInternalID featureset_id)
{
    if (!containsFeatureset(featureset_id) || !containsTrack(track_id))
        return false ;
    return m_featureset_track_mapping.addFeaturesetToTrack(track_id, featureset_id ) ;
}


//
// remove the mapping between a featureset and a track
//
bool ZGSequence::removeFeaturesetFromTrack(ZInternalID track_id, ZInternalID featureset_id)
{
    if (!containsFeatureset(featureset_id) || !containsTrack(track_id))
        return false ;

    return m_featureset_track_mapping.removeFeaturesetFromTrack(track_id, featureset_id) ;
}

size_t ZGSequence::nTrack() const
{
    return m_track_collection.size() ;
}

size_t ZGSequence::nFeatureset() const
{
    return m_featureset_collection.size() ;
}


const ZGFeatureset& ZGSequence::featureset(ZInternalID id) const
{
    ZGFeaturesetCollection::const_iterator
            it = m_featureset_collection.find(id) ;
    if (it == m_featureset_collection.end())
        throw std::runtime_error(std::string("ZGSequence::featureset(); featureset is not present in container ")) ;
    return *it ;
}

const ZGTrack& ZGSequence::track(ZInternalID id) const
{
    ZGTrackCollection::const_iterator
            it = m_track_collection.find(id) ;
    if (it == m_track_collection.end())
        throw std::runtime_error(std::string("ZGSequence::track() ; track is not present in container ")) ;
    return *it ;
}

const ZGTrackCollection& ZGSequence::trackCollection() const
{
    return m_track_collection ;
}
ZGTrackCollection& ZGSequence::trackCollection()
{
    return m_track_collection ;
}

const ZGFeaturesetCollection& ZGSequence::featuresetCollection() const
{
    return m_featureset_collection ;
}
ZGFeaturesetCollection& ZGSequence::featuresetCollection()
{
    return m_featureset_collection ;
}
ZGFeaturesetTrackMapping ZGSequence::featuresetTrackMapping() const
{
    return m_featureset_track_mapping ;
}

std::ostream& operator<<(std::ostream& str, const ZGSequence& sequence)
{
    str << "ID: " << sequence.m_id ;
    if (sequence.m_featureset_collection.size())
    {
        str << "FC: " << sequence.m_featureset_collection  ;
    }
    if (sequence.m_track_collection.size())
    {
        str << "TC: " << sequence.m_track_collection ;
    }
    if (!sequence.m_featureset_track_mapping.isEmpty())
    {
        str << sequence << sequence.m_featureset_track_mapping ;
    }
    return str ;
}

} // namespace GUI

} // namespace ZMap
