/*  File: zmapReadLine.h
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
 * Description: The interface to a line reading package.
 * HISTORY:
 * Last edited: Apr 21 17:11 2010 (edgrif)
 * Created: Thu Jun 24 19:32:55 2004 (edgrif)
 * CVS info:   $Id: zmapReadLine.h,v 1.4 2010-04-22 12:16:04 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_READLINE_H
#define ZMAP_READLINE_H


#include <glib.h>


/*! @addtogroup zmaputils
 * @{
 *  */


/*!
 * ZMapReadLine object containing the set of lines to be returned one at a time. */
typedef struct _ZMapReadLineStruct *ZMapReadLine ;


/*! @} end of zmaputils docs. */


ZMapReadLine zMapReadLineCreate(char *lines, gboolean in_place) ;
gboolean zMapReadLineNext(ZMapReadLine read_line, char **next_line_out, gsize *line_length_out) ;
void zMapReadLineDestroy(ZMapReadLine read_line, gboolean free_string) ;

#endif /* !ZMAP_READLINE_H */
