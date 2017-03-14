/*  File: zmapAppRemoteUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description:
 *
 * Exported functions: See ZMap/zmapAppRemote.h
 *
 * HISTORY:
 * Created: Mon Feb 20 10:21:03 2012 (edgrif)
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <glib.h>

#include <ZMap/zmapAppRemote.hpp>





/*
 *                 External routines
 */


/* To make sure that commands are delivered to the right component of zmap (ZMapManager,
 * ZMap, ZMapView or ZMapWindow an id system is used to identify the component. The
 * id is returned to the caller when a new "view" is created. Each view is associated
 * with a ZMapView and is formed from the memory address of the view in the form
 * "0xZMapView_address", e.g. "0xXXXXXXXX". In subsequent operations the peer provides
 * the id for the view they wish to apply the command to and the zmap request handling
 * code makes sure the command is delivered to the correct component.
 *
 * The following functions create and decode the id.
 *
 *  */



/* Returns TRUE on success and the id GQuark is returned in id_out, FALSE otherwise. */
gboolean zMapAppRemoteViewCreateID(gpointer view_id, GQuark *id_out)
{
  gboolean result = FALSE ;
  char *id_str ;

  id_str = g_strdup_printf("%p", view_id) ;

  *id_out = g_quark_from_string(id_str) ;

  g_free(id_str) ;

  result = TRUE ;

  return result ;
}

/* Returns TRUE on success and the id string is returned in id_str_out, FALSE otherwise.
 * String should be g_free'd when no longer required. */
gboolean zMapAppRemoteViewCreateIDStr(gpointer view_id, char **id_str_out)
{
  gboolean result = FALSE ;

  *id_str_out = g_strdup_printf("%p", view_id) ;

  result = TRUE ;

  return result ;
}


/* Caller can supply a pointer in which case it is filled in with the parsed
 * value.
 *
 * Returns TRUE if string can be parsed, FALSE otherwise.
 *  */
gboolean zMapAppRemoteViewParseID(GQuark id, gpointer *view_id_out)
{
  gboolean result = FALSE ;
  char *id_str ;
  void *view_ptr = NULL ;

  id_str = (char *)g_quark_to_string(id) ;

  if (sscanf(id_str, "%p", &view_ptr) == 1)
    {
      if (view_id_out)
        *view_id_out = view_ptr ;

      result = TRUE ;
    }

  return result ;
}

/* Caller can supply a pointer in which case it is filled in with the parsed
 * value.
 *
 * Returns TRUE if string can be parsed, FALSE otherwise.
 *  */
gboolean zMapAppRemoteViewParseIDStr(char *id_str, gpointer *view_id_out)
{
  gboolean result = FALSE ;
  void *view_ptr = NULL ;

  if (sscanf(id_str, "%p", &view_ptr) == 1)
    {
      if (view_id_out)
        *view_id_out = view_ptr ;

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



