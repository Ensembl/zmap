/*  File: ZGuiTrackConfigureDialogue.cpp
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

#include <QtWidgets>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QRadioButton>
#include <QVariant>

#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

#include "ZGuiTrackConfigureDialogue.h"
#include "ZGuiTrackConfigureDialogueStyle.h"
#include "ZGuiTextDisplayDialogue.h"
#include "ZGuiWidgetCreator.h"

#include <set>
#include <stdexcept>

Q_DECLARE_METATYPE(ZMap::GUI::ZInternalID)
Q_DECLARE_METATYPE(ZMap::GUI::ZGTrackData)

namespace ZMap
{

namespace GUI
{

const int ZGuiTrackConfigureDialogue::m_trackdata_id(qMetaTypeId<ZGTrackData>()) ;
const int ZGuiTrackConfigureDialogue::m_zinternalid_id(qMetaTypeId<ZInternalID>()) ;
template <> const std::string Util::ClassName<ZGuiTrackConfigureDialogue>::m_name("ZGuiTrackConfigureDialogue") ;
const QString ZGuiTrackConfigureDialogue::m_display_name("ZMap Track Configuration") ,

ZGuiTrackConfigureDialogue::m_help_text_general("The ZMap Track Configuration Window allows you to change various track-specific settings.\n"
                                                 "Select one or more rows in the list of tracks. Then you can change the visibility by\n"
                                                 "selecting S/A/H (Show/Auto/Hide - see the Help->Display menu for more information) or you can\n"
                                                 "assign a new style for the tracks by pressing the Choose Style button.\n"
                                                 "\n"
                                                 "You can navigate up/down the list with the up/down arrows on your keyboard. Hold down shift\n"
                                                 "or control to select mutliple rows.\n"
                                                 "\n"
                                                 "Enter a search term in the Search box to jump to rows containing that text in the track name.\n"
                                                 "Press the down/up arrows while the focus is in the Search box to jump to the next/previous match.\n"
                                                 "Enter a search term in the Filter box to filter the rows to track names containing that text.\n"
                                                 "Press Enter to perform the filter. Press the Clear button to clear the Search/Filter boxes.\n"
                                                 "\n"
                                                 "Drag and drop rows to reorder the tracks manually. Click on the track headers to sort the tracks\n"
                                                 "automatically. Click Clear to disable automatic sorting so that you can drag-and-drop rows manually\n"
                                                 "again (note that this will not revert the order, it just enables manual ordering based on the new order).\n"
                                                 "Note that you must click the APPLY track ORDER button to apply the new order (the main Apply button WILL\n"
                                                 "NOT reorder tracks).\n"
                                                 "\n"
                                                 "Click Apply when finished to save and apply the changes you have made (other than track order - see above).\n"
                                                 "If there is no Apply button, changes will be applied immediately. If you want to reset the dialog to the\n"
                                                 "last-saved values, click Revert. If you want to exit without saving the latest changes, click Close.\n"),

ZGuiTrackConfigureDialogue::m_help_text_display("The Show/Auto/Hide buttons allow you to change which tracks are visible in ZMap.\n"
                                                 "\n"
                                                 "The window displays separate options for the forward and reverse strands. You can set\n"
                                                 "the display state for each strand to one of:\n"
                                                 "\n"
                                                 "\"Show\"  - always show the track\n"
                                                 "\n"
                                                 "\"Auto\"  - show the track depending on ZMap settings (e.g. zoom, compress etc.)\n"
                                                 "\n"
                                                 "\"Hide\"  - always hide the track\n"
                                                 "\n"
                                                 "These options are abbreviated to S/A/H respectively in the headers and are preceeded by F/R\n"
                                                 "for the Forward/Reverse strand, i.e. 'RH' means 'Reverse strand - Hide'.\n"
                                                 "\n"
                                                 "By default track display is controlled by settings in the style for that track,\n"
                                                 "including the min and max zoom levels at which the track should be shown, how the\n"
                                                 "the track is bumped, the window mark and compress options. track display can be\n"
                                                 "overridden however to always show or always hide tracks."),

ZGuiTrackConfigureDialogue::m_help_text_title_general("ZMap Track Configuration: General help"),

ZGuiTrackConfigureDialogue::m_help_text_title_display("ZMap Track Configuration: Display help");

int ZGuiTrackConfigureDialogue::m_current_tracks_column_count(9),
    ZGuiTrackConfigureDialogue::m_available_tracks_column_count(4) ;

ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue(QWidget* parent)
    : QMainWindow(parent),
      Util::InstanceCounter<ZGuiTrackConfigureDialogue>(),
      Util::ClassName<ZGuiTrackConfigureDialogue>(),
      m_top_layout(Q_NULLPTR),
      m_tab_widget(Q_NULLPTR),
      m_b1_select_all(Q_NULLPTR),
      m_b1_set_group(Q_NULLPTR),
      m_b2_select_all(Q_NULLPTR),
      m_b2_select_marked(Q_NULLPTR),
      m_b2_select_none(Q_NULLPTR),
      m_button_revert(Q_NULLPTR),
      m_button_apply_order(Q_NULLPTR),
      m_button_apply(Q_NULLPTR),
      m_button_cancel(Q_NULLPTR),
      m_button_OK(Q_NULLPTR),
      m_model_current_tracks(Q_NULLPTR),
      m_model_available_tracks(Q_NULLPTR),
      m_view_current_tracks(Q_NULLPTR),
      m_view_available_tracks(Q_NULLPTR),
      m_style(Q_NULLPTR),
      m_text_display_dialogue(Q_NULLPTR),
      m_menu_file(Q_NULLPTR),
      m_menu_help(Q_NULLPTR),
      m_action_file_close(Q_NULLPTR),
      m_action_help_general(Q_NULLPTR),
      m_action_help_display(Q_NULLPTR),
      m_is_marked(false)
{
    // see comment in constructor for ZGuiMain
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; call to canCreate() failed ")) ;

    // menu bar
    QMenuBar *menu_bar = Q_NULLPTR ;
    try
    {
        menu_bar = menuBar() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught exception on attempt to create menu_bar: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught unknown exception on attempt to create menu_bar")) ;
    }
    if (!menu_bar)
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; unable to create menu_bar")) ;

    // menu bar style
    try
    {
        m_style = ZGuiTrackConfigureDialogueStyle::newInstance() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught exception on attempt to create m_style: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught unknown exception on attempt to create m_style ")) ;
    }
    if (!m_style)
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; unable to create m_style ")) ;
    m_style->setParent(this) ;
    menu_bar->setStyle(m_style) ;

    // status bar
    QStatusBar *status_bar = Q_NULLPTR ;
    try
    {
        status_bar = statusBar() ;
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught exception on attempt to create status_bar: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught unknown exception on attempt to create status_bar")) ;
    }
    if (!status_bar)
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; unable to create status_bar ")) ;

    try
    {
        createFileMenuActions();
        createHelpMenuActions();
    }
    catch (std::exception & error)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught exception on attempt to create actions: ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught unknown exception on attempt to create actions ")) ;
    }

    // create menus
    try
    {
        createAllMenus() ;
    }
    catch (std::exception& error)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught exception on call to createAllMenus(): ") + error.what() ) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught unknown exception on call to createAllMenus() ")) ;
    }

    // top level layout
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; could not create m_top_layout ")) ;

    // central widget; prolly tabs based...
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
         throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; unable to create central widget ")) ;
    try
    {
        setCentralWidget(widget) ;
    }
    catch (std::exception &error)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught exception on attempt to call setCentralWidget(): ") + error.what()) ;
    }
    catch (...)
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; caught unknown exception on attempt to call setCentralWidget()")) ;
    }
    widget->setLayout(m_top_layout) ;

    // create the tab widget
    if (!(m_tab_widget = ZGuiWidgetCreator::createTabWidget()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; unable to create tab_widget")) ;
    m_top_layout->addWidget(m_tab_widget) ;

    // create pages to go in tabs
    if (!(widget = createPageCurrentTracks()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; unable to create page from createPageCurrentTracks() ")) ;
    m_tab_widget->addTab(widget, QString("Current Tracks")) ;
    if (!(widget = createPageAvailableTracks()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; unable to create page from createPageCurrentTracks() ")) ;
    m_tab_widget->addTab(widget, QString("Available Tracks")) ;

    // button layout at the bottom
    if (!(widget = createMainButtons()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; unable to create layout from call to createMainButtons() ")) ;
    m_top_layout->addWidget(widget) ;

    // this is used for opening/updating the help window
    if (!connect(this, SIGNAL(signal_help_display_create(HelpType)), this, SLOT(slot_help_display_create(HelpType))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::ZGuiTrackConfigureDialogue() ; unable to connect help display to appropriate function")) ;

    // other window settings; we generally do not want this one to be modal,
    // but might need to do so for testing...
    //setWindowModality(Qt::ApplicationModal) ;
    setWindowTitle(m_display_name) ;
    setAttribute(Qt::WA_DeleteOnClose) ;
}

ZGuiTrackConfigureDialogue::~ZGuiTrackConfigureDialogue()
{
}

ZGuiTrackConfigureDialogue* ZGuiTrackConfigureDialogue::newInstance(QWidget *parent )
{
    ZGuiTrackConfigureDialogue* dialogue = Q_NULLPTR ;

    try
    {
        dialogue = new ZGuiTrackConfigureDialogue(parent) ;
    }
    catch (...)
    {
        dialogue = Q_NULLPTR ;
    }

    return dialogue ;
}


bool ZGuiTrackConfigureDialogue::setTrackConfigureData(const ZGTrackConfigureData& data)
{
    bool result = false ;

    if (!m_model_current_tracks || !m_model_available_tracks)
         return result ;

    // empty containers
    m_model_current_tracks->setRowCount(0) ;
    m_model_available_tracks->setRowCount(0) ;

    // loop over data and add to appropriate container
    std::set<ZInternalID> ids = data.getIDs() ;
    std::set<ZInternalID>::const_iterator it = ids.begin() ;
    for ( ; it != ids.end() ; ++it)
    {
        ZGTrackData tdata = data.getTrackData(*it) ;
        if (tdata.getIsCurrent())
            addCurrentTrack(tdata) ;
        else
            addAvailableTrack(tdata) ;
    }
    result = true ;

    return result ;
}


bool ZGuiTrackConfigureDialogue::addCurrentTrack(const ZGTrackData& data)
{
    bool result = false ;

    if (!m_model_current_tracks)
        return result ;

    int index = m_model_current_tracks->rowCount() ;
    QStandardItem *item = Q_NULLPTR ;

    QList<QStandardItem*> item_list ;
    try
    {
        item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(data.getTrackName())) ;
        QVariant var ;
        var.setValue(data) ;
        item->setData(var) ;
        item_list.append(item) ;
        item_list.append(ZGuiWidgetCreator::createStandardItem()) ; // group 1, 2, 3,
        item_list.append(ZGuiWidgetCreator::createStandardItem()) ;
        item_list.append(ZGuiWidgetCreator::createStandardItem()) ;
        item_list.append(ZGuiWidgetCreator::createStandardItem()) ; // group 4, 5, 6
        item_list.append(ZGuiWidgetCreator::createStandardItem()) ;
        item_list.append(ZGuiWidgetCreator::createStandardItem()) ;
        item_list.append(ZGuiWidgetCreator::createStandardItem(QString::fromStdString(data.getGroup()))) ; // some string
        item_list.append(ZGuiWidgetCreator::createStandardItem(QString::fromStdString(data.getStyles()))) ; // some other string
    }
    catch (...)
    {
        return result ;
    }

    m_model_current_tracks->appendRow(item_list) ;

    if (m_model_current_tracks->rowCount() != index+1)
        return result ;
    index = m_model_current_tracks->rowCount() - 1 ;

    // now do buttons
    QButtonGroup *button_group = Q_NULLPTR;
    QRadioButton *button = Q_NULLPTR ;
    try
    {
        if ((button_group = ZGuiWidgetCreator::createButtonGroup()))
        {
            button_group->setParent(this) ;

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(index, 1), button) ;
                button->setChecked(data.getShowForward() == ZGTrackData::ShowType::Show) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(index, 2), button) ;
                button->setChecked(data.getShowForward() == ZGTrackData::ShowType::Auto) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(index, 3), button) ;
                button->setChecked(data.getShowForward() == ZGTrackData::ShowType::Hide) ;
            }

        }

        if ((button_group = ZGuiWidgetCreator::createButtonGroup()))
        {
            button_group->setParent(this) ;

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(index, 4), button) ;
                button->setChecked(data.getShowReverse() == ZGTrackData::ShowType::Show) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(index, 5), button) ;
                button->setChecked(data.getShowReverse() == ZGTrackData::ShowType::Auto) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(index, 6), button) ;
                button->setChecked(data.getShowReverse() == ZGTrackData::ShowType::Hide) ;
            }

        }
    }
    catch (...)
    {
        return result ;
    }
    result = true ;

    return result ;

}

bool ZGuiTrackConfigureDialogue::addAvailableTrack(const ZGTrackData& data)
{
    bool result = false ;

    if (!m_model_available_tracks)
        return result ;

    int index = m_model_available_tracks->rowCount() ;
    QStandardItem *item = Q_NULLPTR ;

    QList<QStandardItem*> item_list ;
    try
    {
        item = ZGuiWidgetCreator::createStandardItem(QString::fromStdString(data.getTrackName())) ;
        QVariant var ;
        var.setValue(data) ;
        item->setData(var) ;
        item_list.append(item) ;
        item_list.append(ZGuiWidgetCreator::createStandardItem()) ; // group 1, 2, 3,
        item_list.append(ZGuiWidgetCreator::createStandardItem()) ;
        item_list.append(ZGuiWidgetCreator::createStandardItem()) ;
    }
    catch (...)
    {
        return result ;
    }

    m_model_available_tracks->appendRow(item_list) ;

    if (m_model_available_tracks->rowCount() != index+1)
        return result ;
    index = m_model_available_tracks->rowCount() - 1 ;

    // now do buttons
    QButtonGroup *button_group = Q_NULLPTR ;
    QRadioButton *button = Q_NULLPTR ;
    try
    {
        if ((button_group = ZGuiWidgetCreator::createButtonGroup()))
        {
            button_group->setParent(this) ;

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_available_tracks->setIndexWidget(m_model_available_tracks->index(index, 1), button) ;
                button->setChecked(data.getLoad() == ZGTrackData::LoadType::All) ;
                if (m_is_marked)
                    button->setDisabled(true) ;
                else
                    button->setDisabled(false) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_available_tracks->setIndexWidget(m_model_available_tracks->index(index, 2), button) ;
                button->setChecked(data.getLoad() == ZGTrackData::LoadType::Marked) ;
                if (!m_is_marked)
                    button->setDisabled(true) ;
                else
                    button->setDisabled(false) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_available_tracks->setIndexWidget(m_model_available_tracks->index(index, 3), button) ;
                button->setChecked(data.getLoad() == ZGTrackData::LoadType::NoData) ;
            }

        }
    }
    catch (...)
    {
        return result ;
    }
    result = true ;


    return result ;
}

// merge with another dataset of same type
ZGTrackConfigureData ZGuiTrackConfigureDialogue::getTrackConfigureData() const
{
    // current tracks
    ZGTrackConfigureData data = getTrackConfigureDataCurrent() ;

    // available tracks
    data.merge(getTrackConfigureDataAvailable()) ;

    return data ;
}


// recover data on available tracks to return to caller
ZGTrackConfigureData ZGuiTrackConfigureDialogue::getTrackConfigureDataAvailable() const
{
    ZGTrackConfigureData data ;
    if (!m_model_available_tracks ||
            !m_model_available_tracks->rowCount() ||
            (m_model_available_tracks->columnCount() != m_available_tracks_column_count) ||
            !m_view_available_tracks)
    {
        return data ;
    }

    QVariant var  ;
    QStandardItem *item = Q_NULLPTR ;
    QWidget* widget = Q_NULLPTR ;
    QRadioButton *button = Q_NULLPTR ;
    ZGTrackData::LoadType load_type = ZGTrackData::LoadType::Invalid ;
    ZGTrackData tdata ;

    for (int i=0 ; i<m_model_available_tracks->rowCount() ; ++i)
    {

        if ((item = m_model_available_tracks->item(i, 0)))
        {
            var = item->data() ;
            if (var.canConvert<ZGTrackData>() && var.convert(m_trackdata_id))
                tdata = var.value<ZGTrackData>() ;
        }

        // load type
        if ((widget = getIndexWidget(m_view_available_tracks, m_model_available_tracks, i, 1)))
        {
            if ((button = dynamic_cast<QRadioButton*>(widget)))
                if (button->isChecked() && !m_is_marked)
                    load_type = ZGTrackData::LoadType::All ;
        }

        if ((widget = getIndexWidget(m_view_available_tracks, m_model_available_tracks, i, 2)))
        {
            if ((button = dynamic_cast<QRadioButton*>(widget)))
                if (button->isChecked() && m_is_marked)
                    load_type = ZGTrackData::LoadType::Marked ;
        }
        if ((widget = getIndexWidget(m_view_available_tracks, m_model_available_tracks, i, 3)))
        {
            if ((button = dynamic_cast<QRadioButton*>(widget)))
                if (button->isChecked())
                    load_type = ZGTrackData::LoadType::NoData ;
        }

        try
        {
            tdata.setShowForward(ZGTrackData::ShowType::Show) ;
            tdata.setShowReverse(ZGTrackData::ShowType::Show) ;
            tdata.setLoad(load_type) ;
            data.addTrackData(tdata) ;
        }
        catch (...)
        {

        }
    }

    return data ;
}

// recover data from current tracks model to return to caller
ZGTrackConfigureData ZGuiTrackConfigureDialogue::getTrackConfigureDataCurrent() const
{
    ZGTrackConfigureData data ;
    if (!m_model_current_tracks ||
            !m_model_current_tracks->rowCount() ||
            (m_model_current_tracks->columnCount() != m_current_tracks_column_count) ||
            !m_view_current_tracks )
    {
        return data ;
    }

    QVariant var ;
    QStandardItem * item = Q_NULLPTR ;
    QWidget* widget = Q_NULLPTR ;
    QRadioButton *button = Q_NULLPTR ;
    ZGTrackData::ShowType show_forward = ZGTrackData::ShowType::Invalid,
            show_reverse = ZGTrackData::ShowType::Invalid ;
    std::string s1, s2 ;
    ZGTrackData tdata ;

    for (int i=0; i<m_model_current_tracks->rowCount() ; ++i)
    {
        if ((item = m_model_current_tracks->item(i, 0)))
        {
            var = item->data() ;
            if (var.canConvert<ZGTrackData>() && var.convert(m_trackdata_id))
                tdata = var.value<ZGTrackData>() ;
        }

        // forward show
        if ((widget = getIndexWidget(m_view_current_tracks, m_model_current_tracks, i, 1)))
        {
            if ((button = dynamic_cast<QRadioButton*>(widget)))
                if (button->isChecked())
                    show_forward = ZGTrackData::ShowType::Show ;
        }

        if ((widget = getIndexWidget(m_view_current_tracks, m_model_current_tracks, i, 2)))
        {
            if ((button = dynamic_cast<QRadioButton*>(widget)))
                if (button->isChecked())
                    show_forward = ZGTrackData::ShowType::Auto ;
        }
        if ((widget = getIndexWidget(m_view_current_tracks, m_model_current_tracks, i, 3)))
        {
            if ((button = dynamic_cast<QRadioButton*>(widget)))
                if (button->isChecked())
                    show_forward = ZGTrackData::ShowType::Hide ;
        }

        // reverse show
        if ((widget = getIndexWidget(m_view_current_tracks, m_model_current_tracks, i, 4)))
        {
            if ((button = dynamic_cast<QRadioButton*>(widget)))
                if (button->isChecked())
                    show_reverse = ZGTrackData::ShowType::Show ;
        }
        if ((widget = getIndexWidget(m_view_current_tracks, m_model_current_tracks, i, 5)))
        {
            if ((button = dynamic_cast<QRadioButton*>(widget)))
                if (button->isChecked())
                    show_reverse = ZGTrackData::ShowType::Auto ;
        }
        if ((widget = getIndexWidget(m_view_current_tracks, m_model_current_tracks, i, 6)))
        {
            if ((button = dynamic_cast<QRadioButton*>(widget)))
                if (button->isChecked())
                    show_reverse = ZGTrackData::ShowType::Hide ;
        }

        if ((item = m_model_current_tracks->item(i, 7)))
        {
            s1 = item->text().toStdString() ;
        }
        if ((item = m_model_current_tracks->item(i, 8)))
        {
            s2 = item->text().toStdString() ;
        }

        try
        {
            tdata.setShowForward(show_forward) ;
            tdata.setShowReverse(show_reverse) ;
            tdata.setLoad(ZGTrackData::LoadType::Invalid) ;
            tdata.setGroup(s1) ;
            tdata.setStyles(s2) ;
            tdata.setIsCurrent(true) ;
            data.addTrackData(tdata) ;
        }
        catch (...)
        {

        }
    }

    return data ;
}

QWidget* ZGuiTrackConfigureDialogue::getIndexWidget(QTableView *view,
                                                    QStandardItemModel* model,
                                                    int row, int col) const
{
    QWidget * widget = Q_NULLPTR ;

    if (view && model)
    {
        widget = view->indexWidget(model->index(row, col)) ;
    }

    return widget ;
}

void ZGuiTrackConfigureDialogue::closeEvent(QCloseEvent*)
{
    emit signal_received_close_event() ;
}


void ZGuiTrackConfigureDialogue::createAllMenus()
{
    createFileMenu() ;
    QMenuBar* menu_bar = menuBar() ;
    if (menu_bar)
        menu_bar->addSeparator() ;
    createHelpMenu() ;
}

void ZGuiTrackConfigureDialogue::createFileMenu()
{
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_file = menu_bar->addMenu(QString("&File")) ;
        if (!m_menu_file)
            throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createFileMenu() ; could not create menu")) ;

        // add actions to menu
        if (m_action_file_close)
            m_menu_file->addAction(m_action_file_close) ;
    }
    else
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createFileMenu() ; could not access menu_bar")) ;
    }
}
void ZGuiTrackConfigureDialogue::createHelpMenu()
{
    QMenuBar *menu_bar = menuBar() ;
    if (menu_bar)
    {
        m_menu_help = menu_bar->addMenu(QString("&Help")) ;
        if (!m_menu_help)
            throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createHelpMenu() ; could not create menu ")) ;

        // add actions to menu
        if (m_action_help_general)
            m_menu_help->addAction(m_action_help_general) ;
        if (m_action_help_display)
            m_menu_help->addAction(m_action_help_display) ;

    }
    else
    {
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createHelpMenu() ; could not access menu_bar")) ;
    }
}

void ZGuiTrackConfigureDialogue::createFileMenuActions()
{
    if (!m_action_file_close && (m_action_file_close = ZGuiWidgetCreator::createAction(QString("Close"), this)))
    {
        m_action_file_close->setShortcut(QKeySequence::Close) ;
        m_action_file_close->setShortcutContext(Qt::WindowShortcut) ;
        m_action_file_close->setStatusTip(QString("Close Track configuration dialogue")) ;
        addAction(m_action_file_close) ;
        connect(m_action_file_close, SIGNAL(triggered(bool)), this, SLOT(slot_action_file_close())) ;
    }
}

void ZGuiTrackConfigureDialogue::createHelpMenuActions()
{
    if (!m_action_help_general && (m_action_help_general = ZGuiWidgetCreator::createAction(QString("General"), this)))
    {
        m_action_help_general->setStatusTip("Help with general aspects of this dialogue") ;
        addAction(m_action_help_general) ;
        connect(m_action_help_general, SIGNAL(triggered(bool)), this, SLOT(slot_action_help_general())) ;
    }

    if (!m_action_help_display && (m_action_help_display = ZGuiWidgetCreator::createAction(QString("Display"),this)))
    {
        m_action_help_display->setStatusTip("Help with display options") ;
        addAction(m_action_help_display) ;
        connect(m_action_help_display, SIGNAL(triggered(bool)), this, SLOT(slot_action_help_display()));
    }
}


// create page widget for current Tracks
QWidget* ZGuiTrackConfigureDialogue::createPageCurrentTracks()
{
    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageCurrentTracks() ; unable to create QWidget")) ;

    QVBoxLayout *top_layout = Q_NULLPTR ;
    if (!(top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageCurrentTracks() ; unable to create top_layout ")) ;
    widget->setLayout(top_layout) ;

    // first label
    QLabel *label = Q_NULLPTR ;
    if (!(label  = ZGuiWidgetCreator::createLabel(QString("FS/FA/FH => forward strand show/auto/hide; "
                                                          "RS/RA/RH => reverse strand show/auto/hide" ))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageCurrentTracks() ; unable to create label ")) ;

    top_layout->addWidget(label, 0, Qt::AlignLeft ) ;

    // button layout
    QHBoxLayout *button_layout = Q_NULLPTR ;
    if (!(button_layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageCurrentTracks() ; unable to create button_layout ")) ;

    // label and buttons for this layout
    label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Selection:"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageCurrentTracks() ; unable to create second label")) ;
    button_layout->addWidget(label, 0, Qt::AlignLeft) ;

    if (!(m_b1_select_all = ZGuiWidgetCreator::createPushButton(QString("Select All"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageCurrentTracks() ; unable to create m_button_select_all")) ;
    button_layout->addWidget(m_b1_select_all, 0, Qt::AlignLeft) ;

    if (!(m_b1_set_group = ZGuiWidgetCreator::createPushButton(QString("Set Group"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageCurrentTracks() ; unable to create m_button_set_group")) ;
    button_layout->addWidget(m_b1_set_group, 0, Qt::AlignLeft) ;
    button_layout->addStretch(100) ;

    // add button layout to top layout
    top_layout->addLayout(button_layout) ;

    // create and populate the model
    int nItemRows = 100 ;
    if (!(m_model_current_tracks = ZGuiWidgetCreator::createStandardItemModel()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageCurrentTracks() ; unable to create m_model_current_tracks ")) ;
    m_model_current_tracks->setParent(this) ;
    m_model_current_tracks->setRowCount(nItemRows) ;
    m_model_current_tracks->setColumnCount(m_current_tracks_column_count) ;

    // add model headers
    QStringList list ;
    list.append(QString("Track")) ;
    list.append(QString("FS")) ;
    list.append(QString("FA")) ;
    list.append(QString("FH")) ;
    list.append(QString("RS")) ;
    list.append(QString("RA")) ;
    list.append(QString("RH")) ;
    list.append(QString("Styles")) ;
    list.append(QString("Group")) ;
    m_model_current_tracks->setHorizontalHeaderLabels(list) ;

    // add items to model
    for (int i=0; i<nItemRows; ++i)
    {

        if (    !addStandardItem(m_model_current_tracks, i, 0, QString("row_%0").arg(i)) ||
                !addStandardItem(m_model_current_tracks, i, 1) ||
                !addStandardItem(m_model_current_tracks, i, 2) ||
                !addStandardItem(m_model_current_tracks, i, 3) ||
                !addStandardItem(m_model_current_tracks, i, 4) ||
                !addStandardItem(m_model_current_tracks, i, 5) ||
                !addStandardItem(m_model_current_tracks, i, 6) ||
                !addStandardItem(m_model_current_tracks, i, 7) ||
                !addStandardItem(m_model_current_tracks, i, 8) )
            throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageCurrentTracks() ; unable to add an item to the m_model_current_tracks ")) ;
    }

    // create and decorate the view
    if (!(m_view_current_tracks = ZGuiWidgetCreator::createTableView()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageCurrentTracks() ; unable to create m_view_current_tracks")) ;

    m_view_current_tracks->setModel(m_model_current_tracks) ;
    m_view_current_tracks->verticalHeader()->setSectionsMovable(true) ;
    QRadioButton *button = Q_NULLPTR ;
    QButtonGroup *button_group = Q_NULLPTR ;
    for (int i=0; i<nItemRows; ++i)
    {
        if ((button_group = ZGuiWidgetCreator::createButtonGroup()))
        {
            button_group->setParent(this) ;

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(i, 1), button) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(i, 2), button) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(i, 3), button) ;
            }

        }

        if ((button_group = ZGuiWidgetCreator::createButtonGroup()))
        {
            button_group->setParent(this) ;

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(i, 4), button) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(i, 5), button) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton()))
            {
                button_group->addButton(button) ;
                m_view_current_tracks->setIndexWidget(m_model_current_tracks->index(i, 6), button) ;
            }

        }

    }

    m_view_current_tracks->resizeColumnsToContents() ;
    m_view_current_tracks->resizeRowsToContents();
    m_view_current_tracks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed) ;
    m_view_current_tracks->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed) ;
    m_view_current_tracks->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;
    m_view_current_tracks->setSizeAdjustPolicy(QTableView::SizeAdjustPolicy::AdjustToContents);

    top_layout->addWidget(m_view_current_tracks) ;

    return widget ;
}

// create page widget for available Tracks
QWidget* ZGuiTrackConfigureDialogue::createPageAvailableTracks()
{
    // similar kind of list as above, but not draggable...
    // Track name, radio group of three as with the previous one...
    // titles to be        Track, All data, Marked area, Do not load/None

    QWidget *widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageAvailableTracks() ; unable to create widget ")) ;

    QVBoxLayout *top_layout = Q_NULLPTR ;
    if (!(top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageAvailableTracks() ; unable to create top_layout ")) ;
    widget->setLayout(top_layout) ;

    // create a view and a model using QTableView and QStandardItemModel
    int nItemRows = 100 ;
    if (!(m_model_available_tracks = ZGuiWidgetCreator::createStandardItemModel()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageAvailableTracks() ; unable to create m_model_available_tracks")) ;
    m_model_available_tracks->setRowCount(nItemRows) ;
    m_model_available_tracks->setColumnCount(m_available_tracks_column_count) ;
    m_model_available_tracks->setParent(this) ;

    // headers
    QStringList list ;
    list.append(QString("Track")) ;
    list.append(QString("All Data")) ;
    list.append(QString("Marked Region")) ;
    list.append(QString("Do Not Load")) ;
    m_model_available_tracks->setHorizontalHeaderLabels(list) ;

    // add items
    for (int i=0; i<nItemRows; ++i)
    {
        if (    !addStandardItem(m_model_available_tracks, i, 0, QString("row_%0").arg(i)) ||
                !addStandardItem(m_model_available_tracks, i, 1) ||
                !addStandardItem(m_model_available_tracks, i, 2) ||
                !addStandardItem(m_model_available_tracks, i, 3))
            throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageAvailableTracks() ; unable to add item to m_model_available_tracks ")) ;
    }
#ifndef QT_NO_DEBUG
    qDebug() << "m_model_available_tracks->rowCount() = " << m_model_available_tracks->rowCount()
             << ", columnCount() = " << m_model_available_tracks->columnCount() ;
#endif

    // create view
    if (!(m_view_available_tracks = ZGuiWidgetCreator::createTableView()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageAvailableTracks() ; unable to create m_view_available_tracks")) ;
    m_view_available_tracks->setModel(m_model_available_tracks) ;
    m_view_available_tracks->verticalHeader()->setSectionsMovable(true) ;

    // some more content; set widgets on items
    QRadioButton *button = Q_NULLPTR ;
    QButtonGroup *button_group = Q_NULLPTR ;
    for (int i=0; i<nItemRows; ++i)
    {
        if ((button_group = ZGuiWidgetCreator::createButtonGroup()))
        {
            button_group->setParent(this) ;

            if ((button = ZGuiWidgetCreator::createRadioButton() ))
            {
                button_group->addButton(button) ;
                m_view_available_tracks->setIndexWidget(m_model_available_tracks->index(i, 1), button) ;
            }

            if ((button = ZGuiWidgetCreator::createRadioButton() ))
            {
                button_group->addButton(button) ;
                m_view_available_tracks->setIndexWidget(m_model_available_tracks->index(i, 2), button) ;
                button->setDisabled(true) ;
            }

            if ( (button = ZGuiWidgetCreator::createRadioButton() ))
            {
                button_group->addButton(button) ;
                m_view_available_tracks->setIndexWidget(m_model_available_tracks->index(i, 3), button) ;
                button->setChecked(true) ;
            }

        }

    }

    m_view_available_tracks->resizeColumnsToContents() ;
    m_view_available_tracks->resizeRowsToContents();
    m_view_available_tracks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed) ;
    m_view_available_tracks->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed) ;
    m_view_available_tracks->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;
    m_view_available_tracks->setSizeAdjustPolicy(QTableView::SizeAdjustPolicy::AdjustToContents);
    top_layout->addWidget(m_view_available_tracks) ;

    // button layout ...
    QHBoxLayout *button_layout = Q_NULLPTR ;
    if (!(button_layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageAvailableTracks() ; unable to create button_layout ")) ;

    // create and add buttons
    if (!( m_b2_select_all = ZGuiWidgetCreator::createPushButton(QString("Select All Tracks Fully"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageAvailableTracks() ; unable to create m_b2_select_all")) ;
    button_layout->addWidget(m_b2_select_all, 0, Qt::AlignLeft) ;

    if (!(m_b2_select_marked = ZGuiWidgetCreator::createPushButton(QString("Select All Tracks Marked"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageAvailableTracks() ; unable to create m_b2_select_marked")) ;
    button_layout->addWidget(m_b2_select_marked, 0, Qt::AlignLeft) ;

    if (!(m_b2_select_none = ZGuiWidgetCreator::createPushButton(QString("Select None"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createPageAvailableTracks() ; unable to create m_b2_select_none")) ;
    button_layout->addWidget(m_b2_select_none, 0, Qt::AlignBottom | Qt::AlignLeft) ;
    button_layout->addStretch(100) ;

    top_layout->addLayout(button_layout, 0) ;

    return widget ;
}

// create layout box for main control buttons
QWidget* ZGuiTrackConfigureDialogue::createMainButtons()
{
    QWidget* widget = Q_NULLPTR ;
    if (!(widget = ZGuiWidgetCreator::createWidget()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createMainButtons() ; unable to create widget ")) ;
    QHBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createMainButtons() ; unable to create layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    if (!(m_button_revert = ZGuiWidgetCreator::createPushButton(QString("Revert"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createMainButtons()  ; unable to create m_button_revert ")) ;
    layout->addWidget(m_button_revert, 0, Qt::AlignLeft) ;

    if (!(m_button_apply_order = ZGuiWidgetCreator::createPushButton(QString("Apply Order"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createMainButtons() ; unable to create m_button_apply_order ")) ;
    layout->addWidget(m_button_apply_order, 0, Qt::AlignLeft) ;

    if (!(m_button_apply = ZGuiWidgetCreator::createPushButton(QString("Apply"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createMainButtons() ; unable to create m_button_apply")) ;
    layout->addWidget(m_button_apply, 0, Qt::AlignLeft) ;
    layout->addStretch(100) ;

    if (!(m_button_cancel = ZGuiWidgetCreator::createPushButton(QString("Cancel"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createMainButtons() ; unable to create m_button_cancel")) ;
    layout->addWidget(m_button_cancel, 0, Qt::AlignRight) ;

    if (!(m_button_OK = ZGuiWidgetCreator::createPushButton(QString("OK"))))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createMainButtons() ; unable to create m_button_OK")) ;
    layout->addWidget(m_button_OK, 0, Qt::AlignRight) ;

    if (!connect(m_button_apply, SIGNAL(clicked(bool)), this, SLOT(slot_button_clicked_apply())))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createMainButtons() ; unable to connect m_button_apply")) ;
    if (!connect(m_button_OK, SIGNAL(clicked(bool)), this, SLOT(slot_button_clicked_OK())))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createMainButtons() ; unable to connect m_button_OK")) ;
    if (!connect(m_button_cancel, SIGNAL(clicked(bool)), this, SLOT(slot_button_clicked_cancel())))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::createMainButtons() ; unable to connect m_button_cancel")) ;

    return widget ;
}

////////////////////////////////////////////////////////////////////////////////
/// Other slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiTrackConfigureDialogue::slot_button_clicked_OK()
{
    // perform actions associated with manipulations the user has made and
    // the exit the dialogue
    slot_button_clicked_apply() ;
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiTrackConfigureDialogue::slot_button_clicked_OK() " ;
#endif
    emit signal_received_close_event() ;
}

void ZGuiTrackConfigureDialogue::slot_button_clicked_apply()
{
    // perform actions associated manipulations the user has made, and
    // keep the dialogue open
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiTrackConfigureDialogue::slot_button_clicked_apply() " ;
#endif
}

void ZGuiTrackConfigureDialogue::slot_button_clicked_cancel()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiTrackConfigureDialogue::slot_button_clicked_cancel() " ;
#endif
    emit signal_received_close_event() ;
}

// slot to open the help display window
void ZGuiTrackConfigureDialogue::slot_help_display_create(HelpType type)
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
    if (type == HelpType::General)
    {
        m_text_display_dialogue->setText(m_help_text_general) ;
        m_text_display_dialogue->setWindowTitle(m_help_text_title_general) ;
    }
    else if (type == HelpType::Display)
    {
        m_text_display_dialogue->setText(m_help_text_display) ;
        m_text_display_dialogue->setWindowTitle(m_help_text_title_display) ;
    }
    m_text_display_dialogue->show() ;
}

void ZGuiTrackConfigureDialogue::slot_help_display_closed()
{
    if (m_text_display_dialogue)
    {
        m_text_display_dialogue->close() ;
        m_text_display_dialogue = Q_NULLPTR ;
    }
}


////////////////////////////////////////////////////////////////////////////////
/// Action slots; attached to menu actions
////////////////////////////////////////////////////////////////////////////////


void ZGuiTrackConfigureDialogue::slot_action_file_close()
{
    emit signal_received_close_event() ;
}

void ZGuiTrackConfigureDialogue::slot_action_help_general()
{
    emit signal_help_display_create(HelpType::General) ;
}

void ZGuiTrackConfigureDialogue::slot_action_help_display()
{
    emit signal_help_display_create(HelpType::Display) ;
}


// add a standard item to a standard item model, with optional text
bool ZGuiTrackConfigureDialogue::addStandardItem(QStandardItemModel* model, int i, int j, const QString& data)
{
    bool result = false ;

    if (!model || (i >= model->rowCount()) || (j >= model->columnCount()))
        return result ;

    QStandardItem *item = Q_NULLPTR ;
    if (!(item = ZGuiWidgetCreator::createStandardItem(data)))
        throw std::runtime_error(std::string("ZGuiTrackConfigureDialogue::addStandardItem() ; could not create item to add ")) ;

    if (item)
    {
        model->setItem(i, j, item) ;
        result = true ;
    }

    return result ;
}

} // namespace GUI

} // namespace ZMap

