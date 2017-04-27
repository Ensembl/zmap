/*  File: ZGuiViewControl.cpp
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

#include "ZGuiViewControl.h"
#include "ZGuiGraphicsView.h"
#include "ZGuiSequence.h"
#include "ZGuiTextDisplayDialogue.h"
#include "ZGuiTrackConfigureDialogue.h"
#include "ZGTrackConfigureData.h"
#include "ZGTrackData.h"
#include "ZGuiEditStyleDialogue.h"
#include "ZGuiChooseStyleDialogue.h"
#include "ZGuiDialogueSearchDNA.h"
#include "ZGSearchDNAData.h"
#include "ZGuiDialogueSearchFeatures.h"
#include "ZGuiDialogueListFeatures.h"
#include "ZGuiDialogueNewSource.h"
#include "ZGuiDialogueSelectSequence.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiDialoguePreferences.h"
#include "ZGuiFileImportDialogue.h"
#include "ZGuiWidgetControlPanel.h"
#include "ZGuiWidgetInfoPanel.h"
#include "ZGZoomActionType.h"
#include <stdexcept>
#include <memory>
#include <sstream>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

namespace ZMap
{

namespace GUI
{

unsigned int min_button_size = 20 ;
template <> const std::string Util::ClassName<ZGuiViewControl>::m_name("ZGuiViewControl") ;

ZGuiViewControl::ZGuiViewControl(QWidget *parent)
    : QWidget(parent),
      Util::InstanceCounter<ZGuiViewControl>(),
      Util::ClassName<ZGuiViewControl>(),
      m_top_layout(Q_NULLPTR),
      m_label_sequence(Q_NULLPTR),
      m_control_panel(Q_NULLPTR),
      m_info_panel(Q_NULLPTR),
      m_current_view(Q_NULLPTR),
      m_current_sequence(Q_NULLPTR),
      m_current_track(0),
      m_current_featureset(0),
      m_current_feature(0)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to create instance ")) ;

    // create the layout for the widget
    if (!(m_top_layout = ZGuiWidgetCreator::createVBoxLayout()))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to create layout ")) ;
    m_top_layout->setMargin(0) ;
    setLayout(m_top_layout) ;

    // a label layout
    QHBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to create labels layout ")) ;
    layout->setMargin(0) ;
    if (!(m_label_sequence = ZGuiWidgetCreator::createLabel(QString("<sequence>"))))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to create label1 ")) ;
    layout->addWidget(m_label_sequence) ;
    layout->addStretch(100) ;
    m_top_layout->addLayout(layout) ;

    // control panel
    if (!(m_control_panel = ZGuiWidgetControlPanel::newInstance()))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to create control panel instance ")) ;
    m_top_layout->addWidget(m_control_panel) ;

    // make connections with control panel
    if (!connect(m_control_panel, SIGNAL(signal_stop()), this, SLOT(slot_stop())))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to conenct control_panel::signal_stop() to slot")) ;
    if (!connect(m_control_panel, SIGNAL(signal_reload()), this, SLOT(slot_reload())))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to connect control_panel::signal_reload() to slot")) ;
    if (!connect(m_control_panel, SIGNAL(signal_back()), this, SLOT(slot_back())))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to connect control_panel::signal_back() to slot ")) ;
    if (!connect(m_control_panel, SIGNAL(signal_revcomp()), this, SLOT(slot_revcomp())))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to connect control_panel::signal_revcomp() to slot")) ;
    if (!connect(m_control_panel, SIGNAL(signal_3frame(ZGTFTActionType)), this, SLOT(slot_3frame(ZGTFTActionType))))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to connect control_panel::signal_3frame() to slot")) ;
    if (!connect(m_control_panel, SIGNAL(signal_dna()), this, SLOT(slot_dna())))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to connect control_panel::signal_dna() to slot ")) ;
    if (!connect(m_control_panel, SIGNAL(signal_columns()), this, SLOT(slot_columns())))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to connect control_panel::signal_columns() to slot")) ;
    if (!connect(m_control_panel, SIGNAL(signal_filter(int)), this, SLOT(slot_filter(int))))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to connect control_panel::signal_filter() to appropriate slot")) ;
    if (!connect(m_control_panel, SIGNAL(signal_zoomaction(ZGZoomActionType)), this, SLOT(slot_zoomaction(ZGZoomActionType))))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to connect control_panel::signal_zoomaction() to appropriate slot")) ;

    // info panel
    if (!(m_info_panel = ZGuiWidgetInfoPanel::newInstance()))
        throw std::runtime_error(std::string("ZGuiViewControl::ZGuiViewControl() ; unable to create info panel instance")) ;
    m_top_layout->addWidget(m_info_panel) ;

    // some example data (just for testing)
    QStringList list ;
    list.append(QString("name")) ;
    list.append(QString("+")) ;
    list.append(QString("coords")) ;
    list.append(QString("subfeature")) ;
    list.append(QString("score")) ;
    list.append(QString("soterm")) ;
    list.append(QString("featureset")) ;
    list.append(QString("featuresource")) ;
    m_info_panel->setAll(list) ;

    // other widget settings
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed) ;
}

ZGuiViewControl::~ZGuiViewControl()
{
}

ZGuiViewControl* ZGuiViewControl::newInstance(QWidget* parent)
{
    ZGuiViewControl* control = Q_NULLPTR ;

    try
    {
        control = new ZGuiViewControl(parent) ;
    }
    catch (...)
    {
        control = Q_NULLPTR ;
    }

    return control ;
}

void ZGuiViewControl::setStatusLabel(const QString & label,
                                     const QString& tooltip)
{
    if (m_control_panel)
    {
        m_control_panel->setStatusLabel(label) ;
        m_control_panel->setStatusLabelTooltip(tooltip) ;
    }
}


// ... note we need to be able to handle the case when there is no
// current view set anywhere...
void ZGuiViewControl::setCurrentView(ZGuiGraphicsView* current_view)
{
    if (m_current_view != current_view)
    {
        m_current_view = current_view ;
        if (m_control_panel)
        {
            m_control_panel->disconnectCurrentView() ;
            m_control_panel->setCurrentView(m_current_view) ;
            m_control_panel->connectCurrentView();
        }
    }
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiViewControl::setCurrentView() called " ;
#endif
}

// ... note we also need to handle the case where there's no sequence or
// indeed no current view
void ZGuiViewControl::setCurrentSequence(ZGuiSequence* current_sequence)
{
    if (m_current_sequence != current_sequence)
    {
        m_current_sequence = current_sequence ;
    }

    std::stringstream str ;
    if (m_current_sequence)
    {
        str << "Sequence: " << m_current_sequence->getID() << "; " << m_current_sequence ;
        setCurrentView(m_current_sequence->getCurrentView()) ;
    }
    else
    {
        str << "No current sequence." ;
        setCurrentView(Q_NULLPTR) ;
    }
    QString sequence_name = QString::fromStdString(str.str()) ;
    m_label_sequence->setText(sequence_name) ;

    // set some data in the info panel
    if (m_info_panel)
    {
        m_info_panel->setFeaturesource(sequence_name) ;
    }

#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiViewControl::setCurrentSequence() called with "
             << m_current_sequence << m_current_view ;
#endif
}



////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiViewControl::slot_setCurrentSequence(ZGuiSequence *current_sequence)
{
    setCurrentSequence(current_sequence) ;
}

void ZGuiViewControl::slot_stop()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiViewControl::slot_stop() called" ;
#endif
}

void ZGuiViewControl::slot_reload()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiViewControl::slot_reload() called" ;
#endif
}

void ZGuiViewControl::slot_back()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiViewControl::slot_back() called" ;
#endif
}

void ZGuiViewControl::slot_revcomp()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiViewControl::slot_revcomp() called" ;
#endif
}

void ZGuiViewControl::slot_3frame(ZGTFTActionType type)
{
#ifndef QT_NO_DEBUG
    std::stringstream str ;
    str << type ;
    qDebug() << "ZGuiViewControl::slot_3frame() called" << QString::fromStdString(str.str());
#endif
}

void ZGuiViewControl::slot_zoomaction(ZGZoomActionType type)
{
    if (m_current_view)
    {
        // do summat to this here view
    }
#ifndef QT_NO_DEBUG
    std::stringstream str ;
    str << type ;
    qDebug() << "ZGuiViewControl::slot_zoomaction() called " << QString::fromStdString(str.str()) ;
#endif
}

void ZGuiViewControl::slot_dna()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiViewControl::slot_dna() called" ;
#endif
}


// change in the filter combo box
void ZGuiViewControl::slot_filter(int value)
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiViewControl::slot_filter(int value) called " << value ;
#endif
}

void ZGuiViewControl::slot_columns()
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiViewControl::slot_columns() called " ;
#endif
}

} // namespace GUI

} // namespace ZMap










////////////////////////////////////////////////////////////////////////////////
/// Slots; OK note that most of these were originally for testing and don't
/// do anything relevant to this class; useful as demos of the dialogues and
/// data transfer classes though.
////////////////////////////////////////////////////////////////////////////////


// text display dialogue
//void ZGuiViewControl::slot_text_display_create()
//{
//    if (!m_text_display_dialogue)
//    {
//        try
//        {
//            m_text_display_dialogue = ZGuiTextDisplayDialogue::newInstance(this) ;
//            connect(m_text_display_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_text_display_closed())) ;
//        }
//        catch (...)
//        {
//            return ;
//        }
//    }
//    m_text_display_dialogue->show() ;
//}

//void ZGuiViewControl::slot_text_display_closed()
//{
//    if (m_text_display_dialogue)
//    {
//        m_text_display_dialogue->close() ;
//        m_text_display_dialogue = Q_NULLPTR ;
//    }
//}

// test of the track configuration
//void ZGuiViewControl::slot_track_config_create()
//{
//    if (!m_track_config_dialogue)
//    {
//        try
//        {
//            // create dialogue and connect
//            m_track_config_dialogue = ZGuiTrackConfigureDialogue::newInstance(this) ;
//            connect(m_track_config_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_track_config_closed())) ;

//            // create some config data and send to dialogue
//            ZGTrackConfigureData track_config_data ;
//            ZGTrackData::ShowType s_show = ZGTrackData::ShowType::Show,
//                    s_auto = ZGTrackData::ShowType::Auto,
//                    s_hide = ZGTrackData::ShowType::Hide ;
//            ZGTrackData::LoadType load_none = ZGTrackData::LoadType::NoData,
//                    load_all = ZGTrackData::LoadType::All ;
//            ZGTrackData t1(1, "first_track_name", s_show, s_show, load_none, "group name", "styles"),
//                    t2(2, "second_track_name", s_auto, s_auto, load_none, "group nam", "style"),
//                    t3(3, "third_track_name", s_hide, s_hide, load_none, "group na", "styl"),
//                    t4(4, "fourth_track_name", s_hide, s_hide, load_all, "group n", "sty"),
//                    t5(5, "fifth_track_name", s_hide, s_hide, load_none, "group", "st"),
//                    t6(6, "sixth_track_name", s_hide, s_hide, load_none, "grou", "s") ;
//            t1.setIsCurrent(true) ;
//            t2.setIsCurrent(true) ;
//            t3.setIsCurrent(true) ;
//            track_config_data.addTrackData(t1) ;
//            track_config_data.addTrackData(t2) ;
//            track_config_data.addTrackData(t3) ;
//            track_config_data.addTrackData(t4) ;
//            track_config_data.addTrackData(t5) ;
//            track_config_data.addTrackData(t6) ;
//            bool result = m_track_config_dialogue->setTrackConfigureData(track_config_data) ;

//#ifndef QT_NO_DEBUG
//            // on debug, inspect the output copied out of the dialogue again, and check that the
//            // appropriate elements are the same; note that with the way that this is done, not all
//            // of the data are copied... might need to modify this at some point
//            qDebug() << "ZGuiViewControl::slot_track_config_create() ; result of adding data: " << result ;
//            std::set<ZInternalID> ids = track_config_data.getIDs() ;
//            qDebug() << "track_config_data.size() " << track_config_data.size() ;
//            for (std::set<ZInternalID>::iterator it = ids.begin() ; it != ids.end() ; ++it)
//            {
//                ZGTrackData data = track_config_data.getTrackData(*it) ;
//                qDebug() << data ;
//            }
//            track_config_data.clear() ;
//            track_config_data = m_track_config_dialogue->getTrackConfigureData() ;
//            qDebug() << "track_config_data.size() " << track_config_data.size() ;
//            ids = track_config_data.getIDs() ;
//            for (std::set<ZInternalID>::iterator it = ids.begin() ; it!=ids.end() ; ++it)
//            {
//                ZGTrackData data = track_config_data.getTrackData(*it) ;
//                qDebug() << data ;
//            }
//#endif
//        }
//        catch (...)
//        {
//            return ;
//        }
//    }
//    m_track_config_dialogue->show() ;
//}

//void ZGuiViewControl::slot_track_config_closed()
//{
//    if (m_track_config_dialogue)
//    {
//        m_track_config_dialogue->close() ;
//        m_track_config_dialogue = Q_NULLPTR ;
//    }
//}


//void ZGuiViewControl::slot_style_edit_create()
//{
//    if (!m_edit_style_dialogue)
//    {
//        try
//        {
//            m_edit_style_dialogue = ZGuiEditStyleDialogue::newInstance(this) ;
//            connect(m_edit_style_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_style_edit_closed())) ;

//            // create some data to send to the dialogue
//            ZGEditStyleData edit_style_data ;
//            ZGStyleData s1(1, 0, "style1_name", QColor(123, 123, 0), QColor(0, 123, 234), false) ;
//            std::set<std::string> featuresets ;
//            featuresets.insert("featureset1") ;
//            featuresets.insert("featureset2") ;
//            featuresets.insert("featureset3") ;
//            s1.setFeaturesetNames(featuresets) ;
//            edit_style_data.setStyleData(s1) ;

//            bool result = m_edit_style_dialogue->setEditStyleData(edit_style_data) ;
//#ifndef QT_NO_DEBUG
//            // on debug, inspect the data and compare with what was fed in...
//            qDebug() << "ZGuiViewControl::slot_style_edit_create() ; result of adding data: " << result ;
//            edit_style_data = m_edit_style_dialogue->getEditStyleData() ;
//            qDebug() << edit_style_data.getStyleData() ;
//            qDebug() << edit_style_data.getStyleDataChild() ;
//#endif

//        }
//        catch (...)
//        {
//            return ;
//        }
//    }
//    m_edit_style_dialogue->show() ;
//}


//void ZGuiViewControl::slot_style_edit_closed()
//{
//    if (m_edit_style_dialogue)
//    {
//        m_edit_style_dialogue->close() ;
//        m_edit_style_dialogue = Q_NULLPTR ;
//    }
//}

//void ZGuiViewControl::slot_style_choose_create()
//{
//    if (!m_choose_style_dialogue)
//    {
//        try
//        {
//            m_choose_style_dialogue = ZGuiChooseStyleDialogue::newInstance(this) ;
//            connect(m_choose_style_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_style_choose_closed())) ;

//            // create some data to send to the dialogue
//            ZGChooseStyleData choose_style_data ;
//            ZGStyleData s1(1, 0, "style1_name", QColor(123,123,0), QColor(0, 100, 200), false),
//                    s2(2, 1, "style2_name", QColor(12, 12, 0), QColor(0, 10, 20), false) ;
//            std::set<std::string> featuresets ;
//            featuresets.insert("fsname1") ;
//            featuresets.insert("fsname2") ;
//            featuresets.insert("fsname3") ;
//            s1.setFeaturesetNames(featuresets) ;
//            featuresets.clear() ;
//            featuresets.insert("fsname3234") ;
//            featuresets.insert("fsname987234" ) ;
//            s2.setFeaturesetNames(featuresets) ;
//            choose_style_data.addStyleData(s1) ;
//            choose_style_data.addStyleData(s2) ;

//            bool result = m_choose_style_dialogue->setChooseStyleData(choose_style_data) ;
//#ifndef QT_NO_DEBUG
//            // on debug, inspect the data that have been set
//            qDebug() << "ZGuiViewControl::slot_style_choose_create() ; result of adding data: " << result ;
//            choose_style_data = m_choose_style_dialogue->getChooseStyleData() ;
//            qDebug() << "ZGuiViewControl::slot_style_choose_create() ; choose_style_data.size() = " << choose_style_data.size() ;
//            try
//            {
//                s1 = choose_style_data.getStyleData(1) ;
//                s2 = choose_style_data.getStyleData(2) ;
//                qDebug() << "ZGuiViewControl::slot_style_choose_create() ; s1 " << s1 ;
//                qDebug() << "ZGuiViewControl::slot_style_choose_create() ; s2 " << s2 ;
//            }
//            catch (...)
//            {}

//#endif
//        }
//        catch (...)
//        {
//            return ;
//        }
//    }
//    m_choose_style_dialogue->show() ;
//}

//void ZGuiViewControl::slot_style_choose_closed()
//{
//    if (m_choose_style_dialogue)
//    {
//        m_choose_style_dialogue->close() ;
//        m_choose_style_dialogue = Q_NULLPTR ;
//    }
//}




//void ZGuiViewControl::slot_search_dna_create()
//{
//    if (!m_search_dna_dialogue)
//    {
//        try
//        {
//            m_search_dna_dialogue = ZGuiDialogueSearchDNA::newInstance(this) ;
//            connect(m_search_dna_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_search_dna_closed())) ;

//            // create some data to send to the dialogue
//            ZGSearchDNAData search_data ;
//            search_data.setSequence(std::string("acacacacacacac")) ;
//            search_data.setStrand(ZGStrandType::Dot) ;
//            search_data.setStart(100) ;
//            search_data.setEnd(100000) ;
//            search_data.setMismatches(10) ;
//            search_data.setBases(30) ;
//            search_data.setColorForward(QColor(123,123,0)) ;
//            search_data.setColorReverse(QColor(0,12,200)) ;
//            search_data.setPreserve(true) ;

//            bool result = m_search_dna_dialogue->setSearchDNAData(search_data) ;
//#ifndef QT_NO_DEBUG
//            // on debug, inspect the data that have been set
//            qDebug() << search_data ;
//            qDebug() << "ZGuiViewControl::slot_search_dna_create() ; result of adding data: " << result ;
//            search_data = m_search_dna_dialogue->getSearchDNAData() ;
//            qDebug() << search_data ;
//#endif

//        }
//        catch (...)
//        {
//            return ;
//        }
//    }
//    m_search_dna_dialogue->show() ;
//}

//void ZGuiViewControl::slot_search_dna_closed()
//{
//    if (m_search_dna_dialogue)
//    {
//        m_search_dna_dialogue->close() ;
//        m_search_dna_dialogue = Q_NULLPTR ;
//    }
//}




//void ZGuiViewControl::slot_search_features_create()
//{
//    if (!m_search_features_dialogue)
//    {
//        try
//        {
//            m_search_features_dialogue = ZGuiDialogueSearchFeatures::newInstance(this) ;
//            connect(m_search_features_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_search_features_closed())) ;

//            ZGSearchFeaturesData search_data ;
//            std::set<std::string> str_set ;
//            str_set.insert("align1") ; str_set.insert("align2") ; search_data.setAlign(str_set) ;
//            str_set.clear() ;
//            str_set.insert("block1") ;
//            search_data.setBlock(str_set) ;
//            str_set.clear() ;
//            str_set.insert("col1") ; str_set.insert("col2") ;
//            search_data.setTrack(str_set) ;
//            str_set.clear() ;
//            str_set.insert("featureset1"); str_set.insert("featureset2") ;str_set.insert("featureset3") ;
//            search_data.setFeatureset(str_set) ;
//            str_set.clear() ;
//            search_data.setFeature("feature_name") ;
//            search_data.setStrand(ZGStrandType::Plus) ;
//            search_data.setFrame(ZGFrameType::One) ;
//            search_data.setStart(132) ;
//            search_data.setEnd(4321) ;
//            search_data.setLocus(true) ;

//            bool result = m_search_features_dialogue->setSearchFeaturesData(search_data) ;
//#ifndef QT_NO_DEBUG
//            qDebug() << search_data ;
//            qDebug() << "ZGuiViewControl::slot_search_features_create() ; result of adding data: " << result ;
//            search_data = m_search_features_dialogue->getSearchFeaturesData() ;
//            qDebug() << search_data ;
//#endif

//        }
//        catch (...)
//        {
//            return ;
//        }
//    }
//    m_search_features_dialogue->show() ;
//}

//void ZGuiViewControl::slot_search_features_closed()
//{
//    if (m_search_features_dialogue)
//    {
//        m_search_features_dialogue->close() ;
//        m_search_features_dialogue = Q_NULLPTR ;
//    }
//}



//void ZGuiViewControl::slot_list_features_create()
//{
//    if (!m_list_features_dialogue)
//    {
//        try
//        {
//            m_list_features_dialogue = ZGuiDialogueListFeatures::newInstance(this) ;
//            connect(m_list_features_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_list_features_closed())) ;

//            // test this one out, same as above...
//            ZGListFeaturesData list_data ;
//            ZGFeatureData f1(1, "feature1_name", ZGFeatureBounds(1, 1000), ZGStrandType::Plus, ZGFrameType::Zero, ZGFeatureBounds(10, 100), ZGStrandType::Minus, 123.321, "featureset_name1", "source_name1", "style_name1"),
//                    f2(123, "feature_name2", ZGFeatureBounds(234, 987234), ZGStrandType::Minus, ZGFrameType::One, ZGFeatureBounds(1, 200), ZGStrandType::Star, 9827934.3, "fs_name2", "souname2", "style_name2") ;
//            list_data.addFeatureData(f1) ;
//            list_data.addFeatureData(f2) ;

//            bool result = m_list_features_dialogue->setListFeaturesData(list_data) ;
//#ifndef QT_NO_DEBUG
//            qDebug() << "ZGuiViewControl::slot_list_features_create() ; result of adding data: " << result ;
//            qDebug() << list_data ;
//            list_data = m_list_features_dialogue->getListFeaturesData() ;
//            qDebug() << list_data ;
//#endif

//        }
//        catch (...)
//        {
//            return ;
//        }
//    }
//    m_list_features_dialogue->show() ;
//}

//void ZGuiViewControl::slot_list_features_closed()
//{
//    if (m_list_features_dialogue)
//    {
//        m_list_features_dialogue->close() ;
//        m_list_features_dialogue = Q_NULLPTR ;
//    }
//}




//void ZGuiViewControl::slot_new_source_create()
//{
//    if (!m_new_source_dialogue)
//    {
//        try
//        {
//            m_new_source_dialogue = ZGuiDialogueNewSource::newInstance(this) ;
//            connect(m_new_source_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_new_source_closed())) ;

//            std::set<std::string> featuresets, biotypes ;
//            featuresets.insert("fsname1") ;
//            featuresets.insert("fsname2") ;
//            biotypes.insert("bt1") ;
//            biotypes.insert("bt2") ;
//            ZGNewSourceData source_data (ZGSourceType::Ensembl, "source_name",
//                                         "http:asdfasdf.com", "human",
//                                         "pfx", "ensembldb.ensembl.org",
//                                         "anon", "",
//                                         featuresets, biotypes, 3306, false ) ;
//            bool result = m_new_source_dialogue->setNewSourceData(source_data) ;

//#ifndef QT_NO_DEBUG
//            qDebug() << "ZGuiViewControl::slot_new_source_create() ; result of adding data: " << result ;
//            qDebug() << source_data ;
//            source_data = m_new_source_dialogue->getNewSourceData() ;
//            qDebug() << source_data ;
//#endif
//        }
//        catch (...)
//        {
//            return ;
//        }
//    }
//    m_new_source_dialogue->show() ;
//}

//void ZGuiViewControl::slot_new_source_closed()
//{
//    if (m_new_source_dialogue)
//    {
//        m_new_source_dialogue->close() ;
//        m_new_source_dialogue = Q_NULLPTR ;
//    }
//}


//void ZGuiViewControl::slot_select_sequence()
//{
//    std::unique_ptr<ZGuiDialogueSelectSequence> selector(ZGuiDialogueSelectSequence::newInstance()) ;

//    // add some data in here...
//    ZGSelectSequenceData data ;
//    std::set<std::pair<std::string, ZGSourceType> > sources ;
//    sources.insert(std::pair<std::string, ZGSourceType>("source1", ZGSourceType::Ensembl)) ;
//    sources.insert(std::pair<std::string, ZGSourceType>("source123", ZGSourceType::FileURL)) ;
//    data.setSources(sources) ;
//    data.setSequence("asdfasdf") ;
//    data.setStart(123) ;
//    data.setEnd(123123) ;
//    data.setConfigFile("filein/here/probably.zmap") ;

//    bool result = selector->setSelectSequenceData(data) ;

//#ifndef QT_NO_DEBUG
//    qDebug() << "ZGuiViewControl::slot_select_sequence() ; result of adding data: " << result ;
//    qDebug() << data ;
//    data = selector->getSelectSequenceData() ;
//    qDebug() << data ;
//#endif

//    if (selector->exec() == QDialog::Accepted)
//    {
//        // the usual...
//    }
//}



//void ZGuiViewControl::slot_preferences_create()
//{
//    if (!m_preferences_dialogue)
//    {
//        try
//        {
//            m_preferences_dialogue = ZGuiDialoguePreferences::newInstance(this) ;
//            connect(m_preferences_dialogue, SIGNAL(signal_received_close_event()), this, SLOT(slot_preferences_closed())) ;

//            bool result = false ;
//            ZGPreferencesDisplayData display_data ;
//            display_data.setShrinkableWindow(true) ;
//            display_data.setHighlightFiltered(false) ;
//            display_data.setEnableAnnotation(true) ;
//            ZGPreferencesBlixemData blixem_data ;
//            blixem_data.setRestrictScope(true) ;
//            blixem_data.setRestrictFeatures(true) ;
//            blixem_data.setKeepTemporary(false ) ;
//            blixem_data.setSleep(true) ;
//            blixem_data.setKill(false) ;
//            blixem_data.setScope(100) ;
//            blixem_data.setMax(321) ;
//            blixem_data.setPort(28828) ;
//            blixem_data.setNetworkID("asdfasdfnetworkidasdf") ;
//            blixem_data.setConfigFile("file/in/here/or/something.txt") ;
//            blixem_data.setLaunchScript("asdfasdf.sh") ;

//#ifndef QT_NO_DEBUG
//            result = m_preferences_dialogue->setPreferencesDisplayData(display_data) ;
//            qDebug() << "ZGuiViewControl::slot_preferences_create() : set display data: " << result ;
//            qDebug() << display_data ;
//            display_data = m_preferences_dialogue->getPreferencesDisplayData() ;
//            qDebug() << display_data ;

//            result = m_preferences_dialogue->setPreferencesBlixemData(blixem_data) ;
//            qDebug() << "ZGuiViewControl::slot_preferences_create() : set blixem data: " << result ;
//            qDebug() << blixem_data ;
//            blixem_data = m_preferences_dialogue->getPreferencesBlixemData() ;
//            qDebug() << blixem_data ;
//#endif
//        }
//        catch (...)
//        {
//            return ;
//        }
//    }
//    m_preferences_dialogue->show() ;
//}


//void ZGuiViewControl::slot_preferences_closed()
//{
//    if (m_preferences_dialogue)
//    {
//        m_preferences_dialogue->close() ;
//        m_preferences_dialogue = Q_NULLPTR ;
//    }
//}



//void ZGuiViewControl::slot_file_import()
//{
//    std::unique_ptr<ZGuiFileImportDialogue> file_import(ZGuiFileImportDialogue::newInstance()) ;

//    // add some data in here...
//    ZGResourceImportData data ;
//    data.setResourceName("name") ;
//    data.setSequenceName("data") ;
//    data.setAssemblyName("grc38") ;
//    data.setSourceName("experimentname") ;
//    data.setStrand(ZGStrandType::Star) ;
//    data.setStart(123) ;
//    data.setEnd(123123) ;
//    data.setMapFlag(true) ;
//    bool result = file_import->setResourceImportData(data) ; ;

//#ifndef QT_NO_DEBUG
//    qDebug() << "ZGuiViewControl::slot_file_import() ; result of adding data: " << result ;
//    qDebug() << data ;
//    data = file_import->getResourceImportData() ;
//    qDebug() << data ;
//#endif

//    if (file_import->exec() == QDialog::Accepted)
//    {
//        data = file_import->getResourceImportData() ;
//#ifndef QT_NO_DEBUG
//        qDebug() << data ;
//#endif
//    }
//}
