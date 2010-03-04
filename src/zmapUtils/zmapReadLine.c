/*  File: zmapReadLine.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Package to read lines from a string, currently this
 *              is done by altering the string, the code to do this
 *              leaving the string unaltered and returning the next
 *              line in a separate buffer is not currently implemented.
 *              
 * Exported functions: See ZMap/zmapReadLine.h
 * HISTORY:
 * Last edited: Jul 16 09:40 2004 (edgrif)
 * Created: Thu Jun 24 19:03:50 2004 (edgrif)
 * CVS info:   $Id: zmapReadLine.c,v 1.4 2010-03-04 15:11:20 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <zmapReadLine_P.h>


static gboolean inPlaceRead(ZMapReadLine read_line, char **next_line_out) ;

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
 * FALSE and returns the remainging string in next_line_out. NOTE that this
 * means that when there is no more string left the function will return
 * FALSE and next_line_out will contain the null string "".
 *
 * @param read_line     The ZMapReadLine object from which the next line is to be read.
 * @param next_line_out The next complete line or the remaining text in the string if it
 *                      is not terminated with a '\n' or the empty string "" if there is
 *                      no more text.
 * @return              TRUE if a complete line could be returned, FALSE otherwise.
 *  */
gboolean zMapReadLineNext(ZMapReadLine read_line, char **next_line_out)
{
  gboolean result = FALSE ;
  char *next_line = NULL ;

  if (read_line->in_place)
    {
      result = inPlaceRead(read_line, &next_line) ;

      *next_line_out = next_line ;
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


static gboolean inPlaceRead(ZMapReadLine read_line, char **next_line_out)
{
  gboolean result = FALSE ;
  char *curr ;

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
      read_line->next = curr + 1 ;
      *curr = '\0' ;
      result = TRUE ;
    }
  else							    /* curr = '\0' */
    {
      /* no end of line before end of string. */
      read_line->next = curr ;
      result = FALSE ;
    }

  *next_line_out = read_line->current ;

  return result ;
}

