/*  File: ZGuiAbstraction.h
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

#ifndef ZGABSTRACTION_H
#define ZGABSTRACTION_H

#include <cstddef>
#include <string>
#include "ZGPacComponent.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGCache.h"
#include "ZGUserData.h"


namespace ZMap
{

namespace GUI
{

class ZGMessageSequenceOperation ;
class ZGMessageFeaturesetOperation ;
class ZGMessageTrackOperation ;
class ZGMessageFeaturesetToTrack ;

// Gui abstraction
class ZGuiAbstraction: public ZGPacComponent,
        public Util::InstanceCounter<ZGuiAbstraction>,
        public Util::ClassName<ZGuiAbstraction>
{
public:
    static ZGuiAbstraction* newInstance(ZGPacControl *control) ;

    ~ZGuiAbstraction() ;

    void receive(const std::shared_ptr<ZGMessage> & message) override ;
    std::shared_ptr<ZGMessage> query(const std::shared_ptr<ZGMessage> & message) override ;
    bool contains(ZInternalID id) const ;

private:

    ZGuiAbstraction(ZGPacControl *control);
    ZGuiAbstraction(const ZGuiAbstraction & abstraction) = delete ;
    ZGuiAbstraction& operator=(const ZGuiAbstraction& abstraction) = delete ;

    void processSequenceOperation(const std::shared_ptr<ZGMessageSequenceOperation>& message) ;
    void processFeaturesetOperation(const std::shared_ptr<ZGMessageFeaturesetOperation>& message) ;
    void processTrackOperation(const std::shared_ptr<ZGMessageTrackOperation>& message) ;
    void processFeaturesetToTrack(const std::shared_ptr<ZGMessageFeaturesetToTrack>& message) ;

    ZGCache m_cache ;
    ZGUserData m_user_data ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGABSTRACTION_H
