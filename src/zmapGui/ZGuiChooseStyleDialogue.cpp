/*  File: ZGuiChooseStyleDialogue.cpp
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

#include "ZGuiChooseStyleDialogue.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeView>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVariant>
#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

Q_DECLARE_METATYPE(ZMap::GUI::ZGStyleData)

namespace ZMap
{

namespace GUI
{

const int ZGuiChooseStyleDialogue::m_styledata_id(qMetaTypeId<ZGStyleData>()) ;
const QString ZGuiChooseStyleDialogue::m_display_name("ZMap Choose Style");
template <> const std::string Util::ClassName<ZGuiChooseStyleDialogue>::m_name("ZGuiChooseStyleDialogue") ;

ZGuiChooseStyleDialogue::ZGuiChooseStyleDialogue(QWidget* parent)
    : QDialog(parent),
      Util::InstanceCounter<ZGuiChooseStyleDialogue>(),
      Util::ClassName<ZGuiChooseStyleDialogue>(),
      m_top_layout(Q_NULLPTR),
      m_styles_view(Q_NULLPTR),
      m_styles_model(Q_NULLPTR),
      m_button_edit(Q_NULLPTR),
      m_button_delete(Q_NULLPTR),
      m_button_add(Q_NULLPTR),
      m_button_apply(Q_NULLPTR),
      m_button_cancel(Q_NULLPTR)
{
    // top level layout
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::ZGuiChooseStyleDialogue() ; unable to create top_layout ")) ;
    setLayout(m_top_layout) ;

    // model instance
    if (!(m_styles_model = createControls00()))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::ZGuiChooseStyleDialogue() ; failed in call to createControls00() ")) ;
    m_styles_model->setParent(this) ;

    // view instance
    if (!(m_styles_view = createControls01()))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::ZGuiChooseStyleDialogue() ; failed in call to createControls01() ")) ;
    m_styles_view->setModel(m_styles_model) ;
    m_top_layout->addWidget(m_styles_view) ;

    QWidget* widget = Q_NULLPTR ;
    if (!(widget = createControls02()))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::ZGuiChooseStyleDialogue() ; failed in call to createControls02() ")) ;
    m_top_layout->addWidget(widget) ;

    // some other window settings
    setWindowTitle(m_display_name) ;
    setAttribute(Qt::WA_DeleteOnClose) ;
}

ZGuiChooseStyleDialogue::~ZGuiChooseStyleDialogue()
{
}

ZGuiChooseStyleDialogue* ZGuiChooseStyleDialogue::newInstance(QWidget *parent )
{
    ZGuiChooseStyleDialogue *dialogue = Q_NULLPTR ;

    try
    {
        dialogue = new ZGuiChooseStyleDialogue(parent) ;
    }
    catch (...)
    {
        dialogue = Q_NULLPTR ;
    }

    return dialogue ;
}


void ZGuiChooseStyleDialogue::closeEvent(QCloseEvent*)
{
    emit signal_received_close_event() ;
}

bool ZGuiChooseStyleDialogue::addStandardItem(QStandardItemModel* model, int i, int j, const QString& data)
{
    bool result = false ;

    if (!model || (i >= model->rowCount()) || (j >= model->columnCount()))
        return result ;

    QStandardItem* item = Q_NULLPTR ;
    if ((item = ZGuiWidgetCreator::createStandardItem(data)))
    {
        model->setItem(i, j, item) ;
        result = true ;
    }

    return result ;
}

bool ZGuiChooseStyleDialogue::setChooseStyleData(const ZGChooseStyleData &data)
{
    bool result = false ;

    if (!m_styles_model || !m_styles_view)
        return result ;

    // firstly remove everything currently stored
    m_styles_model->setRowCount(0) ;

    // resize according to the incoming data
    int nCols = 3 ;
    m_styles_model->setColumnCount(nCols) ;

    // get ids from incoming dataset
    std::set<ZInternalID> ids = data.getIDs() ;
    for (auto it = ids.begin() ; it != ids.end() ; ++it )
    {
        ZGStyleData style_data = data.getStyleData(*it) ;
        ZInternalID parent_id = style_data.getIDParent() ;
        std::string parent_name ;
        if (parent_id)
        {
            try
            {
                ZGStyleData parent_data = data.getStyleData(parent_id) ;
                parent_name = parent_data.getStyleName() ;
            }
            catch (...)
            {

            }
        }
        std::set<std::string> featureset_data = style_data.getFeaturesetNames() ;
        std::string featureset_names ;
        if (featureset_data.size())
        {
            auto sit = featureset_data.begin() ;
            featureset_names = *sit ;
            ++sit ;
            for ( ; sit != featureset_data.end() ; ++sit)
                featureset_names += ", " + *sit ;
        }
        QStandardItem *item = Q_NULLPTR ;
        QList<QStandardItem*> item_list ;
        try
        {
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(style_data.getStyleName()))))
            {
                QVariant var ;
                var.setValue(style_data) ;
                item->setData(var) ;
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(parent_name))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(featureset_names))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
        }
        catch (...)
        {
            return result ;
        }

        // now add that list as a new row
        m_styles_model->appendRow(item_list) ;

    }
    result = true ;

    return result ;
}

ZGChooseStyleData ZGuiChooseStyleDialogue::getChooseStyleData() const
{
    ZGChooseStyleData data ;
    if (!m_styles_model || !m_styles_model->rowCount())
        return data ;

    QStandardItem *item = Q_NULLPTR ;
    QVariant var ;
    ZGStyleData style_data ;
    for (int i=0; i<m_styles_model->rowCount() ; ++i)
    {
        if ((item = m_styles_model->item(i)))
        {
            var = item->data() ;
            if (var.canConvert<ZGStyleData>() && var.convert(m_styledata_id))
            {
                style_data = var.value<ZGStyleData>() ;
                data.addStyleData(style_data) ;
            }
        }
    }

    return data ;
}


QStandardItemModel* ZGuiChooseStyleDialogue::createControls00()
{
    QStandardItemModel* model = Q_NULLPTR ;

    if (!(model = ZGuiWidgetCreator::createStandardItemModel()))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::createControls00() ; unable to create model")) ;
    QStringList list ;
    list.append(QString("Style")) ;
    list.append(QString("Parent")) ;
    list.append(QString("Featuresets")) ;
    model->setHorizontalHeaderLabels(list) ;

    return model ;
}


QTreeView* ZGuiChooseStyleDialogue::createControls01()
{
    QTreeView *view = Q_NULLPTR ;


    // view instance
    if (!(view = ZGuiWidgetCreator::createTreeView()))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::createControls01() ; unable to create view ")) ;
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;
    view->setSizeAdjustPolicy(QTreeView::SizeAdjustPolicy::AdjustToContents) ;

    return view ;
}

QWidget* ZGuiChooseStyleDialogue::createControls02()
{
    QWidget *widget = Q_NULLPTR ;

    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::createControls02() ; unable to create widget ")) ;

    QHBoxLayout *button_layout = Q_NULLPTR ;
    if (!(button_layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::createControls02() ; unable to create button_layout ")) ;
    widget->setLayout(button_layout) ;
    button_layout->setMargin(0) ;

    if (!(m_button_edit = ZGuiWidgetCreator::createPushButton(QString("Edit"))))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::createControls02() ; unable to create m_button_edit ")) ;
    button_layout->addWidget(m_button_edit, 0, Qt::AlignLeft) ;
    if (!(m_button_delete = ZGuiWidgetCreator::createPushButton(QString("Delete"))))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::createControls02() ; unable to create m_button_delete")) ;
    button_layout->addWidget(m_button_delete, 0, Qt::AlignLeft) ;
    if (!(m_button_add = ZGuiWidgetCreator::createPushButton(QString("Add"))))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::createControls02() ; unable to create m_button_add ")) ;
    button_layout->addWidget(m_button_add, 0, Qt::AlignLeft) ;
    button_layout->addStretch(100) ;
    if (!(m_button_apply = ZGuiWidgetCreator::createPushButton(QString("Apply"))))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::createControls02() ; unable to create m_button_apply")) ;
    button_layout->addWidget(m_button_apply, 0, Qt::AlignRight) ;
    if (!(m_button_cancel = ZGuiWidgetCreator::createPushButton(QString("Cancel"))))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::createControls02() ; unable to create m_button_cancel")) ;
    button_layout->addWidget(m_button_cancel, 0, Qt::AlignRight) ;


    // connect some of the controls
    if (!connect(m_button_cancel, SIGNAL(clicked(bool)), this, SLOT(slot_close())))
        throw std::runtime_error(std::string("ZGuiChooseStyleDialogue::createControls02() ; unable to connect m_button_cancel to slot_close() ")) ;


    return widget ;
}





////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////


void ZGuiChooseStyleDialogue::slot_close()
{
    emit signal_received_close_event() ;
}



} // namespace GUI

} // namespace ZMap

