/*  File: ZGPacControl.cpp
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

#include "ZGPacControl.h"
#include "ZGMessage.h"


namespace ZMap
{

namespace GUI
{


ZGPacControl::ZGPacControl()
    : m_peer()
{

}

void ZGPacControl::setPeer(const std::shared_ptr<ZGPacControl>& control)
{
    if (control.get() == this)
        return ;
    m_peer = control ;
}

ZGPacControl* ZGPacControl::detatchPeer()
{
    ZGPacControl *temp = m_peer.get() ;
    m_peer.reset() ;
    return temp ;
}

void ZGPacControl::send(const std::shared_ptr<ZGMessage>& message) const
{
    if (m_peer)
    {
        m_peer->receive(this, message) ;
    }
}

} // namespace GUI

} // namespace ZMap
