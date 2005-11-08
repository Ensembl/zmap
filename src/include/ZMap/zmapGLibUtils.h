/*  File: zmapGLibUtils.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *
 * Description: Extra Home grown functions that are compatible with
 *              glib but not included with their distribution.
 *
 * HISTORY:
 * Last edited: Oct 13 16:06 2005 (edgrif)
 * Created: Thu Oct 13 15:56:54 2005 (edgrif)
 * CVS info:   $Id: zmapGLibUtils.h,v 1.1 2005-11-08 17:03:05 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_GLIBUTILS_H
#define ZMAP_GLIBUTILS_H

#include <glib.h>


void my_g_list_foreach_reverse(GList *list, GFunc func, gpointer user_data) ;


#endif /* !ZMAP_GLIBUTILS_H */
