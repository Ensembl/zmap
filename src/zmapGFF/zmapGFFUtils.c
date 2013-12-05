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
#include <stdio.h>
#include <ZMap/zmapGFF.h>
#include "zmapGFF_P.h"

/* #define LOCAL_DEBUG_CODE 1 */

#ifdef LOCAL_DEBUG_CODE
static FILE *pFile = NULL ;
static const char *sFilename = "/nfs/users/nfs_s/sm23/Work/ZMap_develop/test_data.txt" ;
static unsigned int iCount = 0 ;
#endif


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
      if ((iValue == ZMAPGFF_VERSION_2) || (iValue == ZMAPGFF_VERSION_3))
        {
          bResult = TRUE ;
          *piOut = iValue ;
        }
    }

  return bResult ;
}


/*
 * This function expects a valid GIOChannel and that the next line
 * to be read is of the form expected by zMapGFFGetVersionFromString(). Then parse
 * the line with this function and rewind the GIOChannel to the same point at which
 * we entered. Returns TRUE if this operation returns 2 or 3, FALSE otherwise.
 * If the line encountered is blank, we return with version 2 set as default,
 * and return TRUE. If an error associated with the GIOChannel occurrs, then we
 * return FALSE.
 */
gboolean zMapGFFGetVersionFromGIO(GIOChannel * const pChannel, int * const piOut )
{
  static const char
    cNewline = '\n',
    cNull = '\0'
  ;
  char *pPos = NULL ;
  gboolean bResult = TRUE ;
  GString *pString ;
  gsize
    iTerminatorPos = 0
  ;
  GError *pError = NULL ;
  GIOStatus cStatus ;

  /*
   * Set the default version to be returned.
   */
  *piOut = GFF_DEFAULT_VERSION ;

  /*
   * Create string object to use as buffer.
   */
  pString = g_string_new(NULL) ;
  if (!pString )
    goto return_point ;

#ifdef LOCAL_DEBUG_CODE
  if (pFile == NULL )
    {
      pFile = fopen(sFilename, "aw") ;
    }
#endif

  /*
   * Read a line from the GIOChannel and remove newline if present (replace it with NULL).
   */
  cStatus = g_io_channel_read_line_string(pChannel, pString, &iTerminatorPos, &pError) ;
  if (pError)
    goto return_point ;
  pPos = strchr(pString->str, cNewline) ;
  if (pPos)
    *pPos = cNull ;

  if (cStatus == G_IO_STATUS_NORMAL)
    {
      /* (sm23) This _should_ seek to the start of the file; however, there is
         some unexplained error when doing so running under otterlace.
       */
      /*
        cStatus = g_io_channel_seek_position(pChannel, (gint)0, G_SEEK_SET, &pError ) ;
      */
      if (pError)
        goto return_point ;

    /*
     * If the line read in contains some data, attempt to parse; if it was
     * blank then we just return with default return values.
     */
      if (strcmp(pString->str, ""))
        {
          /*
           * Parse the string line that was read in.
           */
          bResult = zMapGFFGetVersionFromString(pString->str, piOut);

        }
    }
  else /* We encountered an error in reading the stream, or seeking back */
    {
      bResult = FALSE ;
    }

#ifdef LOCAL_DEBUG_CODE
  if (pFile != NULL )
    {
      ++iCount ;
      fprintf(pFile, "parse string = '%s', i = %i, v = %i\n", pString->str, iCount, *piOut) ;
      fflush(pFile) ;
    }
#endif

  /*
   * Free the allocated string and other data.
   */
return_point:
  if (pString)
    g_string_free(pString, TRUE) ;
  if (pError)
    g_error_free(pError) ;

  return bResult ;
}

