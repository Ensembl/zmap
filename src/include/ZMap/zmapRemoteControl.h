/*  File: zmapRemoteControl.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2010: Genome Research Ltd.
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *      Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: External interface to remote control package.
 *
 * HISTORY:
 * Last edited: Jun 27 11:13 2011 (edgrif)
 * Created: Fri Sep 24 14:51:35 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REMOTE_CONTROL_H
#define ZMAP_REMOTE_CONTROL_H

#include <gtk/gtk.h>


/* temporary while we are testing....???? This needs to be advertised on a per zmap instance
 * basis so naming is crucial...in practice one application will tell the other it's id, then
 * the other can say hello and pass it's id over.... */
#define ZMAP_REMOTE_ID "ZMAP_REMOTE_INIT_ID"




#define TEST_REMOTE_ID "TEST_REMOTE_ID"




#define ZMAP_ANNOTATION_DATA_TYPE   "ZMAP_ANNOTATION_STR"
#define ZMAP_ANNOTATION_DATA_FORMAT 8			    /* Bits per unit. */




/* errr...what's this for ??? */
#define ANNOTATION_ICC "ANNOTATION_DATA"





typedef struct ZMapRemoteControlStructName *ZMapRemoteControl ;


/* RemoteControl callback. */
typedef gboolean (*ZMapRemoteControlCallWithReplyFunc)(void *remote_data, void *reply, int reply_len) ;

/* App Callbacks. */
typedef gboolean (*ZMapRemoteControlRequestHandlerFunc)(ZMapRemoteControlCallWithReplyFunc remote_reply_func,
							void *remote_reply_data,
							void *request, void *user_data) ;
typedef gboolean (*ZMapRemoteControlReplyHandlerFunc)(void *reply, void *user_data) ;
typedef gboolean (*ZMapRemoteControlTimeoutHandlerFunc)(void *user_data) ;
typedef gboolean (*ZMapRemoteControlErrorFunc)(void *user_data, char *err_msg) ;

ZMapRemoteControl zMapRemoteControlCreate(char *app_id,
					  char *remote_control_unique_str,
					  ZMapRemoteControlRequestHandlerFunc request_func, gpointer request_data,
					  ZMapRemoteControlReplyHandlerFunc reply_func, gpointer reply_data,
					  ZMapRemoteControlTimeoutHandlerFunc timeout_func, gpointer timeout_data) ;
gboolean zMapRemoteControlSetErrorCB(ZMapRemoteControl remote_control,
				     ZMapRemoteControlErrorFunc err_func, gpointer err_data) ;
gboolean zMapRemoteControlSetDebug(ZMapRemoteControl remote_control, gboolean debug_on) ;
gboolean zMapRemoteControlSetTimeout(ZMapRemoteControl remote_control, int timeout_secs) ;
gboolean zMapRemoteControlInit(ZMapRemoteControl remote_control, GtkWidget *remote_control_widget) ;
gboolean zMapRemoteControlRequest(ZMapRemoteControl remote_control,
				  char *peer_unique_str, char *peer_xml_request, guint32 time) ;
void zMapRemoteControlDestroy(ZMapRemoteControl remote_control) ;


#endif /* ZMAP_REMOTE_CONTROL_H */
