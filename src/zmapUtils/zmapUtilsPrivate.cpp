/*  File: zmapUtilsPrivate.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
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


