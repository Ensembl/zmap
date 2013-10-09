/*  File: zmapGFFUtils.c
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Some functions that are used in parser construction and
 * interfaces. It is necessary to query the input stream for the version
 * of GFF being passed in _before_ creating the parser - because we use
 * different data structure and associated functions for each of
 * versions 2 and 3. Note that this is a little more complex when passed
 * a GIOStream.
 *-------------------------------------------------------------------
 */



#include <glib.h>
#include <string.h>
#include <ZMap/zmapGFF.h>



/*
 * This function expects a string of the form
 *
 * ##gff-version <i>
 *
 * and parses this for the integer value <i> and writes this to the output
 * integer argument. Returns TRUE if this operation gives either 2 or 3,
 * and FALSE otherwise.
 */
gboolean zMapGFFGetVersionFromString(const char* const sString, int * const piOut)
{
  static const char *sFormat = "##gff-version %i" ;
  static const char iExpectedFields = 1 ;
  int iValue ;
  gboolean bResult = FALSE ;

  if (sscanf(sString, sFormat, &iValue) == iExpectedFields)
  {
    if ((iValue == 2) || (iValue == 3))
    {
      bResult = TRUE ;
      *piOut = iValue ;
    }
  }

  return bResult ;
}


/*
 * This function expects to be at the start of a GIOChannel and that the next line
 * to be read is of the form expected by zMapGFFGetVersionFromString(). Then parse
 * the line with this function and rewind the GIOChannel to the same point at which
 * we entered. Returns TRUE if this operation returns 2 or 3, FALSE otherwise.
 *
 * This is currently a dummy version of the function that returns the default
 * value.
 *
 */
gboolean zMapGFFGetVersionFromGIO(GIOChannel * const pChannel, int * const piOut )
{
  gboolean bResult = FALSE ;

  bResult = TRUE ;
  *piOut = 2 ;

  return bResult ;
}

