/*  File: ZGPacControl.h
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

#ifndef ZGCONTROL_H
#define ZGCONTROL_H
#include <cstddef>
#include <string>
#include <memory>


namespace ZMap
{

namespace GUI
{

class ZGPacComponent ;
class ZGMessage ;
class ZGPacMessage ;

// Base class for PAC controller, as a mediator class.
// Peer communications are modelled as being either synchronous (query(...))
// or asynchronous (receive(...)).
class ZGPacControl
{
public:
    ZGPacControl() ;
    virtual ~ZGPacControl() {}

    virtual void create() = 0 ;
    virtual void changed(const ZGPacComponent * const component,
                         const std::shared_ptr<ZGMessage>& message) { }
    virtual void receive(const ZGPacControl * const sender,
                         std::shared_ptr<ZGMessage> message) = 0 ;
    virtual std::shared_ptr<ZGMessage> query(const ZGPacControl * const sender,
                                             const std::shared_ptr<ZGMessage>& message) = 0 ;
    void setPeer(const std::shared_ptr<ZGPacControl>& control) ;
    ZGPacControl* detatchPeer() ;

protected:

    void send(const std::shared_ptr<ZGMessage> &message) const ;
    std::shared_ptr<ZGPacControl> m_peer ;

private:

    ZGPacControl(const ZGPacControl& control) = delete ;
    ZGPacControl& operator=(const ZGPacControl& control) = delete ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGCONTROL_H
