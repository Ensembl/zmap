/*  File: ZGuiChooseStyleDialogue.h
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

#ifndef ZGUICHOOSESTYLEDIALOGUE_H
#define ZGUICHOOSESTYLEDIALOGUE_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGChooseStyleData.h"
#include <cstddef>
#include <string>
#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QPushButton ;
class QStandardItem ;
class QStandardItemModel ;
class QTreeView ;
QT_END_NAMESPACE


namespace ZMap
{

namespace GUI
{

class ZGuiChooseStyleDialogue : public QDialog,
        public Util::InstanceCounter<ZGuiChooseStyleDialogue>,
        public Util::ClassName<ZGuiChooseStyleDialogue>
{

    Q_OBJECT

public:
    static ZGuiChooseStyleDialogue* newInstance(QWidget *parent = Q_NULLPTR) ;

    ~ZGuiChooseStyleDialogue() ;

    bool setChooseStyleData(const ZGChooseStyleData& data) ;
    ZGChooseStyleData getChooseStyleData() const ;

signals:
    void signal_received_close_event() ;

public slots:

private slots:
    void slot_close() ;

protected:
    void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE ;

private:

    explicit ZGuiChooseStyleDialogue(QWidget* parent=Q_NULLPTR);
    ZGuiChooseStyleDialogue(const ZGuiChooseStyleDialogue& ) = delete ;
    ZGuiChooseStyleDialogue& operator=(const ZGuiChooseStyleDialogue&) = delete ;

    bool addStandardItem(QStandardItemModel* model, int i, int j, const QString& data = QString()) ;

    static const QString m_display_name ;
    static const int m_styledata_id ;

    QStandardItemModel* createControls00() ;
    QTreeView* createControls01() ;
    QWidget* createControls02() ;

    QVBoxLayout *m_top_layout ;
    QTreeView *m_styles_view ;
    QStandardItemModel* m_styles_model ;
    QPushButton *m_button_edit,
        *m_button_delete,
        *m_button_add,
        *m_button_apply,
        *m_button_cancel ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUICHOOSESTYLEDIALOGUE_H
