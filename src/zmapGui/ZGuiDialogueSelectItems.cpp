/*  File: ZGuiDialogueSelectItems.cpp
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

#include "ZGuiDialogueSelectItems.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QTableView>
#include <QHeaderView>
#include <QCheckBox>
#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGuiDialogueSelectItems>::m_name("ZGuiDialogueSelectItems") ;
const QString ZGuiDialogueSelectItems::m_display_name("ZMap Select Items") ;

ZGuiDialogueSelectItems::ZGuiDialogueSelectItems(QWidget *parent)
    : QDialog(parent),
      Util::InstanceCounter<ZGuiDialogueSelectItems>(),
      Util::ClassName<ZGuiDialogueSelectItems>(),
      m_top_layout(Q_NULLPTR),
      m_search_edit(Q_NULLPTR),
      m_search_case(Q_NULLPTR),
      m_button_cancel(Q_NULLPTR),
      m_button_OK(Q_NULLPTR),
      m_data_model(Q_NULLPTR),
      m_data_view(Q_NULLPTR),
      m_data_selection(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::ZGuiDialogueSelectItems() ; cannot create instance without QApplication existing ")) ;

    // now to top level layout
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::ZGuiDialogueSelectItems() ; unable to create top layout ")) ;
    setLayout(m_top_layout) ;

    // first control box
    QWidget *control_box = Q_NULLPTR ;
    if (!(control_box = createControls01()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::ZGuiDialogueSelectItems() ; unable to create top control box")) ;
    m_top_layout->addWidget(control_box) ;
    control_box = Q_NULLPTR ;

    // model for table of data
    if (!(m_data_model = ZGuiWidgetCreator::createStandardItemModel()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::ZGuiDialogueSelectItems() ; unable to create standard item model ")) ;
    m_data_model->setParent(this) ;

    // view of data
    if (!(m_data_view = ZGuiWidgetCreator::createTableView()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::ZGuiDialogueSelectItems() ; unable to create table view ")) ;
    m_data_view->setModel(m_data_model) ;
    m_data_view->resizeRowsToContents() ;
    m_data_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed) ;
    m_data_view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed) ;
    m_data_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;
    m_data_view->setSizeAdjustPolicy(QTableView::SizeAdjustPolicy::AdjustToContents);
    m_top_layout->addWidget(m_data_view) ;

    // selection model
    if (!(m_data_selection = ZGuiWidgetCreator::createItemSelectionModel()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::ZGuiDialogueSelectItems() ; unable to create item selection model ")) ;
    m_data_selection->setModel(m_data_model) ;
    m_data_selection->setParent(this) ;
    m_data_view->setSelectionModel(m_data_selection) ;

    // second control box
    if (!(control_box = createControls02()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::ZGuiDialogueSelectItems() ; unable to create second control box ")) ;
    m_top_layout->addWidget(control_box) ;

    // final window settings
    setWindowTitle(m_display_name) ;

}

ZGuiDialogueSelectItems::~ZGuiDialogueSelectItems()
{
}

ZGuiDialogueSelectItems* ZGuiDialogueSelectItems::newInstance(QWidget *parent)
{
    ZGuiDialogueSelectItems * selector = Q_NULLPTR ;

    try
    {
        selector = new ZGuiDialogueSelectItems(parent) ;
    }
    catch (...)
    {
        selector = Q_NULLPTR ;
    }

    return selector ;
}

void ZGuiDialogueSelectItems::closeEvent(QCloseEvent *)
{
    emit signal_received_close_event() ;
}


QWidget* ZGuiDialogueSelectItems::createControls01()
{
    QWidget* widget = Q_NULLPTR ;
    QGridLayout* layout = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls01() ; unable to create widget ")) ;
    if (!(layout = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls01() ; unable to create layout ")) ;
    widget->setLayout(layout) ;

    int row = 0 ;
    QLabel *label = Q_NULLPTR ;
    if (!(label =  ZGuiWidgetCreator::createLabel(QString("Search:"))))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls01() ; unable to create search label ")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_search_edit = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls01() ; unable to create search line edit ")) ;
    layout->addWidget(m_search_edit, row, 1, 1, 1 ) ;
    if (!connect(m_search_edit, SIGNAL(textChanged(QString)), this, SLOT(slot_search_box_text_changed(QString))))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls01() ; unable to connect search line edit to slot ")) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Case sensitive:"))))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls01() ; unable to create case sensitive label ")) ;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_search_case = ZGuiWidgetCreator::createCheckBox()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls01() ; unable to create case check box " )) ;
    if (!connect(m_search_case, SIGNAL(stateChanged(int)), this, SLOT(slot_case_sensitivity_changed())))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls019) ; unable to connect case check box to slot ")) ;
    layout->addWidget(m_search_case, row, 1, 1, 1, Qt::AlignLeft ) ;
    m_search_case->setChecked(false);

    return widget ;
}


QWidget* ZGuiDialogueSelectItems::createControls02()
{
    QWidget *widget = Q_NULLPTR ;
    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls02() ; unable to create widget ")) ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls02() ; unable to create layout ")) ;
    widget->setLayout(layout) ;
    if (!(m_button_cancel = ZGuiWidgetCreator::createPushButton(QString("Cancel"))))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls02() ; unable to create cancel button ")) ;
    if (!connect(m_button_cancel, SIGNAL(clicked(bool)), this, SLOT(reject())))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls02() ; unable to connect cancel button to slot ")) ;
    layout->addWidget(m_button_cancel, 0, Qt::AlignLeft) ;
    layout->addStretch(100) ;
    if (!(m_button_OK = ZGuiWidgetCreator::createPushButton(QString("OK"))))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls02() ; unable to create OK button ")) ;
    if (!connect(m_button_OK, SIGNAL(clicked(bool)), this, SLOT(accept())))
        throw std::runtime_error(std::string("ZGuiDialogueSelectItems::createControls02() ; unable to connect OK button to slot ")) ;
    layout->addWidget(m_button_OK, 0, Qt::AlignRight) ;

    return widget ;
}

// perhaps should also have the option to do this on construction...
void ZGuiDialogueSelectItems::setData(const QStringList & data)
{
    if (m_data_model)
    {
        int i = 0 ;
        m_data_model->setRowCount(data.size()) ;
        QStandardItem * item = Q_NULLPTR ;
        for (QStringList::const_iterator it = data.begin() ; it != data.end() ; ++it, ++i )
        {
            if ((item = ZGuiWidgetCreator::createStandardItem(*it)))
            {
                item->setEditable(false) ;
                m_data_model->setItem(i, 0, item ) ;
            }
        }
    }
}


// extract the selection; we only want items that are not hidden
QStringList ZGuiDialogueSelectItems::getSelectedData() const
{
    QStringList selected_items ;

    if (m_data_selection && m_data_model)
    {
        QModelIndexList indices = m_data_selection->selectedRows() ;
        QStandardItem *the_item = Q_NULLPTR ;
        int row_index = 0 ;
        for (QModelIndexList::const_iterator it = indices.begin() ; it != indices.end() ; ++it )
        {
            row_index = it->row() ;
            if ((the_item = m_data_model->item(row_index, 0)) && !m_data_view->isRowHidden(it->row()))
                selected_items.append(the_item->text()) ;
        }
    }

#ifndef QT_NO_DEBUG
    qDebug() << "selected items: " ;
    for (QStringList::const_iterator it=selected_items.begin() ; it!=selected_items.end() ; ++it)
        qDebug() << *it ;
#endif

    return selected_items ;
}

QStringList ZGuiDialogueSelectItems::getData() const
{
    QStringList list ;

    if (m_data_model)
    {
        for (int i=0; i<m_data_model->rowCount() ; ++i)
        {
            QStandardItem *item = m_data_model->item(i) ;
            if (item)
            {
                list.append(item->text()) ;
            }
        }
    }

    return list ;
}



////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiDialogueSelectItems::slot_close()
{
    emit signal_received_close_event() ;
}

// note comments below!
void ZGuiDialogueSelectItems::slot_search_box_text_changed(const QString&data)
{
    if (m_data_model && m_search_edit && m_search_case )
    {
        QStandardItem * the_item = Q_NULLPTR ;
        QString item_text ;
        Qt::CaseSensitivity sense = m_search_case->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive ;
        for (int i=0; i<m_data_model->rowCount() ; ++i)
        {
            the_item = m_data_model->item(i) ;
            if (the_item)
            {
                item_text = the_item->text() ;
                if (item_text.contains(data, sense))
                    m_data_view->showRow(i) ;
                else
                {
                    // this will deselect items that are selected if they do not match
                    // the search criterion and _then_ hide them... they will not be
                    // selected if they are shown again...
                    //QModelIndex index = m_data_model->indexFromItem(the_item) ;
                    //m_data_selection->select(index, QItemSelectionModel::Deselect) ;
                    m_data_view->hideRow(i) ;
                }
            }
        }

    }
}

void ZGuiDialogueSelectItems::slot_case_sensitivity_changed()
{
    if (m_data_model && m_search_edit && m_search_case)
    {
        slot_search_box_text_changed(m_search_edit->text()) ;
    }
}


} // namespace GUI

} // namespace ZMap

