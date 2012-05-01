/*  File: zmapReadLine_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Internal data structs for the line reading package.
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_READLINE_p_h
#define ZMAP_READLINE_p_h


#include <glib.h>
#include <ZMap/zmapReadLine.h>


/* Represents a single C string from which to return successive lines. */
typedef struct _ZMapReadLineStruct
{
  char *original ;					    /* The original string. */
  char *current ;					    /* The current string which is
							       returned by zMapReadLineNext(). */
  char *next ;						    /* The string following the current
							       string. */

  gboolean in_place ;					    /* TRUE => return strings operating on
							       the original string and returning
							       pointers to substrings within it.*/
} ZMapReadLineStruct ;


#endif /* !ZMAP_READLINE_p_h */

