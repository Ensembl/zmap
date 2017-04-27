/*  File: ZGuiAbstraction.cpp
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


#include "ZGuiAbstraction.h"
#include "ZGuiControl.h"
#include "ZGMessageHeader.h"
#include "ZDebug.h"
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

// data classes
#include "ZGCache.h"

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiAbstraction>::m_name("ZGuiAbstraction") ;

ZGuiAbstraction::ZGuiAbstraction(ZGPacControl *control)
    : ZGPacComponent(control),
      Util::InstanceCounter<ZGuiAbstraction>(),
      Util::ClassName<ZGuiAbstraction>(),
      m_cache(),
      m_user_data()
{
}

ZGuiAbstraction::~ZGuiAbstraction()
{
}

ZGuiAbstraction* ZGuiAbstraction::newInstance(ZGPacControl *control )
{
    ZGuiAbstraction* abstraction = nullptr ;

    try
    {
        abstraction = new ZGuiAbstraction(control) ;
    }
    catch (...)
    {
        abstraction = nullptr ;
    }

    return abstraction ;
}


bool ZGuiAbstraction::contains(ZInternalID id) const
{
    return m_cache.contains(id) ;
}

std::shared_ptr<ZGMessage> ZGuiAbstraction::query(const std::shared_ptr<ZGMessage> & message)
{
    std::shared_ptr<ZGMessage> answer(std::make_shared<ZGMessageInvalid>()) ;
    if (!message)
        return answer ;

    ZGMessageType message_type = message->type() ;
    ZInternalID incoming_message_id = message->id() ;

    switch (message_type)
    {

    case ZGMessageType::QuerySequenceIDs:
    {
        std::set<ZInternalID> dataset = m_cache.getSequenceIDs() ;
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QueryFeaturesetIDs:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQueryFeaturesetIDs> specific_message
                = std::dynamic_pointer_cast<ZGMessageQueryFeaturesetIDs, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID() ;
            if (m_cache.contains(sequence_id))
            {
                dataset = m_cache[sequence_id].getFeaturesetIDs() ;
            }
        }
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QueryTrackIDs:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQueryTrackIDs> specific_message
                = std::dynamic_pointer_cast<ZGMessageQueryTrackIDs, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID() ;
            if (m_cache.contains(sequence_id))
            {
                dataset = m_cache[sequence_id].getTrackIDs() ;
            }
        }
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QueryTrackFromFeatureset:
    {
        std::shared_ptr<ZGMessageQueryTrackFromFeatureset> specific_message
                = std::dynamic_pointer_cast<ZGMessageQueryTrackFromFeatureset, ZGMessage>(message) ;
        ZInternalID track_id = 0 ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    featureset_id = specific_message->getFeaturesetID() ;
            ZGCache::iterator it = m_cache.find(sequence_id) ;
            if (it != m_cache.end())
            {
                if (it->containsFeatureset(featureset_id))
                {
                    track_id = it->getTrackID(featureset_id) ;
                }
            }
        }
        answer = std::make_shared<ZGMessageReplyDataID>(incoming_message_id, incoming_message_id, track_id) ;
        break ;
    }

    case ZGMessageType::QueryFeaturesetsFromTrack:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQueryFeaturesetsFromTrack> specific_message
                = std::dynamic_pointer_cast<ZGMessageQueryFeaturesetsFromTrack,ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    track_id = specific_message->getTrackID() ;
            ZGCache::iterator it = m_cache.find(sequence_id) ;
            if (it != m_cache.end())
            {
                if (it->containsTrack(track_id))
                {
                    dataset = it->getFeaturesetIDs(track_id) ;
                }
            }
        }
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QueryFeatureIDs:
    {
        std::set<ZInternalID> dataset ;
        std::shared_ptr<ZGMessageQueryFeatureIDs> specific_message
                = std::dynamic_pointer_cast<ZGMessageQueryFeatureIDs, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    featureset_id = specific_message->getFeaturesetID() ;
            ZGCache::iterator it = m_cache.find(sequence_id) ;
            if (it != m_cache.end())
            {
                if (it->containsFeatureset(featureset_id))
                {
                    dataset = it->featureset(featureset_id).getFeatureIDs() ;
                }
            }
        }
        answer = std::make_shared<ZGMessageReplyDataIDSet>(incoming_message_id, incoming_message_id, dataset) ;
        break ;
    }

    case ZGMessageType::QueryUserHome:
    {
        std::string user_home ;
        std::shared_ptr<ZGMessageQueryUserHome> specific_message
              = std::dynamic_pointer_cast<ZGMessageQueryUserHome, ZGMessage>(message) ;
        if (specific_message)
        {
            user_home = m_user_data.getUserHome() ;
        }
        answer = std::make_shared<ZGMessageReplyString>(incoming_message_id, incoming_message_id, user_home) ;
        break ;
    }

    default:
        break ;

    }

    return answer ;
}

void ZGuiAbstraction::receive(const std::shared_ptr<ZGMessage>& message)
{
    if (!message)
        return ;
    ZGMessageType message_type = message->type() ;

    switch (message_type)
    {

    case ZGMessageType::SequenceOperation:
    {
        std::shared_ptr<ZGMessageSequenceOperation> specific_message =
                std::dynamic_pointer_cast<ZGMessageSequenceOperation, ZGMessage>(message) ;
        if (specific_message)
        {
            processSequenceOperation(specific_message) ;
        }
        break ;
    }

    case ZGMessageType::TrackOperation:
    {
        std::shared_ptr<ZGMessageTrackOperation> specific_message =
                std::dynamic_pointer_cast<ZGMessageTrackOperation, ZGMessage>(message) ;
        if (specific_message)
        {
            processTrackOperation(specific_message) ;
        }
        break ;
    }

    case ZGMessageType::FeaturesetOperation:
    {
        std::shared_ptr<ZGMessageFeaturesetOperation> specific_message =
                std::dynamic_pointer_cast<ZGMessageFeaturesetOperation, ZGMessage>(message) ;
        if (specific_message)
        {
            processFeaturesetOperation(specific_message) ;
        }
        break ;
    }

    case ZGMessageType::AddFeatures:
    {
        std::shared_ptr<ZGMessageAddFeatures> specific_message =
                std::dynamic_pointer_cast<ZGMessageAddFeatures, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID() ;
            std::shared_ptr<ZGFeatureset> featureset = specific_message->getFeatureset() ;
            ZInternalID featureset_id = featureset->getID() ;
            ZGCache::iterator it = m_cache.find(sequence_id) ;
            if (it != m_cache.end() && it->containsFeatureset(featureset_id) && featureset && featureset->size() && m_control)
            {
                it->addFeatureset(*featureset.get()) ;
            }
        }
        break ;
    }

    case ZGMessageType::DeleteFeatures:
    {
        std::shared_ptr<ZGMessageDeleteFeatures> specific_message =
                std::dynamic_pointer_cast<ZGMessageDeleteFeatures, ZGMessage>(message) ;
        if (specific_message)
        {
            ZInternalID sequence_id = specific_message->getSequenceID(),
                    featureset_id = specific_message->getFeaturesetID() ;
            std::set<ZInternalID> dataset = specific_message->getFeatures() ;
            ZGCache::iterator it = m_cache.find(sequence_id) ;
            if (it != m_cache.end() && it->containsFeatureset(featureset_id) && dataset.size() && m_control)
            {
                it->removeFeatures(featureset_id, dataset) ;
            }
        }
        break ;
    }

    case ZGMessageType::FeaturesetToTrack:
    {
        std::shared_ptr<ZGMessageFeaturesetToTrack> specific_message
                = std::dynamic_pointer_cast<ZGMessageFeaturesetToTrack, ZGMessage>(message) ;
        if (specific_message)
        {
            processFeaturesetToTrack(specific_message) ;
        }
        break ;
    }

    case ZGMessageType::SetUserHome:
    {
        std::shared_ptr<ZGMessageSetUserHome> specific_message
            = std::dynamic_pointer_cast<ZGMessageSetUserHome, ZGMessage>(message) ;
        if (specific_message)
        {
            std::string user_home = specific_message->getUserHome() ;
            m_user_data.setUserHome(user_home) ;
#ifndef QT_NO_DEBUG
            qDebug() << "ZGuiAbstraction::receive() ; set user home to " << QString::fromStdString(m_user_data.getUserHome())  ;
#endif
        }
        break ;
    }

    case ZGMessageType::ReplyStatus:
        break ;

    default:
        break ;

    }
}


void ZGuiAbstraction::processSequenceOperation(const std::shared_ptr<ZGMessageSequenceOperation>& message)
{
    if (!message)
        return ;

    ZGMessageSequenceOperation::Type type = message->getOperationType() ;
    ZInternalID sequence_id = message->getSequenceID() ;
    switch (type)
    {
    case ZGMessageSequenceOperation::Type::Create:
    {
        if (!m_cache.contains(sequence_id) && m_cache.add(sequence_id) && m_control)
        {

        }
        break ;
    }
    case ZGMessageSequenceOperation::Type::Delete:
    {
        if (m_cache.contains(sequence_id) && m_cache.remove(sequence_id) && m_control)
        {

        }
        break ;
    }
    default:
        break ;
    }

#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiAbstraction::processSequenceOperation() ; m_cache.size() " << m_cache.size() ;
#endif

}

void ZGuiAbstraction::processFeaturesetOperation(const std::shared_ptr<ZGMessageFeaturesetOperation>& message)
{
    if (!message)
        return ;

    ZGMessageFeaturesetOperation::Type type = message->getOperationType() ;
    ZInternalID sequence_id = message->getSequenceID(),
            featureset_id = message->getFeaturesetID() ;

    switch (type)
    {

    case ZGMessageFeaturesetOperation::Type::Create:
    {
        ZGCache::iterator it = m_cache.find(sequence_id) ;
        if (it != m_cache.end() && !it->containsFeatureset(featureset_id) && it->addFeatureset(featureset_id) && m_control)
        {

        }
        break ;
    }

    case ZGMessageFeaturesetOperation::Type::Delete:
    {
        ZGCache::iterator it = m_cache.find(sequence_id) ;
        if (it != m_cache.end() && it->containsFeatureset(featureset_id) && it->removeFeatureset(featureset_id) && m_control)
        {

        }
        break ;
    }

    default:
        break ;

    }
}

void ZGuiAbstraction::processTrackOperation(const std::shared_ptr<ZGMessageTrackOperation>& message)
{
    if (!message)
        return ;

    ZGMessageTrackOperation::Type type = message->getOperationType() ;
    ZInternalID sequence_id = message->getSequenceID(),
            track_id = message->getTrackID() ;

    switch (type)
    {

    case ZGMessageTrackOperation::Type::Create:
    {
        ZGCache::iterator it = m_cache.find(sequence_id) ;
        if (it != m_cache.end() && !it->containsTrack(track_id) && it->addTrack(track_id) && m_control)
        {

        }
        break ;
    }

    case ZGMessageTrackOperation::Type::Delete:
    {
        ZGCache::iterator it = m_cache.find(sequence_id) ;
        if (it != m_cache.end() && it->containsTrack(track_id) && it->removeTrack(track_id) && m_control)
        {

        }
        break ;
    }

    // maybe do nothing in the abstraction for these two, presentation only; maybe have a
    // shown/hidden flag in the object stored here...
    case ZGMessageTrackOperation::Type::Show:
    {
        break ;
    }
    case ZGMessageTrackOperation::Type::Hide:
    {
        break ;
    }

    default:
        break ;
    }
}


void ZGuiAbstraction::processFeaturesetToTrack(const std::shared_ptr<ZGMessageFeaturesetToTrack>& message)
{
    if (!message)
        return ;

    ZGMessageFeaturesetToTrack::Type type = message->getOperationType() ;
    ZInternalID sequence_id = message->getSequenceID(),
            featureset_id = message->getFeaturesetID(),
            track_id = message->getTrackID() ;

    switch (type)
    {

    case ZGMessageFeaturesetToTrack::Type::Add:
    {
        ZGCache::iterator it = m_cache.find(sequence_id) ;
        if (it != m_cache.end())
        {
            it->addFeaturesetToTrack(track_id, featureset_id) ;
        }
        break ;
    }

    case ZGMessageFeaturesetToTrack::Type::Remove:
    {
        ZGCache::iterator it = m_cache.find(sequence_id) ;
        if (it != m_cache.end())
        {
            it->removeFeaturesetFromTrack(track_id, featureset_id) ;
        }
        break ;
    }

    default:
        break ;
    }
}

} // namespace GUI

} // namespace ZMap
