/*  File: zmapManager.c
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
 * and was written by
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *       Roy Storey (Sanger Institute, UK) rnc@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Handles a list of zmaps which it can delete and add
 *              to.
 *
 * Exported functions: See zmapManager.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <ZMap/zmapUtils.h>
#include <zmapManager_P.h>



static void destroyedCB(ZMap zmap, void *cb_data) ;
static void quitReqCB(ZMap zmap, void *cb_data) ;
static void removeZmapEntry(ZMapManager zmaps, ZMap zmap) ;


/* Holds callbacks the level above us has asked to be called back on. */
static ZMapManagerCallbacks manager_cbs_G = NULL ;


/* Holds callbacks we set in the level below us (ZMap window) to be called back on. */
ZMapCallbacksStruct zmap_cbs_G = {destroyedCB, quitReqCB} ;



/* This routine must be called just once before any other zmaps routine, it is a fatal error
 * if the caller calls this routine more than once. The caller must supply all of the callback
 * routines.
 *
 * Note that since this routine is called once per application we do not bother freeing it
 * via some kind of zmaps terminate routine. */
void zMapManagerInit(ZMapManagerCallbacks callbacks)
{
  zMapAssert(!manager_cbs_G) ;

  zMapAssert(callbacks && callbacks->zmap_deleted_func && callbacks->zmap_set_info_func
	     && callbacks->quit_req_func) ;

  manager_cbs_G = g_new0(ZMapManagerCallbacksStruct, 1) ;

  manager_cbs_G->zmap_deleted_func  = callbacks->zmap_deleted_func ; /* called when a zmap closes */
  manager_cbs_G->zmap_set_info_func = callbacks->zmap_set_info_func ;
							    /* called when zmap does something that gui needs to know about (remote calls) */
  manager_cbs_G->quit_req_func = callbacks->quit_req_func ; /* called when layer below requests
							       that zmap app exits. */


  /* Init control callbacks.... */
  zMapInit(&zmap_cbs_G) ;

  return ;
}


ZMapManager zMapManagerCreate(void *gui_data)
{
  ZMapManager manager ;

  manager = g_new0(ZMapManagerStruct, 1) ;

  manager->zmap_list = NULL ;

  manager->gui_data = gui_data ;

  return manager ;
}


/* Add a new zmap window with associated thread and all the gubbins.
 * Return indicates what happened on trying to add zmap.
 *
 * Policy is:
 *
 * If no connection can be made for the view then the zmap is destroyed,
 * if any connection succeeds then the zmap is added but errors are reported.
 * 
 * sequence_map is assumed to be filled in correctly.
 * 
 * 
 * load_view is a hack that can go once the new view stuff is here...
 * 
 *  */
ZMapManagerAddResult zMapManagerAdd(ZMapManager zmaps, ZMapFeatureSequenceMap sequence_map, ZMap *zmap_out,
				    gboolean load_view)
{
  ZMapManagerAddResult result = ZMAPMANAGER_ADD_FAIL ;
  ZMap zmap = NULL ;
  ZMapView view = NULL ;

  zMapAssert(zmaps) ;

  if ((zmap = zMapCreate((void *)zmaps, sequence_map)))
    {
      if (!load_view)
	{
	  result = ZMAPMANAGER_ADD_OK ;
	}
      else
	{
	  /* Try to load the sequence. */
	  if ((view = zMapAddView(zmap, sequence_map)) && zMapConnectView(zmap, view))
	    {
	      result = ZMAPMANAGER_ADD_OK ;
	    }
	  else
	    {
	      /* Remove zmap we just created, if this fails then return disaster.... */
	      if (zMapDestroy(zmap))
		result = ZMAPMANAGER_ADD_FAIL ;
	      else
		result = ZMAPMANAGER_ADD_DISASTER ;
	    }
	}
    }

  if (result == ZMAPMANAGER_ADD_OK)
    {
      zmaps->zmap_list = g_list_append(zmaps->zmap_list, zmap) ;
      *zmap_out = zmap ;

      (*(manager_cbs_G->zmap_set_info_func))(zmaps->gui_data, zmap) ;
    }

  return result ;
}

guint zMapManagerCount(ZMapManager zmaps)
{
  guint zmaps_count = 0;
  zmaps_count = g_list_length(zmaps->zmap_list);
  return zmaps_count;
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


gboolean zMapManagerRaise(ZMap zmap)
{
  gboolean result = TRUE ;

  result = zMapRaise(zmap) ;

  return result ;
}


/* Note the zmap will get removed from managers list once it signals via destroyedCB()
 * that it has died. */
gboolean zMapManagerKill(ZMapManager zmaps, ZMap zmap)
{
  gboolean result = TRUE ;

  result = zMapDestroy(zmap) ;

  return result ;
}

gboolean zMapManagerKillAllZMaps(ZMapManager zmaps)
{
  if (zmaps->zmap_list)
    {
      GList *next_zmap ;

      next_zmap = g_list_first(zmaps->zmap_list) ;
      do
	{
	  zMapManagerKill(zmaps, (ZMap)(next_zmap->data)) ;
	}
      while ((next_zmap = g_list_next(next_zmap))) ;
    }

  return TRUE;
}

/* Frees zmapmanager, the list of zmaps is expected to already have been
 * free'd. This needs to be done separately because the zmaps are threaded and
 * may take some time to die. */
gboolean zMapManagerDestroy(ZMapManager zmaps)
{
  gboolean result = TRUE ;

  zMapAssert(!(zmaps->zmap_list)) ;

  g_free(zmaps) ;

  return result ;
}




/*
 *  ------------------- Internal functions -------------------
 */


/* Gets called when a single ZMap window closes from "under our feet" as a result of user interaction,
 * we then make sure we clean up. */
static void destroyedCB(ZMap zmap, void *cb_data)
{
  ZMapManager zmaps = (ZMapManager)cb_data ;

  removeZmapEntry(zmaps, zmap) ;

  return ;
}



/* Gets called when a ZMap requests that the application "quits" as a result of user interaction,
 * we signal the layer above us to start the clean up prior to quitting. */
static void quitReqCB(ZMap zmap, void *cb_data)
{
  ZMapManager zmaps = (ZMapManager)cb_data ;

  (*(manager_cbs_G->quit_req_func))(zmaps->gui_data, zmap) ;

  return ;
}


/* Removes a zmap entry from the managers list and signals the layer above that it has gone. */
static void removeZmapEntry(ZMapManager zmaps, ZMap zmap)
{
  zmaps->zmap_list = g_list_remove(zmaps->zmap_list, zmap) ;

  (*(manager_cbs_G->zmap_deleted_func))(zmaps->gui_data, zmap) ;

  return ;
}


