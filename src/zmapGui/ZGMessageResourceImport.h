/*  File: ZGMessageResourceImport.h
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

#ifndef ZGMESSAGERESOURCEIMPORT_H
#define ZGMESSAGERESOURCEIMPORT_H

#include "ZGMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGResourceImportData.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

// Data to import a resource (e.g. sequence_id, name (file or url, etc), ...)

class ZGMessageResourceImport : public ZGMessage,
        public Util::InstanceCounter<ZGMessageResourceImport>,
        public Util::ClassName<ZGMessageResourceImport>
{
public:

    ZGMessageResourceImport();
    ZGMessageResourceImport(ZInternalID message_id, const ZGResourceImportData & data) ;
    ZGMessageResourceImport(const ZGMessageResourceImport& message) ;
    ZGMessageResourceImport& operator=(const ZGMessageResourceImport& message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageResourceImport() ;

    ZGResourceImportData getResouceImportData() const {return m_resource_data ; }

private:
    static const ZGMessageType m_specific_type ;

    ZGResourceImportData m_resource_data ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGMESSAGERESOURCEIMPORT_H
