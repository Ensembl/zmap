/*  File: zmapApp.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *
 * Description: Private header for application level of zmap.
 * 
 * HISTORY:
 * Last edited: Mar  2 14:22 2010 (edgrif)
 * Created: Thu Jul 24 14:35:41 2003 (edgrif)
 * CVS info:   $Id: zmapApp_P.h,v 1.28 2010-03-03 11:02:18 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_APP_PRIV_H
#define ZMAP_APP_PRIV_H

#include <gtk/gtk.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapManager.h>
#include <ZMap/zmapXRemote.h>


/* Minimum GTK version supported. */
enum {ZMAP_GTK_MAJOR = 2, ZMAP_GTK_MINOR = 2, ZMAP_GTK_MICRO = 4} ;


/* States for the application, needed especially because we can be waiting for threads to die. */
typedef enum
  {
    ZMAPAPP_INIT,					    /* Initialising, no threads. */
    ZMAPAPP_RUNNING,					    /* Normal execution. */
    ZMAPAPP_DYING					    /* Waiting for child ZMaps to dies so
							       app can exit. */
  } ZMapAppState ;


/* Default time out (in seconds) for exitting zmap, different systems seem to take very times to
 * kill threads. */
#ifdef DARWIN
enum {ZMAP_DEFAULT_EXIT_TIMEOUT = 10} ;
#else
enum {ZMAP_DEFAULT_EXIT_TIMEOUT = 10} ;
#endif

/* Max size of log file before we start warning user that log file is very big (in megabytes). */
enum {ZMAP_DEFAULT_MAX_LOG_SIZE = 100} ;


/* Overall application control struct. */
typedef struct _ZMapAppContextStruct
{
  ZMapAppState state ;					    /* Needed to control exit in a clean way. */

  int exit_timeout ;					    /* time (s) to wait before forced exit. */

  int exit_rc ;
  char *exit_msg ;

  GtkWidget *app_widg ;

  GtkWidget *sequence_widg ;
  GtkWidget *start_widg ;
  GtkWidget *end_widg ;

  GtkTreeStore *tree_store_widg ;

  GError *info;                 /* This is an object to hold a code
                                 * and a message as info for the
                                 * remote control simple IPC stuff */
  ZMapManager zmap_manager ;
  ZMap selected_zmap ;

  gulong property_notify_event_id;
  ZMapXRemoteObj xremote_client ;			    /* The external program we are sending
							       commands to. */

  gboolean show_mainwindow ;				    /* Should main window be displayed. */

  char *default_sequence ;				    /* Was a default sequence specified in
							       the config. file.*/

  char *locale;
  gboolean sent_finalised ;
  
  char *script_dir;					    /* where scripts are kept for the pipeServer module
							     * can be set in [ZMap] or defaults to run-time directory
							     */

  gboolean xremote_debug ;				    /* Turn on/off debugging for xremote connections. */

  
} ZMapAppContextStruct, *ZMapAppContext ;


/* cols in connection list. */
enum {ZMAP_NUM_COLS = 4} ;

enum {
  ZMAPID_COLUMN,
  ZMAPSEQUENCE_COLUMN,
  ZMAPSTATE_COLUMN,
  ZMAPLASTREQUEST_COLUMN,
  ZMAPDATA_COLUMN,
  ZMAP_N_COLUMNS
};

int zmapMainMakeAppWindow(int argc, char *argv[]) ;
GtkWidget *zmapMainMakeMenuBar(ZMapAppContext app_context) ;
GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context) ;
GtkWidget *zmapMainMakeManage(ZMapAppContext app_context) ;
void zmapAppCreateZMap(ZMapAppContext app_context, char *sequence, int start, int end) ;
void zmapAppExit(ZMapAppContext app_context) ;

void zmapAppRemoteInstaller(GtkWidget *widget, gpointer app_context_data);
void zmapAppRemoteSendFinalised(ZMapAppContext app_context);

#endif /* !ZMAP_APP_PRIV_H */
