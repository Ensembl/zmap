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
 * Last edited: Apr  2 11:49 2004 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapManager.c,v 1.6 2004-04-08 16:47:38 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapManager_P.h>
#include <ZMap/zmapConfig.h>

static void managerCB(ZMap zmap, void *cb_data) ;
static void removeZmapEntry(ZMapManager zmaps, ZMap zmap) ;



ZMapManager zMapManagerCreate(zmapAppCallbackFunc zmap_deleted_func, void *gui_data)
{
  ZMapManager manager ;

  manager = g_new(ZMapManagerStruct, sizeof(ZMapManagerStruct)) ;

  manager->zmap_list = NULL ;

  manager->config = zMapConfigCreate("any_old_file") ;

  manager->gui_zmap_deleted_func = zmap_deleted_func ;
  manager->gui_data = gui_data ;

  return manager ;
}


/* Add a new zmap window with associated thread and all the gubbins.
 * Returns FALSE on failure. */
gboolean zMapManagerAdd(ZMapManager zmaps, char *sequence, ZMap *zmap_out)
{
  gboolean result = FALSE ;
  ZMap zmap = NULL ;

  if ((zmap = zMapCreate(sequence, (void *)zmaps, managerCB, zmaps->config))
      && zMapConnect(zmap))
    {
      zmaps->zmap_list = g_list_append(zmaps->zmap_list, zmap) ;
      *zmap_out = zmap ;
      result = TRUE ;
    }

  return result ;
}


gboolean zMapManagerLoadData(ZMapManager zmaps_currently_unused, ZMap zmap)
{
  gboolean result = FALSE ;

  /* For now I've put in a blank sequence, in the end we would like the user to be able
   * to interactively set this.... */
  result = zMapLoad(zmap, "") ;

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


/* A dummy entry at the moment.... */
gboolean zMapManagerDestroy(ZMapManager zmaps)
{
  gboolean result = TRUE ;

  printf("zMapManagerDestroy() not implemented at the moment....\n") ;
  
  return result ;
}




/* Gets called when ZMap quits from "under our feet" as a result of user interaction,
 * we then make sure we clean up. */
static void managerCB(ZMap zmap, void *cb_data)
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


