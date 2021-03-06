/*  File: zmapReadLine.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Package to read lines from a string, currently this
 *              is done by altering the string, the code to do this
 *              leaving the string unaltered and returning the next
 *              line in a separate buffer is not currently implemented.
 *              
 * Exported functions: See ZMap/zmapReadLine.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>






#include <glib.h>
#include <zmapReadLine_P.hpp>


static gboolean inPlaceRead(ZMapReadLine read_line, char **next_line_out, gsize *line_length_out) ;

/*! @addtogroup zmaputils
 * @{
 *  */


/*!
 * Create a ZMapReadLine object which can then be used to return each successive
 * line in the input text by making repeated calls to zMapReadLineNext(). Lines
 * must be demarcated in the usual C string manner by a '\n' character.
 *
 * @param input_lines A null terminated C string containing the lines to be parsed.
 * @param in_place    If TRUE, lines will be returned by substituting the line terminator
 *                    character '\n' with a string terminator '\0' and returning a pointer
 *                    to the now null-terminated string.
 *                    NOTE that in_place == TRUE is the only implemented option currently.
 * @return            The new ZMapReadLine object.
 *  */
ZMapReadLine zMapReadLineCreate(char *input_lines, gboolean in_place)
{
  ZMapReadLine read_line ;

  read_line = g_new0(ZMapReadLineStruct, 1) ;

  read_line->original = input_lines ;
  read_line->current = read_line->next = NULL ;

  read_line->in_place = in_place ;

  return read_line ;
}


/*!
 * Read the next line from a ZMapReadLine object. If a '\n' terminated line can
 * be read the function returns TRUE and returns the line in next_line_out.
 * If no terminating '\n' character could be found then the function returns
 * FALSE and returns the remaining string in next_line_out. NOTE that this
 * means that when there is no more string left the function will return
 * FALSE and next_line_out will contain the null string "".
 *
 * read_line     The ZMapReadLine object from which the next line is to be read.
 * next_line_out The next complete line or the remaining text in the string if it
 *                      is not terminated with a '\n' or the empty string "" if there is
 *                      no more text.
 * line_length_out The length of the line returned.
 * returns              TRUE if a complete line could be returned, FALSE otherwise.
 *  */
gboolean zMapReadLineNext(ZMapReadLine read_line, char **next_line_out, gsize *line_length_out)
{
  gboolean result = FALSE ;
  char *next_line = NULL ;
  gsize line_length = 0 ;

  if (read_line->in_place)
    {
      result = inPlaceRead(read_line, &next_line, &line_length) ;

      *next_line_out = next_line ;
      *line_length_out = line_length ;
    }
  /* else do a buffer read..... */

  return result ;
}


/*!
 * Destroy the ZMapReadLine object and optionally free the original string.
 *
 * @param read_line     The ZMapReadLine object to be destroyed.
 * @param free_string   If TRUE the original string used to create the
 *                      ZMapReadLine object will be freed as well.
 * @return              nothing.
 *  */
void zMapReadLineDestroy(ZMapReadLine read_line, gboolean free_string)
{
  if (free_string)
    g_free(read_line->original) ;

  g_free(read_line) ;

  return ;
}


/*! @} end of zmaputils docs. */



static gboolean inPlaceRead(ZMapReadLine read_line, char **next_line_out, gsize *line_length_out)
{
  gboolean result = FALSE ;
  char *curr ;
  gsize line_length = 0 ;
  
  if (!read_line->current)
    {
      read_line->current = read_line->original ;
    }
  else
    read_line->current = read_line->next ;

  curr = read_line->current ;
  while (*curr && *curr != '\n')
    {
      curr++ ;
    }


  if (*curr == '\n')
    {
      line_length = curr - read_line->current + 1 ;
      read_line->next = curr + 1 ;
      *curr = '\0' ;
      result = TRUE ;
    }
  else
    {
      /* curr = '\0' so no end of line before end of string. */
      read_line->next = curr ;
      result = FALSE ;
    }

  *next_line_out = read_line->current ;
  *line_length_out = line_length ;

  return result ;
}

