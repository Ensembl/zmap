/*  File: ZGMessageSequenceToGui.h
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

#ifndef ZGMESSAGESEQUENCETOGUI_H
#define ZGMESSAGESEQUENCETOGUI_H

#include "ZGMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{


// add or remove a sequence from a gui

class ZGMessageSequenceToGui: public ZGMessage,
        public Util::InstanceCounter<ZGMessageSequenceToGui>,
        public Util::ClassName<ZGMessageSequenceToGui>
{
public:

    enum class Type: unsigned char
    {
        Invalid,
        Add,
        Remove
    };

    ZGMessageSequenceToGui() ;
    ZGMessageSequenceToGui(ZInternalID message_id,
                              ZInternalID sequence_id,
                              ZInternalID gui_id,
                              Type operation_type ) ;
    ZGMessageSequenceToGui(const ZGMessageSequenceToGui& message) ;
    ZGMessageSequenceToGui& operator=(const ZGMessageSequenceToGui& message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageSequenceToGui() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getGuiID() const {return m_gui_id ; }
    Type getOperationType() const {return m_operation_type ; }

private:
    static const ZGMessageType m_specific_type ;
    static const Type m_default_operation_type ;

    ZInternalID m_sequence_id,
        m_gui_id ;
    Type m_operation_type ;
};

} // namespace GUI

} // namespace ZMap

#endif
