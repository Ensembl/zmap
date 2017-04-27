/*  File: ZGuiDialogueSearchFeatures.h
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

#ifndef ZGUIDIALOGUESEARCHFEATURES_H
#define ZGUIDIALOGUESEARCHFEATURES_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGSearchFeaturesData.h"

#include <QApplication>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QHBoxLayout ;
class QGridLayout ;
class QMenu ;
class QAction ;
class QGroupBox ;
class QLabel ;
class QLineEdit ;
class QComboBox ;
class QSpinBox ;
class QCheckBox ;
class QPushButton ;
QT_END_NAMESPACE

#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

class ZGuiTextDisplayDialogue ;
class ZGuiWidgetComboStrand ;
class ZGuiWidgetComboFrame ;

class ZGuiDialogueSearchFeatures : public QMainWindow,
        public Util::InstanceCounter<ZGuiDialogueSearchFeatures>,
        public Util::ClassName<ZGuiDialogueSearchFeatures>
{

    Q_OBJECT

public:

    static ZGuiDialogueSearchFeatures* newInstance(QWidget* parent = Q_NULLPTR) ;

    ~ZGuiDialogueSearchFeatures() ;

    bool setSearchFeaturesData(const ZGSearchFeaturesData& data) ;
    ZGSearchFeaturesData getSearchFeaturesData() const ;

signals:
    void signal_received_close_event() ;

public slots:
    void slot_help_closed() ;

private slots:
    void slot_close() ;
    void slot_action_file_close() ;
    void slot_action_help_general() ;

protected:
    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE ;

private:

    explicit ZGuiDialogueSearchFeatures(QWidget *parent = Q_NULLPTR) ;
    ZGuiDialogueSearchFeatures(const ZGuiDialogueSearchFeatures&) = delete ;
    ZGuiDialogueSearchFeatures& operator=(const ZGuiDialogueSearchFeatures& ) = delete ;

    static const QString m_display_name,
        m_help_text_general,
        m_help_text_title_general ;
    static const int m_max_int_input_value ;

    QWidget * createControls01() ;
    QWidget * createControls02() ;
    QWidget * createControls03() ;

    void createAllMenus() ;
    void createFileMenu() ;
    void createHelpMenu() ;
    void createFileMenuActions() ;
    void createHelpMenuActions() ;

    QVBoxLayout *m_top_layout ;

    ZGuiTextDisplayDialogue *m_text_display_dialogue ;

    QMenu *m_menu_file,
        *m_menu_help ;

    QAction *m_action_file_close,
        *m_action_help_general ;

    QComboBox *m_entry_align,
        *m_entry_block,
        *m_entry_track,
        *m_entry_featureset ;
    ZGuiWidgetComboStrand *m_entry_strand ;
    ZGuiWidgetComboFrame *m_entry_frame ;
    QLineEdit *m_entry_feature ;
    QSpinBox *m_entry_start,
        *m_entry_end ;
    QCheckBox *m_entry_locus ;
    QPushButton *m_button_search,
        *m_button_cancel ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIDIALOGUESEARCHFEATURES_H
