/*  File: ZFeature.h
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

#ifndef NAMECONVERTER_H
#define NAMECONVERTER_H

#include "ZInternalID.h"
//#include "some_gquark_stuff_used_in_zmap.h"
#include <string>

//
// This is a singleton class to provide access to conversions
// between internal id and std::string. It requires access to the
// GQuark stuff of ZMap... hopefully this should only be required
// temporarily. It could be done without that stuff if I put in
// messages to do the conversions, but it's something that needs
// to be done so much that this would be inefficient.
//
// Internals managed using std::call_once().
//

namespace ZMap {

namespace Util {

class NameConverter
{
public:

    static NameConverter& NameConverterInstance() ;

    std::string id2string(GUI::ZInternalID id) ;
    GUI::ZInternalID string2id(const std::string& str) ;

private:

    NameConverter();
    NameConverter(const NameConverter&) = delete ;
    NameConverter& operator= (const NameConverter& ) = delete ;
    ~NameConverter() { }

    static void create() ;

    static NameConverter* m_name_converter_instance ;

};

} // namespace GUI

} // namespace ZMap

#endif // NAMECONVERTER_H
