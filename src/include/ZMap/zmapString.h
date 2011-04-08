/*  File: zmapString.h
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: General string functions for ZMap.
 *
 * HISTORY:
 * Last edited: Apr  8 12:30 2011 (edgrif)
 * Created: Thu Sep 20 15:32:36 2007 (edgrif)
 * CVS info:   $Id: zmapString.h,v 1.4 2011-04-08 11:46:02 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_STRING_H
#define ZMAP_STRING_H

#include <glib.h>


int zMapStringFindMatch(char *target, char *query) ;
int zMapStringFindMatchCase(char *target, char *query, gboolean caseSensitive) ;

char *zMapStringFlatten(char *string_with_spaces) ;

#endif /* ZMAP_STRING_H */
