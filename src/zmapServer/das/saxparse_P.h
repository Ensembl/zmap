/*  File: saxparse.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * Description: 
 * HISTORY:
 * Last edited: Jul  2 14:55 2004 (edgrif)
 * Created: Wed Jan 28 16:17:58 2004 (edgrif)
 * CVS info:   $Id: saxparse_P.h,v 1.2 2004-07-19 10:13:36 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef SAXPARSE_P_H
#define SAXPARSE_P_H

#include <saxparse.h>

typedef enum {SAXPARSE_NOWHERE, SAXPARSE_STARTDOC,
	      SAXPARSE_START, SAXPARSE_INSIDE, SAXPARSE_END, SAXPARSE_ENDDOC} SaxParseState ;

/* For an XML tag holds the element name and attributes, i.e.   <element_name attributes>
 * Used to track which tag we are processing. */
typedef struct SaxTagStructName
{
  char *element_name ;

  char **attributes ;
} SaxTagStruct, *SaxTag ;


typedef struct SaxParserStructName
{
  XML_Parser p ;
  gboolean debug ;

  GQueue *tag_stack ;					    /* Stack of SaxTagStruct */
  int Depth ;

  GString *content ;

  char *last_errmsg ;

  void *user_data ;					    /* Caller stores any data they need here. */

} SaxParserStruct ;


#endif /* !SAXPARSE_P_H */
