/*  File: ZGColor.h
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

#ifndef ZGCOLOR_H
#define ZGCOLOR_H

// This is part of the cache/transfer set of classes.

// Values for RGB\alpha are [0,255]

#include "InstanceCounter.h"
#include "ClassName.h"
#include <QColor>
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

class ZGColor: public Util::InstanceCounter<ZGColor>,
        public Util::ClassName<ZGColor>
{
public:

    ZGColor();
    ZGColor(const ZGColor &color) ;
    ZGColor& operator=(const ZGColor& color) ;
    ZGColor(unsigned char v1, unsigned char v2, unsigned char v3, unsigned char v4) ;
    ~ZGColor() ;

    unsigned char getR() const {return m_r ; }
    unsigned char getG() const {return m_g ; }
    unsigned char getB() const {return m_b ; }
    unsigned char getA() const {return m_a ; }

    QColor asQColor() const ;

private:
    unsigned char m_r, m_g, m_b, m_a ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGCOLOR_H
