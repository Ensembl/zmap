/*  File: ZGuiScene.cpp
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

#include <new>
#include "ZGuiScene.h"
#include "ZWTrackSimple.h"
#include "ZWTrackCollection.h"
#include "ZWTrackSeparator.h"
#include "ZFeature.h"
#include "ZGFeatureset.h"
#include "ZGConverter.h"
#include "ZGStyle.h"
#include "ZGStrandType.h"
#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
// #define TEST_ITEM 1
#ifdef TEST_ITEM
#include "ZItemTest.h"
#endif

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiScene>::m_name("ZGuiScene") ;
const QString ZGuiScene::m_name_base("ZGuiScene_") ;
const QColor ZGuiScene::m_color_background_default(255, 255, 255),
    ZGuiScene::m_color_strand_default(255, 255, 0) ;
const ZInternalID ZGuiScene::m_locus_id_value = 0 ;

QString ZGuiScene::getNameFromID(ZInternalID id)
{
    QString str ;
    return m_name_base + str.setNum(id) ;
}

ZGuiScene::ZGuiScene(ZInternalID id, bool locus_required)
    : QGraphicsScene(),
      Util::InstanceCounter<ZGuiScene>(),
      Util::ClassName<ZGuiScene>(),
      m_id(id),
      m_color_background(m_color_background_default),
      m_color_strand(m_color_strand_default),
      m_track_ordering(),
      m_track_map(),
      m_f2t_map(),
      m_featuresets(),
      m_locus_scene(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiScene::ZGuiScene() ; call to canCreate() failed.")) ;

    // background color
    setBackgroundColor(m_color_background) ;

    // these are just a couple of items to test some stuff
#ifdef TEST_ITEM
    QGraphicsItem *item_test = new (std::nothrow) ZItemTest ;
    addItem(item_test) ;
    item_test->setPos(-100.0, -100.0) ;

    item_test = new (std::nothrow) ZItemTest ;
    addItem(item_test) ;
    item_test->setPos(300.0, 300.0) ;
#endif

    //
    // OK so the meaning of this is that if the scene represents a sequence
    // (that is, locus_required = true) then we create a second scene to represent
    // the locus data. If the current scene itself represents the locus data, then
    // this should not be done...
    //
    if (locus_required)
    {
        m_locus_scene = newInstance(m_locus_id_value, false) ;
        if (m_locus_scene)
        {
            m_locus_scene->setParent(this) ;
        }
    }

    if (!connect(this, SIGNAL(selectionChanged()), this, SLOT(slot_selectionChanged())))
        throw std::runtime_error(std::string("ZGuiScene::ZGuiScene() ; could not connect selectionChanged() to appropriate receiver")) ;
}

ZGuiScene::~ZGuiScene()
{
    for (auto it = m_featuresets.begin() ; it != m_featuresets.end() ; ++it)
    {
        delete it->second ;
    }
}


// well the fun here is that this call does in fact schedule a redraw,
// so nothing extra is required...
void ZGuiScene::setBackgroundColor(const QColor& color)
{
    m_color_background = color ;
    setBackgroundBrush(m_color_background) ;
}

////////////////////////////////////////////////////////////////////////////////
/// Locus scene managment
////////////////////////////////////////////////////////////////////////////////

bool ZGuiScene::createLocusScene()
{
    bool result = false ;
    if (!m_locus_scene)
    {
        m_locus_scene = newInstance(m_locus_id_value, false) ;
        if (m_locus_scene)
        {
            m_locus_scene->setParent(this) ;
            result = true ;
        }
    }
    return result ;
}

bool ZGuiScene::deleteLocusScene()
{
    bool result = false ;
    if (m_locus_scene)
    {
        m_locus_scene->setParent(Q_NULLPTR) ;
        delete m_locus_scene ;
        result = true ;
    }
    return result ;
}


////////////////////////////////////////////////////////////////////////////////
/// Managment of the strand separator
////////////////////////////////////////////////////////////////////////////////

//
//
//
ZWTrackSeparator* ZGuiScene::findStrandSeparator() const
{
    ZWTrackSeparator* separator = Q_NULLPTR ;
    auto it = m_track_map.find(ZWTrackSeparator::getDefaultID()) ;
    if (it != m_track_map.end())
    {
        separator = dynamic_cast<ZWTrackSeparator*>(it->second) ;
    }
    return separator ;
}

// set the separator background color
void ZGuiScene::setSeparatorColor(const QColor& color)
{
    ZWTrackSeparator* separator = findStrandSeparator() ;
    if (separator)
    {
        separator->setColor(color) ;
    }
}

bool ZGuiScene::containsStrandSeparator() const
{
    return static_cast<bool> (findStrandSeparator()) ;
}

void ZGuiScene::addStrandSeparator()
{
    ZWTrackSeparator * separator = findStrandSeparator() ;
    if (!separator && (separator = ZWTrackSeparator::newInstance()))
    {
        addItem(separator) ;
        m_track_map.insert(std::pair<ZInternalID, ZWTrack*>(ZWTrackSeparator::getDefaultID(), separator)) ;
        connect(separator, SIGNAL(internalReorderChange()), this, SLOT(internalReorder())) ;
        internalReorder() ;
    }
}

void ZGuiScene::deleteStrandSeparator()
{
    ZWTrackSeparator * separator = findStrandSeparator() ;
    if (separator)
    {
        removeItem(separator) ;
        delete separator ;
        m_track_map.erase(ZWTrackSeparator::getDefaultID()) ;
        internalReorder() ;
    }
}

void ZGuiScene::showStrandSeparator()
{
    ZWTrackSeparator* separator = findStrandSeparator() ;
    if (separator)
    {
        separator->show() ;
    }
}

void ZGuiScene::hideStrandSeparator()
{
    ZWTrackSeparator * separator = findStrandSeparator() ;
    if (separator)
    {
        separator->hide() ;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// Featureset managment
////////////////////////////////////////////////////////////////////////////////

void ZGuiScene::addFeatureset(ZInternalID id)
{
    if (!id)
        return ;

    auto it = m_featuresets.find(id) ;
    if (it == m_featuresets.end())
    {
        ZGuiFeatureset *featureset = ZGuiFeatureset::newInstance(id) ;
        if (featureset)
        {
            m_featuresets.insert(std::pair<ZInternalID, ZGuiFeatureset*>(id, featureset)) ;
        }
    }
}

void ZGuiScene::deleteFeatureset(ZInternalID id)
{
    if (!id)
        return ;

    // remove from this mapping
    m_f2t_map.removeFeatureset(id) ;

    // delete contents
    deleteAllFeatures(id) ;

    // remove from this mapping as well
    auto it = m_featuresets.find(id) ;
    if (it != m_featuresets.end())
    {
        delete it->second ;
        m_featuresets.erase(it) ;
    }
}

std::set<ZInternalID> ZGuiScene::getFeaturesets() const
{
    std::set<ZInternalID> data ;
    for (auto it = m_featuresets.begin() ; it != m_featuresets.end() ; ++it )
        data.insert(it->first) ;
    return data ;
}

std::set<ZInternalID> ZGuiScene::getFeatures(ZInternalID featureset_id) const
{
    std::set<ZInternalID> dataset ;
    auto it = m_featuresets.find(featureset_id) ;
    if (it != m_featuresets.end())
    {
        ZGuiFeatureset *featureset = it->second ;
        if (featureset)
        {
            dataset = featureset->getFeatureIDs() ;
        }
    }
    return dataset ;
}

bool ZGuiScene::containsFeatureset(ZInternalID id) const
{
    return m_featuresets.find(id) != m_featuresets.end() ;
}

////////////////////////////////////////////////////////////////////////////////
/// Track management
////////////////////////////////////////////////////////////////////////////////

bool ZGuiScene::containsTrack(ZInternalID id) const
{
    return m_track_map.find(id) != m_track_map.end() ;
}

bool ZGuiScene::setTrackColor(ZInternalID id, const QColor &color)
{
    bool result = false ;
    ZWTrack* track = Q_NULLPTR ;
    if (!containsTrack(id))
        return result ;
    auto pit = m_track_map.equal_range(id) ;
    for (auto it = pit.first ; it != pit.second ; ++it)
    {
        track = it->second ;
        if (track)
        {
            track->setColor(color) ;
            result = true ;
        }
    }
    return result ;
}

void ZGuiScene::addTrack(ZInternalID id, bool sensitive)
{
    ZWTrackSimple *track = Q_NULLPTR ;
    if (containsTrack(id))
        return ;
    if (sensitive)
    {
        if ((track = ZWTrackSimple::newInstance(id)))
        {
            track->setStrand(ZWTrackSimple::Strand::Plus);
            m_track_map.insert(std::pair<ZInternalID, ZWTrack*>(id, track)) ;
            addItem(track) ;
            connect(track, SIGNAL(internalReorderChange()), this, SLOT(internalReorder())) ;
        }
        if ((track = ZWTrackSimple::newInstance(id)))
        {
            track->setStrand(ZWTrackSimple::Strand::Minus) ;
            m_track_map.insert(std::pair<ZInternalID, ZWTrack*>(id, track)) ;
            addItem(track) ;
            connect(track, SIGNAL(internalReorderChange()), this, SLOT(internalReorder())) ;
        }
    }
    else
    {
        if ((track = ZWTrackSimple::newInstance(id)))
        {
            track->setStrand(ZWTrackSimple::Strand::Independent) ;
            m_track_map.insert(std::pair<ZInternalID, ZWTrack*>(id, track)) ;
            addItem(track) ;
            connect(track, SIGNAL(internalReorderChange()), this, SLOT(internalReorder())) ;
        }
    }
    if (std::find(m_track_ordering.begin(), m_track_ordering.end(), id) == m_track_ordering.end())
    {
        m_track_ordering.push_back(id) ;
    }
    internalReorder() ;
}

void ZGuiScene::deleteTrack(ZInternalID id)
{
    if (!id)
        return ;

    ZWTrack* track = Q_NULLPTR ;
    if (!containsTrack(id))
        return ;
    auto pit = m_track_map.equal_range(id) ;
    for (auto it = pit.first ; it != pit.second ; ++it)
    {
        track = it->second ;
        if (track)
        {
            removeItem(track) ;
            delete track ;
        }
    }
    m_track_map.erase(id) ;
    internalReorder() ;
    m_f2t_map.removeTrack(id) ;
}

void ZGuiScene::showTrack(ZInternalID id)
{
    ZWTrack* track = Q_NULLPTR ;
    auto pit = m_track_map.equal_range(id) ;
    for (auto it = pit.first ; it != pit.second ; ++it )
    {
        track = it->second ;
        if (track)
        {
            track->show() ;
        }
    }
}

void ZGuiScene::hideTrack(ZInternalID id)
{
    ZWTrack* track = Q_NULLPTR ;
    auto pit = m_track_map.equal_range(id) ;
    for (auto it = pit.first ; it != pit.second ; ++it )
    {
        track = it->second ;
        if (track)
        {
            track->hide() ;
        }
    }
}

void ZGuiScene::setDefaultTrackOrdering()
{
    // do summat with m_track_ordering here...
    m_track_ordering.clear() ;

    for (auto it = m_track_map.begin() ; it != m_track_map.end() ; ++it )
    {
        ZWTrack* track = it->second ;
        if (track)
        {
            ZInternalID id = track->getID() ;
            if (std::find(m_track_ordering.begin(), m_track_ordering.end(), id ) == m_track_ordering.end())
            {
                m_track_ordering.push_back(id) ;
            }
        }
    }
    internalReorder() ;
}

bool ZGuiScene::setTrackOrdering(const std::vector<ZInternalID> &data)
{
    // should probably check here that the argument contains one and one
    // only entry for all of the track ids that are in the container...
    m_track_ordering = data ;
    internalReorder() ;
    return true ;
}

std::vector<ZInternalID> ZGuiScene::getTrackOrdering() const
{
    return m_track_ordering ;
}

std::set<ZInternalID> ZGuiScene::getTracks() const
{
    std::set<ZInternalID> data ;
    for (auto it = m_track_map.begin() ; it != m_track_map.end() ; ++it )
    {
        data.insert(it->first) ;
    }
    return data ;
}

////////////////////////////////////////////////////////////////////////////////
/// Featureset to track mapping management
////////////////////////////////////////////////////////////////////////////////

void ZGuiScene::addFeaturesetToTrack(ZInternalID track_id, ZInternalID featureset_id)
{
    if (!track_id || !featureset_id || !containsTrack(track_id) || !containsFeatureset(featureset_id))
        return ;
    m_f2t_map.addFeaturesetToTrack(track_id, featureset_id) ;
}

void ZGuiScene::removeFeaturesetFromTrack(ZInternalID track_id, ZInternalID featureset_id)
{
    if (!track_id || !featureset_id || !containsTrack(track_id) || !containsFeatureset(featureset_id))
        return ;
    m_f2t_map.removeFeaturesetFromTrack(track_id, featureset_id) ;
}


void ZGuiScene::setFeaturesetStyle(ZInternalID featureset_id, const std::shared_ptr<ZGStyle>& style)
{
    // convert in here from ZGStyle to ZFeaturePainter ... and set for this featureset
    if (!containsFeatureset(featureset_id) || !style)
        return ;

    // first of all, look up the ZGuiFeatureset from featureset_id
    ZGuiFeatureset *gui_featureset = nullptr ;
    auto fit = m_featuresets.find(featureset_id) ;
    if (fit == m_featuresets.end())
        return ;
    gui_featureset = fit->second ;
    if (!gui_featureset)
        return ;

    // perform conversion from ZGStyle to ZFeaturePainter ...
    ZFeaturePainter * feature_painter = nullptr ;
    feature_painter = ZGConverter::convertStyle(*style) ;

    // set that painter for the featureset
    if (feature_painter)
    {
        gui_featureset->setFeaturePainter(feature_painter) ;
    }

    // things such as colors or feature width might change as a consequence of
    // setting the feature painter, so we should
    internalReorder() ;
}

////////////////////////////////////////////////////////////////////////////////
/// Features managment
////////////////////////////////////////////////////////////////////////////////

void ZGuiScene::addFeatures(const std::shared_ptr<ZGFeatureset>& featureset)
{
    if (!featureset)
        return ;
    ZInternalID featureset_id = featureset->getID() ;
    if (!containsFeatureset(featureset_id) || !m_f2t_map.featuresetPresent(featureset_id))
        return ;

    ZGuiFeatureset * gui_featureset = nullptr ;
    auto fit = m_featuresets.find(featureset_id) ;
    if (fit == m_featuresets.end())
        return ;
    gui_featureset = fit->second ;
    if (!gui_featureset)
        return ;

    // lookup the tracks
    ZInternalID track_id = m_f2t_map.getTrack(featureset_id) ;
    auto pit = m_track_map.equal_range(track_id) ;
    if (pit.first == m_track_map.end())
        return ;
    std::ptrdiff_t track_number = std::distance(pit.first, pit.second) ;
    if (!track_number)
    {
        return ;
    }
    else if (track_number == 1)
    {
        // only one graphics object for this id, so this is interpreted as
        // meaning strand insensitive
        ZWTrack* track = pit.first->second ;
        if (track)
        {
            for (auto it = featureset->begin() ; it != featureset->end() ; ++it)
            {
                ZInternalID feature_id = it->getID() ;
                if (!gui_featureset->contains(feature_id))
                {
                    ZFeature* feature = ZGConverter::convertFeature(featureset_id, it.feature()) ;
                    if (feature)
                    {
                        bool feature_added = false ;
                        if (track->addChildItem(feature))
                        {
                            gui_featureset->insertFeature(feature) ;
                            feature_added = true ;
                        }
                        if (!feature_added)
                            delete feature ;
                    }
                }
            }
        }
    }
    else if (track_number == 2)
    {
        // we have two graphics objects corresponding to this track id
        // so this means strand sensitive
        auto ti = pit.first ;
        ZWTrack *track1 = ti->second ;
        ++ti ;
        ZWTrack *track2 = ti->second ;
        if (track1 && track2)
        {
            for (auto it = featureset->begin() ; it != featureset->end() ; ++it )
            {
                ZInternalID feature_id = it->getID() ;
                ZGStrandType strand = it->getStrand() ;
                if (!gui_featureset->contains(feature_id))
                {
                    ZFeature* feature = ZGConverter::convertFeature(featureset_id, it.feature()) ;
                    if (feature)
                    {
                        bool feature_added = false ;
                        if (strand == ZGStrandType::Plus || strand == ZGStrandType::Dot || strand == ZGStrandType::Star)
                        {
                            if (track1->addChildItem(feature))
                            {
                                gui_featureset->insertFeature(feature) ;
                                feature_added = true ;
                            }
                        }
                        else if (strand == ZGStrandType::Minus)
                        {
                            if (track2->addChildItem(feature))
                            {
                                gui_featureset->insertFeature(feature) ;
                                feature_added = true ;
                            }
                        }
                        if (!feature_added)
                            delete feature ;
                    }
                }
            }
        }
    }

    internalReorder() ;

}

// delete all features in the featureset specified; note that in order to remove a feature
// we do not really have to check that it's in the appropriate track; the 'removeItem()'
// function operates on the scene itself, and so will operate correctly irrespective of
// whether the item in question is the child of another
void ZGuiScene::deleteAllFeatures(ZInternalID featureset_id)
{
    if (!containsFeatureset(featureset_id) || !m_f2t_map.featuresetPresent(featureset_id))
        return ;

    auto fit = m_featuresets.find(featureset_id) ;
    if (fit == m_featuresets.end())
        return ;
    ZGuiFeatureset * gui_featureset = nullptr ;
    gui_featureset = fit->second ;
    if (!gui_featureset)
        return ;

    std::set<ZFeature*> features = gui_featureset->removeFeatures() ;
    for (auto it = features.begin() ; it != features.end() ; ++it )
    {
        QGraphicsItem *item = *it ;
        if (item)
        {
            removeItem(item) ;
            delete item ;
        }
    }

    internalReorder() ;
}

// delete features with ids as given
void ZGuiScene::deleteFeatures(ZInternalID featureset_id, const std::set<ZInternalID> & features)
{
    if (!containsFeatureset(featureset_id) || !m_f2t_map.featuresetPresent(featureset_id) || !features.size())
        return ;

    auto fit = m_featuresets.find(featureset_id) ;
    if (fit == m_featuresets.end())
        return ;
    ZGuiFeatureset * gui_featureset = nullptr ;
    gui_featureset = fit->second ;
    if (!gui_featureset)
        return ;

    for (auto it = features.begin() ; it != features.end() ; ++it )
    {
        QGraphicsItem * item = gui_featureset->removeFeature(*it) ;
        if (item)
        {
            removeItem(item) ;
            delete item ;
        }
    }

    internalReorder() ;
}


std::set<ZInternalID> ZGuiScene::getFeaturesetsFromTrack(ZInternalID track_id) const
{
    return m_f2t_map.getFeaturesets(track_id) ;
}

ZInternalID ZGuiScene::getTrackFromFeatureset(ZInternalID featureset_id) const
{
    return m_f2t_map.getTrack(featureset_id) ;
}



////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiScene::internalReorder()
{
    QPointF pos ;
    ZWTrackSimple * track = Q_NULLPTR ;
    ZWTrackSimple::Strand strand = ZWTrackSimple::Strand::Invalid ;

    // we first find the strand separator and get its width; the offset of the
    // positive side starts at zero and the offset of the negative side starts
    // at the width of the separator
    ZWTrackSeparator* separator = findStrandSeparator() ;
    qreal offset_plus = 0.0,
            offset_minus = 0.0 ;
    if (separator)
    {
        offset_minus = separator->boundingRect().height() ;
    }

    // now we just do the same thing but in the order
    // in which the ids are present in the m_track_ordering container.
    offset_plus = 0.0 ;
    offset_minus = 0.0 ;
    if (separator)
    {
        offset_minus = separator->boundingRect().height() ;
    }
    for (auto it = m_track_ordering.begin() ; it != m_track_ordering.end() ; ++it)
    {
        ZInternalID current_id = *it ;
        auto pit = m_track_map.equal_range(current_id) ;
        for (auto ti = pit.first ; ti != pit.second ; ++ti )
        {
            if ((track = dynamic_cast<ZWTrackSimple*>(ti->second)))
            {
                pos = track->pos() ;
                strand = track->getStrand() ;
                if (strand == ZWTrackSimple::Strand::Plus || strand == ZWTrackSimple::Strand::Independent)
                {
                    offset_plus -= track->boundingRect().height() ;
                    pos.setY(offset_plus) ;
                }
                else if (strand == ZWTrackSimple::Strand::Minus)
                {
                    pos.setY(offset_minus) ;
                    offset_minus += track->boundingRect().height() ;
                }
                track->setPos(pos) ;
            }
        }
    }
    update(sceneRect()) ;
}

void ZGuiScene::slot_selectionChanged()
{
    QList<QGraphicsItem*> list = selectedItems() ;
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiScene::slot_selectionChanged() ; list.size() = " << list.size() ;
#endif
}


////////////////////////////////////////////////////////////////////////////////
/// Event handlers
////////////////////////////////////////////////////////////////////////////////

// scene to query for information concerning the event (e.g. position)
//void ZGuiScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
//{
//    QPointF pos = event->scenePos() ;
//#ifndef QT_NO_DEBUG
//    qDebug() << "ZGuiScene::contextMenuEvent() " << this << pos ;
//#endif
//    emit signal_contextMenuEvent(pos) ;
//}



ZGuiScene * ZGuiScene::newInstance(ZInternalID id, bool locus_required )
{
    ZGuiScene * scene = Q_NULLPTR ;

    try
    {
        scene = new ZGuiScene (id, locus_required) ;
    }
    catch (...)
    {
        scene = Q_NULLPTR ;
    }

    if (scene)
    {
        scene->setObjectName(getNameFromID(id)) ;
    }

    return scene ;
}

} // namespace GUI

} // namespace ZMap

