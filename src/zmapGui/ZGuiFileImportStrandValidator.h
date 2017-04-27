/*  File: ZGuiFileImportStrandValidator.h
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

#ifndef ZGUIFILEIMPORTSTRANDVALIDATOR_H
#define ZGUIFILEIMPORTSTRANDVALIDATOR_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <QValidator>
#include <QString>

//
// Validator to be used with the file import dialogue for the strand
// data item
//


namespace ZMap
{

namespace GUI
{

class ZGuiFileImportStrandValidator : public QValidator,
        public Util::InstanceCounter<ZGuiFileImportStrandValidator>,
        public Util::ClassName<ZGuiFileImportStrandValidator>
{

    Q_OBJECT

public:
    static QString inputMask() {return m_input_mask ; }
    static ZGuiFileImportStrandValidator* newInstance() ;

    ~ZGuiFileImportStrandValidator() ;

    State validate(QString &, int &) const Q_DECL_OVERRIDE ;

private:
    ZGuiFileImportStrandValidator() ;

    static const char m_plus, m_minus, m_dot ;
    static const QString m_input_mask ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIFILEIMPORTSTRANDVALIDATOR_H
