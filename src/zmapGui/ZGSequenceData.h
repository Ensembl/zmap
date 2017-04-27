/*  File: ZGSequenceData.h
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

#ifndef ZGSEQUENCEDATA_H
#define ZGSEQUENCEDATA_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <cstdint>

//
// Class to store and pass around data associated with a sequence.
//
// Note that coordinates are stored/handled as uint32_t.
//


namespace ZMap
{

namespace GUI
{

class ZGSequenceData: public Util::InstanceCounter<ZGSequenceData>,
        public Util::ClassName<ZGSequenceData>
{
public:

    ZGSequenceData();
    ZGSequenceData(const ZGSequenceData& data) ;
    ZGSequenceData& operator=(const ZGSequenceData &data) ;
    ~ZGSequenceData() ;

    bool setSequenceName(const std::string& data) ;
    std::string getSequenceName() const {return m_sequence_name ; }
    bool setDatasetName(const std::string& data) ;
    std::string getDatasetName() const {return m_dataset_name ; }
    void setStart(uint32_t start) {m_start = start ; }
    uint32_t getStart() const {return m_start ; }
    void setEnd(uint32_t end) {m_end = end ; }
    uint32_t getEnd() const {return m_end ; }

private:

    std::string m_sequence_name,
        m_dataset_name ;
    uint32_t m_start,
        m_end ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGSEQUENCEDATA_H
