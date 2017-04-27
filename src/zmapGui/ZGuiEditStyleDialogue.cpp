/*  File: ZGuiEditStyleDialogue.cpp
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


#include "ZGuiEditStyleDialogue.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiWidgetButtonColour.h"

#include <stdexcept>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListView>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QColorDialog>
#include <QCheckBox>
#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

namespace ZMap
{

namespace GUI
{

const QString ZGuiEditStyleDialogue::m_display_name("ZMap Edit Style") ;
template <> const std::string Util::ClassName<ZGuiEditStyleDialogue>::m_name("ZGuiEditStyleDialogue") ;

ZGuiEditStyleDialogue::ZGuiEditStyleDialogue(QWidget* parent)
    : QDialog(parent),
      Util::InstanceCounter<ZGuiEditStyleDialogue>(),
      Util::ClassName<ZGuiEditStyleDialogue>(),
      m_top_layout(Q_NULLPTR),
      m_new_name(Q_NULLPTR),
      m_original_name(Q_NULLPTR),
      m_featuresets_view(Q_NULLPTR),
      m_featuresets_model(Q_NULLPTR),
      m_checkbox_stranded(Q_NULLPTR),
      m_button_fill_col(Q_NULLPTR),
      m_button_border_col(Q_NULLPTR),
      m_button_create_child(Q_NULLPTR),
      m_button_revert(Q_NULLPTR),
      m_button_close(Q_NULLPTR),
      m_button_apply(Q_NULLPTR),
      m_button_OK(Q_NULLPTR)
{
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; unable to create top_layout ")) ;
    setLayout(m_top_layout) ;

    // now create h box for controls on left and data view on right
    QHBoxLayout *layout1 = Q_NULLPTR ;
    if (!(layout1 = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; unable to create layout1")) ;
    m_top_layout->addLayout(layout1, 100) ;

    // first box of controls
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = createControls01()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; call to createControls01() failed  ")) ;
    layout1->addWidget(widget) ;

    // second box of controls
    if (!(widget = createControls02()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; call to createControls02() failed ")) ;
    layout1->addWidget(widget) ;

    // third box of controls
    if (!(widget = createControls03()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; call to createControls03()  failed ")) ;
    m_top_layout->addWidget(widget) ;

    // window settings
    setWindowTitle(m_display_name) ;
    setAttribute(Qt::WA_DeleteOnClose) ;
}

ZGuiEditStyleDialogue::~ZGuiEditStyleDialogue()
{
}

ZGuiEditStyleDialogue* ZGuiEditStyleDialogue::newInstance(QWidget *parent)
{
    ZGuiEditStyleDialogue * edit = Q_NULLPTR;

    try
    {
        edit = new ZGuiEditStyleDialogue(parent) ;
    }
    catch (...)
    {
        edit = Q_NULLPTR ;
    }

    return edit ;
}

void ZGuiEditStyleDialogue::closeEvent(QCloseEvent*)
{
    emit signal_received_close_event() ;
}


QWidget* ZGuiEditStyleDialogue::createControls01()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create widget ")) ;

    QVBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    QGridLayout *grid = Q_NULLPTR ;
    if (!(grid = ZGuiWidgetCreator::createGridLayout()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create grid layout ")) ;

    int row = 0 ;
    QLabel *label = Q_NULLPTR ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("New name: "))))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create new name label  ; ")) ;
    if (!(m_new_name = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create new name line edit ")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_new_name, row, 1, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Original name: "))))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create original name label ")) ;
    if (!(m_original_name = ZGuiWidgetCreator::createLineEdit()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create original name line edit  ; ")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_original_name, row, 1, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(m_button_create_child = ZGuiWidgetCreator::createPushButton(QString("Create child style"))))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create child style button ")) ;
    if (!connect(m_button_create_child, SIGNAL(clicked(bool)), this, SLOT(slot_create_child())))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to connect child style button to appropriate slot ")) ;
    grid->addWidget(m_button_create_child, row, 1, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Fill colour: "))))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create fill color label ")) ;
    if (!(m_button_fill_col = ZGuiWidgetButtonColour::newInstance(QColor())))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create fill color button ")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_button_fill_col, row, 1, 1, 1, Qt::AlignLeft) ;
    ++row ;

    if (!(label = ZGuiWidgetCreator::createLabel(QString("Border colour: "))))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create border color label")) ;
    if (!(m_button_border_col = ZGuiWidgetButtonColour::newInstance(QColor())) )
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create border color button ")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    grid->addWidget(m_button_border_col, row, 1, 1, 1, Qt::AlignLeft) ;
    ++row ;

    //checkbox...
    if (!(m_checkbox_stranded = ZGuiWidgetCreator::createCheckBox(QString("Stranded"))))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls01() ; unable to create stranded checkbox ")) ;
    grid->addWidget(m_checkbox_stranded, row, 1, 1, 1, Qt::AlignLeft)  ;
    ++row ;

    // add the grid to the top level layout
    layout->addLayout(grid) ;
    layout->addStretch(100) ;

    return widget ;
}

QWidget * ZGuiEditStyleDialogue::createControls02()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls02() ; unable to create widget ")) ;

    QVBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls02() ; unable to create layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Featuresets:"))))
        throw std::runtime_error(std::string("")) ;
    layout->addWidget(label, 0, Qt::AlignLeft) ;

    if (!(m_featuresets_model = ZGuiWidgetCreator::createStandardItemModel()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls02() ; unable to create model ")) ;
    m_featuresets_model->setParent(this) ;

    if (!(m_featuresets_view = ZGuiWidgetCreator::createListView()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls02() ; unable to create model view")) ;

    // hook up the model and the view
    m_featuresets_view->setModel(m_featuresets_model) ;
    m_featuresets_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;

    layout->addWidget(m_featuresets_view) ;

    return widget  ;
}


QWidget * ZGuiEditStyleDialogue::createControls03()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::createControls03() ; unable to create widget ")) ;

    // third box of controls
    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; unable to create button_layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    if (!(m_button_revert = ZGuiWidgetCreator::createPushButton(QString("Revert"))))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; unable to create button_revert")) ;
    layout->addWidget(m_button_revert) ;
    if (!(m_button_close = ZGuiWidgetCreator::createPushButton(QString("Close"))))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; unable to create button_close")) ;
    layout->addWidget(m_button_close) ;
    if (!connect(m_button_close, SIGNAL(clicked(bool)), this, SLOT(slot_close())))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; unable to connect button_close")) ;
    if (!(m_button_apply = ZGuiWidgetCreator::createPushButton(QString("Apply"))))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; unable to create button_apply ")) ;
    layout->addWidget(m_button_apply) ;
    if (!(m_button_OK = ZGuiWidgetCreator::createPushButton(QString("OK"))))
        throw std::runtime_error(std::string("ZGuiEditStyleDialogue::ZGuiEditStyleDialogue() ; unable to create button_OK")) ;
    layout->addWidget(m_button_OK) ;
    layout->addStretch(100) ;

    return widget ;
}


// add a standard item to a standard item model, with optional text
bool ZGuiEditStyleDialogue::addStandardItem(QStandardItemModel* model, int i, int j, const QString& data)
{
    bool result = false ;

    if (!model || (i >= model->rowCount()) || (j >= model->columnCount()))
        return result ;

    QStandardItem *item = Q_NULLPTR ;
    if ((item = ZGuiWidgetCreator::createStandardItem(data)))
    {
        item->setEditable(false) ;
        model->setItem(i, j, item) ;
        result = true ;
    }

    return result ;
}


bool ZGuiEditStyleDialogue::setEditStyleData(const ZGEditStyleData& data)
{
    const ZGStyleData& style_data = data.getStyleData() ;
    m_featuresets_model->setRowCount(0);
    m_featuresets_model->setColumnCount(1) ;
    m_featuresets_model->setRowCount(style_data.getFeaturesetNumber()) ;
    const std::set<std::string>& featuresets = style_data.getFeaturesetNames() ;
    size_t i = 0 ;
    for (auto it = featuresets.begin() ; it != featuresets.end() ; ++i, ++it)
        addStandardItem(m_featuresets_model, i, 0, QString::fromStdString(*it)) ;
    m_new_name->setText(QString::fromStdString(style_data.getStyleName())) ;
    m_original_name->setText(QString::fromStdString(style_data.getStyleName())) ;
    m_button_fill_col->setColour(style_data.getColorFill()) ;
    m_button_border_col->setColour(style_data.getColorBorder()) ;
    m_checkbox_stranded->setChecked(style_data.getIsStranded()) ;
    m_style_id = style_data.getID() ;
    return true ;
}

ZGEditStyleData ZGuiEditStyleDialogue::getEditStyleData() const
{
    ZGEditStyleData data ;
    ZGStyleData style_data ;

    // do the primary one
    if (m_original_name)
    {
        style_data.setStyleName(m_original_name->text().toStdString()) ;
    }
    std::set<std::string> featuresets ;
    if (m_featuresets_model)
    {
        for (int i=0; i<m_featuresets_model->rowCount() ; ++i)
        {
            QStandardItem *item = m_featuresets_model->item(i) ;
            if (item)
            {
                featuresets.insert(item->text().toStdString()) ;
            }
        }
    }
    style_data.setFeaturesetNames(featuresets) ;
    if (m_button_fill_col)
    {
        style_data.setColorFill(m_button_fill_col->getColour()) ;
    }
    if (m_button_border_col)
    {
        style_data.setColorBorder(m_button_border_col->getColour()) ;
    }
    if (m_checkbox_stranded)
    {
        style_data.setIsStranded(m_checkbox_stranded->isChecked()) ;
    }
    style_data.setID(m_style_id) ;
    data.setStyleData(style_data) ;

    // now do the child one; strictly speaking we have to make up an id
    // here as well
    ZGStyleData style_child;
    if (m_new_name)
    {
        style_child.setStyleName(m_new_name->text().toStdString()) ;
    }
    data.setStyleDataChild(style_child) ;

    return data ;
}


////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiEditStyleDialogue::slot_close()
{
    emit signal_received_close_event() ;
}


void ZGuiEditStyleDialogue::slot_create_child()
{
    // well this is what we do when the create_child button is clicked ... not filled in of course...
}

} // namespace GUI

} // namespace ZMap

