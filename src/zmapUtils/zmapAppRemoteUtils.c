/*  File: zmapAppRemoteUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2012: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See ZMap/zmapAppRemote.h
 *              
 * HISTORY:
 * Created: Mon Feb 20 10:21:03 2012 (edgrif)
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <glib.h>

#include <ZMap/zmapAppRemote.h>





/* 
 *                 External routines
 */



/* To make sure that commands are delivered to the right component of zmap (ZMapManager,
 * ZMap, ZMapView or ZMapWindow an id system is used to identify the component. The
 * id is returned to the caller when a new "view" is created. Each view is associated
 * with a ZMap, a ZMapView and a ZMapWindow and is formed from a tuple of their
 * memory addresses in the form "ZMap_address.ZMapView_address.ZMapWindow_address",
 * e.g. "0xXXXXXXXX.0xYYYYYYYY.0xZZZZZZZZ". In subsequent operations the peer provides
 * the id for the view they wish to apply the command to and the zmap request handling
 * code makes sure the command is delivered to the correct component.
 * 
 * The following functions create and decode the id.
 * 
 *  */



/* Returns TRUE on success and the id GQuark is returned in id_out, FALSE otherwise. */
gboolean zMapAppRemoteViewCreateID(ZMapAppRemoteViewID view_id, GQuark *id_out)
{
  gboolean result = FALSE ;

  if (view_id->zmap && view_id->view && view_id->window)
    {
      char *id_str ;

      id_str = g_strdup_printf("%p.%p.%p", view_id->zmap, view_id->view, view_id->window) ;

      *id_out = g_quark_from_string(id_str) ;

      g_free(id_str) ;

      result = TRUE ;
    }

  return result ;
}

/* Returns TRUE on success and the id string is returned in id_str_out, FALSE otherwise.
 * String should be g_free'd when no longer required. */
gboolean zMapAppRemoteViewCreateIDStr(ZMapAppRemoteViewID view_id, char **id_str_out)
{
  gboolean result = FALSE ;

  if (view_id->zmap && view_id->view && view_id->window)
    {
      *id_str_out = g_strdup_printf("%p.%p.%p", view_id->zmap, view_id->view, view_id->window) ;
      result = TRUE ;
    }

  return result ;
}


/* Caller can supply struct in which case it is filled in (so they can choose to have it
 * on the stack or not) or if view_id_out is NULL one is allocated which should be
 * g_free'd when no longer required.
 * 
 * Returns TRUE and a filled in struct if string can be parsed, FALSE otherwise.
 *  */
gboolean zMapAppRemoteViewParseID(GQuark id, ZMapAppRemoteViewID *view_id_out)
{
  gboolean result = FALSE ;
  char *id_str ;
  void *zmap_ptr = NULL, *view_ptr = NULL, *window_ptr = NULL ;

  id_str = (char *)g_quark_to_string(id) ;

  if (sscanf(id_str, "%p.%p.%p", &zmap_ptr, &view_ptr, &window_ptr) == 3)
    {
      ZMapAppRemoteViewID view_id = *view_id_out ;

      if (!view_id)
	view_id = g_new0(ZMapAppRemoteViewIDStruct, 1) ;

      view_id->zmap = zmap_ptr ;
      view_id->view = view_ptr ;
      view_id->window = window_ptr ;

      *view_id_out = view_id ;
      result = TRUE ;
    }

  return result ;
}

/* Caller can supply struct in which case it is filled in (so they can choose to have it
 * on the stack or not) or if view_id_out is NULL one is allocated which should be
 * g_free'd when no longer required.
 * 
 * Returns TRUE and a filled in struct if string can be parsed, FALSE otherwise.
 *  */
gboolean zMapAppRemoteViewParseIDStr(char *id_str, ZMapAppRemoteViewID *view_id_out)
{
  gboolean result = FALSE ;
  void *zmap_ptr = NULL, *view_ptr = NULL, *window_ptr = NULL ;

  if (sscanf(id_str, "%p.%p.%p", &zmap_ptr, &view_ptr, &window_ptr) == 3)
    {
      ZMapAppRemoteViewID view_id = *view_id_out ;

      if (!view_id)
	view_id = g_new0(ZMapAppRemoteViewIDStruct, 1) ;

      view_id->zmap = zmap_ptr ;
      view_id->view = view_ptr ;
      view_id->window = window_ptr ;

      *view_id_out = view_id ;
      result = TRUE ;
    }

  return result ;
}

/* Returns TRUE if all elements of struct filled in, FALSE otherwise. */
gboolean zMapAppRemoteViewIsValidID(ZMapAppRemoteViewID view_id)
{
  gboolean result = FALSE ;

  if (view_id && view_id->zmap && view_id->view && view_id->window)
    result = TRUE ;

  return result ;
}

/* Simply NULL the view_id. */
gboolean zMapAppRemoteViewResetID(ZMapAppRemoteViewID view_id)
{
  gboolean result = FALSE ;

  if (view_id)
    {
      view_id->zmap = view_id->view = view_id->window = NULL ;
      result = TRUE ;
    }

  return result ;
}




/* 
 *                 Package routines
 */





/* 
 *                 Internal routines
 */



