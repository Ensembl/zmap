/*  File: ZGuiControl.h
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

#ifndef ZGUICONTROL_H
#define ZGUICONTROL_H

#include <cstddef>
#include <string>

#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGPacControl.h"
#include "ZGMessageID.h"

namespace ZMap
{

namespace GUI
{

class ZGPacMessage ;
class ZGMessage ;
class ZGMessageGuiOperation ;
class ZGMessageSequenceOperation ;
class ZGMessageToplevelOperation ;
class ZGMessageSequenceToGui ;
class ZGMessageTrackOperation ;
class ZGMessageFeaturesetOperation ;
class ZGMessageFeaturesetToTrack ;
class ZGMessageSeparatorOperation ;
class ZGMessageScaleBarOperation ;

// Gui control/concrete mediator
class ZGuiControl: public ZGPacControl,
        public Util::InstanceCounter<ZGuiControl>,
        public Util::ClassName<ZGuiControl>
{
public:
    static ZGuiControl* newInstance() ;

    ZGuiControl() ;
    ~ZGuiControl() ;
    void receive(const ZGPacControl* const sender,
                 std::shared_ptr<ZGMessage>  message) override ;
    void changed(const ZGPacComponent * const component,
                 const std::shared_ptr<ZGMessage> &message) override ;
    std::shared_ptr<ZGMessage> query(const ZGPacControl * const sender,
                                     const std::shared_ptr<ZGMessage>& message ) override ;

private:

    ZGuiControl(const ZGuiControl& control) = delete ;
    ZGuiControl& operator=(const ZGuiControl& control) = delete ;

    static const std::string m_name ;
    static const ZInternalID m_id_init ;

    void create() override ;

    ZGMessageID m_id_generator ;
    std::unique_ptr<ZGPacComponent> m_presentation ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUICONTROL_H
