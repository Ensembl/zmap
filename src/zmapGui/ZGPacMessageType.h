/*  File: ZGPacMessageType.h
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

#ifndef ZGPacMessageType_H
#define ZGPacMessageType_H

#include <cstdint>

namespace ZMap
{

namespace GUI
{

//
// Types for internal PAC messages only.
//
enum class ZGPacMessageType: std::uint8_t
{

    Invalid,

    ToplevelOperation,
    GuiOperation, // now replaces OpenGui, CloseGui,
    SequenceOperation, // should replace AddSequence and DeleteSequence
    SequenceToGui, //
    FeaturesetOperation,
    TrackOperation,
    FeaturesetToTrack,
    SwitchOrientation,
    SetSequenceBGColor,
    SetTrackBGColor,
    SeparatorOperation,
    SetSeparatorColor,
    SetGuiMenuBarColor,
    SetGuiSessionColor,

    FeatureSelected,
    FeatureUnselected,

    TrackSelected,
    TrackUnselected,

    AddSeparator,
    DeleteSeparator,

    DeleteFeatureset,
    DeleteFeaturesetFromTrack,
    SetFeaturesetStyle,

    SetViewOrientation,
    SetSequenceOrder,
    SetStatusLabel,
    ScaleBarOperation,
    ScaleBarPosition,

    AddFeatures,
    DeleteFeatures,
    DeleteAllFeatures,

    SetUserHome,

    ResourceImport,

    QueryGuiIDs,
    QuerySequenceIDs,
    QuerySequenceIDsAll,
    QueryFeaturesetIDs,
    QueryTrackIDs,
    QueryFeatureIDs,
    QueryFeaturesetsFromTrack,
    QueryTrackFromFeatureset,
    QueryUserHome,

    ReplyStatus,
    ReplyDataID,
    ReplyDataIDSet,
    ReplyLogMessage,
    ReplyString,

    Quit

} ;

} // namespace GUI

} // namespace ZMap

#endif
