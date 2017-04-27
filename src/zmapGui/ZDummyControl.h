/*  File: ZDummyControl.h
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

#ifndef ZDUMMYCONTROL_H
#define ZDUMMYCONTROL_H

#include <QApplication>
#include <QWidget>
#include <QString>
class QPushButton;
class QLayout;
class QGridLayout ;
class QLineEdit ;
class QCheckBox ;

#include <cstddef>
#include <string>
#include <set>
#include <map>
#include <ostream>

#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGPacControl.h"
#include "ZGMessageID.h"
#include "ZGFeature.h"
#include "ZGFeatureset.h"
#include "ZGStrandType.h"
class ZGMessage ;


namespace ZMap
{

namespace GUI
{

//
// dummy controller for testing communications
//
class ZDummyControl : public QWidget,
        public ZGPacControl,
        public Util::InstanceCounter<ZDummyControl>,
        public Util::ClassName<ZDummyControl>
{
    Q_OBJECT

public:

    ZDummyControl() ;
    ~ZDummyControl() ;

    void create() {}
    void receive(const ZGPacControl* const sender,
                 std::shared_ptr<ZGMessage> message) override ;
    std::shared_ptr<ZGMessage> query(const ZGPacControl * const sender,
                                     const std::shared_ptr<ZGMessage>& message) override ;

public slots:
    void commandFunction000() ;
    void commandFunction001() ;
    void commandFunction002() ;
    void commandFunction003() ;

    void commandFunction004() ;
    void commandFunction005() ;
    void commandFunction006() ;
    void commandFunction007() ;

    void commandFunction03() ;
    void commandFunction04() ;
    void commandFunction05() ;
    void commandFunction06() ;
    void commandFunction07() ;
    void commandFunction08() ;
    void commandFunction09() ;
    void commandFunction10() ;
    void commandFunction11() ;
    void commandFunction12() ;
    void commandFunction13() ;
    void commandFunction14() ;
    void commandFunction15() ;
    void commandFunction16() ;
    void commandFunction17() ;
    void commandFunction18() ;
    void commandFunction19() ;
    void commandFunction20() ;
    void commandFunction21() ;
    void commandFunction22() ;
    void commandFunction23() ;
    void commandFunction24() ;
    void commandFunction25() ;
    void commandFunction26() ;
    void commandFunction27() ;
    void commandFunction28() ;
    void commandFunction29() ;
    void commandFunction30() ;

private:
    ZDummyControl(const ZDummyControl&) = delete ;
    ZDummyControl& operator=(const ZDummyControl&) = delete ;

    unsigned int uintFromQLineEdit(QLineEdit *line_edit) ;
    void outputPeerData() ;
    ZInternalID outputPeerData_01(ZInternalID sequence_id, ZInternalID featureset_id) ;
    std::set<ZInternalID> outputPeerData_02(ZInternalID sequence_id, ZInternalID track_id) ;

    void populateFeatureset(std::shared_ptr<ZGFeatureset>& featureset,
                            ZGFeatureType type,
                            ZGStrandType strand,
                            unsigned int nFeatures,
                            const double &start,
                            const double &end,
                            unsigned int seed) ;
    ZGColor generateRandomColor() ;

    QWidget* createControls00() ;
    QWidget* createControls01() ;
    QWidget* createControls02() ;
    QWidget* createControls03() ;
    QWidget* createControls04() ;
    QWidget* createControls05() ;
    QWidget* createControls06() ;
    QWidget* createControls07() ;
    QWidget* createControls08() ;

    static const QString m_display_name, m_numerical_target_mask ;
    static const ZInternalID m_id_init ;

    ZGMessageID m_id_generator ;
    QPushButton
        *m_button000,
        *m_button001,
        *m_button002,
        *m_button003,

        *m_button004,
        *m_button005,
        *m_button006,
        *m_button007,

        *m_button03,
        *m_button04,
        *m_button05,
        *m_button06,
        *m_button07,
                *m_button08,
                *m_button09,
                *m_button10,
                *m_button11,
                *m_button12,
                *m_button13,
                *m_button14,
                *m_button15,
                *m_button16,
                *m_button17,
                *m_button18,
                *m_button19,
                *m_button20,
                *m_button21,
                *m_button22,
                *m_button_queries_gui_ids,
                *m_button_queries_sequence_ids,
                *m_button_queries_featureset_ids,
                *m_button_queries_track_ids,
                *m_button_queries_feature_ids,
                *m_button_set_user_home,
                *m_button_switch_orientation,
                *m_button_add_sequence_to_gui,
                *m_button_delete_sequence_from_gui,
                *m_button_EXIT ;

    QLineEdit *m_edit_gui_id,
              *m_edit_sequence_id,
              *m_edit_featureset01,
              *m_edit_featureset02,
              *m_edit_track01,
              *m_edit_track02,
              *m_edit_ft1,
              *m_edit_ft2,
              *m_edit_ft3,
              *m_edit_features1,
              *m_edit_features2,
              *m_edit_features3,
              *m_edit_features4,
              *m_edit_queries_gui_list,
              *m_edit_queries_sequence_ids,
              *m_edit_queries_seqid_input,
              *m_edit_queries_featureset_ids,
              *m_edit_queries_track_ids,
              *m_edit_queries_fset_input,
              *m_edit_queries_feature_ids,
              *m_edit_seqgui_guiid_input,
              *m_edit_seqgui_seqid_input,
              *m_edit_user_home ;
    QCheckBox *m_check_features,
        *m_check_track_strand ;

};

std::ostream& operator<<(std::ostream& str, const std::set<ZInternalID>& data) ;

} // namespace GUI

} // namespace ZMap


#endif // ZDUMMYCONTROL_H
