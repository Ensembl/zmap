/*  File: ZGuiWidgetSelectSequence.cpp
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

#include "ZGuiWidgetSelectSequence.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiCoordinateValidator.h"
#include "Utilities.h"
#include <stdexcept>
#include <sstream>
#include <utility>
#include <set>

#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QFrame>
#include <QTableView>
#include <QHeaderView>
#include <QList>
#include <QStringList>
#include <QVariant>
#include <QStandardItem>
#include <QStandardItemModel>

Q_DECLARE_METATYPE(ZMap::GUI::ZGSourceType)

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetSelectSequence>::m_name("ZGuiWidgetSelectSequence") ;
const QStringList ZGuiWidgetSelectSequence::m_model_column_titles =
{
    QString("Name"),
    QString("Type")
} ;
const int ZGuiWidgetSelectSequence::m_model_column_number(2) ;
const int ZGuiWidgetSelectSequence::m_sourcetype_id(qMetaTypeId<ZGSourceType>()) ;

ZGuiWidgetSelectSequence::ZGuiWidgetSelectSequence(QWidget *parent)
    : QWidget(parent),
      Util::InstanceCounter<ZGuiWidgetSelectSequence>(),
      Util::ClassName<ZGuiWidgetSelectSequence>(),
      m_top_layout(Q_NULLPTR),
      m_sources_view(Q_NULLPTR),
      m_sources_model(Q_NULLPTR),
      m_entry_sequence(Q_NULLPTR),
      m_entry_start(Q_NULLPTR),
      m_entry_end(Q_NULLPTR),
      m_entry_config(Q_NULLPTR),
      m_button_new(Q_NULLPTR),
      m_button_remove(Q_NULLPTR),
      m_button_save(Q_NULLPTR),
      m_button_load(Q_NULLPTR),
      m_button_default(Q_NULLPTR),
      m_button_start(Q_NULLPTR),
      m_button_close_selected(Q_NULLPTR),
      m_button_quit_all(Q_NULLPTR),
      m_start_validator(Q_NULLPTR),
      m_end_validator(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::ZGuiWidgetSelectSequence() ; unable to create without instance of QApplicatoin existing")) ;

    // top level layout
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::ZGuiWidgetSelectSequence() ; unable to create top layout ")) ;
    setLayout(m_top_layout) ;

    // first set of controls
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = createControls01()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::ZGuiWidgetSelectSequence() ; unable to create object from createControls01() ")) ;
    m_top_layout->addWidget(widget) ;

    // separator
    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::ZGuiWidgetSelectSequence() ; unable to create frame as separator ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // second set of controls
    QHBoxLayout *hlayout = Q_NULLPTR ;
    if (!(hlayout = createControls04()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::ZGuiWidgetSelectSequence() ; unable to create object from createControls04() ")) ;
    m_top_layout->addLayout(hlayout) ;
}

ZGuiWidgetSelectSequence::~ZGuiWidgetSelectSequence()
{
}


QWidget* ZGuiWidgetSelectSequence::createControls01()
{
    QWidget* widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls01() ; unable to create widget ")) ;

    QVBoxLayout *vlayout = Q_NULLPTR ;
    if (!(vlayout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls01() ; unable to create vlayout ")) ;
    widget->setLayout(vlayout) ;

    // hbox to contain our two sets of controls
    QHBoxLayout * hlayout = Q_NULLPTR ;
    if (!(hlayout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls01() ; unable to create hlayout ")) ;
    vlayout->addLayout(hlayout) ;

    // box 1
    QWidget *box = Q_NULLPTR ;
    if (!(box = createControls02()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls01() ; unable to create object from createControls02()")) ;
    hlayout->addWidget(box) ;

    // box 2
    if (!(box = createControls03()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls01() ; unable to create object from createControls03()")) ;
    hlayout->addWidget(box) ;

    return widget ;
}

// box 1
QWidget* ZGuiWidgetSelectSequence::createControls02()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createGroupBox(QString("Sources:"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls02() ; unable to create group box ")) ;

    QVBoxLayout *vlayout = Q_NULLPTR ;
    if (!(vlayout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls02() ; unable to create vlayout ")) ;
    widget->setLayout(vlayout) ;

    if (!(m_sources_model = ZGuiWidgetCreator::createStandardItemModel()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls02() ; unable to create sources standard item model ")) ;
    m_sources_model->setParent(this) ;
    m_sources_model->setColumnCount(m_model_column_number) ;
    m_sources_model->setHorizontalHeaderLabels(m_model_column_titles) ;

    if (!(m_sources_view = ZGuiWidgetCreator::createTableView()) )
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls02() ; unable to create sources table view ")) ;
    m_sources_view->setModel(m_sources_model) ;
    vlayout->addWidget(m_sources_view) ;


    m_sources_view->resizeColumnsToContents() ;
    m_sources_view->resizeRowsToContents();
    m_sources_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;
    m_sources_view->setSizeAdjustPolicy(QTableView::SizeAdjustPolicy::AdjustToContents);

    return widget ;
}

// box 2
QWidget* ZGuiWidgetSelectSequence::createControls03()
{
    QWidget* widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createGroupBox()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create group box ")) ;

    QGridLayout *glayout = Q_NULLPTR ;
    QVBoxLayout *vlayout = Q_NULLPTR ;

    if (!(widget = ZGuiWidgetCreator::createGroupBox(QString("Sequence:"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create sequence group box ")) ;
    if (!(vlayout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create vlayout ")) ;
    if (!(glayout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create glayout ")) ;

    widget->setLayout(vlayout) ;
    vlayout->addLayout(glayout) ;

    QLabel * label = Q_NULLPTR ;
    int row = 0 ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Sequence:"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create sequence label ")) ;
    if (!(m_entry_sequence = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03(); unable to create sequence line edit ")) ;
    glayout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_entry_sequence, row, 1, 1, 1) ;
    ++row ;
    label = Q_NULLPTR ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Start:"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create start label ")) ;
    if (!(m_entry_start = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create start entry ")) ;
    glayout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_entry_start, row, 1, 1, 1) ;
    if (!(m_start_validator = ZGuiCoordinateValidator::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create start validator ")) ;
    m_start_validator->setParent(this) ;
    m_entry_start->setValidator(m_start_validator) ;
    ++row ;
    label = Q_NULLPTR ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("End:"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03(); unable to create end label ")) ;
    if (!(m_entry_end = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create end entry ")) ;
    glayout->addWidget(label, row, 0,1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_entry_end, row, 1, 1, 1) ;
    if (!(m_end_validator = ZGuiCoordinateValidator::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create end validator ")) ;
    m_end_validator->setParent(this) ;
    m_entry_end->setValidator(m_end_validator) ;
    ++row ;
    label = Q_NULLPTR ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Config file:"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create config label")) ;
    if (!(m_entry_config = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls03() ; unable to create config entry ")) ;
    glayout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    glayout->addWidget(m_entry_config, row, 1, 1, 1) ;
    ++row ;
    label = Q_NULLPTR ;

    vlayout->addStretch(100) ;

    return widget ;
}

// row of buttons
QHBoxLayout *ZGuiWidgetSelectSequence::createControls04()
{
    QHBoxLayout* hlayout = Q_NULLPTR ;

    if (!(hlayout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to create hlayout ")) ;

    if (!(m_button_new = ZGuiWidgetCreator::createPushButton(QString("New"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to create new button ")) ;
    if (!connect(m_button_new, SIGNAL(clicked(bool)), this, SLOT(slot_create_new_source())))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to connect new button to slot ")) ;
    hlayout->addWidget(m_button_new, 0, Qt::AlignLeft) ;
    m_button_new->setToolTip(QString("Create a new source ")) ;

    if (!(m_button_remove = ZGuiWidgetCreator::createPushButton(QString("Remove"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to create remove button ")) ;
    if (!connect(m_button_remove, SIGNAL(clicked(bool)), this, SLOT(slot_delete_selected_source())))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to connect remove button to slot ")) ;
    hlayout->addWidget(m_button_remove, 0, Qt::AlignLeft) ;
    m_button_remove->setToolTip(QString("Remove the selected source(s)")) ;

    if (!(m_button_save = ZGuiWidgetCreator::createPushButton(QString("Save"))))
        throw std::runtime_error(std::string("ZGuiToplevel::createControls02() ; unable to create save button ")) ;
    if (!connect(m_button_save, SIGNAL(clicked(bool)), this, SLOT(slot_save_to_config())))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to connect save button to slot ")) ;
    hlayout->addWidget(m_button_save, 0, Qt::AlignLeft) ;
    m_button_save->setToolTip(QString("Save the source and sequence details to a configuration file")) ;

    if (!(m_button_load = ZGuiWidgetCreator::createPushButton(QString("Load"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to create load button ")) ;
    if (!connect(m_button_load, SIGNAL(clicked(bool)), this, SLOT(slot_load_from_config())) )
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to connect load button to slot ")) ;
    hlayout->addWidget(m_button_load, 0, Qt::AlignLeft) ;
    m_button_load->setToolTip(QString("Load source/sequence details from a configuration file")) ;

    if (!(m_button_default = ZGuiWidgetCreator::createPushButton(QString("Set defaults"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to create set defaults button")) ;
    if (!connect(m_button_default, SIGNAL(clicked(bool)), this, SLOT(slot_set_from_default())))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to connect set defaults button to slot ")) ;
    hlayout->addWidget(m_button_default, 0, Qt::AlignLeft) ;
    m_button_load->setToolTip(QString("Set sequence details from default values")) ;

    if (!(m_button_start = ZGuiWidgetCreator::createPushButton(QString("Start ZMap"))))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to create start zmap button ")) ;
    if (!connect(m_button_start, SIGNAL(clicked(bool)), this, SLOT(slot_start_zmap())))
        throw std::runtime_error(std::string("ZGuiWidgetSelectSequence::createControls04() ; unable to connect start zmap button to slot ")) ;
    hlayout->addWidget(m_button_start, 0, Qt::AlignLeft) ;
    m_button_start->setToolTip(QString("Create a new ZMap View of the selected source/sequence")) ;

    hlayout->addStretch(100) ;

    return hlayout ;
}

ZGuiWidgetSelectSequence* ZGuiWidgetSelectSequence::newInstance()
{
    ZGuiWidgetSelectSequence *selector = Q_NULLPTR ;

    try
    {
        selector = new ZGuiWidgetSelectSequence ;
    }
    catch (...)
    {
        selector = Q_NULLPTR ;
    }

    return selector ;
}

bool ZGuiWidgetSelectSequence::setSelectSequenceData(const ZGSelectSequenceData &data)
{
    bool result = false ;

    std::set<std::pair<std::string, ZGSourceType> > sources = data.getSources() ;
    if (m_sources_model)
    {
        m_sources_model->setRowCount(0) ;

        for (auto it = sources.begin() ; it != sources.end() ; ++it )
        {
            QList<QStandardItem*> item_list ;
            QStandardItem *item = Q_NULLPTR ;
            std::stringstream str ;
            str << it->second ;
            try
            {
                if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(it->first))))
                {
                    // also at some point want to set the source data as a
                    // QVariant on this item...
                    item->setEditable(false) ;
                    item_list.append(item) ;
                }
                if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(str.str()))))
                {
                    QVariant var ;
                    var.setValue(it->second) ;
                    item->setData(var) ;
                    item->setEditable(false) ;
                    item_list.append(item) ;
                }
                m_sources_model->appendRow(item_list) ;
            }
            catch (...)
            {

            }

        }
    }

    if (m_sources_view)
    {
        m_sources_view->resizeColumnsToContents() ;
    }

    if (m_entry_sequence)
    {
        m_entry_sequence->setText(QString::fromStdString(data.getSequence())) ;
    }

    if (m_entry_start)
    {
        std::stringstream str ;
        str << data.getStart() ;
        m_entry_start->setText(QString::fromStdString(str.str())) ;
    }

    if (m_entry_end)
    {
        std::stringstream str ;
        str << data.getEnd() ;
        m_entry_end->setText(QString::fromStdString(str.str())) ;
    }

    if (m_entry_config)
    {
        m_entry_config->setText(QString::fromStdString(data.getConfigFile())) ;
    }

    result = true ;

    return result ;
}

ZGSelectSequenceData ZGuiWidgetSelectSequence::getSelectSequenceData() const
{
    ZGSelectSequenceData data ;

    std::set<std::pair<std::string, ZGSourceType> > sources ;
    QStandardItem *item = Q_NULLPTR ;
    if (m_sources_model)
    {
        for (int iRow=0; iRow<m_sources_model->rowCount() ; ++iRow)
        {
            std::pair<std::string, ZGSourceType> source ;
            if ((item = m_sources_model->item(iRow, 0)))
            {
                source.first = item->text().toStdString() ;
            }

            if ((item = m_sources_model->item(iRow, 1)))
            {
                QVariant var = item->data() ;
                if (var.canConvert<ZGSourceType>() && var.convert(m_sourcetype_id))
                {
                    source.second = var.value<ZGSourceType>() ;
                }
            }

            sources.insert(source) ;
        }
    }
    data.setSources(sources) ;

    if (m_entry_sequence)
    {
        data.setSequence(m_entry_sequence->text().toStdString()) ;
    }

    if (m_entry_start)
    {
        uint32_t value = m_entry_start->text().toUInt() ;
        data.setStart(value) ;
    }

    if (m_entry_end)
    {
        uint32_t value = m_entry_end->text().toUInt() ;
        data.setEnd(value) ;
    }

    if (m_entry_config)
    {
        data.setConfigFile(m_entry_config->text().toStdString()) ;
    }

    return data ;
}



////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiWidgetSelectSequence::slot_create_new_source()
{
    emit signal_create_new_source() ;
}

void ZGuiWidgetSelectSequence::slot_delete_selected_source()
{
    emit signal_delete_selected_source();
}

void ZGuiWidgetSelectSequence::slot_save_to_config()
{
    emit signal_save_to_config();
}

void ZGuiWidgetSelectSequence::slot_load_from_config()
{
    emit signal_load_from_config();
}

void ZGuiWidgetSelectSequence::slot_set_from_default()
{
    emit signal_set_from_default();
}

void ZGuiWidgetSelectSequence::slot_start_zmap()
{
    emit signal_start_zmap();
}


} // namespace GUI

} // namespace ZMap


