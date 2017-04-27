/*  File: ZGuiDialogueSelectItems.h
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

#ifndef ZGUIDIALOGUESELECTITEMS_H
#define ZGUIDIALOGUESELECTITEMS_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <QApplication>
#include <QDialog>
#include <QString>
#include <QStringList>
#include <QItemSelection>

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QLineEdit ;
class QStandardItemModel ;
class QTableView ;
class QPushButton ;
class QCheckBox ;
class QItemSelectionModel ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

class ZGuiDialogueSelectItems : public QDialog,
        public Util::InstanceCounter<ZGuiDialogueSelectItems>,
        public Util::ClassName<ZGuiDialogueSelectItems>
{

    Q_OBJECT

public:
    static ZGuiDialogueSelectItems* newInstance(QWidget *parent = Q_NULLPTR) ;

    ~ZGuiDialogueSelectItems() ;

    void setData(const QStringList & data) ;
    QStringList getData() const ;
    QStringList getSelectedData() const ;

signals:
    void signal_received_close_event() ;

public slots:
    void slot_close() ;

private slots:
    void slot_search_box_text_changed(const QString& data) ;
    void slot_case_sensitivity_changed() ;

protected:
    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE ;

private:

    explicit ZGuiDialogueSelectItems(QWidget *parent = Q_NULLPTR) ;
    ZGuiDialogueSelectItems(const ZGuiDialogueSelectItems&) = delete ;
    ZGuiDialogueSelectItems& operator=(const ZGuiDialogueSelectItems& ) = delete ;

    static const QString m_display_name ;

    QWidget* createControls01() ;
    QWidget* createControls02() ;

    QVBoxLayout *m_top_layout ;
    QLineEdit *m_search_edit ;
    QCheckBox *m_search_case ;
    QPushButton *m_button_cancel,
        *m_button_OK ;
    QStandardItemModel *m_data_model ;
    QTableView * m_data_view ;
    QItemSelectionModel * m_data_selection ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIDIALOGUESELECTITEMS_H
