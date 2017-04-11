/*  File: zmapApp_P.h
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
 * Description: Private remote control header for application level of zmap.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_APPREMOTE_PRIV_H
#define ZMAP_APPREMOTE_PRIV_H


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapControl.hpp>
#include <ZMap/zmapAppServices.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <zmapApp_P.hpp>


/* Struct for zmap's remote control object. */
typedef struct ZMapAppRemoteStructType
{

  /* Peer's names/text etc. */
  const char *peer_name ;
  char *peer_socket ;

  /* Our names/text etc. */
  char *app_id ;                                            /* zmap app name. */
  char *app_socket ;

  ZMapRemoteControl remote_controller ;


  ZMapAppRemoteExitFunc exit_routine ;                      /* Called to exit application after we
                                                               have signalled to peer we are going. */


  /* Remote Control callback function that we call with our reply to a request. */
  ZMapRemoteControlReturnReplyFunc return_reply_func ;
  void *return_reply_func_data ;


  /* App function to be called with peer's reply to the request. */
  ZMapRemoteAppProcessReplyFunc process_reply_func ;
  gpointer process_reply_func_data ;

  /* App function to be called if there was an error in sending the request, e.g. timeout. */
  ZMapRemoteAppErrorHandlerFunc error_handler_func ;
  gpointer error_handler_func_data ;


  /* Incoming request FROM a peer. */
  char *curr_peer_request ;                                 /* DO WE NEED TO CACHE THIS ?? */
  char *curr_peer_command ;
  ZMapView curr_view_id ;


  /* Outgoing request TO a peer */
  char *curr_zmap_request ;



  /* There are some requests that can only be serviced _after_ we are sure the peer
   * has received our reply to their request e.g. "shutdown" where we can't exit
   * otherwise peer will never receive our reply. */
  gboolean deferred_action ;


  /* When operating with a peer we have an overall timeout time, this allows us to
   * warn the user if we have no view displayed and there has been no activity with
   * the peer for a long time. */
  guint inactive_timeout_interval_s ;                       /* Time out interval. */
  guint inactive_func_id ;                                  /* glib timeout handler func. id. */
  time_t last_active_time_s ;                               /* Last remote request, in or out. */


} ZMapAppRemoteStruct ;


#endif /* !ZMAP_APPREMOTE_PRIV_H */
