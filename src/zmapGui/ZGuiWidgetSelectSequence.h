/*  File: ZGuiWidgetSelectSequence.h
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

#ifndef ZGUIWIDGETSELECTSEQUENCE_H
#define ZGUIWIDGETSELECTSEQUENCE_H

#include <QApplication>
#include <QWidget>
#include <QString>
#include <cstddef>
#include <string>

#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGSelectSequenceData.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QHBoxLayout ;
class QPushButton ;
class QStandardItemModel ;
class QTableView ;
class QLineEdit ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

class ZGuiCoordinateValidator ;

class ZGuiWidgetSelectSequence : public QWidget,
        public Util::InstanceCounter<ZGuiWidgetSelectSequence>,
        public Util::ClassName<ZGuiWidgetSelectSequence>
{

    Q_OBJECT

public:
    static ZGuiWidgetSelectSequence* newInstance() ;

    ~ZGuiWidgetSelectSequence() ;

    bool setSelectSequenceData(const ZGSelectSequenceData& data) ;
    ZGSelectSequenceData getSelectSequenceData() const ;

signals:
    void signal_create_new_source() ;
    void signal_delete_selected_source() ;
    void signal_save_to_config() ;
    void signal_load_from_config() ;
    void signal_set_from_default() ;
    void signal_start_zmap() ;

public slots:

private slots:
    void slot_create_new_source() ;
    void slot_delete_selected_source() ;
    void slot_save_to_config() ;
    void slot_load_from_config() ;
    void slot_set_from_default() ;
    void slot_start_zmap() ;

private:

    explicit ZGuiWidgetSelectSequence(QWidget *parent = Q_NULLPTR) ;
    ZGuiWidgetSelectSequence(const ZGuiWidgetSelectSequence&) = delete ;
    ZGuiWidgetSelectSequence& operator=(const ZGuiWidgetSelectSequence&) = delete ;

    static const QStringList m_model_column_titles ;
    static const int m_model_column_number,
        m_sourcetype_id ;

    QWidget* createControls01() ;
    QWidget* createControls02() ;
    QWidget* createControls03() ;
    QHBoxLayout *createControls04() ;

    QVBoxLayout *m_top_layout ;
    QTableView *m_sources_view ;
    QStandardItemModel *m_sources_model ;
    QLineEdit *m_entry_sequence,
        *m_entry_start,
        *m_entry_end,
        *m_entry_config ;
    QPushButton *m_button_new,
        *m_button_remove,
        *m_button_save,
        *m_button_load,
        *m_button_default,
        *m_button_start,
        *m_button_close_selected,
        *m_button_quit_all;
    ZGuiCoordinateValidator *m_start_validator,
        *m_end_validator ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIWIDGETSELECTSEQUENCE_H
