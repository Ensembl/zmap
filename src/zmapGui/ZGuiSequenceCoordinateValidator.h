/*  File: ZGuiSequenceCoordinateValidator.h
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

#ifndef ZGUISEQUENCECOORDINATEVALIDATOR_H
#define ZGUISEQUENCECOORDINATEVALIDATOR_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <cstdint>
#include <QValidator>

//
// Validator to be used with various dialogues
//

namespace ZMap
{

namespace GUI
{

class ZGuiSequenceCoordinateValidator : public QValidator,
        public Util::InstanceCounter<ZGuiSequenceCoordinateValidator>,
        public Util::ClassName<ZGuiSequenceCoordinateValidator>
{
    Q_OBJECT

public:
    static ZGuiSequenceCoordinateValidator* newInstance() ;

    ~ZGuiSequenceCoordinateValidator() ;

    State validate(QString &, int &) const Q_DECL_OVERRIDE ;

private:

    ZGuiSequenceCoordinateValidator() ;
    ZGuiSequenceCoordinateValidator(const ZGuiSequenceCoordinateValidator&) = delete ;
    ZGuiSequenceCoordinateValidator& operator=(const ZGuiSequenceCoordinateValidator&) = delete ;

    static const uint32_t m_min ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGUISEQUENCECOORDINATEVALIDATOR_H
