/*  File: ZGuiDialogueSelectSequence.h
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

#ifndef ZGUIDIALOGUESELECTSEQUENCE_H
#define ZGUIDIALOGUESELECTSEQUENCE_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <QApplication>
#include <QDialog>
#include <cstddef>
#include <string>
#include "ZGSelectSequenceData.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QPushButton ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

class ZGuiWidgetSelectSequence ;

class ZGuiDialogueSelectSequence : public QDialog,
        public Util::InstanceCounter<ZGuiDialogueSelectSequence>,
        public Util::ClassName<ZGuiDialogueSelectSequence>
{

    Q_OBJECT

public:
    static ZGuiDialogueSelectSequence* newInstance(QWidget *parent = Q_NULLPTR) ;

    ~ZGuiDialogueSelectSequence() ;

    bool setSelectSequenceData(const ZGSelectSequenceData& data) ;
    ZGSelectSequenceData getSelectSequenceData() const ;

signals:
    void signal_received_close_event() ;

public slots:
    void slot_close() ;

protected:
    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE ;

private:

    explicit ZGuiDialogueSelectSequence(QWidget* parent = Q_NULLPTR) ;
    ZGuiDialogueSelectSequence(const ZGuiDialogueSelectSequence&) = delete ;
    ZGuiDialogueSelectSequence& operator=(const ZGuiDialogueSelectSequence&) = delete ;

    static const QString m_display_name ;

    QWidget *createControls01() ;

    QVBoxLayout* m_top_layout ;
    ZGuiWidgetSelectSequence* m_select_sequence ;
    QPushButton *m_button_close ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIDIALOGUESELECTSEQUENCE_H
