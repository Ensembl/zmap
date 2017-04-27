/*  File: ZGuiScene.h
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

#ifndef ZGUISCENE_H
#define ZGUISCENE_H

#include <cstddef>
#include <memory>
#include <string>
#include <set>
#include <map>
#include <QApplication>
#include <QGraphicsScene>
#include <QColor>
#include "ZInternalID.h"
#include "ClassName.h"
#include "InstanceCounter.h"
#include "Utilities.h"
#include "ZGFeaturesetTrackMapping.h"
#include "ZGuiFeatureset.h"

namespace ZMap
{

namespace GUI
{

class ZWTrack ;
class ZWTrackSeparator ;
class ZFeature ;
class ZGFeatureset ;
class ZFeaturePainter ;
class ZGStyle ;

// map of track_id to graphics object(s)
typedef std::multimap<ZInternalID, ZWTrack*> ZTrackMap ;

class ZGuiScene : public QGraphicsScene,
        public Util::InstanceCounter<ZGuiScene>,
        public Util::ClassName<ZGuiScene>
{
    Q_OBJECT

public:
    static ZGuiScene * newInstance(ZInternalID id, bool locus_required = true ) ;
    static QString getNameFromID(ZInternalID id) ;

    ~ZGuiScene() ;
    ZInternalID getID() const {return m_id ; }

    void setBackgroundColor(const QColor& color) ;
    QColor getBackgroundColor() const {return m_color_background; }

    void addStrandSeparator() ;
    void deleteStrandSeparator() ;
    void showStrandSeparator() ;
    void hideStrandSeparator() ;
    bool containsStrandSeparator() const ;
    void setSeparatorColor(const QColor& color)  ;

    void addTrack(ZInternalID id, bool sensitive) ;
    void deleteTrack(ZInternalID id) ;
    void showTrack(ZInternalID id) ;
    void hideTrack(ZInternalID id) ;
    bool containsTrack(ZInternalID id) const ;
    bool setTrackColor(ZInternalID id, const QColor& color) ;
    std::set<ZInternalID> getTracks() const ;

    void setDefaultTrackOrdering() ;
    bool setTrackOrdering(const std::vector<ZInternalID> & data) ;
    std::vector<ZInternalID> getTrackOrdering() const ;

    void addFeatureset(ZInternalID id) ;
    void deleteFeatureset(ZInternalID id) ;
    bool containsFeatureset(ZInternalID id) const ;
    std::set<ZInternalID> getFeaturesets() const ;
    std::set<ZInternalID> getFeatures(ZInternalID featureset_id) const ;
    void setFeaturesetStyle(ZInternalID featureset_id, const std::shared_ptr<ZGStyle>& style) ;

    void addFeaturesetToTrack(ZInternalID track_id, ZInternalID featureset_id) ;
    void removeFeaturesetFromTrack(ZInternalID track_id, ZInternalID featureset_id) ;

    std::set<ZInternalID> getFeaturesetsFromTrack(ZInternalID track_id) const ;
    ZInternalID getTrackFromFeatureset(ZInternalID featureset_id) const ;

    void addFeatures(const std::shared_ptr<ZGFeatureset>& featureset) ;
    void deleteFeatures(ZInternalID featureset_id, const std::set<ZInternalID>& features) ;
    void deleteAllFeatures(ZInternalID featureset_id) ;

    bool hasLocusScene() const {return static_cast<bool>(m_locus_scene) ; }
    bool createLocusScene() ;
    bool deleteLocusScene() ;
    ZGuiScene* getLocusScene() const {return m_locus_scene; }

public slots:
    void internalReorder() ;
    void slot_selectionChanged() ;

signals:
    void signal_contextMenuEvent(const QPointF& pos) ;

protected:
    //void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) Q_DECL_OVERRIDE ;

private:
    static const QString m_name_base ;
    static const QColor m_color_background_default,
        m_color_strand_default ;
    static const ZInternalID m_locus_id_value ;

    ZGuiScene(ZInternalID id, bool is_locus);
    ZGuiScene(const ZGuiScene& scene) = delete ;
    ZGuiScene& operator=(const ZGuiScene& scene) = delete ;

    ZWTrackSeparator* findStrandSeparator() const ;

    ZInternalID m_id ;
    QColor m_color_background,
        m_color_strand ;
    std::vector<ZInternalID> m_track_ordering ;
    ZTrackMap m_track_map ;
    ZGFeaturesetTrackMapping m_f2t_map ;

    std::map<ZInternalID, ZGuiFeatureset*> m_featuresets ;

    ZGuiScene *m_locus_scene ;
} ;

class ZGuiSceneCompareByID
{
public:
    bool operator() (const ZGuiScene& s1, const ZGuiScene& s2) const
    {
        return s1.getID() < s2.getID() ;
    }
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUISCENE_H
