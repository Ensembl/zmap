/*  File: zmapGFFUtils.c
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
#include <ZMap/zmapGFF.hpp>
#include <zmapGFF_P.hpp>

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
  static const int iExpectedFields = 1 ;
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
 * and return TRUE. If an error associated with the GIOChannel occurs, then we
 * return FALSE and return the status and Error (if there is one) from the GIOChannel call.
 */
gboolean zMapGFFGetVersionFromGIO(GIOChannel * const pChannel, GString *pString,
                                  int * const piOut, GIOStatus *cStatusOut, GError **pError_out)
{
  gboolean bResult = FALSE ;
  static const char
    cNewline = '\n',
    cNull = '\0'
  ;
  char *pPos = NULL ;
  gsize
    iTerminatorPos = 0
  ;
  GError *pError = NULL ;
  GIOStatus cStatus = G_IO_STATUS_NORMAL ;

  /*
   * Set the default version to be returned.
   */
  *piOut = GFF_DEFAULT_VERSION ;

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

 return_point:
  if (!bResult)
    {
      if (pError)
        *pError_out = pError ;

      if (cStatus != G_IO_STATUS_NORMAL)
        *cStatusOut = cStatus ;
    }


  return bResult ;
}

