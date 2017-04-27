/*  File: ZGuiMain.h
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

#ifndef ZGUIMAIN_H
#define ZGUIMAIN_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGResourceImportData.h"
#include "ZGViewOrientationType.h"
#include "ZGScaleRelativeType.h"
#include <QApplication>
#include <QMainWindow>
#include <QPoint>
#include <QByteArray>

QT_BEGIN_NAMESPACE
class QAction ;
class QActionGroup ;
class QLabel ;
class QMenu ;
class QErrorMessage ;
class QSignalMapper ;
QT_END_NAMESPACE

#include <cstddef>
#include <string>
#include <set>

namespace ZMap
{

namespace GUI
{

class ZGuiSequences ;
class ZGuiViewControl ;
class ZGuiMainStyle ;
class ZGuiScene ;

class ZGuiMain : public QMainWindow,
        public Util::InstanceCounter<ZGuiMain>,
        public Util::ClassName<ZGuiMain>
{

    Q_OBJECT

public:
    static ZGuiMain* newInstance(ZInternalID id) ;
    static QString getNameFromID(ZInternalID id) ;

    ~ZGuiMain() ;

    ZInternalID getID() const {return m_id ; }
    std::set<ZInternalID> getSequenceIDs() const ;
    void setUserHome(const std::string& data) ;
    std::string getUserHome() const {return m_user_home ; }
    bool addSequence(ZInternalID id, ZGuiScene * scene) ;
    bool deleteSequence(ZInternalID id) ;
    bool containsSequence(ZInternalID id) ;
    bool orientation() const ;
    bool setOrientation(bool orient) ;
    bool setSequenceOrder(const std::vector<ZInternalID> & data) ;
    void setStatusLabel(const std::string & label,
                        const std::string & tooltip ) ;
    void setViewOrientation(ZInternalID sequence_id,
                            ZGViewOrientationType orientation) ;
    void scaleBarShow(ZInternalID sequence_id) ;
    void scaleBarHide(ZInternalID sequence_id) ;
    void scaleBarPosition(ZInternalID sequence_id,
                             ZGScaleRelativeType position) ;
    void setMenuBarColor(const QColor& color) ;


signals:
    void signal_log_message(const std::string& data) ;
    void signal_received_quit_event() ;
    void signal_resource_import_event(const ZGResourceImportData& data) ;

public slots:

private slots:

    void slot_contextMenuEvent(const QPointF & pos) ;
    void slot_sequenceSelectionChanged() ;

    void slot_action_file_new_sequence() ;
    void slot_action_file_save() ;
    void slot_action_file_save_as() ;
    void slot_action_file_import() ;
    void slot_action_file_print_screenshot() ;
    void slot_action_file_close() ;
    void slot_action_file_quit() ;
    void slot_action_export_dna() ;
    void slot_action_export_features() ;
    void slot_action_export_features_marked() ;
    void slot_action_export_configuration() ;
    void slot_action_export_styles() ;
    void slot_action_edit_copy() ;
    void slot_action_edit_ucopy() ;
    void slot_action_edit_paste() ;
    void slot_action_edit_styles() ;
    void slot_action_edit_preferences() ;
    void slot_action_edit_dev() ;
    void slot_action_view_details() ;
    void slot_action_view_toggle() ;
    void slot_action_ticket_viewz() ;
    void slot_action_ticket_zmap() ;
    void slot_action_ticket_seq() ;
    void slot_action_ticket_ana() ;
    void slot_action_help_quick_start() ;
    void slot_action_help_keyboard() ;
    void slot_action_help_filtering() ;
    void slot_action_help_alignment() ;
    void slot_action_help_release_notes() ;
    void slot_action_help_about_zmap() ;
    void slot_action_blixem() ;
    void slot_action_track_bump() ;
    void slot_action_track_hide() ;
    void slot_action_toggle_mark() ;
    void slot_action_compress_cols() ;
    void slot_action_track_config() ;
    void slot_action_show_feature() ;
    void slot_action_show_featureitem() ;
    void slot_action_show_featuresetitem() ;
    void slot_action_print_style() ;
    void slot_action_show_windowstats() ;
    void slot_action_filter_test() ;
    void slot_action_filter_cs(int op) ;
    void slot_action_filter_ni(int op) ;
    void slot_action_filter_ne(int op) ;
    void slot_action_filter_nc(int op) ;
    void slot_action_filter_me(int op) ;
    void slot_action_filter_mc(int op) ;
    void slot_action_filter_un() ;
    void slot_action_search_dna() ;
    void slot_action_search_peptide() ;
    void slot_action_search_feature() ;
    void slot_action_search_list() ;

protected:
    void hideEvent(QHideEvent *event) Q_DECL_OVERRIDE ;
    void showEvent(QShowEvent* event) Q_DECL_OVERRIDE ;

private:
    static const QString m_display_name,
        m_name_base,
        m_context_menu_title ;
    static const int m_filter_op_single,
        m_filter_op_all ;

    ZGuiMain(ZInternalID id) ;
    ZGuiMain(const ZGuiMain& main) = delete ;
    ZGuiMain& operator=(const ZGuiMain& main) = delete ;

    void createMappers() ;

    void createAllMenus() ;
    void createFileMenu() ;
    void createFileMenuActions() ;
    void createExportMenu() ;
    void createExportMenuActions() ;
    void createEditMenu() ;
    void createEditMenuActions() ;
    void createViewMenu() ;
    void createViewMenuActions() ;
    void createTicketMenu() ;
    void createTicketMenuActions() ;
    void createHelpMenu() ;
    void createHelpMenuActions() ;
    void createFilterActions() ;
    void createMiscActions() ;

    QMenu* createContextMenu() ;
    QMenu* createSubmenuDeveloper() ;
    QMenu* createSubmenuFiltering() ;
    QMenu* createSubmenuSearch() ;
    QMenu* createSubmenuExport() ;

    QPoint m_pos ;
    QByteArray m_geometry ;
    ZInternalID m_id ;
    std::string m_user_home ;
    ZGuiViewControl *m_view_control ;
    ZGuiSequences *m_sequences ;
    ZGuiMainStyle *m_style ;

    QSignalMapper *m_mapper_filter_cs,
        *m_mapper_filter_ni,
        *m_mapper_filter_ne,
        *m_mapper_filter_nc,
        *m_mapper_filter_me,
        *m_mapper_filter_mc ;

    // menu data
    QMenu *m_menu_file,
        *m_menu_export,
        *m_menu_edit,
        *m_menu_view,
        *m_menu_ticket,
        *m_menu_help ;

    QAction *m_action_context_menu_title,
        *m_action_file_new_sequence,
        *m_action_file_save,
        *m_action_file_save_as,
        *m_action_file_import,
        *m_action_file_print_screenshot,
        *m_action_file_close,
        *m_action_file_quit,
        *m_action_export_dna,
        *m_action_export_features,
        *m_action_export_features_marked,
        *m_action_export_configuration,
        *m_action_export_styles,
        *m_action_edit_copy,
        *m_action_edit_ucopy,
        *m_action_edit_paste,
        *m_action_edit_styles,
        *m_action_edit_preferences,
        *m_action_edit_dev,
        *m_action_view_details,
        *m_action_toggle_coords,
        *m_action_ticket_viewz,
        *m_action_ticket_zmap,
        *m_action_ticket_seq,
        *m_action_ticket_ana,
        *m_action_help_quick_start,
        *m_action_help_keyboard,
        *m_action_help_filtering,
        *m_action_help_alignment,
        *m_action_help_release_notes,
        *m_action_help_about_zmap,
        *m_action_blixem,
        *m_action_track_bump,
        *m_action_track_hide,
        *m_action_toggle_mark,
        *m_action_compress_tracks,
        *m_action_track_config,
        *m_action_show_feature,
        *m_action_show_featureitem,
        *m_action_show_featuresetitem,
        *m_action_print_style,
        *m_action_show_windowstats,
        *m_action_filter_test,
        *m_action_filter_un,
        *m_action_filter_subtitle,
        *m_action_filter1_cs,
        *m_action_filter1_ni,
        *m_action_filter1_ne,
        *m_action_filter1_nc,
        *m_action_filter1_me,
        *m_action_filter1_mc,
        *m_action_filter2_cs,
        *m_action_filter2_ni,
        *m_action_filter2_ne,
        *m_action_filter2_nc,
        *m_action_filter2_me,
        *m_action_filter2_mc,
        *m_action_search_dna,
        *m_action_search_peptide,
        *m_action_search_feature,
        *m_action_search_list
    ;

    QErrorMessage *m_error_message ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIMAIN_H
