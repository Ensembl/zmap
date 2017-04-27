/*  File: ZGuiEditStyleDialogue.h
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

#ifndef ZGUIEDITSTYLEDIALOGUE_H
#define ZGUIEDITSTYLEDIALOGUE_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGEditStyleData.h"
#include <cstddef>
#include <string>
#include <QDialog>
#include <QColor>
#include <QString>

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QPushButton ;
class QCheckBox ;
class QLabel ;
class QLineEdit ;
class QListView ;
class QStandardItemModel ;
class QCheckBox ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

class ZGuiWidgetButtonColour ;

class ZGuiEditStyleDialogue : public QDialog,
        public Util::InstanceCounter<ZGuiEditStyleDialogue>,
        public Util::ClassName<ZGuiEditStyleDialogue>
{

    Q_OBJECT

public:
    static ZGuiEditStyleDialogue* newInstance(QWidget *parent = Q_NULLPTR) ;

    ~ZGuiEditStyleDialogue() ;

    bool setEditStyleData(const ZGEditStyleData& data) ;
    ZGEditStyleData getEditStyleData() const ;

signals:
    void signal_received_close_event() ;

private slots:
    void slot_close() ;
    void slot_create_child() ;

protected:
    void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE ;

private:

    explicit ZGuiEditStyleDialogue(QWidget* parent=Q_NULLPTR) ;
    ZGuiEditStyleDialogue(const ZGuiEditStyleDialogue&) = delete ;
    ZGuiEditStyleDialogue& operator=(const ZGuiEditStyleDialogue&) = delete ;

    QWidget* createControls01() ;
    QWidget* createControls02() ;
    QWidget* createControls03() ;
    bool addStandardItem(QStandardItemModel* model,
                         int i,
                         int j,
                         const QString& data = QString()) ;

    static const QString m_display_name ;

    QVBoxLayout *m_top_layout ;
    QLineEdit *m_new_name,
        *m_original_name ;
    QListView *m_featuresets_view ;
    QStandardItemModel *m_featuresets_model ;
    QCheckBox *m_checkbox_stranded ;
    ZGuiWidgetButtonColour *m_button_fill_col,
        *m_button_border_col ;
    QPushButton *m_button_create_child,
        *m_button_revert,
        *m_button_close,
        *m_button_apply,
        *m_button_OK ;
    ZInternalID m_style_id ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIEDITSTYLEDIALOGUE_H
