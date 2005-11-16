/*  File: zmapGLibUtils.c
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: A collection of "add-ons" to GLib that seem useful.
 *
 * Exported functions: See ZMap/zmapGLibUtils.h
 * HISTORY:
 * Last edited: Nov 14 11:48 2005 (rds)
 * Created: Thu Oct 13 15:22:35 2005 (edgrif)
 * CVS info:   $Id: zmapGLibUtils.c,v 1.2 2005-11-16 10:37:10 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapGLibUtils.h>

#ifdef UTILISE_SINGLE_FUNCTION
/* Go through a list in reverse calling the supplied GFunc.
 * This was lifted from the glib code of g_list. */

/* changed this slightly to be the g_list_foreach_directional
 * function, as it was working differently to g_list_foreach, by
 * the addition of the g_list_last.  g_list_foreach doesn't do a
 * g_list_first, but then it "shouldn't" need to as the all their
 * function return the first item of the list again. Here we need to 
 * get to the end of the list to do the reverse! but the only place
 * it's getting called from at the moment is zmapWindowDrawFeatures.c 
 * positionColumns, where the list is the item list of the
 * foocanvasgroup.  the group holds a pointer to the end of the list, 
 * so we can use that instead and the other function below... 
 * However, in the dna highlighting it's better not to go all the way 
 * to the end of the list... Maybe we need 2 functions as it is nice
 * to have the g_list_last encapsulated in reverse function...
 */
void zMap_g_list_foreach_reverse(GList *list, GFunc func, gpointer user_data)
{
  list = g_list_last(list) ;    

  while (list)
    {
      GList *prev = list->prev ;

      (*func)(list->data, user_data) ;
      list = prev ;
    }

  return ;
}
#endif

void zMap_g_list_foreach_directional(GList   *list, 
                                     GFunc    func,
                                     gpointer user_data,
                                     ZMapGListDirection forward)
{
  while (list)
    {
      GList *next = (forward == ZMAP_GLIST_FORWARD ? list->next : list->prev);
      (*func) (list->data, user_data);
      list = next;
    }
  return ;
}

