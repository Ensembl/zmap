/*  File: ZGuiFileImportDialogue.h
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

#ifndef ZGUIFILEIMPORTDIALOGUE_H
#define ZGUIFILEIMPORTDIALOGUE_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGResourceImportData.h"
#include "ZGSequenceData.h"
#include <cstddef>
#include <string>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QVBoxLayout ;
class QHBoxLayout ;
class QPushButton ;
class QLabel ;
class QGroupBox ;
class QLineEdit ;
class QCheckBox ;
class QValidator ;
class QErrorMessage ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

class ZGuiFileImportStrandValidator ;
class ZGuiCoordinateValidator ;
class ZGuiWidgetComboStrand ;

//
// This is a relatively simple dialogue that does not
// require menu/tool/status facilities.
//

class ZGuiFileImportDialogue : public QDialog,
        public Util::InstanceCounter<ZGuiFileImportDialogue>,
        public Util::ClassName<ZGuiFileImportDialogue>
{
    Q_OBJECT

public:
    static ZGuiFileImportDialogue* newInstance(QWidget* parent = Q_NULLPTR ) ;

    ~ZGuiFileImportDialogue() ;

    void setUserHome(const std::string& data) ;
    std::string getUserHome() const {return m_user_home ; }

    bool isValid() const ;

    bool setSequenceData(const ZGSequenceData& data) ;
    ZGSequenceData getSequenceData() const ;

    bool setResourceImportData(const ZGResourceImportData & data) ;
    ZGResourceImportData getResourceImportData() const ;

public slots:
    void slot_file_chooser() ;

private slots:
    void slot_user_accept() ;

private:

    explicit ZGuiFileImportDialogue(QWidget* parent=Q_NULLPTR) ;
    ZGuiFileImportDialogue(const ZGuiFileImportDialogue& ) = delete ;
    ZGuiFileImportDialogue& operator=(const ZGuiFileImportDialogue&) = delete ;

    static const QString m_display_name,
        m_current_directory,
        m_error_invalid_input,
        m_error_number_chosen,
        m_sequence_title,
        m_sequence_label1,
        m_sequence_label2,
        m_sequence_label3,
        m_sequence_label4,
        m_import_data_title,
        m_import_label1,
        m_import_label2,
        m_import_label3,
        m_import_label4,
        m_import_label5,
        m_import_label6,
        m_import_label7,
        m_import_label8;

    QWidget* createControls01() ;
    QWidget* createControls02() ;
    QWidget* createControls03() ;

    std::string m_user_home ;

    QVBoxLayout *m_top_layout ;
    QPushButton *m_button_ok,
        *m_button_cancel ;
    QLineEdit *m_sequence_edit1,
        *m_sequence_edit2,
        *m_sequence_edit3,
        *m_sequence_edit4,
        *m_import_edit1,
        *m_import_edit2,
        *m_import_edit3,
        *m_import_edit4,
        *m_import_edit5,
        *m_import_edit6;
    ZGuiWidgetComboStrand *m_combo_strand ;
    QPushButton *m_import_button1 ;
    QCheckBox *m_import_check1 ;
    ZGuiFileImportStrandValidator *m_import_strand_validator ;
    ZGuiCoordinateValidator *m_import_start_validator,
        *m_import_end_validator ;
    QErrorMessage *m_error_message ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIFILEIMPORTDIALOGUE_H
