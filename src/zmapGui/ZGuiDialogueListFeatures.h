/*  File: ZGuiDialogueListFeatures.h
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

#ifndef ZGUIDIALOGUELISTFEATURES_H
#define ZGUIDIALOGUELISTFEATURES_H

#include <QApplication>
#include <QMainWindow>
#include <cstddef>
#include <string>

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGListFeaturesData.h"

QT_BEGIN_NAMESPACE
class QMenu ;
class QAction ;
class QStandardItem ;
class QStandardItemModel ;
class QTableView ;
class QVBoxLayout ;
class QHBoxLayout ;
class QPushButton ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

class ZGuiTextDisplayDialogue ;
class ZGuiDialogueListFeaturesStyle ;

class ZGuiDialogueListFeatures : public QMainWindow,
        public Util::InstanceCounter<ZGuiDialogueListFeatures>,
        public Util::ClassName<ZGuiDialogueListFeatures>
{

    Q_OBJECT

public:

    enum class HelpType: unsigned char
    {
        FeatureList,
        About
    } ;

    static ZGuiDialogueListFeatures* newInstance(QWidget*parent = Q_NULLPTR) ;

    ~ZGuiDialogueListFeatures() ;

    bool setListFeaturesData(const ZGListFeaturesData& data) ;
    ZGListFeaturesData getListFeaturesData() const ;


signals:
    void signal_received_close_event() ;
    void signal_help_display_create(HelpType type) ;

public slots:
    void slot_help_display_create(HelpType type) ;
    void slot_help_display_closed() ;

private slots:
    void slot_close() ;
    void slot_action_help_list() ;
    void slot_action_help_about() ;

protected:
    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE ;

private:

    explicit ZGuiDialogueListFeatures(QWidget *parent = Q_NULLPTR) ;
    ZGuiDialogueListFeatures(const ZGuiDialogueListFeatures&) = delete ;
    ZGuiDialogueListFeatures& operator=(const ZGuiDialogueListFeatures&) = delete ;

    static int m_model_column_number ;
    static const QStringList m_model_column_titles ;
    static const QString m_display_name,
        m_help_text_feature_list,
        m_help_text_about,
        m_help_text_title_feature_list,
        m_help_text_title_about ;
    static const int m_featuredata_id ;

    QWidget * createControls01() ;
    void createAllMenus() ;
    void createFileMenu() ;
    void createOperateMenu() ;
    void createHelpMenu() ;
    void createFileMenuActions() ;
    void createOperateMenuActions() ;
    void createHelpMenuActions() ;
    void setModelParams() ;
    ZGuiTextDisplayDialogue *m_text_display_dialogue ;
    QVBoxLayout *m_top_layout ;
    ZGuiDialogueListFeaturesStyle *m_style ;

    QMenu *m_menu_file,
        *m_menu_operate,
        *m_menu_help ;

    QAction *m_action_file_search,
        *m_action_file_preserve,
        *m_action_file_export,
        *m_action_file_close,
        *m_action_operate_rshow,
        *m_action_operate_rhide,
        *m_action_operate_sshow,
        *m_action_operate_shide,
        *m_action_help_feature_list,
        *m_action_help_about
    ;

    QTableView *m_feature_view ;
    QStandardItemModel *m_feature_model ;

    QPushButton *m_button_close ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIDIALOGUELISTFEATURES_H
