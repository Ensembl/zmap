/*  File: zmapManager.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * and was written by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See zmapManager.h
 * HISTORY:
 * Last edited: Jul 13 16:02 2004 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapManager.c,v 1.10 2004-07-14 09:09:13 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapManager_P.h>

static void zmapDestroyedCB(ZMap zmap, void *cb_data) ;
static void removeZmapEntry(ZMapManager zmaps, ZMap zmap) ;



/* Do we want a full callback struct for callbacks here ? Might be overkill.... */

/* ZMap callbacks passed to zMapInit() */
ZMapCallbacksStruct zmap_cbs_G = {zmapDestroyedCB} ;



ZMapManager zMapManagerCreate(zmapAppCallbackFunc zmap_deleted_func, void *gui_data)
{
  ZMapManager manager ;

  manager = g_new(ZMapManagerStruct, sizeof(ZMapManagerStruct)) ;

  manager->zmap_list = NULL ;

  manager->gui_zmap_deleted_func = zmap_deleted_func ;
  manager->gui_data = gui_data ;

  zMapInit(&zmap_cbs_G) ;

  return manager ;
}


/* Add a new zmap window with associated thread and all the gubbins.
 * Returns FALSE on failure. */
gboolean zMapManagerAdd(ZMapManager zmaps, char *sequence, ZMap *zmap_out)
{
  gboolean result = FALSE ;
  ZMap zmap = NULL ;
  ZMapView view = NULL ;

  if ((zmap = zMapCreate((void *)zmaps)))
    {
      zmaps->zmap_list = g_list_append(zmaps->zmap_list, zmap) ;
      *zmap_out = zmap ;

      /* If there is no sequence return an empty ZMap, if there is a sequence then create/connect
       * a view. */
      if (!sequence
	  || (sequence
	      && ((view = zMapAddView(zmap, sequence)))
	      && ((zMapConnectView(zmap, view)))))
	result = TRUE ;
    }

  return result ;
}



/* Reset an existing ZMap, this call will:
 * 
 *    - leave the ZMap window displayed and hold onto user information such as machine/port
 *      sequence etc so user does not have to add the data again.
 *    - Free all ZMap window data that was specific to the view being loaded.
 *    - Kill the existing server thread and start a new one from scratch.
 * 
 * After this call the ZMap will be ready for the user to try again with the existing
 * machine/port/sequence or to enter data for a new sequence etc.
 * 
 *  */
gboolean zMapManagerReset(ZMap zmap)
{
  gboolean result = TRUE ;

  result = zMapReset(zmap) ;

  return result ;
}



gboolean zMapManagerKill(ZMapManager zmaps, ZMap zmap)
{
  gboolean result = TRUE ;

  result = zMapDestroy(zmap) ;

  removeZmapEntry(zmaps, zmap) ;

  return result ;
}



/* Frees all resources held by a zmapmanager and then frees the manager itself. */
gboolean zMapManagerDestroy(ZMapManager zmaps)
{
  gboolean result = TRUE ;

  /* Free all the existing zmaps. */
  if (zmaps->zmap_list)
    {
      GList *next_zmap ;

      while ((next_zmap = g_list_next(zmaps->zmap_list)))
	{
	  removeZmapEntry(zmaps, (ZMap)(next_zmap->data)) ;
	}
    }

  g_free(zmaps) ;

  return result ;
}




/* Gets called when ZMap quits from "under our feet" as a result of user interaction,
 * we then make sure we clean up. */
static void zmapDestroyedCB(ZMap zmap, void *cb_data)
{
  ZMapManager zmaps = (ZMapManager)cb_data ;

  removeZmapEntry(zmaps, zmap) ;

  return ;
}



/*
 *  ------------------- Internal functions -------------------
 */


static void removeZmapEntry(ZMapManager zmaps, ZMap zmap)
{
  zmaps->zmap_list = g_list_remove(zmaps->zmap_list, zmap) ;

  (*(zmaps->gui_zmap_deleted_func))(zmaps->gui_data, zmap) ;

  return ;
}


