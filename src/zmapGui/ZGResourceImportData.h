/*  File: ZGResourceImportData.h
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

#ifndef ZGRESOURCEIMPORTDATA_H
#define ZGRESOURCEIMPORTDATA_H

#include "ZGStrandType.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <cstddef>
#include <string>
#include <cstdint>

//
// Class to store and pass around all of the data associated with import
// of some resource, e.g. file or ftp:bam thing.
//
// Note that coordinates are stored/handled as uint32_t.
//

namespace ZMap
{

namespace GUI
{

class ZGResourceImportData: public Util::InstanceCounter<ZGResourceImportData>,
        public Util::ClassName<ZGResourceImportData>
{
public:

    ZGResourceImportData();
    ZGResourceImportData(const ZGResourceImportData& data) ;
    ZGResourceImportData& operator=(const ZGResourceImportData& data) ;
    ~ZGResourceImportData() ;

    void setResourceName(const std::string& name) ;
    std::string getResourceName() const {return m_resource_name ;}
    void setSequenceName(const std::string& name) ;
    std::string getSequenceName() const {return m_sequence_name ; }
    void setAssemblyName(const std::string& name) ;
    std::string getAssemblyName() const {return m_assembly_name ; }
    void setSourceName(const std::string& name) ;
    std::string getSourceName() const {return m_source_name ; }
    void setStrand(ZGStrandType strand) ;
    ZGStrandType getStrand() const {return m_strand ; }
    void setMapFlag(bool flag) {m_map_flag = flag ; }
    bool getMapFlag() const {return m_map_flag ; }
    void setStart(uint32_t start) {m_start = start ;}
    uint32_t getStart() const {return m_start; }
    void setEnd(uint32_t end) {m_end = end ; }
    uint32_t getEnd() const {return m_end ; }

private:

    std::string m_resource_name,
        m_sequence_name,
        m_assembly_name,
        m_source_name ;
    uint32_t m_start, m_end ;
    ZGStrandType m_strand ;
    bool m_map_flag ;

};

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGResourceImportData& data) ;
#endif

} // namespace GUI

} // namespace ZMap

#endif // ZGRESOURCEIMPORTDATA_H
