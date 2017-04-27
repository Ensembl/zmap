/*  File: ZGuiTrackFilterDialogue.h
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

#ifndef ZGUITRACKFILTERDIALOGUE_H
#define ZGUITRACKFILTERDIALOGUE_H


#include <QDialog>

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QHBoxLayout ;
class QPushButton ;
class QLabel ;
class QGroupBox ;
class QPushButton ;
class QSpinBox ;
class QComboBox ;
QT_END_NAMESPACE

#include <cstddef>
#include <string>
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"

namespace ZMap
{

namespace GUI
{

class ZGuiTrackFilterDialogue : public QDialog,
        public Util::InstanceCounter<ZGuiTrackFilterDialogue>,
        public Util::ClassName<ZGuiTrackFilterDialogue>
{

    Q_OBJECT

public:
    static ZGuiTrackFilterDialogue* newInstance(QWidget *parent=Q_NULLPTR) ;

    ~ZGuiTrackFilterDialogue() ;

public slots:
    void slot_unfilter() ;

private:

    explicit ZGuiTrackFilterDialogue(QWidget* parent=Q_NULLPTR) ;
    ZGuiTrackFilterDialogue(const ZGuiTrackFilterDialogue& ) = delete ;
    ZGuiTrackFilterDialogue& operator=(const ZGuiTrackFilterDialogue&) = delete ;

    static const QString m_display_name ;
    static const int m_bases_to_match_min,
        m_bases_to_match_max ;

    QWidget *createBox1() ;
    QWidget *createBox2() ;
    QWidget *createBox3() ;
    QComboBox * createComboBox1() ;
    QComboBox * createComboBox2() ;
    QComboBox * createComboBox3() ;
    QComboBox * createComboBox4() ;
    QComboBox * createComboBox5() ;
    QComboBox * createComboBox6() ;

    QVBoxLayout *m_top_layout ;
    QGroupBox *m_box1, *m_box2, *m_box3 ;
    QLabel *m_top_label ;
    QSpinBox *m_bases_to_match ;
    QPushButton *m_button_set_to_current,
        *m_button_filter_all,
        *m_button_cancel,
        *m_button_unfilter,
        *m_button_filter;
    QComboBox *m_combo_box1,
        *m_combo_box2,
        *m_combo_box3,
        *m_combo_box4,
        *m_combo_box5,
        *m_combo_box6 ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGUITRACKFILTERDIALOGUE_H
