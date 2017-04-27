/*  File: ZDebug.cpp
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

#include "ZDebug.h"
#include "ZFeatureBasic.h"
#include "ZFeatureComplex.h"
#include "ZFeatureGeneral.h"
#include "ZTrackSimple.h"
#include "ZTrackCompound.h"
#include "ZWTrackSimple.h"
#include "ZWTrackGeneral.h"
#include "ZWTrackCollection.h"
#include "ZWTrackSeparator.h"
#include "TestView01.h"
#include "TestView02.h"
#include "TestGraphicsView02.h"
#include "ZViewNode.h"
#include "ZFeaturePainterBox01.h"
#include "ZFeaturePainterCompound.h"
#include "ZFeaturePainterConn01.h"
#include "ZFeaturePainterConn02.h"

#include "ZGSequence.h"
#include "ZGFeatureset.h"
#include "ZGFeaturesetSet.h"
#include "ZGStyle.h"
#include "ZGFeature.h"
#include "ZGTrack.h"
#include "ZGColor.h"
#include "ZGFeatureBounds.h"
#include "ZGResourceImportData.h"
#include "ZGSequenceData.h"

#include "ZGMessageID.h"
#include "ZGMessageHeader.h"

#include "ZGPacMessageHeader.h"

#include "ZGCache.h"
#include "ZGFeaturesetCollection.h"
#include "ZGTrackCollection.h"

#include "ZGuiControl.h"
#include "ZDummyControl.h"
#include "ZGuiAbstraction.h"
#include "ZGuiPresentation.h"

#include "ZGui.h"
#include "ZGuiMain.h"
#include "ZGuiMainStyle.h"
#include "ZGuiView.h"
#include "ZGuiViewNode.h"
#include "ZGuiViewControl.h"
#include "ZGuiTopControl.h"
#include "ZGuiGraphicsView.h"
#include "ZGuiScene.h"
#include "ZGuiSequences.h"
#include "ZGuiFileImportDialogue.h"
#include "ZGuiFileImportStrandValidator.h"
#include "ZGuiSequenceCoordinateValidator.h"
#include "ZGuiCoordinateValidator.h"
#include "ZGuiSequenceContextMenu.h"
#include "ZGuiTextDisplayDialogue.h"
#include "ZGuiTrackFilterDialogue.h"
#include "ZGuiTrackConfigureDialogue.h"
#include "ZGuiEditStyleDialogue.h"
#include "ZGuiChooseStyleDialogue.h"
#include "ZGuiDialogueSearchDNA.h"
#include "ZGuiDialogueSearchDNAStyle.h"
#include "ZGuiDialogueSearchFeatures.h"
#include "ZGuiDialogueSearchFeaturesStyle.h"
#include "ZGuiDialogueListFeatures.h"
#include "ZGuiContextMenuDeveloper.h"
#include "ZGuiContextMenuFiltering.h"
#include "ZGuiContextMenuSearch.h"
#include "ZGuiContextMenuExport.h"
#include "ZGuiToplevel.h"
#include "ZGuiToplevelStyle.h"
#include "ZGuiDialogueNewSource.h"
#include "ZGuiDialogueSelectItems.h"
#include "ZGuiWidgetSelectSequence.h"
#include "ZGuiWidgetButtonColour.h"
#include "ZGuiSceneCollection.h"
#include "ZGuiFeatureset.h"
#include "ZGuiWidgetOverlayScale.h"

#include <iostream>
#include <sstream>
using namespace std ;


namespace ZMap
{

namespace GUI
{

ZDebug::ZDebug()
{

}

void ZDebug::String2cout(const std::string &s)
{
    cout << s << endl;
}

void ZDebug::String2cerr(const std::string &s)
{
    cerr << s << endl;
}

template <typename C>
std::string instanceTestFunction()
{
    std::stringstream str ;
    str << C::className() << ", instances = " << C::instances() ;
    return str.str() ;
}

template<typename C>
void instanceTestFunction(std::ostream & str)
{
    unsigned int i = C::instances() ;
    if (i)
        str << C::className() << " (" << i << ") " ;
}


// an instance test of everything associated with this project
void ZDebug::instanceTest_All()
{
    using namespace std ;

    cout << "ZDebug::instanceTest_All: " ;

    // features
    instanceTestFunction<ZFeatureBasic>(cout)  ;
    instanceTestFunction<ZFeatureComplex>(cout)  ;
    instanceTestFunction<ZFeatureGeneral>(cout)  ;

    // tracks
    instanceTestFunction<ZTrackSimple>(cout)  ;
    instanceTestFunction<ZTrackCompound>(cout)  ;
    instanceTestFunction<ZTrackSimple>(cout)  ;
    instanceTestFunction<ZWTrackGeneral>(cout)  ;
    instanceTestFunction<ZWTrackCollection>(cout)  ;
    instanceTestFunction<ZWTrackSeparator>(cout) ;

    // feature painters
    instanceTestFunction<ZFeaturePainterBox01>(cout)  ;
    instanceTestFunction<ZFeaturePainterCompound>(cout)  ;
    instanceTestFunction<ZFeaturePainterConn01>(cout)  ;
    instanceTestFunction<ZFeaturePainterConn02>(cout)  ;

    // test view 01 and 02
    instanceTestFunction<TestGraphicsView01>(cout)  ;
    instanceTestFunction<TestGraphicsView02>(cout)  ;
    instanceTestFunction<TestView01>(cout)  ;
    instanceTestFunction<TestView02>(cout)  ;

    // graph nodes
    instanceTestFunction<ZViewNode01>(cout)  ;
    instanceTestFunction<ZViewNode02>(cout)  ;
    instanceTestFunction<ZViewNode03>(cout)  ;
    instanceTestFunction<ZViewNode04>(cout)  ;
    instanceTestFunction<ZViewNode05>(cout)  ;

    // data cache classes
    instanceTestFunction<ZGSequence>(cout)  ;
    instanceTestFunction<ZGStyle>(cout)  ;
    instanceTestFunction<ZGFeatureset>(cout)  ;
    instanceTestFunction<ZGFeaturesetSet>(cout)  ;
    instanceTestFunction<ZGFeature>(cout)  ;
    instanceTestFunction<ZGTrack>(cout)  ;
    instanceTestFunction<ZGColor>(cout)  ;
    instanceTestFunction<ZGCache>(cout)  ;
    instanceTestFunction<ZGFeaturesetCollection>(cout)  ;
    instanceTestFunction<ZGTrackCollection>(cout)  ;
    instanceTestFunction<ZGResourceImportData>(cout) ;
    instanceTestFunction<ZGSequenceData>(cout) ;

    // external message classes
    instanceTestFunction<ZGMessageID>(cout)  ;
    instanceTestFunction<ZGMessageAddFeatures>(cout)  ;
    instanceTestFunction<ZGMessageFeaturesetOperation>(cout)  ;
    instanceTestFunction<ZGMessageTrackOperation>(cout)  ;
    instanceTestFunction<ZGMessageSequenceToGui>(cout)  ;
    instanceTestFunction<ZGMessageDeleteFeatures>(cout)  ;
    instanceTestFunction<ZGMessageDeleteFeatureset>(cout)  ;
    instanceTestFunction<ZGMessageDeleteTrack>(cout)  ;
    instanceTestFunction<ZGMessageDeleteSequence>(cout)  ;
    instanceTestFunction<ZGMessageReplyStatus>(cout)  ;
    instanceTestFunction<ZGMessageReplyDataIDSet>(cout)  ;
    instanceTestFunction<ZGMessageReplyLogMessage>(cout) ;
    instanceTestFunction<ZGMessageGuiOperation>(cout)  ;
    instanceTestFunction<ZGMessageQuerySequenceIDs>(cout)  ;
    instanceTestFunction<ZGMessageQueryFeaturesetIDs>(cout)  ;
    instanceTestFunction<ZGMessageQueryTrackIDs>(cout)  ;
    instanceTestFunction<ZGMessageQueryFeatureIDs>(cout)  ;
    instanceTestFunction<ZGMessageQueryGuiIDs>(cout)  ;
    instanceTestFunction<ZGMessageSwitchOrientation>(cout)  ;
    instanceTestFunction<ZGMessageResourceImport>(cout) ;

    // concrete pac classes
    instanceTestFunction<ZGuiControl>(cout)  ;
    instanceTestFunction<ZDummyControl>(cout)  ;
    instanceTestFunction<ZGuiAbstraction>(cout)  ;
    instanceTestFunction<ZGuiPresentation>(cout)  ;
    instanceTestFunction<ZGui>(cout)  ;
    instanceTestFunction<ZGuiMain>(cout)  ;
    instanceTestFunction<ZGuiMainStyle>(cout)  ;
    instanceTestFunction<ZGuiFileImportDialogue>(cout) ;
    instanceTestFunction<ZGuiFileImportStrandValidator>(cout) ;
    instanceTestFunction<ZGuiView>(cout) ;
    instanceTestFunction<ZGuiViewNode>(cout)  ;
    instanceTestFunction<ZGuiViewControl>(cout)  ;
    instanceTestFunction<ZGuiTopControl>(cout)  ;
    instanceTestFunction<ZGuiGraphicsView>(cout)  ;
    instanceTestFunction<ZGuiScene>(cout)  ;
    instanceTestFunction<ZGuiSequences>(cout)  ;
    instanceTestFunction<ZGuiSequenceCoordinateValidator>(cout) ;
    instanceTestFunction<ZGuiCoordinateValidator>(cout) ;
    instanceTestFunction<ZGuiTextDisplayDialogue>(cout) ;
    instanceTestFunction<ZGuiSequenceContextMenu>(cout) ;
    instanceTestFunction<ZGuiContextMenuDeveloper>(cout) ;
    instanceTestFunction<ZGuiContextMenuFiltering>(cout) ;
    instanceTestFunction<ZGuiContextMenuSearch>(cout) ;
    instanceTestFunction<ZGuiContextMenuExport>(cout) ;
    instanceTestFunction<ZGuiTrackFilterDialogue>(cout) ;
    instanceTestFunction<ZGuiTrackConfigureDialogue>(cout) ;
    instanceTestFunction<ZGuiEditStyleDialogue>(cout) ;
    instanceTestFunction<ZGuiChooseStyleDialogue>(cout) ;
    instanceTestFunction<ZGuiDialogueSearchDNA>(cout) ;
    instanceTestFunction<ZGuiDialogueSearchDNAStyle>(cout) ;
    instanceTestFunction<ZGuiDialogueSearchFeatures>(cout) ;
    instanceTestFunction<ZGuiDialogueSearchFeaturesStyle>(cout) ;
    instanceTestFunction<ZGuiDialogueListFeatures>(cout) ;
    instanceTestFunction<ZGuiToplevel>(cout) ;
    instanceTestFunction<ZGuiToplevelStyle>(cout) ;
    instanceTestFunction<ZGuiDialogueNewSource>(cout) ;
    instanceTestFunction<ZGuiDialogueSelectItems>(cout) ;
    instanceTestFunction<ZGuiWidgetSelectSequence>(cout) ;
    instanceTestFunction<ZGuiWidgetButtonColour>(cout) ;
    instanceTestFunction<ZGuiSceneCollection>(cout) ;
    instanceTestFunction<ZGuiFeatureset>(cout) ;
    instanceTestFunction<ZGuiWidgetOverlayScale>(cout) ;

    // concrete pac message classes
    instanceTestFunction<ZGPacMessageFeatureSelected>(cout)  ;
    instanceTestFunction<ZGPacMessageFeatureUnselected>(cout)  ;
    instanceTestFunction<ZGPacMessageTrackSelected>(cout)  ;
    instanceTestFunction<ZGPacMessageTrackUnselected>(cout)  ;
    instanceTestFunction<ZGPacMessageAddFeatures>(cout)  ;
    instanceTestFunction<ZGPacMessageDeleteFeatures>(cout)  ;
    instanceTestFunction<ZGPacMessageFeaturesetOperation>(cout)  ;
    instanceTestFunction<ZGPacMessageDeleteFeatureset>(cout)  ;
    instanceTestFunction<ZGPacMessageTrackOperation>(cout)  ;
    instanceTestFunction<ZGPacMessageDeleteTrack>(cout)  ;
    instanceTestFunction<ZGPacMessageSequenceOperation>(cout)  ;
    instanceTestFunction<ZGPacMessageDeleteSequence>(cout)  ;
    instanceTestFunction<ZGPacMessageGuiOperation>(cout)  ;
    instanceTestFunction<ZGPacMessageReplyStatus>(cout)  ;
    instanceTestFunction<ZGPacMessageReplyDataIDSet>(cout)  ;
    instanceTestFunction<ZGPacMessageReplyLogMessage>(cout) ;
    instanceTestFunction<ZGPacMessageQuerySequenceIDs>(cout)  ;
    instanceTestFunction<ZGPacMessageQueryFeaturesetIDs>(cout)  ;
    instanceTestFunction<ZGPacMessageQueryTrackIDs>(cout)  ;
    instanceTestFunction<ZGPacMessageQueryFeatureIDs>(cout)  ;
    instanceTestFunction<ZGPacMessageQueryGuiIDs>(cout)  ;
    instanceTestFunction<ZGPacMessageSwitchOrientation>(cout)  ;
    instanceTestFunction<ZGPacMessageResourceImport>(cout) ;

    cout << endl ;

}

} // namespace GUI

} // namespace ZMap

