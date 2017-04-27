/*  File: ZGuiFileImportDialogue.cpp
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

#include "ZGuiFileImportDialogue.h"
#include "ZGuiFileImportStrandValidator.h"
#include "ZGuiCoordinateValidator.h"
#include "ZGuiWidgetComboStrand.h"
#include "ZGuiFileChooser.h"
#include "ZGuiWidgetCreator.h"
#include <memory>
#include <stdexcept>
#include <sstream>
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFontMetrics>
#include <QLineEdit>
#include <QCheckBox>
#include <QValidator>
#include <QErrorMessage>
#include <QMessageBox>
#include <QFileDialog>
#include <QFrame>

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif


namespace ZMap
{

namespace GUI
{

const QString
    ZGuiFileImportDialogue::m_display_name("ZMap File Import"),
    ZGuiFileImportDialogue::m_current_directory("."),
    ZGuiFileImportDialogue::m_error_invalid_input("ZGuiFileImportDialogue_error_invalid_input"),
    ZGuiFileImportDialogue::m_error_number_chosen("ZGuiFileImportDialogue_error_invalid_input"),
    ZGuiFileImportDialogue::m_sequence_title("ZMap sequence"),
    ZGuiFileImportDialogue::m_sequence_label1("Sequence"),
    ZGuiFileImportDialogue::m_sequence_label2("Dataset"),
    ZGuiFileImportDialogue::m_sequence_label3("Start"),
    ZGuiFileImportDialogue::m_sequence_label4("End"),
    ZGuiFileImportDialogue::m_import_data_title("Import data"),
    ZGuiFileImportDialogue::m_import_label1("Resource"),
    ZGuiFileImportDialogue::m_import_label2("Sequence"),
    ZGuiFileImportDialogue::m_import_label3("Start"),
    ZGuiFileImportDialogue::m_import_label4("End"),
    ZGuiFileImportDialogue::m_import_label5("Assembly"),
    ZGuiFileImportDialogue::m_import_label6("Source"),
    ZGuiFileImportDialogue::m_import_label7("Strand"),
    ZGuiFileImportDialogue::m_import_label8("Map features");
template <>
const std::string Util::ClassName<ZGuiFileImportDialogue>::m_name("ZGuiFileImportDialogue") ;

ZGuiFileImportDialogue::ZGuiFileImportDialogue(QWidget *parent)
    : QDialog(parent),
      Util::InstanceCounter<ZGuiFileImportDialogue>(),
      Util::ClassName<ZGuiFileImportDialogue>(),
      m_user_home(),
      m_top_layout(Q_NULLPTR),
      m_button_ok(Q_NULLPTR),
      m_button_cancel(Q_NULLPTR),
      m_sequence_edit1(Q_NULLPTR),
      m_sequence_edit2(Q_NULLPTR),
      m_sequence_edit3(Q_NULLPTR),
      m_sequence_edit4(Q_NULLPTR),
      m_import_edit1(Q_NULLPTR),
      m_import_edit2(Q_NULLPTR),
      m_import_edit3(Q_NULLPTR),
      m_import_edit4(Q_NULLPTR),
      m_import_edit5(Q_NULLPTR),
      m_import_edit6(Q_NULLPTR),
      m_combo_strand(Q_NULLPTR),
      m_import_button1(Q_NULLPTR),
      m_import_check1(Q_NULLPTR),
      m_import_strand_validator(Q_NULLPTR),
      m_import_start_validator(Q_NULLPTR),
      m_import_end_validator(Q_NULLPTR),
      m_error_message(Q_NULLPTR)
{
    // top level layout
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; could not create top layout object")) ;
    setLayout(m_top_layout) ;

    // sequence data box
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = createControls01()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; call to createControls01() failed ")) ;
    m_top_layout->addWidget(widget) ;

    // import data box
    if (!(widget = createControls02()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; call to createControls02() failed ")) ;
    m_top_layout->addWidget(widget) ;

    // add a separator
    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // OK/Cancel button layout
    if (!(widget = createControls03()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; call to createControls03() failed ")) ;
    m_top_layout->addWidget(widget) ;
    m_top_layout->addStretch(100) ;

    // error message thing
    if (!(m_error_message = ZGuiWidgetCreator::createErrorMessage(this)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; could not create error message object ")) ;

    // other window properties
    setWindowTitle(m_display_name) ;
}

ZGuiFileImportDialogue::~ZGuiFileImportDialogue()
{
}

ZGuiFileImportDialogue* ZGuiFileImportDialogue::newInstance(QWidget*parent)
{
    ZGuiFileImportDialogue *thing = Q_NULLPTR ;

    try
    {
        thing = new ZGuiFileImportDialogue(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

// create the group box for the sequence data
QWidget * ZGuiFileImportDialogue::createControls01()
{

    QGroupBox* sequence_box = Q_NULLPTR ;
    if (!(sequence_box = ZGuiWidgetCreator::createGroupBox(m_sequence_title)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createSequenceBox() ; unable to create sequence group box ")) ;

    QGridLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createSequenceBox() ; unable to create grid layout ")) ;

    QLabel *label = Q_NULLPTR ;
    int row = 0 ;

    if (!(label = ZGuiWidgetCreator::createLabel(m_sequence_label1)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createSequenceBox() ; unable to create sequence label ")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_sequence_edit1 = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createSequenceBox() ; unable to create sequence line edit ")) ;
    layout->addWidget(m_sequence_edit1, row, 1) ;
    m_sequence_edit1->setEnabled(false);
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(m_sequence_label2)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createSequenceBox() ; unable to create sequence label2 ")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_sequence_edit2 = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createSequenceBox() ; unable to create sequence 2 line edit ")) ;
    layout->addWidget(m_sequence_edit2, row, 1) ;
    m_sequence_edit2->setEnabled(false) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(m_sequence_label3)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createSequenceBox() ; unable to create sequence label 3")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_sequence_edit3 = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createSequenceBox() ; unable to create sequence line edit 3 ")) ;
    layout->addWidget(m_sequence_edit3, row, 1) ;
    m_sequence_edit3->setEnabled(false) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(m_sequence_label4)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createSequenceBox() ; unable to create sequence label 4")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_sequence_edit4 = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createSequenceBox() ; unable to create sequence line edit 4 ")) ;
    layout->addWidget(m_sequence_edit4, row, 1) ;
    m_sequence_edit4->setEnabled(false) ;
    ++row ;

    sequence_box->setLayout(layout) ;


    return sequence_box ;
}


QWidget* ZGuiFileImportDialogue::createControls02()
{
    QGroupBox * import_data_box = Q_NULLPTR ;
    if (!(import_data_box = ZGuiWidgetCreator::createGroupBox(m_import_data_title)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; could not create import_data_box ")) ;

    QGridLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; could not create grid layout ")) ;

    QLabel *label = Q_NULLPTR ;
    int row = 0 ;

    if (!(m_import_button1 = ZGuiWidgetCreator::createPushButton(m_import_label1)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import button 1 ")) ;
    connect(m_import_button1, SIGNAL(clicked(bool)), this, SLOT(slot_file_chooser())) ;
    layout->addWidget(m_import_button1, row, 0, Qt::AlignRight) ;
    if (!(m_import_edit1 = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create line edit ")) ;
    layout->addWidget(m_import_edit1, row, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(m_import_label2)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import label 2 ")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_import_edit2 = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import edit 2 ")) ;
    layout->addWidget(m_import_edit2, row, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(m_import_label3)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import label 3")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_import_edit3 = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import edit 3 ")) ;
    if (!(m_import_start_validator = ZGuiCoordinateValidator::newInstance()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import start validator ")) ;
    m_import_start_validator->setParent(this) ;
    m_import_edit3->setValidator(m_import_start_validator) ;
    layout->addWidget(m_import_edit3, row, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(m_import_label4)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import label 4 ")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_import_edit4 = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import line edit 4")) ;
    if (!(m_import_end_validator = ZGuiCoordinateValidator::newInstance()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import end validator ")) ;
    m_import_end_validator->setParent(this) ;
    m_import_edit4->setValidator(m_import_end_validator) ;
    layout->addWidget(m_import_edit4, row, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(m_import_label5)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import label 5 ")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_import_edit5 = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import line edit 5")) ;
    layout->addWidget(m_import_edit5, row, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(m_import_label6)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import label 6")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_import_edit6 = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import line edit 6")) ;
    layout->addWidget(m_import_edit6, row, 1) ;
    ++row ;

    // strand
    if (!(label = ZGuiWidgetCreator::createLabel(m_import_label7)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import label 7")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_combo_strand = ZGuiWidgetComboStrand::newInstance()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox(); unable to create strand combo box ")) ;
    m_combo_strand->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) ;
    layout->addWidget(m_combo_strand, row, 1) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(m_import_label8)))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import label 8")) ;
    layout->addWidget(label, row, 0, Qt::AlignRight) ;
    if (!(m_import_check1 = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createImportDataBox() ; unable to create import check box ")) ;
    m_import_check1->setCheckState(Qt::Unchecked) ;
    layout->addWidget(m_import_check1, row, 1) ;
    ++row ;

    import_data_box->setLayout(layout) ;


    return import_data_box ;
}


QWidget *ZGuiFileImportDialogue::createControls03()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::createControls03() ; unable to create widget ")) ;

    // OK/Cancel button layout
    QHBoxLayout *button_layout = Q_NULLPTR ;
    if (!(button_layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; could not create button layout object")) ;
    widget->setLayout(button_layout) ;
    button_layout->setMargin(0) ;

    // OK button
    if (!(m_button_ok = ZGuiWidgetCreator::createPushButton(QString("OK"))))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; could not create OK button")) ;
    m_button_ok->setDefault(true) ;
    button_layout->addWidget(m_button_ok) ;
    button_layout->addStretch(100) ;

    // cancel button
    if (!(m_button_cancel = ZGuiWidgetCreator::createPushButton(QString("Cancel"))))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; could not create cancel button")) ;
    button_layout->addWidget(m_button_cancel) ;

    // connect up the accept/reject events
    if (!connect(m_button_ok, SIGNAL(clicked(bool)), this, SLOT(slot_user_accept())))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; could not make connection on ok button")) ;
    if (!connect(m_button_cancel, SIGNAL(clicked(bool)), this, SLOT(reject())))
        throw std::runtime_error(std::string("ZGuiFileImportDialogue::ZGuiFileImportDialogue() ; could not make connection on cancel button")) ;

    return widget ;
}

// validate the data in the file input section; basic checking only...
bool ZGuiFileImportDialogue::isValid() const
{
    bool result = true ;

    if (m_import_edit1)
    {
        result = static_cast<bool>(m_import_edit1->text().length()) ;
    }

    if (result && m_import_edit2)
    {
        result = static_cast<bool>(m_import_edit2->text().length()) ;
    }

    // start and end coordinates
    if (result &&
            m_import_edit3 &&
            m_import_edit4 &&
            m_import_edit3->hasAcceptableInput() &&
            m_import_edit4->hasAcceptableInput())
    {
        uint32_t start = m_import_edit3->text().toUInt(),
                end = m_import_edit4->text().toUInt() ;
        if ((start > end))
            result = false ;
    }
    else
        result = false ;

    return result ;
}

bool ZGuiFileImportDialogue::setSequenceData(const ZGSequenceData& data)
{
    bool result = false ;

    if (m_sequence_edit1)
        m_sequence_edit1->setText(QString::fromStdString(data.getSequenceName())) ;
    if (m_sequence_edit2)
        m_sequence_edit2->setText(QString::fromStdString(data.getDatasetName())) ;
    if (m_sequence_edit3)
    {
        std::stringstream str ;
        str << data.getStart() ;
        m_sequence_edit3->setText(QString::fromStdString(str.str())) ;
    }
    if (m_sequence_edit4)
    {
        std::stringstream str ;
        str << data.getEnd() ;
        m_sequence_edit3->setText(QString::fromStdString(str.str())) ;
    }

    return result ;
}


bool ZGuiFileImportDialogue::setResourceImportData(const ZGResourceImportData& data)
{
    if (m_import_edit1)
        m_import_edit1->setText(QString::fromStdString(data.getResourceName())) ;
    if (m_import_edit2)
        m_import_edit2->setText(QString::fromStdString(data.getSequenceName())) ;
    if (m_import_edit3)
    {
        std::stringstream str ;
        str << data.getStart() ;
        m_import_edit3->setText(QString::fromStdString(str.str())) ;
    }
    if (m_import_edit4)
    {
        std::stringstream str ;
        str << data.getEnd() ;
        m_import_edit4->setText(QString::fromStdString(str.str())) ;
    }
    if (m_import_edit5)
        m_import_edit5->setText(QString::fromStdString(data.getAssemblyName())) ;
    if (m_import_edit6)
        m_import_edit6->setText(QString::fromStdString(data.getSourceName())) ;

    if (m_combo_strand)
    {
        m_combo_strand->setStrand(data.getStrand()) ;
    }

    if (m_import_check1)
        m_import_check1->setChecked(data.getMapFlag()) ;

    return true ;
}


ZGSequenceData ZGuiFileImportDialogue::getSequenceData() const
{
    ZGSequenceData data ;

    if (m_sequence_edit1)
        data.setSequenceName(m_sequence_edit1->text().toStdString()) ;

    if (m_sequence_edit2)
        data.setDatasetName(m_sequence_edit2->text().toStdString()) ;

    if (m_sequence_edit3)
        data.setStart(m_sequence_edit3->text().toUInt()) ;

    if (m_sequence_edit4)
        data.setEnd(m_sequence_edit4->text().toUInt()) ;

    return data ;
}

ZGResourceImportData ZGuiFileImportDialogue::getResourceImportData() const
{
    ZGResourceImportData data ;
    if (!isValid())
        return data ;

    if (m_import_edit1)
        data.setResourceName(m_import_edit1->text().toStdString()) ;

    if (m_import_edit2)
        data.setSequenceName(m_import_edit2->text().toStdString()) ;

    if (m_import_edit3)
        data.setStart(m_import_edit3->text().toUInt()) ;

    if (m_import_edit4)
        data.setEnd(m_import_edit4->text().toUInt()) ;

    if (m_import_edit5)
        data.setAssemblyName(m_import_edit5->text().toStdString()) ;

    if (m_import_edit6)
        data.setSourceName(m_import_edit6->text().toStdString()) ;

    if (m_combo_strand)
    {
        data.setStrand(m_combo_strand->getStrand()) ;
    }

    if (m_import_check1)
        data.setMapFlag(m_import_check1->isChecked()) ;

    return data ;
}

void ZGuiFileImportDialogue::setUserHome(const std::string& data)
{
    m_user_home = data ;
}

////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////


// user chooses to browse for resource name
void ZGuiFileImportDialogue::slot_file_chooser()
{
    QString user_home = m_user_home.length() ? QString::fromStdString(m_user_home) : m_current_directory ;
    std::unique_ptr<ZGuiFileChooser> file_chooser(ZGuiFileChooser::newInstance(Q_NULLPTR, QString("ZMap: select a file"), user_home)) ;
    if (file_chooser->exec() == QDialog::Accepted)
    {
        QStringList file_names = file_chooser->selectedFiles() ;
        if ((file_names.size() == 1) && m_import_edit1)
        {
            m_import_edit1->setText(file_names.at(0)) ;
        }
        else if (m_error_message)
        {
            m_error_message->showMessage(QString("Choose one file only."), m_error_number_chosen) ;
        }
    }
}

// called when the user accepts
void ZGuiFileImportDialogue::slot_user_accept()
{
    if (!isValid() && m_error_message)
    {
        // this is always on top, but does not block this
        m_error_message->showMessage(QString("Data given are not valid."), m_error_invalid_input) ;
        // could do this instead, but it blocks
//        QMessageBox box ;
//        box.setText("Data given are not valid.") ;
//        box.exec() ;
    }
    else
    {
        emit accept() ;
    }
}

} // namespace GUI

} // namespace ZMap
