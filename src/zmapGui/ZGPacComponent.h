/*  File: ZGPacComponent.h
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

#ifndef ZGPACCOMPONENT_H
#define ZGPACCOMPONENT_H

#include <string>
#include <memory>


namespace ZMap
{

namespace GUI
{

class ZGPacControl ;
class ZGMessage ;

// Base class for the PAC components in the system. This models the component
// as a mediator colleague.
class ZGPacComponent
{
public:
    ZGPacComponent(ZGPacControl *control) ;
    virtual ~ZGPacComponent() {}

    virtual void receive(const std::shared_ptr<ZGMessage> &message) = 0 ;
    virtual std::shared_ptr<ZGMessage> query(const std::shared_ptr<ZGMessage>& message) = 0 ;

protected:

    ZGPacControl *m_control ;

private:

    ZGPacComponent(const ZGPacComponent& component) = delete ;
    ZGPacComponent& operator=(const ZGPacComponent& component) = delete ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGPACCOMPONENT_H
