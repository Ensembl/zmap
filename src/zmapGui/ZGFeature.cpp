/*  File: ZGFeature.cpp
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

#include "ZGFeature.h"
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGFeature>::m_name("ZGFeature") ;

ZGFeature::ZGFeature()
    : Util::InstanceCounter<ZGFeature>(),
      Util::ClassName<ZGFeature>(),
      m_id(0),
      m_bounds(),
      m_type(ZGFeatureType::Invalid),
      m_strand(ZGStrandType::Invalid),
      m_subparts()
{
}

ZGFeature::ZGFeature(ZInternalID id)
    : Util::InstanceCounter<ZGFeature>(),
      Util::ClassName<ZGFeature>(),
      m_id(id),
      m_bounds(),
      m_type(ZGFeatureType::Invalid),
      m_strand(ZGStrandType::Invalid),
      m_subparts()
{
    if (!id)
        throw std::runtime_error(std::string("ZGFeature::ZGFeature() ; id may not be 0 ")) ;
}

ZGFeature::ZGFeature(ZInternalID id,
                     ZGFeatureType feature_type,
                     ZGStrandType strand_type,
                     const ZGFeatureBounds &bounds,
                     const std::vector<ZGFeatureBounds> &subparts)
    : Util::InstanceCounter<ZGFeature>(),
      Util::ClassName<ZGFeature>(),
      m_id(id),
      m_bounds(bounds),
      m_type(feature_type),
      m_strand(strand_type),
      m_subparts(subparts)
{
    if (!id)
        throw std::runtime_error(std::string("ZGFeature::ZGFeature() ; id may not be 0 ")) ;
    if (m_type == ZGFeatureType::Invalid)
        throw std::runtime_error(std::string("ZGFeature::ZGFeature() ; type may not be set to be Invalid")) ;
    if (m_strand == ZGStrandType::Invalid)
        throw std::runtime_error(std::string("ZGFeature::ZGFeature() ; strand may not be set to be Invalid ")) ;
}

ZGFeature::ZGFeature(const ZGFeature& feature)
    : Util::InstanceCounter<ZGFeature>(),
      Util::ClassName<ZGFeature>(),
      m_id(feature.m_id),
      m_bounds(feature.m_bounds),
      m_type(feature.m_type),
      m_strand(feature.m_strand),
      m_subparts(feature.m_subparts)
{
}

ZGFeature& ZGFeature::operator=(const ZGFeature& feature)
{
    if (this != &feature)
    {
        m_id = feature.m_id ;
        m_bounds = feature.m_bounds ;
        m_type = feature.m_type ;
        m_strand = feature.m_strand ;
        m_subparts = feature.m_subparts ;
    }
    return *this ;
}

ZGFeature::~ZGFeature()
{
}

} // namespace GUI

} // namespace ZMap
