/*  File: ZGuiDialogueSearchDNA.h
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

#ifndef ZGUIDIALOGUESEARCHDNA_H
#define ZGUIDIALOGUESEARCHDNA_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGSearchDNAData.h"

#include <QApplication>
#include <QMainWindow>
#include <QColor>

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QMenu ;
class QAction ;
class QPushButton ;
class QLineEdit ;
class QLabel ;
class QSpinBox ;
class QComboBox ;
class QHBoxLayout ;
class QCheckBox ;
class QGroupBox ;
QT_END_NAMESPACE

#include <cstddef>
#include <string>
#include <map>

//
// Dialogue for search of DNA
//

namespace ZMap
{

namespace GUI
{

class ZGuiTextDisplayDialogue ;
class ZGuiWidgetButtonColour ;
class ZGuiSequenceValidatorDNA ;
class ZGuiWidgetComboStrand ;

class ZGuiDialogueSearchDNA : public QMainWindow,
        public Util::InstanceCounter<ZGuiDialogueSearchDNA>,
        public Util::ClassName<ZGuiDialogueSearchDNA>
{

    Q_OBJECT

public:
    static ZGuiDialogueSearchDNA* newInstance(QWidget *parent = Q_NULLPTR) ;

    ~ZGuiDialogueSearchDNA() ;

    bool setSearchDNAData(const ZGSearchDNAData& data) ;
    ZGSearchDNAData getSearchDNAData() const ;

signals:
    void signal_received_close_event() ;

public slots:
    void slot_help_closed() ;

private slots:
    void slot_close() ;
    void slot_action_file_close() ;
    void slot_action_help_overview() ;
    void slot_color_select_forward() ;
    void slot_color_select_reverse() ;

protected:
    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE ;

private:

    explicit ZGuiDialogueSearchDNA(QWidget *parent = Q_NULLPTR) ;
    ZGuiDialogueSearchDNA(const ZGuiDialogueSearchDNA&) = delete ;
    ZGuiDialogueSearchDNA& operator=(const ZGuiDialogueSearchDNA&) = delete ;

    static const QString m_display_name,
        m_help_text_overview,
        m_help_text_title_overview ;

    QWidget *createControls01() ;
    QWidget *createControls02() ;
    QWidget *createControls03() ;
    QWidget *createControls04() ;
    QWidget *createControls05() ;


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
        *m_action_help_overview ;

    QLineEdit *m_sequence_text ;

    ZGuiWidgetComboStrand *m_entry_strand ;
    QSpinBox *m_entry_start,
        *m_entry_end,
        *m_entry_mis,
        *m_entry_bas ;

    ZGuiWidgetButtonColour *m_button_col_forward,
        *m_button_col_reverse ;
    QPushButton *m_button_clear,
        *m_button_find ;
    QCheckBox *m_check_keep ;

    QColor m_col_forward,
        m_col_reverse ;

    ZGuiSequenceValidatorDNA *m_validator ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIDIALOGUESEARCHDNA_H
