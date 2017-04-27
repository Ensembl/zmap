/*  File: ZGuiPresentation.h
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

#ifndef ZGPRESENTATION_H
#define ZGPRESENTATION_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGPacComponent.h"
#include "ZInternalID.h"
#include "ZGMessageID.h"
#include "ZGResourceImportData.h"
#include <cstddef>
#include <set>
#include <QObject>

namespace ZMap
{

namespace GUI
{

class ZGuiMain ;
class ZGuiToplevel ;
class ZGMessageGuiOperation ;
class ZGMessageToplevelOperation ;
class ZGMessageSequenceOperation ;

// Gui presentation
class ZGuiPresentation: public QObject,
        public ZGPacComponent,
        public Util::InstanceCounter<ZGuiPresentation>,
        public Util::ClassName<ZGuiPresentation>
{
    Q_OBJECT

public:
    static ZGuiPresentation * newInstance(ZGPacControl* control) ;

    ZGuiPresentation(ZGPacControl *control) ;
    ~ZGuiPresentation() ;

    void receive(const std::shared_ptr<ZGMessage> & message) override ;
    std::shared_ptr<ZGMessage> query(const std::shared_ptr<ZGMessage>& message) override ;

//signals:
//    void signal_log_message(const std::string& str ) ;

public slots:
    void slot_toplevel_closed() ;
    void slot_toplevel_quit() ;
    void slot_log_message(const std::string& message) ;

private:

    ZGuiPresentation(const ZGuiPresentation& presentation) = delete ;
    ZGuiPresentation& operator=(const ZGuiPresentation& presentation) = delete ;

    bool connect_toplevel() ;
    void processToplevelOperation(const std::shared_ptr<ZGMessageToplevelOperation>& message) ;

    static const ZInternalID m_id_init,
        m_toplevel_default_id ;

    ZGMessageID m_id_generator ;
    ZGuiToplevel * m_toplevel ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGPRESENTATION_H
