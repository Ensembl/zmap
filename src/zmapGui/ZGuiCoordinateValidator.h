/*  File: ZGuiCoordinateValidator.h
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

#ifndef ZGUICOORDINATEVALIDATOR_H
#define ZGUICOORDINATEVALIDATOR_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <cstdint>
#include <QString>
#include <QValidator>

//
// Validator to be used for general coordinate input; default min is set to 1,
// but this can be changed
//


namespace ZMap
{

namespace GUI
{

class ZGuiCoordinateValidator : public QValidator,
        public Util::InstanceCounter<ZGuiCoordinateValidator>,
        public Util::ClassName<ZGuiCoordinateValidator>
{

    Q_OBJECT

public:
    static ZGuiCoordinateValidator* newInstance() ;

    ~ZGuiCoordinateValidator() ;

    State validate(QString&, int &) const Q_DECL_OVERRIDE ;

    bool setRange(uint32_t min, uint32_t max = UINT32_MAX) ;
    uint32_t getMin() const {return m_min ; }
    uint32_t getMax() const {return m_max ; }

private:

    ZGuiCoordinateValidator() ;
    static const uint32_t m_min_default,
        m_max_default ;

    uint32_t m_min,
        m_max ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUICOORDINATEVALIDATOR_H
