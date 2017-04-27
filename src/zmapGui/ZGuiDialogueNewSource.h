/*  File: ZGuiDialogueNewSource.h
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

#ifndef ZGUIDIALOGUENEWSOURCE_H
#define ZGUIDIALOGUENEWSOURCE_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGSourceType.h"
#include "ZGNewSourceData.h"

#include <cstddef>
#include <string>
#include <QApplication>
#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QHBoxLayout ;
class QComboBox ;
class QPushButton ;
class QLineEdit ;
class QToolButton ;
class QCheckBox ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{
class ZGuiPortValidator ;
class ZGuiWidgetComboSource ;

class ZGuiDialogueNewSource : public QMainWindow,
        public Util::InstanceCounter<ZGuiDialogueNewSource>,
        public Util::ClassName<ZGuiDialogueNewSource>
{

    Q_OBJECT

public:

    static ZGuiDialogueNewSource* newInstance(QWidget* parent = Q_NULLPTR) ;

    ~ZGuiDialogueNewSource() ;

    bool setNewSourceData(const ZGNewSourceData& data) ;
    ZGNewSourceData getNewSourceData() const ;

signals:
    void signal_received_close_event() ;

public slots:
    void slot_close() ;
    void slot_action_file_close() ;

private slots:
    void slot_control_panel_changed() ;
    void slot_ensembl_lookup_db() ;
    void slot_ensembl_lookup_featuresets() ;
    void slot_ensembl_lookup_biotypes() ;

protected :
    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE ;


private:

    explicit ZGuiDialogueNewSource(QWidget *parent = Q_NULLPTR) ;
    ZGuiDialogueNewSource(const ZGuiDialogueNewSource&) = delete ;
    ZGuiDialogueNewSource& operator=(const ZGuiDialogueNewSource&) = delete ;

    static const QString m_display_name,
        m_default_host;
    static const QChar m_list_delimiter ;

    QWidget* createControls01() ;
    QWidget* createControls02() ;
    QWidget* createControls03() ;
    QWidget* createControls04() ;

    QVBoxLayout *m_top_layout ;
    QWidget *m_controls_ensembl,
        *m_controls_fileurl ;
    ZGuiWidgetComboSource *m_entry_source_type ;
    QPushButton *m_button_cancel,
        *m_button_OK ;
    QLineEdit *m_entry_source_name,
        *m_entry_database,
        *m_entry_dbprefix,
        *m_entry_host,
        *m_entry_port,
        *m_entry_username,
        *m_entry_password,
        *m_entry_featuresets,
        *m_entry_biotypes,
        *m_entry_source_fileurl,
        *m_entry_pathurl ;
    QToolButton *m_button_lookupdb,
        *m_button_lookup_featuresets,
        *m_button_lookup_biotypes ;
    ZGuiPortValidator *m_port_validator ;
    QCheckBox *m_check_load_dna ;
};


} // namespace GUI

} // namespace ZMap

#endif // ZGUIDIALOGUENEWSOURCE_H
