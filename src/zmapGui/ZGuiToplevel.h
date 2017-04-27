/*  File: ZGuiToplevel.h
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

#ifndef ZGUITOPLEVEL_H
#define ZGUITOPLEVEL_H

#include <QApplication>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QPoint>

#include <cstddef>
#include <set>
#include <memory>
#include <string>

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QHBoxLayout ;
class QAction ;
class QMenu ;
class QPushButton ;
class QLineEdit ;
class QTableView ;
class QStandardItemModel ;
class QLineEdit ;
QT_END_NAMESPACE

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGSelectSequenceData.h"
#include "ZGMessageHeader.h"
#include "ZGZMapData.h"

//
// This is the ZMap top level window.
//


namespace ZMap
{

namespace GUI
{

class ZGuiToplevelStyle ;
class ZGuiCoordinateValidator ;
class ZGuiWidgetSelectSequence ;
class ZGuiScene ;
class ZGuiMain ;

class ZGuiToplevel : public QMainWindow,
        public Util::InstanceCounter<ZGuiToplevel>,
        public Util::ClassName<ZGuiToplevel>
{

    Q_OBJECT

public:

    enum class HelpType: unsigned char
    {
        General,
        Release,
        About
    } ;

    static ZGuiToplevel * newInstance(ZInternalID id, QWidget *parent = Q_NULLPTR ) ;

    ~ZGuiToplevel() ;

    bool setSelectSequenceData(const ZGSelectSequenceData& data) ;
    ZGSelectSequenceData getSelectSequenceData() const ;

    ZInternalID getID() const {return m_id ; }
    std::string getUserHome() const {return m_user_home ; }

    // overall commands and communications
    void receive(const std::shared_ptr<ZGMessage>& message) ;
    std::shared_ptr<ZGMessage> query(const std::shared_ptr<ZGMessage>& message) ;

    // scene management
    std::set<ZInternalID> getSequenceIDs() const ;
    std::set<ZInternalID> getSequenceIDs(ZInternalID gui_id) const ;
    std::set<ZInternalID> getFeaturesetIDs(ZInternalID sequence_id) const ;
    std::set<ZInternalID> getFeatureIDs(ZInternalID sequence_id, ZInternalID featureset_id) const ;
    std::set<ZInternalID> getTrackIDs(ZInternalID track_id) const ;
    std::set<ZInternalID> getFeaturesetFromTrack(ZInternalID sequence_id, ZInternalID track_id) const ;
    ZInternalID getTrackFromFeatureset(ZInternalID sequence_id, ZInternalID featureset_id ) const ;
    inline ZGuiScene* getSequence(ZInternalID id) const ;
    inline bool containsSequence(ZInternalID id) const ;
    void setStatusLabel(ZInternalID id,
                        const std::string & label,
                        const std::string & tooltip ) ;

    // gui main management
    std::set<ZInternalID> getGuiMainIDs() const ;
    inline ZGuiMain* getGuiMain(ZInternalID id) const ;
    inline bool containsGuiMain(ZInternalID id) const ;

signals:
    void signal_log_message(const std::string& str) ;
    void signal_received_close_event() ;
    void signal_received_quit_event() ;

public slots:

private slots:
    void slot_log_message(const std::string &message) ;
    void slot_action_help_general() ;
    void slot_action_help_release() ;
    void slot_action_help_about() ;
    void slot_quit() ;

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE ;
    void hideEvent(QHideEvent *event) Q_DECL_OVERRIDE ;
    void showEvent(QShowEvent* event) Q_DECL_OVERRIDE ;

private:

    ZGuiToplevel(ZInternalID id ,
                          QWidget *parent = Q_NULLPTR) ;
    ZGuiToplevel(const ZGuiToplevel&) = delete ;
    ZGuiToplevel& operator=(const ZGuiToplevel&) = delete ;

    static const QString m_display_name,
        m_help_text_general,
        m_help_text_release,
        m_help_text_about,
        m_help_text_title_general,
        m_help_text_title_release,
        m_help_text_title_about ;
    static const int m_model_column_number ;
    static const QStringList m_model_column_titles ;
    static const int m_zmapdata_id ;

    void createAllMenus() ;
    void createFileMenu() ;
    void createHelpMenu() ;
    void createFileMenuActions() ;
    void createHelpMenuActions() ;
    QWidget *createControls01() ;
    void helpDisplayCreate (HelpType type) ;

    void processTrackOperation(const std::shared_ptr<ZGMessageTrackOperation>& message) ;
    void processFeaturesetOperation(const std::shared_ptr<ZGMessageFeaturesetOperation>& message) ;
    void processFeaturesetToTrack(const std::shared_ptr<ZGMessageFeaturesetToTrack>& message) ;
    void processGuiOperation(const std::shared_ptr<ZGMessageGuiOperation>& message) ;
    void processSequenceOperation(const std::shared_ptr<ZGMessageSequenceOperation>& message) ;
    void processSequenceToGui(const std::shared_ptr<ZGMessageSequenceToGui>& message) ;
    void processSeparatorOperation(const std::shared_ptr<ZGMessageSeparatorOperation>& message) ;
    void processOperation(const std::shared_ptr<ZGMessage> & message) ;

    QPoint m_pos ;
    QByteArray m_geometry ;
    ZInternalID m_id ;
    ZGuiToplevelStyle *m_style ;
    QVBoxLayout *m_top_layout ;
    ZGuiWidgetSelectSequence *m_select_sequence ;
    QMenu *m_menu_file,
        *m_menu_help ;
    QAction *m_action_file_close,
        *m_action_help_general,
        *m_action_help_release,
        *m_action_help_about ;
    QTableView *m_zmaps_view ;
    QStandardItemModel *m_zmaps_model ;
    QPushButton *m_button_close_selected,
        *m_button_quit_all;
    std::string m_user_home ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUITOPLEVEL_H
