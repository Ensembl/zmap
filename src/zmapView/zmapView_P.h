/*  File: zmapView_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * HISTORY:
 * Last edited: May 13 15:41 2004 (edgrif)
 * Created: Thu May 13 15:06:21 2004 (edgrif)
 * CVS info:   $Id: zmapView_P.h,v 1.1 2004-05-13 14:44:45 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONTROL_P_H
#define ZMAP_CONTROL_P_H

#include <glib.h>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapConn.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapConfig.h>
#include <ZMap/ZMap.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <ZMap/zmapView.h>


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* The overall state of the zmap, we need this because both the zmap window and the its threads
 * will die asynchronously so we need to block further operations while they are in this state.
 * We may also need a state for "reset", which would be when the window is reset. */
typedef enum {
  ZMAP_INIT,						    /* Created, display but no threads. */
  ZMAP_RUNNING,						    /* Display with threads in normal state. */
  ZMAP_RESETTING,					    /* Display with no data/threads. */
  ZMAP_DYING,						    /* ZMap is dying for some reason,
							       cannot do anything in this state. */
  ZMAP_DEAD						    /* completely defunct...may not be needed. */
} ZmapState ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* For now I won't expose this beyond this private header...may be necessary later... */

/* A "View" is a set of one or move windows that display data retrieved from one or
 * more servers. Note that the "View" windows are not top level windows, they are panes
 * within a top level window.
 * Each View has window data and connection data, the intent is that the window
 * code knows about the window data and the connection data knows about the
 * connection data.... */
typedef struct _ZMapViewStruct
{
  ZmapState state ;

  gchar *sequence ;

  guint idle_handle ;

  GtkWidget *parent_widget ;

  GList *window_list ;
  GList *connection_list ;

  void *app_data ;
  ZMapCallbackFunc destroy_cb ;

} ZMapViewStruct ;



#endif /* !ZMAP_CONTROL_P_H */
