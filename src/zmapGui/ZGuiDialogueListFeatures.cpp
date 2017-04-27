/*  File: ZGuiDialogueListFeatures.cpp
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

#include "ZGuiDialogueListFeatures.h"
#include "ZGuiTextDisplayDialogue.h"
#include "ZGuiDialogueListFeaturesStyle.h"
#include "ZGuiWidgetCreator.h"

#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QAction>
#include <QTableView>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QHeaderView>
#include <QList>
#include <stdexcept>
#include <sstream>

Q_DECLARE_METATYPE(ZMap::GUI::ZGFeatureData)

namespace ZMap
{

namespace GUI
{

const int ZGuiDialogueListFeatures::m_featuredata_id(qMetaTypeId<ZGFeatureData>()) ;

int ZGuiDialogueListFeatures::m_model_column_number = 12 ;
const QStringList ZGuiDialogueListFeatures::m_model_column_titles =
    { QString("#"),
      QString("Name"),
      QString("Start"),
      QString("End"),
      QString("Strand"),
      QString("Query start"),
      QString("Query end"),
      QString("Query strand"),
      QString("Score"),
      QString("Feature set"),
      QString("Source"),
      QString("Style")
    } ;
template <> const std::string Util::ClassName<ZGuiDialogueListFeatures>::m_name("ZGuiDialogueListFeatures") ;
const QString ZGuiDialogueListFeatures::m_display_name("ZMap Feature List"),
ZGuiDialogueListFeatures::m_help_text_feature_list("Help text for this feature here"),
ZGuiDialogueListFeatures::m_help_text_about("Help text for this feature here"),
ZGuiDialogueListFeatures::m_help_text_title_feature_list("ZMap Feature List: General help"),
ZGuiDialogueListFeatures::m_help_text_title_about("ZMap Feature List: About") ;

ZGuiDialogueListFeatures::ZGuiDialogueListFeatures(QWidget *parent)
    : QMainWindow(parent),
      Util::InstanceCounter<ZGuiDialogueListFeatures>(),
      Util::ClassName<ZGuiDialogueListFeatures>(),
      m_text_display_dialogue(Q_NULLPTR),
      m_top_layout(Q_NULLPTR),
      m_style(Q_NULLPTR),
      m_menu_file(Q_NULLPTR),
      m_menu_operate(Q_NULLPTR),
      m_menu_help(Q_NULLPTR),
      m_action_file_search(Q_NULLPTR),
      m_action_file_preserve(Q_NULLPTR),
      m_action_file_export(Q_NULLPTR),
      m_action_file_close(Q_NULLPTR),
      m_action_operate_rshow(Q_NULLPTR),
      m_action_operate_rhide(Q_NULLPTR),
      m_action_operate_sshow(Q_NULLPTR),
      m_action_operate_shide(Q_NULLPTR),
      m_action_help_feature_list(Q_NULLPTR),
      m_action_help_about(Q_NULLPTR),
      m_feature_view(Q_NULLPTR),
      m_feature_model(Q_NULLPTR),
      m_button_close(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; cannot create without QApplication instance existing ")) ;

    // top layout
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; unable to create top_layout ")) ;

    // do some menu bar
    QMenuBar *menu_bar = Q_NULLPTR ;
    try
    {
        menu_bar = menuBar() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught exception on attempt to create menu_bar: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught unknown exception on attempt to create menu_bar")) ;
    }
    if (!menu_bar)
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; unable to create menu_bar ")) ;

    // menu bar style
    if (!(m_style = ZGuiDialogueListFeaturesStyle::newInstance()))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() : unable to create style ")) ;
    m_style->setParent(this) ;
    menu_bar->setStyle(m_style) ;

    // status bar business
    QStatusBar *status_bar = Q_NULLPTR ;
    try
    {
        status_bar = statusBar() ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught exception on attempt to create status_bar: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught unknown exception on attempt to create status_bar ")) ;
    }
    if (!status_bar)
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; unable to create status_bar ")) ;

    // create actions before menus
    try
    {
        createFileMenuActions() ;
        createOperateMenuActions() ;
        createHelpMenuActions() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught exception on attempt to create actions: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught unknown exception on attempt to create actions ")) ;
    }

    // create all menus, actions, etc...
    try
    {
        createAllMenus() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught exception in attempt to call createAllMenus(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught unknown exception in attempt to call createAllMenus() ")) ;
    }

    // central widget
    QWidget* widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; unable to create central widget ")) ;
    try
    {
        setCentralWidget(widget) ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught exception on attempt to call setCentralWidget(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught unknown exception on attempt to call setCentralWidget() ")) ;
    }
    widget->setLayout(m_top_layout) ;

    // feature list view
    if (!(m_feature_view = ZGuiWidgetCreator::createTableView()))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; unable to create feature_view ")) ;
    m_top_layout->addWidget(m_feature_view) ;

    // feature list model
    if (!(m_feature_model = ZGuiWidgetCreator::createStandardItemModel()))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; unable to create feature_model")) ;
    m_feature_view->setModel(m_feature_model)  ;
    m_feature_view->resizeRowsToContents() ;
    m_feature_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed) ;
    m_feature_view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed) ;
    m_feature_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;
    m_feature_view->setSizeAdjustPolicy(QTableView::SizeAdjustPolicy::AdjustToContents);

    // set some general properties of the model
    try
    {
        setModelParams() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught exception in attempt to call setModelParams(): ") + error.what())  ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; caught unknown exception in attempt to call setModelParams() ")) ;
    }

    // populate the model with some dummy data...

    // add a separator
    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // final button layout
    if (!(widget = createControls01()))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; unable to create button layout ")) ;
    m_top_layout->addWidget(widget) ;

    // connection for help facility for this window
    if (!connect(this, SIGNAL(signal_help_display_create(HelpType)), this, SLOT(slot_help_display_create(HelpType))))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::ZGuiDialogueListFeatures() ; unable to connect help display signals and slots")) ;

    // more final stuff
    setWindowTitle(m_display_name) ;
    setAttribute(Qt::WA_DeleteOnClose) ;
}

ZGuiDialogueListFeatures::~ZGuiDialogueListFeatures()
{
}

ZGuiDialogueListFeatures* ZGuiDialogueListFeatures::newInstance(QWidget* parent)
{
    ZGuiDialogueListFeatures* dialogue = Q_NULLPTR ;

    try
    {
        dialogue = new ZGuiDialogueListFeatures(parent) ;
    }
    catch (...)
    {
        dialogue = Q_NULLPTR ;
    }

    return dialogue ;
}

void ZGuiDialogueListFeatures::closeEvent(QCloseEvent *)
{
    emit signal_received_close_event() ;
}


void ZGuiDialogueListFeatures::createAllMenus()
{
    createFileMenu() ;
    createOperateMenu();
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
        menu_bar->addSeparator() ;
    createHelpMenu() ;

}

void ZGuiDialogueListFeatures::createFileMenu()
{
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_file = menu_bar->addMenu(QString("&File")) ;
        if (!m_menu_file)
            throw std::runtime_error(std::string("ZGuiDialogueListFeatures::createFileMenu() ; could not create menu")) ;

        // add actions to menu
        if (m_action_file_search)
            m_menu_file->addAction(m_action_file_search) ;
        if (m_action_file_preserve)
            m_menu_file->addAction(m_action_file_preserve) ;
        if (m_action_file_export)
            m_menu_file->addAction(m_action_file_export) ;
        if (m_action_file_close)
            m_menu_file->addAction(m_action_file_close) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::createFileMenu() ; could not access menu_bar")) ;
    }
}

void ZGuiDialogueListFeatures::createOperateMenu()
{
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_operate = menu_bar->addMenu(QString("&Operate")) ;
        if (!m_menu_operate)
            throw std::runtime_error(std::string("ZGuiDialogueListFeatures::createOperateMenu() ; could not create menu")) ;

        QMenu *menu_results = m_menu_operate->addMenu(QString("On results")),
                *menu_selection = m_menu_operate->addMenu(QString("On selection")) ;

        if (m_action_operate_rshow)
            menu_results->addAction(m_action_operate_rshow) ;
        if (m_action_operate_rhide)
            menu_results->addAction(m_action_operate_rhide) ;
        if (m_action_operate_sshow)
            menu_selection->addAction(m_action_operate_sshow) ;
        if (m_action_operate_shide)
            menu_selection->addAction(m_action_operate_shide) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::createOperateMenu() ; could not access menu_bar")) ;
    }
}

void ZGuiDialogueListFeatures::createHelpMenu()
{
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_help = menu_bar->addMenu(QString("&Help")) ;
        if (!m_menu_help)
            throw std::runtime_error(std::string("ZGuiDialogueListFeatures::createHelpMenu() ; could not create menu")) ;

        if (m_action_help_feature_list)
            m_menu_help->addAction(m_action_help_feature_list) ;
        if (m_action_help_about)
            m_menu_help->addAction(m_action_help_about) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::createHelpMenu() ; could not access menu_bar")) ;
    }
}

void ZGuiDialogueListFeatures::createFileMenuActions()
{
    if (!m_action_file_search && (m_action_file_search = ZGuiWidgetCreator::createAction(QString("Search"), this)))
    {
        addAction(m_action_file_search) ;
        m_action_file_search->setStatusTip(QString("Search this list of features ")) ;
    }
    if (!m_action_file_preserve && (m_action_file_preserve = ZGuiWidgetCreator::createAction(QString("Preserve"),this)))
    {
        addAction(m_action_file_preserve) ;
        m_action_file_preserve->setStatusTip(QString("Preserve this list of search results")) ;
    }
    if (!m_action_file_export && (m_action_file_export = ZGuiWidgetCreator::createAction(QString("Export"),this)))
    {
        addAction(m_action_file_export) ;
        m_action_file_export->setStatusTip(QString("Export this list of features as GFF ")) ;
    }
    if (!m_action_file_close && (m_action_file_close = ZGuiWidgetCreator::createAction(QString("Close"),this)))
    {
        m_action_file_close->setShortcut(QKeySequence::Close) ;
        m_action_file_close->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_close->setStatusTip(QString("Close this dialogue")) ;
        addAction(m_action_file_close) ;
        connect(m_action_file_close, SIGNAL(triggered(bool)), this, SLOT(slot_close())) ;
    }
}

void ZGuiDialogueListFeatures::createOperateMenuActions()
{
    if (!m_action_operate_rshow && (m_action_operate_rshow = ZGuiWidgetCreator::createAction(QString("Show"),this)))
    {
        addAction(m_action_operate_rshow) ;
        m_action_operate_rshow->setStatusTip(QString("Show search results "))  ;
    }

    if (!m_action_operate_rhide && (m_action_operate_rhide = ZGuiWidgetCreator::createAction(QString("Hide"),this)))
    {
        addAction(m_action_operate_rhide) ;
        m_action_operate_rhide->setStatusTip(QString("Hide search results ")) ;
    }

    if (!m_action_operate_sshow && (m_action_operate_sshow = ZGuiWidgetCreator::createAction(QString("Show"),this)))
    {
        addAction(m_action_operate_sshow) ;
        m_action_operate_sshow->setStatusTip(QString("Show selection ")) ;
    }

    if (!m_action_operate_shide && (m_action_operate_shide = ZGuiWidgetCreator::createAction(QString("Hide"),this)))
    {
        addAction(m_action_operate_shide) ;
        m_action_operate_shide->setStatusTip(QString("Hide selection ")) ;
    }
}

void ZGuiDialogueListFeatures::createHelpMenuActions()
{

    if (!m_action_help_feature_list && (m_action_help_feature_list = ZGuiWidgetCreator::createAction(QString("Feature list"),this)))
    {
        m_action_help_feature_list->setStatusTip(QString("Help with the feature list dialogue ")) ;
        addAction(m_action_help_feature_list) ;
        connect(m_action_help_feature_list, SIGNAL(triggered(bool)), this, SLOT(slot_action_help_list())) ;
    }

    if (!m_action_help_about && (m_action_help_about = ZGuiWidgetCreator::createAction(QString("About"),this)))
    {
        m_action_help_feature_list->setStatusTip(QString("About the feature list dialogue ")) ;
        addAction(m_action_help_about) ;
        connect(m_action_help_about, SIGNAL(triggered(bool)), this, SLOT(slot_action_help_about())) ;
    }

}


QWidget * ZGuiDialogueListFeatures::createControls01()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::createControls01() ; unable to create widget ")) ;
    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::createControls01() ; unable to create layout " ));
    widget->setLayout(layout) ;
    layout->setMargin(0);

    if (!(m_button_close = ZGuiWidgetCreator::createPushButton(QString("Close"))))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::createControls01() ; unable to create button_close ")) ;
    if (!connect(m_button_close, SIGNAL(clicked(bool)), this, SLOT(slot_close())))
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::createControls019) ; could not connect m_button_close to slot_close() ")) ;

    layout->addWidget(m_button_close, 0, Qt::AlignRight) ;

    return widget ;
}


// set some properties of the model that should always be the same...
// we may need to show/hide some columns depending on the type of features that
// are to be displayed; TBD, but can be done with m_model_view->setColumnHidden(i, arg)
// where arg is true or false (innit mate). However this only works after you have
// populated the model!
void ZGuiDialogueListFeatures::setModelParams()
{
    if (!m_feature_model)
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::setModelParams() ; feature_model does not exists ; may not proceed ")) ;

    m_feature_model->setColumnCount(m_model_column_number) ;
    if (m_model_column_number != m_model_column_titles.size() )
        throw std::runtime_error(std::string("ZGuiDialogueListFeatures::setModelParams() ; model data are inconsistent ")) ;
    m_feature_model->setHorizontalHeaderLabels(m_model_column_titles) ;

}


bool ZGuiDialogueListFeatures::setListFeaturesData(const ZGListFeaturesData& data)
{
    bool result = false ;

    m_feature_model->setRowCount(0) ;
    m_feature_model->setColumnCount(m_model_column_number) ;

    std::set<ZInternalID> ids = data.getIDs() ;

    for (auto it = ids.begin() ; it != ids.end() ; ++it)
    {
        try
        {
            ZGFeatureData feature_data = data.getFeatureData(*it) ;
            ZInternalID id = feature_data.getID() ;

            QStandardItem *item = Q_NULLPTR ;
            QList<QStandardItem*> item_list ;

            std::stringstream str ;
            str << id ;
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(str.str()))))
            {
                QVariant var ;
                var.setValue(feature_data) ;
                item->setData(var) ;
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(feature_data.getName()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            str.str("");
            str << feature_data.getBounds().start ;
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(str.str()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            str.str("") ;
            str << feature_data.getBounds().end ;
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(str.str()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            str.str("") ;
            str << feature_data.getStrand() ;
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(str.str()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            str.str("") ;
            str << feature_data.getQueryBounds().start ;
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(str.str()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            str.str("") ;
            str << feature_data.getQueryBounds().end ;
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(str.str()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            str.str("") ;
            str << feature_data.getQueryStrand() ;
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(str.str()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            str.str("") ;
            str << feature_data.getScore() ;
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(str.str()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(feature_data.getFeatureset()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(feature_data.getSource()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }
            if ((item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(feature_data.getStyle()))))
            {
                item->setEditable(false) ;
                item_list.append(item) ;
            }

            m_feature_model->appendRow(item_list) ;
        }
        catch (...)
        {
            return result ;
        }

    }

    result = true ;

    return result ;
}

ZGListFeaturesData ZGuiDialogueListFeatures::getListFeaturesData() const
{
    ZGListFeaturesData data ;

    if (!m_feature_model || !m_feature_model->rowCount() || m_feature_model->columnCount() != m_model_column_number)
        return data ;

    QStandardItem *item = Q_NULLPTR ;
    QVariant var ;
    ZGFeatureData feature_data ;

    for (int i=0; i<m_feature_model->rowCount() ; ++i)
    {
        if ((item = m_feature_model->item(i)))
        {
            var = item->data() ;
            if (var.canConvert<ZGFeatureData>() && var.convert(m_featuredata_id))
            {
                feature_data = var.value<ZGFeatureData>() ;
                data.addFeatureData(feature_data) ;
            }
        }
    }


    return data ;
}



////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////


void ZGuiDialogueListFeatures::slot_close()
{
    emit signal_received_close_event() ;
}

// should actually be called from slots then attached to the menu actions themselves...
void ZGuiDialogueListFeatures::slot_help_display_create(HelpType type)
{

    if (!m_text_display_dialogue)
    {
        try
        {
            m_text_display_dialogue = ZGuiTextDisplayDialogue::newInstance(this) ;
            connect(m_text_display_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_help_display_closed())) ;
        }
        catch (...)
        {
            return ;
        }
    }

    if (type == HelpType::FeatureList)
    {
        m_text_display_dialogue->setText(m_help_text_feature_list) ;
        m_text_display_dialogue->setWindowTitle(m_help_text_title_feature_list) ;
    }
    else if (type == HelpType::About)
    {
        m_text_display_dialogue->setText(m_help_text_about) ;
        m_text_display_dialogue->setWindowTitle(m_help_text_title_about) ;
    }
    m_text_display_dialogue->show() ;
}

void ZGuiDialogueListFeatures::slot_help_display_closed()
{
    if (m_text_display_dialogue)
    {
        m_text_display_dialogue->close() ;
        m_text_display_dialogue = Q_NULLPTR ;
    }
}

void ZGuiDialogueListFeatures::slot_action_help_list()
{
    emit signal_help_display_create(HelpType::FeatureList) ;
}

void ZGuiDialogueListFeatures::slot_action_help_about()
{
    emit signal_help_display_create(HelpType::About) ;
}


} // namespace GUI

} // namespace ZMap
