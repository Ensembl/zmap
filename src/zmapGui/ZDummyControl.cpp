/*  File: ZDummyControl.cpp
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

#include <new>
#include <iostream>
#include <stdexcept>
#include <climits>

#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QFontMetrics>
#include <QLabel>
#include <QTabWidget>
#include <QCheckBox>

#include "ZDebug.h"
#include "ZDummyControl.h"
#include "ZGUtil.h"
#include "ZGColor.h"
#include "Utilities.h"

// external message classes
#include "ZGMessageHeader.h"


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZDummyControl>::m_name("ZDummyControl") ;

const QString ZDummyControl::m_display_name("ZDummyControl window") ;
const ZInternalID ZDummyControl::m_id_init = 1000 ;
const QString ZDummyControl::m_numerical_target_mask("999999") ;

ZDummyControl::ZDummyControl()
    : QWidget(),
      ZGPacControl(),
      Util::InstanceCounter<ZDummyControl>(),
      Util::ClassName<ZDummyControl>(),
      m_id_generator(ZDummyControl::m_id_init),
      m_button000(Q_NULLPTR),
      m_button001(Q_NULLPTR),
      m_button002(Q_NULLPTR),
      m_button003(Q_NULLPTR),
      m_button004(Q_NULLPTR),
      m_button005(Q_NULLPTR),
      m_button006(Q_NULLPTR),
      m_button007(Q_NULLPTR),
      m_button03(Q_NULLPTR),
      m_button04(Q_NULLPTR),
      m_button05(Q_NULLPTR),
      m_button06(Q_NULLPTR),
      m_button07(Q_NULLPTR),
      m_button08(Q_NULLPTR),
      m_button09(Q_NULLPTR),
      m_button10(Q_NULLPTR),
      m_button11(Q_NULLPTR),
      m_button12(Q_NULLPTR),
      m_button13(Q_NULLPTR),
      m_button14(Q_NULLPTR),
      m_button15(Q_NULLPTR),
      m_button16(Q_NULLPTR),
      m_button17(Q_NULLPTR),
      m_button18(Q_NULLPTR),
      m_button19(Q_NULLPTR),
      m_button20(Q_NULLPTR),
      m_button21(Q_NULLPTR),
      m_button22(Q_NULLPTR),
      m_button_queries_gui_ids(Q_NULLPTR),
      m_button_queries_sequence_ids(Q_NULLPTR),
      m_button_queries_featureset_ids(Q_NULLPTR),
      m_button_queries_track_ids(Q_NULLPTR),
      m_button_queries_feature_ids(Q_NULLPTR),
      m_button_set_user_home(Q_NULLPTR),
      m_button_switch_orientation(Q_NULLPTR),
      m_button_add_sequence_to_gui(Q_NULLPTR),
      m_button_EXIT(Q_NULLPTR),
      m_edit_gui_id(Q_NULLPTR),
      m_edit_sequence_id(Q_NULLPTR),
      m_edit_featureset01(Q_NULLPTR),
      m_edit_featureset02(Q_NULLPTR),
      m_edit_track01(Q_NULLPTR),
      m_edit_track02(Q_NULLPTR),
      m_edit_ft1(Q_NULLPTR),
      m_edit_ft2(Q_NULLPTR),
      m_edit_ft3(Q_NULLPTR),
      m_edit_features1(Q_NULLPTR),
      m_edit_features2(Q_NULLPTR),
      m_edit_features3(Q_NULLPTR),
      m_edit_features4(Q_NULLPTR),
      m_edit_queries_gui_list(Q_NULLPTR),
      m_edit_queries_sequence_ids(Q_NULLPTR),
      m_edit_queries_seqid_input(Q_NULLPTR),
      m_edit_queries_featureset_ids(Q_NULLPTR),
      m_edit_queries_track_ids(Q_NULLPTR),
      m_edit_queries_fset_input(Q_NULLPTR),
      m_edit_queries_feature_ids(Q_NULLPTR),
      m_edit_seqgui_guiid_input(Q_NULLPTR),
      m_edit_seqgui_seqid_input(Q_NULLPTR),
      m_edit_user_home(Q_NULLPTR),
      m_check_features(Q_NULLPTR),
      m_check_track_strand(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; can not create ZDummyControl without QApplication instance existing ")) ;

    QVBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = new QVBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; can not create top level layout ")) ;
    setLayout(layout) ;
    layout->setMargin(0) ;

    QTabWidget *tab_widget = Q_NULLPTR ;
    if (!(tab_widget = new QTabWidget))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; could not create tab widget ")) ;
    layout->addWidget(tab_widget) ;

    QWidget *widget = Q_NULLPTR ;

    // toplevel control box
    if (!(widget = createControls00()))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; failed in call to createControls00() ")) ;
    tab_widget->addTab(widget, QString("Toplevel")) ;

    // gui control box
    if (!(widget = createControls01()))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; failed in call to createControls01() ")) ;
    tab_widget->addTab(widget, QString("GUI")) ;

    // sequence controls
    if (!(widget = createControls02()))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; failed in call to createControls02() ")) ;
    tab_widget->addTab(widget, QString("Sequence")) ;

    // featureset controls
    if (!(widget = createControls03()))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; failed in call to createControls03() ")) ;
    tab_widget->addTab(widget, QString("Featureset")) ;

    // track controls
    if (!(widget = createControls04()))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; failed in call to createControls04() ")) ;
    tab_widget->addTab(widget, QString("Tracks")) ;

    // f2t controls
    if (!(widget = createControls05()))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; failed in call to createControls05() ")) ;
    tab_widget->addTab(widget, QString("F2T map")) ;

    // features controls
    if (!(widget = createControls06()))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; failed in call to createControls06() ")) ;
    tab_widget->addTab(widget, QString("Features")) ;

    // query controls
    if (!(widget = createControls07()))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; failed in call to createControls07() ")) ;
    tab_widget->addTab(widget, QString("Queries")) ;

    // misc commands
    if (!(widget = createControls08()))
        throw std::runtime_error(std::string("ZDummyControl::ZDummyControl() ; failed in call to createControls07() ")) ;
    tab_widget->addTab(widget, QString("Sequence/Gui")) ;

    m_button_EXIT = new QPushButton(QString("Exit")) ;
    layout->addWidget(m_button_EXIT) ;

    setWindowTitle(m_display_name) ;
    connect(m_button_EXIT, SIGNAL(clicked(bool)), qApp, SLOT(closeAllWindows())) ;
}

ZDummyControl::~ZDummyControl()
{
}

// this instance receives a message from the peer
void ZDummyControl::receive(const ZGPacControl * const sender,
                            std::shared_ptr<ZGMessage> message)
{

    if (!static_cast<bool>(message) || this == sender )
        return ;



    if (!message || this == sender)
        return ;
    ZGMessageType message_type = message->type() ;
    ZInternalID incoming_message_id = message->id() ;

    switch (message_type)
    {

    case ZGMessageType::ReplyStatus:
    {
        std::shared_ptr<ZGMessageReplyStatus> specific_message =
                std::dynamic_pointer_cast<ZGMessageReplyStatus, ZGMessage>(message) ;
        if (specific_message)
        {
            std::stringstream str ;
            str << name() << "::receive() ; message->id() " << incoming_message_id
                      << ", in reply to message " << specific_message->getReplyToID()
                      << ", status is " << specific_message->getStatus() ;
            ZDebug::String2cout(str.str()) ;
        }
        break ;
    }

    case ZGMessageType::ReplyLogMessage:
    {
        std::shared_ptr<ZGMessageReplyLogMessage> specific_message =
                std::dynamic_pointer_cast<ZGMessageReplyLogMessage, ZGMessage>(message) ;
        if (specific_message)
        {
            std::stringstream str ;
            str << name() << "::receive() ; message->id() " << incoming_message_id
                << ", log message '" << specific_message->getLogMessage() << "'" ;
            ZDebug::String2cout(str.str()) ;
        }
        break ;
    }

    case ZGMessageType::Quit:
    {
        std::shared_ptr<ZGMessageQuit> specific_message =
                std::dynamic_pointer_cast<ZGMessageQuit, ZGMessage>(message) ;
        if (specific_message)
        {
            std::stringstream str ;
            str << name() << "::receive() ; message->id() " << incoming_message_id
                << " requests Quit. Calling qApp->closeAllWindows()" ;
            ZDebug::String2cout(str.str()) ;
            // this will simply close all QT objects and thus terminate gui event processing;
            // at this point, we might want to clean up the peer and objects on the other side
            // as well
            qApp->closeAllWindows() ;
        }
        break ;
    }

    case ZGMessageType::ResourceImport:
    {
        std::shared_ptr<ZGMessageResourceImport> specific_message =
                std::dynamic_pointer_cast<ZGMessageResourceImport, ZGMessage>(message) ;
        if (specific_message)
        {
            std::stringstream str ;
            str << name() << "::receive() ; message->id() " << incoming_message_id
                << "; resource import event" ;
            ZDebug::String2cout(str.str()) ;
        }
        break ;
    }

    default:
        break ;

    }

}

// this object shouldn't really be answering any queries
std::shared_ptr<ZGMessage> ZDummyControl::query(const ZGPacControl * const sender,
                                 const std::shared_ptr<ZGMessage>& message)
{
    std::shared_ptr<ZGMessage> answer ;
    if (!message || this == sender)
        return answer ;

    return answer ;
}

// create toplevel
void ZDummyControl::commandFunction000()
{
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageToplevelOperation>(message_id, ZGMessageToplevelOperation::Type::Create)) ;
    send(message) ;
}

// delete toplevel
void ZDummyControl::commandFunction001()
{
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageToplevelOperation>(message_id, ZGMessageToplevelOperation::Type::Delete)) ;
    send(message) ;
}

// show toplevel
void ZDummyControl::commandFunction002()
{
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageToplevelOperation>(message_id, ZGMessageToplevelOperation::Type::Show)) ;
    send(message) ;
}

// hide toplevel
void ZDummyControl::commandFunction003()
{
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageToplevelOperation>(message_id, ZGMessageToplevelOperation::Type::Hide)) ;
    send(message) ;
}


// create a gui
void ZDummyControl::commandFunction004()
{
    ZInternalID message_id = m_id_generator.new_id(),
            gui_id = static_cast<ZInternalID> (uintFromQLineEdit(m_edit_gui_id)) ;
    if (!gui_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction004(); create gui; id given was not valid  " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageGuiOperation>(message_id,
                                                                               gui_id,
                                                                               ZGMessageGuiOperation::Type::Create)) ;
    send(message) ;

}

// delete a gui
void ZDummyControl::commandFunction005()
{
    ZInternalID message_id = m_id_generator.new_id(),
            gui_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_gui_id)) ;
    if (!gui_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction005(); delete gui; id given was not valid  " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageGuiOperation>(message_id,
                                                                               gui_id,
                                                                               ZGMessageGuiOperation::Type::Delete)) ;
    send(message) ;
}

// show a gui
void ZDummyControl::commandFunction006()
{
    ZInternalID message_id = m_id_generator.new_id(),
            gui_id =  static_cast<ZInternalID>(uintFromQLineEdit(m_edit_gui_id)) ;
    if (!gui_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction006(); show gui ; id given was not valid  " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageGuiOperation>(message_id,
                                                                               gui_id,
                                                                               ZGMessageGuiOperation::Type::Show)) ;
    send(message) ;

}

// hide a gui
void ZDummyControl::commandFunction007()
{
    ZInternalID message_id = m_id_generator.new_id(),
            gui_id =  static_cast<ZInternalID>(uintFromQLineEdit(m_edit_gui_id))  ;
    if (!gui_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction004(); hide a gui ; id given was not valid  " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageGuiOperation>(message_id,
                                                                               gui_id,
                                                                               ZGMessageGuiOperation::Type::Hide)) ;
    send(message) ;

}

// add a sequence to the toplevel data structure
void ZDummyControl::commandFunction03()
{
    ZInternalID message_id = 0,
            sequence_id = static_cast<ZInternalID> (uintFromQLineEdit(m_edit_sequence_id)) ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction03(); add sequence; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // send message to peer
    message_id = m_id_generator.new_id() ;
    ZGMessageSequenceOperation::Type operation_type = ZGMessageSequenceOperation::Type::Create ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSequenceOperation>(message_id,
                                                                                    sequence_id,
                                                                                    operation_type)) ;
    send(message) ;

    // debugging data
    outputPeerData() ;
}

// delete a sequence
void ZDummyControl::commandFunction04()
{
    ZInternalID message_id = 0,
            sequence_id = static_cast<ZInternalID> (uintFromQLineEdit(m_edit_sequence_id)) ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction04(); delete sequence; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // send message to peer
    message_id = m_id_generator.new_id() ;
    ZGMessageSequenceOperation::Type operation_type = ZGMessageSequenceOperation::Type::Delete ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSequenceOperation>(message_id,
                                                                                    sequence_id,
                                                                                    operation_type)) ;
    send(message) ;

    // some debugging data
    outputPeerData() ;
}

// randomize the color of the background of a sequence ... why? er, to test drawing
void ZDummyControl::commandFunction24()
{
    ZInternalID message_id = m_id_generator.new_id(),
            sequence_id = static_cast<ZInternalID> (uintFromQLineEdit(m_edit_sequence_id)) ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction24() ; randomize color; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    unsigned char r = static_cast<unsigned char>(rand()%UCHAR_MAX),
            g = static_cast<unsigned char>(rand()%UCHAR_MAX),
            b = static_cast<unsigned char>(rand()%UCHAR_MAX),
            a = UCHAR_MAX ;
    ZGColor color(r, g, b, a) ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSetSequenceBGColor>(message_id,
                                                                                    sequence_id,
                                                                                    color)) ;
    send(message) ;
}

// create a strand separator in the sequence specified
void ZDummyControl::commandFunction25()
{
    ZInternalID message_id = m_id_generator.new_id(),
            sequence_id = static_cast<ZInternalID> (uintFromQLineEdit(m_edit_sequence_id)) ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction25() ; create strand separator; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    ZGMessageSeparatorOperation::Type type = ZGMessageSeparatorOperation::Type::Create ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSeparatorOperation>(message_id,
                                                                                    sequence_id,
                                                                                    type)) ;
    send(message) ;

}

// delete a strand separator in the sequence specified
void ZDummyControl::commandFunction26()
{
    ZInternalID message_id = m_id_generator.new_id(),
            sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_sequence_id)) ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction26() ; delete strand separator; sequence_id was not valid" ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    ZGMessageSeparatorOperation::Type type = ZGMessageSeparatorOperation::Type::Delete ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSeparatorOperation>(message_id,
                                                                                     sequence_id,
                                                                                     type)) ;
    send(message) ;
}

// show the strand separator in the specified sequence
void ZDummyControl::commandFunction27()
{
    ZInternalID message_id = m_id_generator.new_id(),
            sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_sequence_id)) ;
    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction27() ; show strand separator; sequence_id was not valid" ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    ZGMessageSeparatorOperation::Type type = ZGMessageSeparatorOperation::Type::Show ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSeparatorOperation>(message_id,
                                                                                     sequence_id,
                                                                                     type)) ;
    send(message) ;
}

// hide the strand separator on the specified sequence
void ZDummyControl::commandFunction28()
{
    ZInternalID message_id = m_id_generator.new_id(),
            sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_sequence_id)) ;
    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction28() ; hide strand separator; sequence_id was not valid" ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    ZGMessageSeparatorOperation::Type type = ZGMessageSeparatorOperation::Type::Hide ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSeparatorOperation>(message_id,
                                                                                     sequence_id,
                                                                                     type)) ;
    send(message) ;
}

// add a featureset
void ZDummyControl::commandFunction05()
{
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_featureset01)),
            featureset_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_featureset02)) ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction05(); add featureset ; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    if (!featureset_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction05(); add featureset; featureset_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // send message to peer
    ZInternalID message_id = m_id_generator.new_id() ;
    ZGMessageFeaturesetOperation::Type operation_type = ZGMessageFeaturesetOperation::Type::Create ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageFeaturesetOperation>(message_id,
                                                                                      sequence_id,
                                                                                      featureset_id,
                                                                                      operation_type)) ;
    send(message) ;

    // debug output
    outputPeerData() ;
}

// delete a featureset
void ZDummyControl::commandFunction06()
{
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_featureset01)),
            featureset_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_featureset02)) ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction06(); delete featureset; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    if (!featureset_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction06(); delete featureset; featureset_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // send message to peer
    ZInternalID message_id = m_id_generator.new_id() ;
    ZGMessageFeaturesetOperation::Type operation_type = ZGMessageFeaturesetOperation::Type::Delete ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageFeaturesetOperation>(message_id,
                                                                                      sequence_id,
                                                                                      featureset_id,
                                                                                      operation_type )) ;
    send(message) ;

    // output some debugging data
    outputPeerData() ;
}

// add a track
void ZDummyControl::commandFunction07()
{
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_track01)),
            track_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_track02)) ;
    bool sensitive = m_check_track_strand->isChecked() ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction07(); add track; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    if (!track_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction07(); add track; track_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // send message to peer
    ZInternalID message_id = m_id_generator.new_id() ;
    ZGMessageTrackOperation::Type operation_type = ZGMessageTrackOperation::Type::Create ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageTrackOperation>(message_id,
                                                                                 sequence_id,
                                                                                 track_id,
                                                                                 sensitive,
                                                                                 operation_type)) ;
    send(message) ;


    // debug output
    outputPeerData() ;
}

// delete a track
void ZDummyControl::commandFunction08()
{
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_track01)),
            track_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_track02)) ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction08(); delete featureset; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    if (!track_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction08(); delete track; track_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // send message to peer
    ZInternalID message_id = m_id_generator.new_id() ;
    ZGMessageTrackOperation::Type operation_type = ZGMessageTrackOperation::Type::Delete ;
    bool sensitive = false ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageTrackOperation>(message_id,
                                                                                 sequence_id,
                                                                                 track_id,
                                                                                 sensitive,
                                                                                 operation_type)) ;
    send(message) ;

    // output some debugging data
    outputPeerData() ;
}

// show a track
void ZDummyControl::commandFunction22()
{
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_track01)),
            track_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_track02)) ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction22(); delete featureset; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    if (!track_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction22(); delete track; track_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // send message to peer
    ZInternalID message_id = m_id_generator.new_id() ;
    ZGMessageTrackOperation::Type operation_type = ZGMessageTrackOperation::Type::Show ;
    bool sensitive = false ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageTrackOperation>(message_id,
                                                                                 sequence_id,
                                                                                 track_id,
                                                                                 sensitive,
                                                                                 operation_type)) ;
    send(message) ;

    // output some debugging data
    outputPeerData() ;
}

// hide a track
void ZDummyControl::commandFunction23()
{
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_track01)),
            track_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_track02)) ;

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction23(); delete featureset; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    if (!track_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction23(); delete track; track_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // send message to peer
    ZInternalID message_id = m_id_generator.new_id() ;
    ZGMessageTrackOperation::Type operation_type = ZGMessageTrackOperation::Type::Hide;
    bool sensitive = false ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageTrackOperation>(message_id,
                                                                                 sequence_id,
                                                                                 track_id,
                                                                                 sensitive,
                                                                                 operation_type)) ;
    send(message) ;

    // output some debugging data
    outputPeerData() ;
}

// set random color for strand separator bg
void ZDummyControl::commandFunction30()
{
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_sequence_id)) ;
    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction30(); separator BG color; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    ZInternalID message_id = m_id_generator.new_id() ;
    unsigned char r = static_cast<unsigned char>(rand()%UCHAR_MAX),
            g = static_cast<unsigned char>(rand()%UCHAR_MAX),
            b = static_cast<unsigned char>(rand()%UCHAR_MAX),
            a = UCHAR_MAX ;
    ZGColor color(r, g, b, a) ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSetSeparatorColor>(message_id,
                                                                                    sequence_id,
                                                                                  color)) ;
    send(message) ;
}

// random track BG color
void ZDummyControl::commandFunction29()
{
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_track01)),
            track_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_track02)) ;
    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction29(); track random BG color; sequence_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    if (!track_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction29(); track random BG color; track_id was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    unsigned char r = static_cast<unsigned char>(rand()%UCHAR_MAX),
            g = static_cast<unsigned char>(rand()%UCHAR_MAX),
            b = static_cast<unsigned char>(rand()%UCHAR_MAX),
            a = UCHAR_MAX ;
    ZGColor color(r, g, b, a) ;
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSetTrackBGColor>(message_id,
                                                                                  sequence_id,
                                                                                  track_id,
                                                                                  color)) ;
    send(message) ;
}


// add a featureset to a track for a given sequence
void ZDummyControl::commandFunction09()
{
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_ft1)),
            featureset_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_ft2)) ,
            track_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_ft3)) ;

    if (!sequence_id || !featureset_id || !track_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction09(); add f2t; an id given was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // send message
    ZInternalID message_id = m_id_generator.new_id() ;
    ZGMessageFeaturesetToTrack::Type operation_type = ZGMessageFeaturesetToTrack::Type::Add ;
    std::shared_ptr<ZGMessage> message =
            std::make_shared<ZGMessageFeaturesetToTrack>(message_id,
                                                         sequence_id,
                                                         featureset_id,
                                                         track_id,
                                                         operation_type) ;
    send(message) ;

    // debugging data
    outputPeerData() ;
}

// delete a featureset from a track for a given sequence
void ZDummyControl::commandFunction10()
{
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_ft1)),
            featureset_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_ft2)),
            track_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_ft3)) ;

    if (!sequence_id || !featureset_id || !track_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction10(); delete f2t; an id given was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // send message
    ZInternalID message_id = m_id_generator.new_id() ;
    ZGMessageFeaturesetToTrack::Type type = ZGMessageFeaturesetToTrack::Type::Remove ;
    std::shared_ptr<ZGMessage> message =
            std::make_shared<ZGMessageFeaturesetToTrack>(message_id,
                                                         sequence_id,
                                                         featureset_id,
                                                         track_id,
                                                         type) ;
    send(message) ;

    // debugging data
    outputPeerData() ;
}


// add features to the specified sequence and featureset;
// create a number at random
void ZDummyControl::commandFunction11()
{
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_features1)),
            featureset_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_features2)) ;
    uint32_t number_of_features = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_features3)) ;

    if (!sequence_id || !featureset_id || !number_of_features)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction11(); add features; an input given was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    // generate some features and insert into container
    double start = 1.0,
            end = 10000.0 ;
    std::shared_ptr<ZGFeatureset> featureset_data(std::make_shared<ZGFeatureset>(featureset_id));
    ZGFeatureType feature_type = ZGFeatureType::Basic ;
    if (featureset_data)
    {
        uint32_t n1 = number_of_features / 2 ;
        uint32_t n2 = number_of_features - n1 ;
        populateFeatureset(featureset_data, feature_type, ZGStrandType::Plus, n1, start, end, rand()) ;
        populateFeatureset(featureset_data, feature_type, ZGStrandType::Minus, n2, start, end, rand()) ;
    }

    // send message; could also check at this point whether the pac
    // contains the sequence and featureset requested
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageAddFeatures>(message_id, sequence_id, featureset_data)) ;

    if (message)
        send(message) ;

    //
    // send a style for this featureset as well
    //
    std::shared_ptr<ZGStyle> style_data(std::make_shared<ZGStyle>(m_id_generator.new_id(),
                       randType<double>(50.0),
                       generateRandomColor(),
                       generateRandomColor(),
                       generateRandomColor(),
                       generateRandomColor(),
                       feature_type)) ;
    message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> message2(std::make_shared<ZGMessageSetFeaturesetStyle>(message_id, sequence_id, featureset_id, style_data)) ;
    if (message2)
        send(message2) ;

    // debugging data
    outputPeerData() ;
}


// delete one or more features from the selected sequence and featureset
void ZDummyControl::commandFunction12()
{   
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_features1)),
            featureset_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_features2)),
            feature_id = 0 ;

    if (!sequence_id || !featureset_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction12(); delete feature; an input given was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    bool delete_all = false ;
    if (m_check_features)
    {
        delete_all = m_check_features->isChecked() ;
    }

    if (delete_all)
    {
        ZInternalID message_id = m_id_generator.new_id() ;
        std::shared_ptr<ZGMessage> message = std::make_shared<ZGMessageDeleteAllFeatures>(message_id, sequence_id, featureset_id) ;
        send(message) ;
    }
    else
    {
        if (m_edit_features4)
        {
            feature_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_features4)) ;
        }
        if (!feature_id)
        {
            std::stringstream str ;
            str << "ZDummyControl::commandFunction12(); delete feature; valid feature id not given " ;
            ZDebug::String2cerr(str.str()) ;
            return ;
        }

        // container for feature ids to remove
        std::set<ZInternalID> features ;
        features.insert(feature_id) ;

        // send message
        ZInternalID message_id = m_id_generator.new_id() ;
        std::shared_ptr<ZGMessage> message =
                std::make_shared<ZGMessageDeleteFeatures>(message_id, sequence_id, featureset_id, features) ;
        send(message) ;
    }

    // debugging data
    outputPeerData() ;
}


// recover the feature ids (as a synchronous query) from a given sequence and featureset
void ZDummyControl::commandFunction13()
{
    // get user data
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_queries_seqid_input)),
            featureset_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_queries_fset_input)) ;

    if (!sequence_id || !featureset_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction13(); query features ; an input given was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> question(std::make_shared<ZGMessageQueryFeatureIDs>(message_id, sequence_id, featureset_id)) ;
    std::shared_ptr<ZGMessage> answer ;
    if (m_peer)
    {
        answer = m_peer->query(this, question) ;
    }
    if (answer && answer->type() == ZGMessageType::ReplyDataIDSet)
    {
        std::shared_ptr<ZGMessageReplyDataIDSet> specific =
                std::dynamic_pointer_cast<ZGMessageReplyDataIDSet, ZGMessage>(answer) ;
        if (specific)
        {
            std::set<ZInternalID> dataset = specific->getData() ;
            std::stringstream str ;
            str << dataset ;
            if (m_edit_queries_feature_ids)
            {
                m_edit_queries_feature_ids->setText(QString::fromStdString(str.str())) ;
            }
        }
    }
}

// query the featuresets from a given sequence_id
void ZDummyControl::commandFunction14()
{
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_queries_seqid_input)) ;
    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction14(); query featuresets ; an input given was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> question(std::make_shared<ZGMessageQueryFeaturesetIDs>(message_id, sequence_id)) ;
    std::shared_ptr<ZGMessage> answer ;
    if (m_peer)
    {
        answer = m_peer->query(this, question) ;
    }
    if (answer && answer->type() == ZGMessageType::ReplyDataIDSet)
    {
        std::shared_ptr<ZGMessageReplyDataIDSet> specific =
                std::dynamic_pointer_cast<ZGMessageReplyDataIDSet, ZGMessage>(answer) ;
        if (specific)
        {
            std::set<ZInternalID> dataset = specific->getData() ;
            std::stringstream str ;
            str << dataset ;
            if (m_edit_queries_featureset_ids)
            {
                m_edit_queries_featureset_ids->setText(QString::fromStdString(str.str())) ;
            }
        }
    }
}

// query the tracks from a given sequence
void ZDummyControl::commandFunction15()
{
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_queries_seqid_input)) ;
    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction15(); query tracks ; an input given was not valid " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> question(std::make_shared<ZGMessageQueryTrackIDs>(message_id, sequence_id)) ;
    std::shared_ptr<ZGMessage> answer ;
    if (m_peer)
    {
        answer = m_peer->query(this, question) ;
    }
    if (answer && answer->type() == ZGMessageType::ReplyDataIDSet)
    {
        std::shared_ptr<ZGMessageReplyDataIDSet> specific =
                std::dynamic_pointer_cast<ZGMessageReplyDataIDSet, ZGMessage>(answer) ;
        if (specific)
        {
            std::set<ZInternalID> dataset = specific->getData() ;
            std::stringstream str ;
            str << dataset ;
            if (m_edit_queries_track_ids)
            {
                m_edit_queries_track_ids->setText(QString::fromStdString(str.str())) ;
            }
        }
        outputPeerData() ;
    }
}

// query the gui ids from the peer container
void ZDummyControl::commandFunction16()
{
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> question(std::make_shared<ZGMessageQueryGuiIDs>(message_id)) ;
    std::shared_ptr<ZGMessage> answer ;
    if (m_peer)
    {
        answer = m_peer->query(this, question) ;
    }
    if (answer && answer->type() == ZGMessageType::ReplyDataIDSet)
    {
        std::shared_ptr<ZGMessageReplyDataIDSet> specific =
                std::dynamic_pointer_cast<ZGMessageReplyDataIDSet, ZGMessage>(answer) ;
        if (specific)
        {
            std::set<ZInternalID> dataset = specific->getData() ;
            std::stringstream str ;
            str << dataset ;
            if (m_edit_queries_gui_list)
            {
                m_edit_queries_gui_list->setText(QString::fromStdString(str.str())) ;
            }
        }
        outputPeerData() ;
    }
}

// switch the orientation of the top level arrangement of sequence views
void ZDummyControl::commandFunction17()
{
    // should also pick up the gui_id from the appropriate line edit here...
    ZInternalID message_id = m_id_generator.new_id() ;
    ZInternalID gui_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_seqgui_guiid_input));

    if (!gui_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction20() ; add sequence to gui ; gui_id was not valid or is not present " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageGuiOperation>(message_id,
                                                                               gui_id,
                                                                               ZGMessageGuiOperation::Type::Switch)) ;
    send(message) ;
}

// get the user home directory and set it on the other side
void ZDummyControl::commandFunction18()
{
    ZInternalID message_id = m_id_generator.new_id() ;
    QString user_home = m_edit_user_home->text() ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSetUserHome>(message_id, user_home.toStdString())) ;
    send(message) ;
}

// this is fetching the sequence ids
void ZDummyControl::commandFunction19()
{
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> question(std::make_shared<ZGMessageQuerySequenceIDsAll>(message_id)) ;
    std::shared_ptr<ZGMessage> answer ;
    if (m_peer)
    {
        answer = m_peer->query(this, question) ;
    }
    if (answer && answer->type() == ZGMessageType::ReplyDataIDSet)
    {
        std::shared_ptr<ZGMessageReplyDataIDSet> specific =
                std::dynamic_pointer_cast<ZGMessageReplyDataIDSet, ZGMessage>(answer) ;
        if (specific)
        {
            std::set<ZInternalID> dataset = specific->getData() ;
            std::stringstream str ;
            str << dataset ;
            if (m_edit_queries_sequence_ids)
            {
                m_edit_queries_sequence_ids->setText(QString::fromStdString(str.str())) ;
            }
        }
        outputPeerData() ;
    }
}

// this is adding a sequence to a gui
void ZDummyControl::commandFunction20()
{
    // both must exist already...
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_seqgui_seqid_input)),
            gui_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_seqgui_guiid_input));

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction20(); add sequence to gui ; sequence_id was not valid or is not present " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    if (!gui_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction20() ; add sequence to gui ; gui_id was not valid or is not present " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }


    // send message to peer
    ZInternalID message_id = m_id_generator.new_id() ;
    ZGMessageSequenceToGui::Type operation_type = ZGMessageSequenceToGui::Type::Add ;
    std::shared_ptr<ZGMessage> message(std::make_shared<ZGMessageSequenceToGui>(message_id,
                                                                                    sequence_id,
                                                                                    gui_id,
                                                                                operation_type)) ;
    send(message) ;
}

// remove a sequence from a gui
void ZDummyControl::commandFunction21()
{
    ZInternalID sequence_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_seqgui_seqid_input)),
            gui_id = static_cast<ZInternalID>(uintFromQLineEdit(m_edit_seqgui_guiid_input));

    if (!sequence_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction21(); remove sequence from gui ; sequence_id was not valid or is not present " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    if (!gui_id)
    {
        std::stringstream str ;
        str << "ZDummyControl::commandFunction21() ; remove sequence from gui ; gui_id was not valid or is not present " ;
        ZDebug::String2cerr(str.str()) ;
        return ;
    }

    ZInternalID message_id = m_id_generator.new_id() ;
    ZGMessageSequenceToGui::Type operation_type = ZGMessageSequenceToGui::Type::Remove ;
    std::shared_ptr<ZGMessage> message (std::make_shared<ZGMessageSequenceToGui>(message_id,
                                                                                 sequence_id,
                                                                                 gui_id,
                                                                                 operation_type)) ;
    send(message) ;

}

// output the data stored in the peer object
void ZDummyControl::outputPeerData()
{
    std::stringstream str ;
    str << "ZDummyControl; peer data : " ;
    ZInternalID message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> question(std::make_shared<ZGMessageQuerySequenceIDsAll>(message_id)) ;
    std::shared_ptr<ZGMessage> answer1 = m_peer->query(this, question) ;
    std::set<ZInternalID> sequence_ids ;
    if (answer1)
    {
        std::shared_ptr<ZGMessageReplyDataIDSet> specific1 =
                std::dynamic_pointer_cast<ZGMessageReplyDataIDSet, ZGMessage>(answer1) ;
        if (specific1)
        {
            sequence_ids = specific1->getData() ;
            std::set<ZInternalID>::const_iterator it = sequence_ids.begin() ;
            std::shared_ptr<ZGMessage> answer2 ;
            std::shared_ptr<ZGMessageReplyDataIDSet> specific2 ;

            for ( ; it!=sequence_ids.end() ; ++it)
            {
                ZInternalID sequence_id = *it ;

                // featuresets for this sequence
                str << "s: " << sequence_id << " " ;
                message_id = m_id_generator.new_id() ;
                question = std::make_shared<ZGMessageQueryFeaturesetIDs>(message_id, sequence_id) ;
                answer2 = m_peer->query(this, question) ;
                specific2 = std::dynamic_pointer_cast<ZGMessageReplyDataIDSet, ZGMessage>(answer2) ;
                if (specific2)
                {
                    std::set<ZInternalID> featureset_ids = specific2->getData() ;
                    str << "f" << featureset_ids ;
                    if (featureset_ids.size())
                    {
                        str << "f2t[" ;
                        for (auto it=featureset_ids.begin() ; it!=featureset_ids.end() ; ++it)
                            str << *it << "->" << outputPeerData_01(sequence_id, *it) << " " ;
                        str << "]" ;
                    }
                }

                // tracks for this sequence
                message_id = m_id_generator.new_id() ;
                question = std::make_shared<ZGMessageQueryTrackIDs>(message_id, sequence_id) ;
                answer2 = m_peer->query(this, question) ;
                specific2 = std::dynamic_pointer_cast<ZGMessageReplyDataIDSet, ZGMessage>(answer2) ;
                if (specific2)
                {
                    std::set<ZInternalID> track_ids = specific2->getData() ;
                    str << "t" << track_ids ;
                    if (track_ids.size())
                    {
                        str << "t2f[" ;
                        for (auto it=track_ids.begin() ; it!=track_ids.end() ; ++it)
                            str << *it << "->" << outputPeerData_02(sequence_id, *it) << " " ;
                        str << "]" ;
                    }
                }
                str << ", " ;

            }
        }

        ZDebug::String2cout(str.str()) ;
    }
}

// send a query to the peer to get the track ID from sequence and featureset ids
ZInternalID ZDummyControl::outputPeerData_01(ZInternalID sequence_id, ZInternalID featureset_id)
{
    ZInternalID track_id = 0, message_id = 0 ;
    if (!m_peer)
        return track_id ;
    message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> question =
            std::make_shared<ZGMessageQueryTrackFromFeatureset>(message_id, sequence_id, featureset_id) ;
    std::shared_ptr<ZGMessage> answer = m_peer->query(this, question) ;
    std::shared_ptr<ZGMessageReplyDataID> specific =
            std::dynamic_pointer_cast<ZGMessageReplyDataID, ZGMessage>(answer) ;
    if (specific)
        track_id = specific->getData() ;
    return track_id ;
}

std::set<ZInternalID> ZDummyControl::outputPeerData_02(ZInternalID sequence_id, ZInternalID track_id )
{
    std::set<ZInternalID> featureset_ids ;
    ZInternalID message_id = 0 ;
    if (!m_peer)
        return featureset_ids ;
    message_id = m_id_generator.new_id() ;
    std::shared_ptr<ZGMessage> question =
            std::make_shared<ZGMessageQueryFeaturesetsFromTrack>(message_id, sequence_id, track_id) ;
    std::shared_ptr<ZGMessage> answer = m_peer->query(this, question) ;
    std::shared_ptr<ZGMessageReplyDataIDSet> specific =
            std::dynamic_pointer_cast<ZGMessageReplyDataIDSet, ZGMessage>(answer) ;
    if (specific)
        featureset_ids = specific->getData() ;
    return featureset_ids ;
}


unsigned int ZDummyControl::uintFromQLineEdit(QLineEdit *line_edit)
{
    unsigned int result = 0 ;
    if (line_edit)
    {
        result = line_edit->text().toUInt() ;
    }
    return result ;
}

std::ostream& operator<<(std::ostream& str, const std::set<ZInternalID>& data)
{
    std::set<ZInternalID>::const_iterator it = data.begin() ;
    str << "(" ;
    for ( ; it!=data.end() ; ++it)
         str << *it << " " ;
    str << ")" ;
    return str ;
}

// this is essentially lifted from the meeting demo example; populates a
// featureset with data of the chosen type, with random starts/ends and
// subparts; id's are generated sequentially (although arbitrary)
void ZDummyControl::populateFeatureset(std::shared_ptr<ZGFeatureset>& featureset,
                        ZGFeatureType feature_type,
                        ZGStrandType strand_type,
                        unsigned int nFeatures,
                        const double &start,
                        const double &end,
                        unsigned int seed)
{
    if (!featureset || feature_type == ZGFeatureType::Invalid || (end <= start))
        return ;

    srand(seed) ;

    unsigned int nSubparts = 0, nMaxSubparts = 20 ;
    double max_length = 100.0,
            width = 10.0 + randType<double>(30.0),
            range = end - start,
            max_subpart_length = 30.0,
            max_gap_length = 30.0 ;
    ZGFeatureBounds bounds ;
    ZInternalID id_start = 0 ;
    if (featureset->size())
        id_start = featureset->size() ;
    if (feature_type == ZGFeatureType::Basic)
    {
        for (unsigned int i=0; i<nFeatures ; ++i)
        {
            bounds.start = static_cast<double>(randType<int>(range)) + 1.0 ;
            bounds.end = bounds.start + static_cast<double>(randType<int>(max_length)) ;
            ZGFeature feature(id_start + static_cast<ZInternalID>(i+1), feature_type, strand_type, bounds) ;
            featureset->addFeature(feature) ;
        }

    }
    else if ((feature_type == ZGFeatureType::Transcript) || (feature_type == ZGFeatureType::Alignment))
    {
        for (unsigned int i=0; i<nFeatures ; ++i)
        {
            ZGFeatureBounds subpart, previous ;

            nSubparts = randType<unsigned int>(nMaxSubparts) + 1 ;
            std::vector<ZGFeatureBounds> subparts(nSubparts, ZGFeatureBounds()) ;
            subpart.start = static_cast<qreal>(randType<int>(range)) + 1.0 ;
            subpart.end = subpart.start + static_cast<qreal>(randType<int>(max_subpart_length)) ;
            subparts[0] = subpart ;
            for (unsigned int j=1; j<nSubparts; ++j)
            {
                previous = subparts[j-1] ;
                subpart.start = previous.end + 2.0 + static_cast<qreal>(randType<int>(max_gap_length));
                subpart.end = subpart.start + static_cast<qreal>(randType<int>(max_subpart_length));
                subparts[j] = subpart ;
            }
            subpart.start = subparts[0].start ;
            subpart.end = subpart.start + subparts[nSubparts-1].end ;
            ZGFeature feature(static_cast<ZInternalID>(i+1), feature_type, strand_type, subpart, subparts) ;
            featureset->addFeature(feature) ;
        }
    }

    // now do style for the featureset; randomly generated colors and width
//    ZInternalID style_id = randType<unsigned int>(100) + 1 ;
//    ZGColor c1 = generateRandomColor() ,
//            c2 = generateRandomColor() ,
//            c3 = generateRandomColor() ,
//            c4 = generateRandomColor() ;
//    ZGStyle style(style_id, width, c1, c2, c3, c4, feature_type) ;
//    featureset->setStyle(style) ;
}


ZGColor ZDummyControl::generateRandomColor()
{
    unsigned char c1 = randType<unsigned char>(255),
            c2 = randType<unsigned char>(255),
            c3 = randType<unsigned char>(255),
            c4 = 255 ;
    return ZGColor(c1, c2, c3, c4) ;
}





// toplevel controls
QWidget* ZDummyControl::createControls00()
{
    QWidget* widget = Q_NULLPTR ;
    if (!(widget = new QWidget))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to create top level widget ")) ;

    QVBoxLayout *vlayout = Q_NULLPTR ;
    if (!(vlayout = new QVBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to create top level layout ")) ;
    widget->setLayout(vlayout) ;
    vlayout->setMargin(0) ;

    QHBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to create top level layout ")) ;
    vlayout->addLayout(layout) ;
    layout->setMargin(0) ;
    vlayout->addStretch(100) ;

    if (!(m_button000 = new QPushButton("Create")))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to create button000")) ;
    if (!connect(m_button000, SIGNAL(clicked(bool)), this, SLOT(commandFunction000())))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to connect button000 to appropriate slot")) ;
    layout->addWidget(m_button000, 0) ;

    if (!(m_button001 = new QPushButton("Delete")))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to create button001")) ;
    if (!connect(m_button001, SIGNAL(clicked(bool)), this, SLOT(commandFunction001())))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to connect button001 to appropriate slot")) ;
    layout->addWidget(m_button001, 0) ;

    if (!(m_button002 = new QPushButton("Show")))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to create button002")) ;
    if (!connect(m_button002, SIGNAL(clicked(bool)), this, SLOT(commandFunction002())))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to connect button002 to appropriate slot ")) ;
    layout->addWidget(m_button002,0 );

    if (!(m_button003 = new QPushButton("Hide")))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to create button003")) ;
    if (!connect(m_button003, SIGNAL(clicked(bool)), this, SLOT(commandFunction003())))
        throw std::runtime_error(std::string("ZDummyControl::createControls00() ; unable to connect button003 to appropriate slot")) ;
    layout->addWidget(m_button003, 0) ;
    layout->addStretch(100) ;

    return widget ;
}

// gui controls
QWidget* ZDummyControl::createControls01()
{
    QWidget * widget = Q_NULLPTR ;
    if (!(widget = new QWidget))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to create widget ")) ;

    QVBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = new QVBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to create top level layout for this widget")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    QHBoxLayout *hlayout = Q_NULLPTR ;

    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to create second hlayout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;
    QLabel *label = Q_NULLPTR ;
    if (!(label = new QLabel(QString("ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to create label for gui id ")) ;
    hlayout->addWidget(label, 0) ;
    if (!(m_edit_gui_id = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to create edit entry for gui id ")) ;
    m_edit_gui_id->setInputMask(m_numerical_target_mask) ;
    hlayout->addWidget(m_edit_gui_id, 0) ;
    hlayout->addStretch(100) ;

    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to create first hlayout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;

    if (!(m_button004 = new QPushButton(QString("Create"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to create button004")) ;
    if (!connect(m_button004, SIGNAL(clicked(bool)), this, SLOT(commandFunction004())))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to connect button004 to appropriate slot")) ;
    hlayout->addWidget(m_button004, 0) ;

    if (!(m_button005 = new QPushButton(QString("Delete"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to create button005")) ;
    if (!connect(m_button005, SIGNAL(clicked(bool)), this, SLOT(commandFunction005())))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to connect button005 to appropriate slot")) ;
    hlayout->addWidget(m_button005, 0) ;

    if (!(m_button006 = new QPushButton(QString("Show"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to create button006")) ;
    if (!connect(m_button006, SIGNAL(clicked(bool)), this, SLOT(commandFunction006())))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to connect button006 to appropriate slot")) ;
    hlayout->addWidget(m_button006, 0) ;

    if (!(m_button007 = new QPushButton(QString("Hide"))))
        throw  std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to create button07")) ;
    if (!connect(m_button007, SIGNAL(clicked(bool)), this, SLOT(commandFunction007())))
        throw std::runtime_error(std::string("ZDummyControl::createControls01() ; unable to connect button007 to appropriate slot")) ;
    hlayout->addWidget(m_button007, 0) ;
    hlayout->addStretch(100) ;

    layout->addStretch(100) ;


    return widget ;
}


// sequence controls
QWidget* ZDummyControl::createControls02()
{
    QWidget* widget = Q_NULLPTR ;
    if (!(widget = new QWidget))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create top level widget ")) ;

    QVBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = new QVBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create top level layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    // first hbox layout
    QHBoxLayout * hlayout = Q_NULLPTR ;
    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create first hbox layout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;
    QLabel *label = Q_NULLPTR ;
    if (!(label = new QLabel(QString("ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create label ")) ;
    hlayout->addWidget(label, 0) ;
    if (!(m_edit_sequence_id = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create first line edit ")) ;
    m_edit_sequence_id->setInputMask(m_numerical_target_mask) ;
    hlayout->addWidget(m_edit_sequence_id, 0) ;
    hlayout->addStretch(100) ;

    // second hbox layout
    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create second hbox layout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;
    if (!(m_button03 = new QPushButton(QString("Add"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create button03")) ;
    if (!connect(m_button03, SIGNAL(clicked(bool)), this, SLOT(commandFunction03())))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to connect button03 to appropriate slot")) ;
    hlayout->addWidget(m_button03, 0) ;
    if (!(m_button04 = new QPushButton(QString("Delete"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create button04")) ;
    if (!connect(m_button04, SIGNAL(clicked(bool)), this, SLOT(commandFunction04())))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to connect button04 to appropriate slot")) ;
    hlayout->addWidget(m_button04, 0) ;
    hlayout->addStretch(100) ;

    // next hbox layout
    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create third hbox layout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;
    if (!(m_button15 = new QPushButton(QString("Random BG"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create button15 ")) ;
    if (!connect(m_button15, SIGNAL(clicked(bool)), this, SLOT(commandFunction24())))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to connect button15 to appropriate slot ")) ;
    hlayout->addWidget(m_button15, 0) ;
    if (!(m_button16 = new QPushButton(QString("Create StS"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create button16")) ;
    if (!connect(m_button16, SIGNAL(clicked(bool)), this, SLOT(commandFunction25())))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to connect button16 to appropriate slot ")) ;
    hlayout->addWidget(m_button16) ;
    if (!(m_button17 = new QPushButton(QString("Delete StS"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create button17")) ;
    if (!connect(m_button17, SIGNAL(clicked(bool)), this, SLOT(commandFunction26())))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to connect button17 to appropriate slot ")) ;
    hlayout->addWidget(m_button17) ;
    if (!(m_button18 = new QPushButton(QString("Show StS"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create button18")) ;
    if (!connect(m_button18, SIGNAL(clicked(bool)), this, SLOT(commandFunction27())))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to connect button18 to appropriate slot ")) ;
    hlayout->addWidget(m_button18) ;
    if (!(m_button19 = new QPushButton(QString("Hide StS"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create button19 ")) ;
    if (!connect(m_button19, SIGNAL(clicked(bool)), this, SLOT(commandFunction28())))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to connect button19 to appropriate slot ")) ;
    hlayout->addWidget(m_button19) ;
    if (!(m_button21 = new QPushButton(QString("StS Random BG"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to create button21 ")) ;
    if (!connect(m_button21, SIGNAL(clicked(bool)), this, SLOT(commandFunction30())))
        throw std::runtime_error(std::string("ZDummyControl::createControls02() ; unable to connect button21 to appropriate slot ")) ;
    hlayout->addWidget(m_button21) ;

    hlayout->addStretch(100) ;

    layout->addStretch(100) ;

    return widget ;
}

// featuresets
QWidget* ZDummyControl::createControls03()
{
    QFontMetrics metrics(QApplication::font()) ;
    int width = metrics.width(m_numerical_target_mask) + 10,
            height = metrics.height() + 10 ;

    QWidget* widget = Q_NULLPTR ;
    if (!(widget = new QWidget))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create top level widget ")) ;

    QVBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = new QVBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create top level layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;


    QHBoxLayout *hlayout = Q_NULLPTR ;
    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create hlayout to contain grid ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;

    QGridLayout *grid = Q_NULLPTR ;
    if (!(grid = new QGridLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create grid layout ")) ;
    hlayout->addLayout(grid) ;
    grid->setMargin(0) ;
    hlayout->addStretch(100) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = new QLabel(QString("Sequence ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create sequence id label ")) ;
    grid->addWidget(label, 0, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_featureset01 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create edit featureset01 ")) ;
    m_edit_featureset01->setInputMask(m_numerical_target_mask) ;
    m_edit_featureset01->setFixedSize(width, height) ;
    grid->addWidget(m_edit_featureset01, 0, 1, 1, 1, Qt::AlignLeft) ;
    if (!(label = new QLabel(QString("Featureset ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create featureset id label ")) ;
    grid->addWidget(label, 1, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_featureset02 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create edit featureset02")) ;
    m_edit_featureset02->setInputMask(m_numerical_target_mask) ;
    m_edit_featureset02->setFixedSize(width, height) ;
    grid->addWidget(m_edit_featureset02, 1, 1, 1, 1, Qt::AlignLeft) ;


    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create first hbox layout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;
    if (!(m_button05 = new QPushButton(QString("Add"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create button05")) ;
    if (!connect(m_button05, SIGNAL(clicked(bool)), this, SLOT(commandFunction05())))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to connect button05 to appropriate slot")) ;
    hlayout->addWidget(m_button05, 0) ;
    if (!(m_button06 = new QPushButton(QString("Delete"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to create button06")) ;
    if (!connect(m_button06, SIGNAL(clicked(bool)), this, SLOT(commandFunction06())))
        throw std::runtime_error(std::string("ZDummyControl::createControls03() ; unable to connect button06 to appropriate slot")) ;
    hlayout->addWidget(m_button06, 0) ;
    hlayout->addStretch(100) ;


    layout->addStretch(100) ;

    return widget ;
}

// track operation controls
QWidget* ZDummyControl::createControls04()
{

    QFontMetrics metrics(QApplication::font()) ;
    int width = metrics.width(m_numerical_target_mask) + 10,
            height = metrics.height() + 10 ;

    QWidget *widget = Q_NULLPTR ;

    if (!(widget = new QWidget))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create top level widget ")) ;

    QVBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = new QVBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create top level layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    QHBoxLayout *hlayout = Q_NULLPTR ;
    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create hlayout to contain grid ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;

    QGridLayout *grid = Q_NULLPTR ;
    if (!(grid = new QGridLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create grid layout ")) ;
    hlayout->addLayout(grid) ;
    grid->setMargin(0) ;
    hlayout->addStretch(100) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = new QLabel(QString("Sequence ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create sequence id label ")) ;
    grid->addWidget(label, 0, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_track01 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create edit track01 ")) ;
    m_edit_track01->setInputMask(m_numerical_target_mask) ;
    m_edit_track01->setFixedSize(width, height) ;
    grid->addWidget(m_edit_track01, 0, 1, 1, 1, Qt::AlignLeft) ;
    if (!(label = new QLabel(QString("Track ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create featureset id label ")) ;
    grid->addWidget(label, 1, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_track02 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create edit track02")) ;
    m_edit_track02->setInputMask(m_numerical_target_mask) ;
    m_edit_track02->setFixedSize(width, height) ;
    grid->addWidget(m_edit_track02, 1, 1, 1, 1, Qt::AlignLeft) ;
    if (!(label = new QLabel(QString("Strand Sensitive:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create strand sensitive label")) ;
    grid->addWidget(label, 1, 2, 1, 1, Qt::AlignRight) ;
    if (!(m_check_track_strand = new QCheckBox))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create strand sensitive check box")) ;
    grid->addWidget(m_check_track_strand, 1, 3, 1, 1) ;

    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create first hbox layout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;
    if (!(m_button07 = new QPushButton(QString("Add"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create button07")) ;
    if (!connect(m_button07, SIGNAL(clicked(bool)), this, SLOT(commandFunction07())))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to connect button07 to appropriate slot")) ;
    hlayout->addWidget(m_button07, 0) ;
    if (!(m_button08 = new QPushButton(QString("Delete"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create button08")) ;
    if (!connect(m_button08, SIGNAL(clicked(bool)), this, SLOT(commandFunction08())))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to connect button08 to appropriate slot")) ;
    hlayout->addWidget(m_button08, 0) ;
    if (!(m_button13 = new QPushButton(QString("Show"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create button13")) ;
    if (!connect(m_button13, SIGNAL(clicked(bool)), this, SLOT(commandFunction22())))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to connect button13 to appropriate slot ")) ;
    hlayout->addWidget(m_button13, 0) ;
    if (!(m_button14 = new QPushButton(QString("Hide"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create button14 ")) ;
    if (!connect(m_button14, SIGNAL(clicked(bool)), this, SLOT(commandFunction23())))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to connect button14 to appropriate slot ")) ;
    hlayout->addWidget(m_button14, 0) ;
    if (!(m_button20 = new QPushButton(QString("Random BG"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to create button20 ")) ;
    if (!connect(m_button20, SIGNAL(clicked(bool)), this, SLOT(commandFunction29())))
        throw std::runtime_error(std::string("ZDummyControl::createControls04() ; unable to connect button20 to appropriate slot ")) ;
    hlayout->addWidget(m_button20, 0) ;
    hlayout->addStretch(100) ;

    layout->addStretch(100) ;

    return widget ;
}






QWidget* ZDummyControl::createControls05()
{
    QWidget* widget = Q_NULLPTR  ;

    QFontMetrics metrics(QApplication::font()) ;
    int width = metrics.width(m_numerical_target_mask) + 10,
            height = metrics.height() + 10 ;

    if (!(widget = new QWidget))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create top level widget ")) ;

    QVBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = new QVBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create top level layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    QHBoxLayout *hlayout = Q_NULLPTR ;
    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create hlayout to contain grid ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;

    QGridLayout *grid = Q_NULLPTR ;
    if (!(grid = new QGridLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create grid layout ")) ;
    hlayout->addLayout(grid) ;
    grid->setMargin(0) ;
    hlayout->addStretch(100) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = new QLabel(QString("Sequence ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create sequence id label ")) ;
    grid->addWidget(label, 0, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_ft1 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create edit ft1 ")) ;
    m_edit_ft1->setInputMask(m_numerical_target_mask) ;
    m_edit_ft1->setFixedSize(width, height) ;
    grid->addWidget(m_edit_ft1, 0, 1, 1, 1, Qt::AlignLeft) ;
    if (!(label = new QLabel(QString("Featureset ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create featureset id label ")) ;
    grid->addWidget(label, 1, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_ft2 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create edit ft2")) ;
    m_edit_ft2->setInputMask(m_numerical_target_mask) ;
    m_edit_ft2->setFixedSize(width, height) ;
    grid->addWidget(m_edit_ft2, 1, 1, 1, 1, Qt::AlignLeft) ;
    if (!(label = new QLabel(QString("Track ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create featureset id label ")) ;
    grid->addWidget(label, 2, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_ft3 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create edit ft3")) ;
    m_edit_ft3->setInputMask(m_numerical_target_mask) ;
    m_edit_ft3->setFixedSize(width, height) ;
    grid->addWidget(m_edit_ft3, 2, 1, 1, 1, Qt::AlignLeft) ;


    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create first hbox layout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;
    if (!(m_button09 = new QPushButton(QString("Add"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create button09")) ;
    if (!connect(m_button09, SIGNAL(clicked(bool)), this, SLOT(commandFunction09())))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to connect button09 to appropriate slot")) ;
    hlayout->addWidget(m_button09, 0) ;
    if (!(m_button10 = new QPushButton(QString("Delete"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to create button10")) ;
    if (!connect(m_button10, SIGNAL(clicked(bool)), this, SLOT(commandFunction10())))
        throw std::runtime_error(std::string("ZDummyControl::createControls05() ; unable to connect button10 to appropriate slot")) ;
    hlayout->addWidget(m_button10, 0) ;
    hlayout->addStretch(100) ;

    layout->addStretch(100) ;

    return widget ;
}


// features controls
QWidget* ZDummyControl::createControls06()
{

    QWidget* widget = Q_NULLPTR  ;

    QFontMetrics metrics(QApplication::font()) ;
    int width = metrics.width(m_numerical_target_mask) + 10,
            height = metrics.height() + 10 ;

    if (!(widget = new QWidget))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create top level widget ")) ;

    QVBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = new QVBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create top level layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    QHBoxLayout *hlayout = Q_NULLPTR ;
    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create hlayout to contain grid ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;

    QGridLayout *grid = Q_NULLPTR ;
    if (!(grid = new QGridLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create grid layout ")) ;
    hlayout->addLayout(grid) ;
    grid->setMargin(0) ;

    QLabel *label = Q_NULLPTR ;
    if (!(label = new QLabel(QString("Sequence ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create sequence id label ")) ;
    grid->addWidget(label, 0, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_features1 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create edit features1 ")) ;
    m_edit_features1->setInputMask(m_numerical_target_mask) ;
    m_edit_features1->setFixedSize(width, height) ;
    grid->addWidget(m_edit_features1, 0, 1, 1, 1, Qt::AlignLeft) ;
    if (!(label = new QLabel(QString("Featureset ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create featureset id label ")) ;
    grid->addWidget(label, 1, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_features2 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create edit features2 ")) ;
    m_edit_features2->setInputMask(m_numerical_target_mask) ;
    m_edit_features2->setFixedSize(width, height) ;
    grid->addWidget(m_edit_features2, 1, 1, 1, 1, Qt::AlignLeft) ;

    if (!(label = new QLabel(QString("Num. of features:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create number of features label ")) ;
    grid->addWidget(label, 0, 2, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_features3 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create edit features3 ")) ;
    m_edit_features3->setInputMask(m_numerical_target_mask) ;
    m_edit_features3->setFixedSize(width, height) ;
    grid->addWidget(m_edit_features3, 0, 3, 1, 1, Qt::AlignLeft) ;
    if (!(label = new QLabel(QString("Feature ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create feature id label ")) ;
    grid->addWidget(label, 1, 2, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_features4 = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create edit features4 ")) ;
    m_edit_features4->setInputMask(m_numerical_target_mask) ;
    m_edit_features4->setFixedSize(width, height) ;
    grid->addWidget(m_edit_features4, 1, 3, 1, 1, Qt::AlignLeft) ;
    if (!(label = new QLabel(QString("All:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create features label ")) ;
    grid->addWidget(label, 2, 2, 1, 1, Qt::AlignRight) ;
    if (!(m_check_features = new QCheckBox()))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create features check box ")) ;
    grid->addWidget(m_check_features, 2, 3, 1, 1, Qt::AlignLeft) ;

    hlayout->addStretch(100) ;


    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create hbox layout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;
    if (!(m_button11 = new QPushButton(QString("Add"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create button11")) ;
    if (!connect(m_button11, SIGNAL(clicked(bool)), this, SLOT(commandFunction11())))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to connect button09 to appropriate slot")) ;
    hlayout->addWidget(m_button11, 0) ;
    if (!(m_button12 = new QPushButton(QString("Delete"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to create button12")) ;
    if (!connect(m_button12, SIGNAL(clicked(bool)), this, SLOT(commandFunction12())))
        throw std::runtime_error(std::string("ZDummyControl::createControls06() ; unable to connect button12 to appropriate slot")) ;
    hlayout->addWidget(m_button12, 0) ;
    hlayout->addStretch(100) ;

    layout->addStretch(100) ;

    return widget ;

}


// control for various queries...
QWidget* ZDummyControl::createControls07()
{

    QFontMetrics metrics(QApplication::font()) ;
    int width = metrics.width(m_numerical_target_mask) + 10,
            height = metrics.height() + 10 ;

    QWidget * widget = Q_NULLPTR ;
    if (!(widget = new QWidget))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create top level widget ")) ;
    QVBoxLayout * layout = Q_NULLPTR ;
    if (!(layout = new QVBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create top level layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    // queries: gui ids present at the other end
    //          sequence_ids present at the other end...
    QHBoxLayout *hlayout = Q_NULLPTR ;
    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create hlayout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;

    QGridLayout *grid = Q_NULLPTR ;
    if (!(grid = new QGridLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create grid layout ")) ;
    hlayout->addLayout(grid) ;

    grid->setMargin(0) ;

    if (!(m_button_queries_gui_ids = new QPushButton(QString("Gui IDs:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create button_queries_gui_id")) ;
    grid->addWidget(m_button_queries_gui_ids, 0, 0, 1, 1) ;
    if (!(m_edit_queries_gui_list = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create edit_queries_gui_list")) ;
    grid->addWidget(m_edit_queries_gui_list, 0, 1, 1, 1 ) ;
    m_edit_queries_gui_list->setEnabled(false) ;
    if (!connect(m_button_queries_gui_ids, SIGNAL(clicked(bool)), this, SLOT(commandFunction16())))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to connect button_queries_gui_ids to appropriate slot ")) ;


    // get all sequence ids, and write them to the line edit
    if (!(m_button_queries_sequence_ids = new QPushButton(QString("Sequence IDs:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create for button_queries_sequence_ids ")) ;
    grid->addWidget(m_button_queries_sequence_ids, 1, 0, 1, 1) ;
    if (!(m_edit_queries_sequence_ids = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create edit_queries_sequence_id")) ;
    m_edit_queries_sequence_ids->setEnabled(false) ;
    grid->addWidget(m_edit_queries_sequence_ids, 1, 1, 1, 1) ;
    if (!connect(m_button_queries_sequence_ids, SIGNAL(clicked(bool)), this, SLOT(commandFunction19())))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to connect queries_sequence_ids ")) ;

    // query the featuresets or tracks in a given sequence
    QLabel* label = Q_NULLPTR ;
    if (!(label = new QLabel(QString("Sequence ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create label for seqid_input ")) ;
    grid->addWidget(label, 2, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_queries_seqid_input = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create edit_queries_seqid_input ")) ;
    m_edit_queries_seqid_input->setFixedSize(width, height) ;
    m_edit_queries_seqid_input->setInputMask(m_numerical_target_mask) ;
    grid->addWidget(m_edit_queries_seqid_input, 2, 1, 1, 1) ;

    // button and line edit pair, the slots will take sequence id input from m_edit_queries_seqid_input
    if (!(m_button_queries_featureset_ids = new QPushButton(QString("Get FS IDs:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create  button_queries_featureset_ids ")) ;
    grid->addWidget(m_button_queries_featureset_ids, 2, 2, 1, 1) ;
    if (!(m_edit_queries_featureset_ids = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create edit_queries_featureset_ids")) ;
    m_edit_queries_featureset_ids->setEnabled(false) ;
    grid->addWidget(m_edit_queries_featureset_ids, 2, 3, 1, 1) ;
    if (!connect(m_button_queries_featureset_ids, SIGNAL(clicked(bool)), this, SLOT(commandFunction14())))
        throw  std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to connect button_queries_featureset_ids to appropriate slot")) ;

    if (!(m_button_queries_track_ids = new QPushButton(QString("Get track IDs:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create button_queries_track_ids")) ;
    grid->addWidget(m_button_queries_track_ids, 3, 2, 1, 1) ;
    if (!(m_edit_queries_track_ids = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create edit_queries_track_ids ")) ;
    m_edit_queries_track_ids->setEnabled(false) ;
    grid->addWidget(m_edit_queries_track_ids, 3, 3, 1, 1) ;
    if (!connect(m_button_queries_track_ids, SIGNAL(clicked(bool)), this, SLOT(commandFunction15())))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to connect m_button_queries_track_ids to appropriate slot ")) ;


    if (!(label = new QLabel(QString("Featureset ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create label for featureset")) ;
    grid->addWidget(label, 4, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_queries_fset_input = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create entry for featureset ids")) ;
    m_edit_queries_fset_input->setFixedSize(width, height) ;
    m_edit_queries_fset_input->setInputMask(m_numerical_target_mask) ;
    grid->addWidget(m_edit_queries_fset_input, 4, 1, 1, 1 ) ;

    if (!(m_button_queries_feature_ids = new QPushButton(QString("Get feature IDs:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to get feature ids ")) ;
    grid->addWidget(m_button_queries_feature_ids, 4, 2, 1, 1) ;
    if (!connect(m_button_queries_feature_ids, SIGNAL(clicked(bool)), this, SLOT(commandFunction13())))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to connect m_button_queries_feature_ids to appropriate slot")) ;

    if (!(m_edit_queries_feature_ids = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls07() ; unable to create line edit for feature ids ")) ;
    m_edit_queries_feature_ids->setEnabled(false) ;
    grid->addWidget(m_edit_queries_feature_ids, 4, 3, 1, 1) ;

    layout->addStretch(100) ;

    return widget ;
}


QWidget* ZDummyControl::createControls08()
{

    QFontMetrics metrics(QApplication::font()) ;
    int width = metrics.width(m_numerical_target_mask) + 10,
            height = metrics.height() + 10 ;

    QWidget *widget = Q_NULLPTR ;
    if (!(widget = new QWidget))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create top level widget")) ;

    QVBoxLayout *layout = Q_NULLPTR ;
    if (!(layout = new QVBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create top level layout ")) ;
    widget->setLayout(layout) ;
    layout->setMargin(0) ;

    QHBoxLayout * hlayout = Q_NULLPTR ;
    if (!(hlayout = new QHBoxLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create hlayout ")) ;
    layout->addLayout(hlayout) ;
    hlayout->setMargin(0) ;

    QGridLayout *grid = Q_NULLPTR ;
    if (!(grid = new QGridLayout))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create grid layout ")) ;
    hlayout->addLayout(grid) ;
    grid->setMargin(0) ;


    int row = 0 ;
    QLabel *label = Q_NULLPTR ;
    if (!(label = new QLabel(QString("Sequence ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create sequence id label")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    // need a line edit here
    if (!(m_edit_seqgui_seqid_input = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create sequence id line edit ")) ;
    grid->addWidget(m_edit_seqgui_seqid_input, row, 1, 1, 1) ;
    m_edit_seqgui_seqid_input->setFixedSize(width, height) ;
    m_edit_seqgui_seqid_input->setInputMask(m_numerical_target_mask) ;
    ++row ;

    if (!(label = new QLabel(QString("Gui ID:"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create gui id label ")) ;
    grid->addWidget(label, row, 0, 1, 1, Qt::AlignRight) ;
    if (!(m_edit_seqgui_guiid_input = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create edit_misc_guiid_input")) ;
    grid->addWidget(m_edit_seqgui_guiid_input, row, 1, 1, 1) ;
    m_edit_seqgui_guiid_input->setFixedSize(width, height) ;
    m_edit_seqgui_guiid_input->setInputMask(m_numerical_target_mask) ;
    ++row ;

    if (!(m_button_add_sequence_to_gui = new QPushButton(QString("Add Sequence to Gui"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create button add_sequence_to_gui ")) ;
    grid->addWidget(m_button_add_sequence_to_gui, row, 0, 1, 1) ;
    if (!connect(m_button_add_sequence_to_gui, SIGNAL(clicked(bool)), this, SLOT(commandFunction20())))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to connect button_add_sequence_to_gui to appropriate slot ")) ;
    ++row ;

    if (!(m_button_delete_sequence_from_gui = new QPushButton("Remove Sequence from Gui")))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create button delete_sequence_from_gui")) ;
    grid->addWidget(m_button_delete_sequence_from_gui, row, 0, 1, 1) ;
    if (!connect(m_button_delete_sequence_from_gui, SIGNAL(clicked(bool)), this, SLOT(commandFunction21())))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to connect button_delete_sequence_from_gui to appropriate slot ")) ;
    ++row ;

    if (!(m_button_switch_orientation = new QPushButton(QString("Switch orientation"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create button_switch_orientation")) ;
    grid->addWidget(m_button_switch_orientation, row, 0, 1, 1) ;
    if (!connect(m_button_switch_orientation, SIGNAL(clicked(bool)), this, SLOT(commandFunction17())))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to connect button_switch_orientation to the appropriate slot ")) ;
    ++row ;

    if (!(m_button_set_user_home = new QPushButton(QString("Set user home"))))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create button_set_user_home")) ;
    grid->addWidget(m_button_set_user_home, row, 0, 1, 1) ;
    if (!(m_edit_user_home = new QLineEdit))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to create edit_user_home")) ;
    grid->addWidget(m_edit_user_home, row, 1, 1, 1) ;
    if (!connect(m_button_set_user_home, SIGNAL(clicked(bool)), this, SLOT(commandFunction18())))
        throw std::runtime_error(std::string("ZDummyControl::createControls08() ; unable to connect button_set_user_home to appropriate slot ")) ;

    layout->addStretch(100) ;

    return widget ;
}

} // namespace GUI

} // namespace ZMap
