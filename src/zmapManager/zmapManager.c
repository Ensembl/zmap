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
 * Last edited: Nov 18 10:50 2003 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapManager.c,v 1.2 2003-11-18 11:27:07 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapManager_P.h>


static void zmapWindowCB(void *cb_data, int reason) ;



ZMapManager zMapManagerInit(zmapAppCallbackFunc zmap_deleted_func,
			    void *gui_data)
{
  ZMapManager manager ;

  manager = g_new(ZMapManagerStruct, sizeof(ZMapManagerStruct)) ;

  manager->zmap_list = NULL ;

  manager->gui_zmap_deleted_func = zmap_deleted_func ;
  manager->gui_data = gui_data ;

  return manager ;
}


/* Add a new zmap window with associated thread and all the gubbins. */
ZMap zMapManagerAdd(ZMapManager zmaps, char *machine, int port, char *sequence)
{
  ZMap zmap ;
  ZMapManagerCB zmap_cb ;

  zmap_cb = g_new(ZMapManagerCBStruct, sizeof(ZMapManagerCBStruct)) ;

  if ((zmap = zMapCreate((void *)zmap_cb, zmapWindowCB))
      && zMapConnect(zmap, machine, port, sequence))
    {
      zmap_cb->zmap_list = zmaps ;
      zmap_cb->zmap = zmap ;

      zmaps->zmap_list = g_list_append(zmaps->zmap_list, zmap) ;
    }
  else
    {
      if (zmap_cb)
	g_free(zmap_cb) ;

      printf("Aggghhhh, not able to create a zmap....\n") ;
    }

  return zmap ;
}


void zMapManagerLoadData(ZMap zmap)
{

  /* For now I've put in a blank sequence, in the end we would like the user to be able
   * to interactively set this.... */
  zMapLoad(zmap, "") ;

  return ;
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
void zMapManagerReset(ZMap zmap)
{

  /* We need a new windows call to reset the window and blank it. */

  /* We need to destroy the existing thread connection and start a new one...watch out,
   * remember the destroy will be asynchronous..... */

  return ;
}



/* Called to kill a zmap window and get the associated thread killed, this latter will be
 * asynchronous. */
/* Calls the control window callback to remove any reference to the zmap and then destroys
 * the actual zmap itself.
 * 
 * The window pointer must be null'd because this prevents us trying to doubly destroy
 * the window when a thread is killed but only signals its own destruction some time
 * later.
 *  */
void zMapManagerKill(ZMapManager zmaps, ZMap zmap)
{

  /* THIS IS ALMOST CERTAINLY MESSED UP NOW..... */

  zMapDestroy(zmap) ;

  zmaps->zmap_list = g_list_remove(zmaps->zmap_list, zmap) ;

  (*(zmaps->gui_zmap_deleted_func))(zmaps->gui_data, zmap) ;

  return ;
}





/*
 *  ------------------- Internal functions -------------------
 */




/* Gets called when zmap Window needs Manager to do something. */
static void zmapWindowCB(void *cb_data, int reason)
{
  ZMapManagerCB zmap_cb = (ZMapManagerCB)cb_data ;
  ZmapWindowCmd window_cmd = (ZmapWindowCmd)reason ;
  char *debug ;


  if (window_cmd == ZMAP_WINDOW_QUIT)
    {
      debug = "ZMAP_WINDOW_QUIT" ;

      zMapManagerKill(zmap_cb->zmap_list, zmap_cb->zmap) ;

      g_free(zmap_cb) ;					    /* Only free on destroy. */
    }
  else if (window_cmd == ZMAP_WINDOW_LOAD)
    {
      debug = "ZMAP_WINDOW_LOAD" ;

      zMapManagerLoadData(zmap_cb->zmap) ;
    }
  else if (window_cmd == ZMAP_WINDOW_STOP)
    {
      debug = "ZMAP_WINDOW_STOP" ;
    }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* We need to pass back the info here....or something..... */

  ZMAP_DEBUG(("GUI: received %s from thread %x\n", debug,
	      zMapConnGetThreadid(zmap_cb->zmap->connection))) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}


