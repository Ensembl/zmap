/*  File: zmapUtilsPrivate.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2017: Genome Research Ltd.
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
 * This file is part of the ZMap genome database package
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See ZMap/zmapUtilsPrivate.hpp
 *
 *-------------------------------------------------------------------
 */


#include <ZMap/zmap.hpp>


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <stddef.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
#include <string.h>
#include <glib.h>

#include <ZMap/zmapUtilsPrivate.hpp>



/* This function is ONLY designed to work on the string produced by
 * the C++ __FILE__ preprocessor macro. Such a string is guaranteed not to have weird characters
 * or to have some out of bounds length etc etc.....
 *
 * The C++ preprocessor __FILE__ macro produces a voluminous string as it includes all of the
 * arguments to the function and this makes it incredibly long....
 * /nfs/team71/acedb/edgrif/ZMap/ZMap_Curr/ZMap/src/build/linux/../../zmapControl/remote/zmapXRemote.c
 *
 * and returns a pointer in the _same_ string to the filename at the end, e.g.
 *
 * zmapXRemote.c
 *
 * Currently we just call g_basename() but this is a glib deprecated function so we may have
 * to do our own version one day....
 *
 *  */
const char *zmapGetFILEBasename(const char *FILE_MACRO_ONLY)
{
  const char *basename = NULL ;

  // Really this test shouldn't be necessary....
  if (FILE_MACRO_ONLY && *FILE_MACRO_ONLY)
    {
      if ((basename = strrchr(FILE_MACRO_ONLY, G_DIR_SEPARATOR)))
        basename++ ;
    }
  
  return basename ;
}


