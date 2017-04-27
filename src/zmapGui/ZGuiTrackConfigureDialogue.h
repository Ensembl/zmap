/*  File: ZGuiTrackConfigureDialogue.h
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

#ifndef ZGUITRACKCONFIGUREDIALOGUE_H
#define ZGUITRACKCONFIGUREDIALOGUE_H

#include <QApplication>
#include <QMainWindow>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QHBoxLayout ;
class QTabWidget ;
class QPushButton ;
class QTableView ;
class QStandardItem ;
class QStandardItemModel ;
QT_END_NAMESPACE

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGTrackConfigureData.h"
#include <cstddef>
#include <string>

//
// Things to do with this class:
//
// (1) It would probably be better if the ZGTrackData for each track
//     is stored completely within the instance; this should be done
//     on call to setTrackConfigureData().
//
// (2) Similarly, we should simply copy these back again on call to
//     getTrackConfigureData().
//
// (3) Internal storage should be modified to store the ZGTrackData
//     for each track as the QVariant meta data associated with
//     one of the items in each row (the first, for simplicity)
//     as is currently implemented with the ZInternalID data.
//
// (4) Consequence will be use of extra storage, but avoiding the
//     need to do something about data that are not used in manipulation
//     of one or other of the "current" or "available" sets of tracks.
//


namespace ZMap
{

namespace GUI
{

class ZGuiMain ;
class ZGuiTrackConfigureDialogueStyle ;
class ZGuiTextDisplayDialogue ;

class ZGuiTrackConfigureDialogue : public QMainWindow,
        public Util::InstanceCounter<ZGuiTrackConfigureDialogue>,
        public Util::ClassName<ZGuiTrackConfigureDialogue>
{

    Q_OBJECT

public:

    enum class HelpType: unsigned char
    {
        General,
        Display
    };

    static ZGuiTrackConfigureDialogue* newInstance(QWidget *parent = Q_NULLPTR) ;

    ~ZGuiTrackConfigureDialogue() ;

    void setIsMarked(bool marked) ;
    bool setTrackConfigureData(const ZGTrackConfigureData& data) ;
    ZGTrackConfigureData getTrackConfigureData() const ;

signals:
    void signal_received_close_event() ;
    void signal_help_display_create(HelpType type) ;

public slots:
    void slot_help_display_create(HelpType type) ;
    void slot_help_display_closed() ;
    void slot_button_clicked_OK() ;
    void slot_button_clicked_apply() ;
    void slot_button_clicked_cancel() ;

private slots:
    void slot_action_file_close() ;
    void slot_action_help_general() ;
    void slot_action_help_display() ;

protected:
    void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE ;

private:

    ZGuiTrackConfigureDialogue(QWidget* parent=Q_NULLPTR) ;
    ZGuiTrackConfigureDialogue(const ZGuiTrackConfigureDialogue& ) = delete ;
    ZGuiTrackConfigureDialogue& operator=(const ZGuiTrackConfigureDialogue&) = delete ;

    static const QString m_display_name,
        m_help_text_general,
        m_help_text_display,
        m_help_text_title_general,
        m_help_text_title_display ;
    static int m_current_tracks_column_count,
        m_available_tracks_column_count ;
    static const int m_zinternalid_id,
        m_trackdata_id ;

    void createAllMenus() ;
    void createFileMenu() ;
    void createHelpMenu() ;
    void createFileMenuActions() ;
    void createHelpMenuActions() ;
    bool addCurrentTrack(const ZGTrackData& data) ;
    bool addAvailableTrack(const ZGTrackData& data) ;
    ZGTrackConfigureData getTrackConfigureDataCurrent() const ;
    ZGTrackConfigureData getTrackConfigureDataAvailable() const ;
    QWidget* getIndexWidget(QTableView *view, QStandardItemModel* model, int row, int col) const ;
    QWidget* createPageCurrentTracks() ;
    QWidget* createPageAvailableTracks() ;
    QWidget* createMainButtons() ;
    bool addStandardItem(QStandardItemModel*model, int i, int j, const QString&data=QString()) ;

    QVBoxLayout *m_top_layout ;
    QTabWidget *m_tab_widget ;
    QPushButton *m_b1_select_all,
        *m_b1_set_group,
        *m_b2_select_all,
        *m_b2_select_marked,
        *m_b2_select_none,
        *m_button_revert,
        *m_button_apply_order,
        *m_button_apply,
        *m_button_cancel,
        *m_button_OK ;
    QStandardItemModel *m_model_current_tracks,
        *m_model_available_tracks ;
    QTableView *m_view_current_tracks,
        *m_view_available_tracks ;
    ZGuiTrackConfigureDialogueStyle *m_style ;
    ZGuiTextDisplayDialogue *m_text_display_dialogue ;

    QMenu *m_menu_file,
        *m_menu_help ;

    QAction *m_action_file_close,
        *m_action_help_general,
        *m_action_help_display ;
    bool m_is_marked ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGUITRACKCONFIGUREDIALOGUE_H
