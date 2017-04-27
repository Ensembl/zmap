/*  File: ZGuiViewControl.h
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

#ifndef ZGUIVIEWCONTROL_H
#define ZGUIVIEWCONTROL_H

#include <cstddef>
#include <string>
#include <QApplication>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QHBoxLayout ;
class QVBoxLayout ;
class QPushButton ;
class QLabel ;
QT_END_NAMESPACE

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGTFTActionType.h"
#include "ZGZoomActionType.h"

//
// This is the control panel for the ZGuiGraphicsView class of which we have possibly
// many instances in a gui.
//


namespace ZMap
{

namespace GUI
{

class ZGuiGraphicsView ;
class ZGuiSequence ;
class ZGuiTextDisplayDialogue ;
class ZGuiTrackConfigureDialogue ;
class ZGuiEditStyleDialogue ;
class ZGuiChooseStyleDialogue ;
class ZGuiDialogueSearchDNA ;
class ZGuiDialogueSearchFeatures ;
class ZGuiDialogueListFeatures ;
class ZGuiDialogueNewSource ;
class ZGuiDialoguePreferences ;
class ZGuiWidgetControlPanel ;
class ZGuiWidgetInfoPanel ;

class ZGuiViewControl : public QWidget,
        public Util::InstanceCounter<ZGuiViewControl>,
        public Util::ClassName<ZGuiViewControl>
{
    Q_OBJECT

public:

    static ZGuiViewControl* newInstance(QWidget* parent = Q_NULLPTR) ;
    ~ZGuiViewControl() ;

    void setStatusLabel(const QString & label,
                        const QString & tooltip) ;

    void setCurrentView(ZGuiGraphicsView* current_view) ;
    ZGuiGraphicsView* getCurrentView() const {return m_current_view ; }

    void setCurrentSequence(ZGuiSequence *current_sequence) ;
    ZGuiSequence* getCurrentSequence() const {return m_current_sequence ; }

signals:

public slots:

    void slot_setCurrentSequence(ZGuiSequence* current_sequence) ;

    void slot_stop() ;
    void slot_reload() ;
    void slot_back() ;
    void slot_revcomp() ;
    void slot_3frame(ZGTFTActionType type) ;
    void slot_dna() ;
    void slot_columns() ;
    void slot_filter(int) ;
    void slot_zoomaction(ZGZoomActionType type) ;

private:

    ZGuiViewControl(QWidget* parent=Q_NULLPTR);
    ZGuiViewControl(const ZGuiViewControl&) = delete ;
    ZGuiViewControl& operator=(const ZGuiViewControl&) = delete ;

    QVBoxLayout *m_top_layout ;
    QLabel *m_label_sequence ;
    ZGuiWidgetControlPanel *m_control_panel ;
    ZGuiWidgetInfoPanel *m_info_panel ;

    ZGuiGraphicsView *m_current_view ;
    ZGuiSequence *m_current_sequence ;

    ZInternalID m_current_track,
        m_current_featureset,
        m_current_feature ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIVIEWCONTROL_H
