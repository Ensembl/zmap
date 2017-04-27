/*  File: ZGResourceImportData.cpp
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

#include "ZGResourceImportData.h"
#ifndef QT_NO_DEBUG
#include <QDebug>
#include <sstream>
#endif

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGResourceImportData>::m_name ("ZGResourceImportData") ;

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGResourceImportData& data)
{
    std::stringstream str ;
    str << data.name() << "(" << data.getResourceName() << ","
        << data.getSequenceName()  << ","
        << data.getAssemblyName() << ","
        << data.getSourceName() << ","
        << data.getStrand() << ","
        << data.getMapFlag() << ","
        << data.getStart() << ","
        << data.getEnd() << ")" ;
    dbg.nospace() << QString::fromStdString(str.str()) ;
    return dbg.space() ;
}

#endif

ZGResourceImportData::ZGResourceImportData()
    : Util::InstanceCounter<ZGResourceImportData>(),
      Util::ClassName<ZGResourceImportData>(),
      m_resource_name(),
      m_sequence_name(),
      m_assembly_name(),
      m_source_name(),
      m_start(),
      m_end(),
      m_strand(ZGStrandType::Invalid),
      m_map_flag(false)
{
}


ZGResourceImportData::ZGResourceImportData(const ZGResourceImportData& data)
    : Util::InstanceCounter<ZGResourceImportData>(),
      Util::ClassName<ZGResourceImportData>(),
      m_resource_name(data.m_resource_name),
      m_sequence_name(data.m_sequence_name),
      m_assembly_name(data.m_assembly_name),
      m_source_name(data.m_source_name),
      m_start(data.m_start),
      m_end(data.m_end),
      m_strand(data.m_strand),
      m_map_flag(data.m_map_flag)
{
}

ZGResourceImportData& ZGResourceImportData::operator =(const ZGResourceImportData& data)
{
    if (this != &data)
    {
        m_resource_name = data.m_resource_name ;
        m_sequence_name = data.m_sequence_name ;
        m_assembly_name = data.m_assembly_name ;
        m_source_name = data.m_source_name ;
        m_start = data.m_start ;
        m_end = data.m_end ;
        m_strand = data.m_strand ;
        m_map_flag = data.m_map_flag ;
    }
    return *this ;
}

ZGResourceImportData::~ZGResourceImportData()
{
}

void ZGResourceImportData::setResourceName(const std::string& name)
{
    m_resource_name = name ;
}

void ZGResourceImportData::setSequenceName(const std::string& name)
{
   m_sequence_name = name ;
}

void ZGResourceImportData::setAssemblyName(const std::string& name)
{
    m_assembly_name = name ;
}

void ZGResourceImportData::setSourceName(const std::string& name)
{
    m_source_name = name ;
}

void ZGResourceImportData::setStrand(ZGStrandType strand)
{
    m_strand = strand ;
}


} // namespace GUI

} // namespace ZMap
