/*  File: zmapControl.c
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
 * Description: This is the ZMap interface code, it controls both
 *              the window code and the threaded server code.
 * Exported functions: See ZMap.h
 * HISTORY:
 * Last edited: May 17 14:57 2004 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapControl.c,v 1.8 2004-05-17 16:31:48 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>

#include <gtk/gtk.h>
#include <ZMap/zmapView.h>
#include <ZMap/zmapUtils.h>
#include <ZMap_P.h>


/* ZMap debugging output. */
gboolean zmap_debug_G = TRUE ; 


static ZMap createZMap(void *app_data, ZMapCallbackFunc app_cb) ;
static void destroyZMap(ZMap zmap) ;

static void killZMap(ZMap zmap) ;
static void viewKilledCB(ZMapView view, void *app_data) ;
static void killFinal(ZMap zmap) ;
static void killViews(ZMap zmap) ;




/*
 *  ------------------- External functions -------------------
 *   (includes callbacks)
 */


/* Create a new zmap, will be blank if sequence is NULL, otherwise it will create a view
 * within the zmap displaying the sequence. Returns NULL on failure. */
ZMap zMapCreate(char *sequence, void *app_data, ZMapCallbackFunc app_zmap_destroyed_cb)
{
  ZMap zmap = NULL ;


  zmap = createZMap(app_data, app_zmap_destroyed_cb) ;

  /* Make the main/toplevel window for the ZMap. */
  zmapTopWindowCreate(zmap, zmap->zmap_id) ;

  zmap->state = ZMAP_INIT ;

  /* Create a zmapView if a sequence is specified. */
  if (sequence)
    {
      ZMapView view ;

      if ((view = zMapViewCreate(zmap->view_parent, sequence, zmap, viewKilledCB))
	  && (zMapViewConnect(view)))
	{
	  /* add to list of views.... */
	  zmap->curr_view = view ;
	  zmap->view_list = g_list_append(zmap->view_list, view) ;

	  zmap->state = ZMAP_VIEWS ;
	}
      else
	{
	  destroyZMap(zmap) ;
	  zmap = NULL ;
	}

    }


  return zmap ;
}



/* We need a load or connect call here..... */



/* Reset an existing ZMap, this call will:
 * 
 *    - Completely reset a ZMap window to blank with no sequences displayed and no
 *      threads attached or anything.
 *    - Frees all ZMap window data
 *    - Kills all existings server threads etc.
 * 
 * After this call the ZMap will be ready for the user to specify a new sequence to be
 * loaded.
 * 
 *  */
gboolean zMapReset(ZMap zmap)
{
  gboolean result = FALSE ;

  if (zmap->state == ZMAP_VIEWS)
    {
      killViews(zmap) ;

      zmap->state = ZMAP_RESETTING ;

      result = TRUE ;
    }

  printf("reset a zmap\n") ;

  return TRUE ;
}



/*
 *    A set of accessor functions.
 */

char *zMapGetZMapID(ZMap zmap)
{
  char *id = NULL ;

  if (zmap->state != ZMAP_DYING)
    id = zmap->zmap_id ;

  return id ;
}


char *zMapGetZMapStatus(ZMap zmap)
{
  /* Array must be kept in synch with ZmapState enum in ZMap_P.h */
  static char *zmapStates[] = {"ZMAP_INIT", "ZMAP_VIEWS", "ZMAP_RESETTING",
			       "ZMAP_DYING"} ;
  char *state ;

  zMapAssert(zmap->state >= ZMAP_INIT && zmap->state <= ZMAP_DYING) ;

  state = zmapStates[zmap->state] ;

  return state ;
}


/* Called to kill a zmap window and get all associated windows/threads destroyed.
 */
gboolean zMapDestroy(ZMap zmap)
{
  killZMap(zmap) ;

  return TRUE ;
}



/* We could provide an "executive" kill call which just left the threads dangling, a kind
 * of "kill -9" style thing this would just send a kill to all the views but not wait for the
 * the reply but is complicated by toplevel window potentially being removed before the views
 * windows... */




/* These functions are internal to zmapControl and get called as a result of user interaction
 * with the gui. */

/* Called when the user kills the toplevel window of the ZMap either by clicking the "quit"
 * button or by using the window manager frame menu to kill the window. */
void zmapControlTopLevelKillCB(ZMap zmap)
{
  if (zmap->state != ZMAP_DYING)
    killZMap(zmap) ;

  return ;
}

void zmapControlLoadCB(ZMap zmap)
{
  if (zmap->state != ZMAP_DYING)
    {
      if (zmap->state == ZMAP_INIT)
	{
	  if (zMapViewConnect(zmap->curr_view))
	    zMapViewLoad(zmap->curr_view, "") ;
	}
    }

  return ;
}

void zmapControlResetCB(ZMap zmap)
{
  if (zmap->state == ZMAP_VIEWS)
    zMapViewReset(zmap->curr_view) ;

  return ;
}



/*
 *  ------------------- Internal functions -------------------
 */


static ZMap createZMap(void *app_data, ZMapCallbackFunc app_cb)
{
  ZMap zmap = NULL ;

  /* GROSS HACK FOR NOW, NEED SOMETHING BETTER LATER, JUST A TACKY ID...... */
  static int zmap_num = 0 ;

  zmap = g_new(ZMapStruct, sizeof(ZMapStruct)) ;

  zmap_num++ ;
  zmap->zmap_id = g_strdup_printf("%d", zmap_num) ;

  zmap->curr_view = NULL ;
  zmap->view_list = NULL ;

  zmap->app_data = app_data ;
  zmap->app_zmap_destroyed_cb = app_cb ;

  return zmap ;
}


/* This is the strict opposite of createZMap(), should ONLY be called once all of the Views
 * and other resources have gone. */
static void destroyZMap(ZMap zmap)
{
  g_free(zmap->zmap_id) ;

  g_free(zmap) ;

  return ;
}




/* This routine gets called when, either via a direct call or a callback (user action) the ZMap
 * needs to be destroyed. It does not do the whole destroy but instead signals all the Views
 * to die, when they die they call our view_detroyed_cb and this will eventually destroy the rest
 * of the ZMap (top level window etc.) when all the views have gone. */
static void killZMap(ZMap zmap)
{
  /* no action, if dying already.... */
  if (zmap->state != ZMAP_DYING)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* set our state to DYING....so we don't respond to anything anymore.... */
      /* Must set this as this will prevent any further interaction with the ZMap as
       * a result of both the ZMap window and the threads dying asynchronously.  */
      zmap->state = ZMAP_DYING ;

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* Call the application callback first so that there will be no more interaction with us */
      (*(zmap->app_zmap_destroyed_cb))(zmap, zmap->app_data) ;

      /* If there are no views we can just go ahead and kill everything, otherwise we just
       * signal all the views to die. */
      if (zmap->state == ZMAP_INIT || zmap->state == ZMAP_RESETTING)
	{
	  killFinal(zmap) ;
	}
      else if (zmap->state == ZMAP_VIEWS)
	{
	  killViews(zmap) ;
	}


      /* set our state to DYING....so we don't respond to anything anymore.... */
      /* Must set this as this will prevent any further interaction with the ZMap as
       * a result of both the ZMap window and the threads dying asynchronously.  */
      zmap->state = ZMAP_DYING ;
    }

  return ;
}



/* Gets called when a ZMapView dies, this is asynchronous because the view has to kill threads
 * and wait for them to die and also because the ZMapView my die of its own accord.
 * BUT NOTE that when this routine is called by the last ZMapView within the fmap to say that
 * it has died then we either reset the ZMap to its INIT state or if its dying we kill it. */
static void viewKilledCB(ZMapView view, void *app_data)
{
  ZMap zmap = (ZMap)app_data ;

  /* A View has died so we should clean up its stuff.... */
  zmap->view_list = g_list_remove(zmap->view_list, view) ;


  /* Something needs to happen about the current view, a policy decision here.... */
  if (view == zmap->curr_view)
    zmap->curr_view = NULL ;


  /* If the last view has gone AND we are dying then we can kill the rest of the ZMap
   * and clean up, otherwise we just set the state to "reset". */
  if (!zmap->view_list)
    {
      if (zmap->state == ZMAP_DYING)
	{
	  killFinal(zmap) ;
	}
      else
	zmap->state = ZMAP_INIT ;
    }

  return ;
}


/* This MUST only be called once all the views have gone. */
static void killFinal(ZMap zmap)
{
  zMapAssert(zmap->state != ZMAP_VIEWS) ;

  /* Free the top window */
  if (zmap->toplevel)
    {
      zmapTopWindowDestroy(zmap) ;
      zmap->toplevel = NULL ;
    }

  destroyZMap(zmap) ;

  return ;
}





static void killViews(ZMap zmap)
{
  GList* list_item ;

  zMapAssert(zmap->view_list) ;

  list_item = zmap->view_list ;
  do
    {
      ZMapView view ;

      view = list_item->data ;

      zMapViewDestroy(view) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  zmap->curr_view = NULL ;

  return ;
}





