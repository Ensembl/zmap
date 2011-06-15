/*  File: zmapAppRemoteControl.c
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Application level functions to set up ZMap remote control
 *              to communicate with peer routines.
 *
 * Exported functions: See zmapApp_P.h
 * HISTORY:
 * Last edited: Nov 25 13:50 2010 (edgrif)
 * Created: Mon Nov  1 10:32:56 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <ZMap/zmapRemoteControl.h>
#include <zmapApp_P.h>



static gboolean requestHandlerCB(ZMapRemoteControlCallWithReplyFunc remote_reply_func, void *remote_reply_data,
				 void *request, void *user_data) ;
static gboolean replyHandlerCB(void *reply, void *user_data) ;
static gboolean timeoutHandlerFunc(void *user_data) ;



gboolean zmapAppRemoteControlCreate(ZMapAppContext app_context)
{
  gboolean result = TRUE ;
  char *unique_id ;
  
  unique_id = zMapMakeUniqueID(zMapGetAppName()) ;


  if (!(app_context->remote_controller = zMapRemoteControlCreate(zMapGetAppName(), unique_id,
								 requestHandlerCB, app_context,
								 replyHandlerCB, app_context,
								 timeoutHandlerFunc, app_context)))
    {
      zMapCritical("%s", "Initialisation of remote control failed.") ;
      result = FALSE ;
    }
  else
    {
      /* set no timeout for now... */

      zMapRemoteControlSetTimeout(app_context->remote_controller, 0) ;
    }


  return result ;
}


gboolean zmapAppRemoteControlInit(ZMapAppContext app_context)
{
  gboolean result = FALSE ;


  if (!(result = zMapRemoteControlInit(app_context->remote_controller, app_context->app_widg)))
    zMapLogCritical("%s", "Cannot initialise zmap remote control interface.") ;

  return result ;
}

gboolean zmapAppRemoteControlConnect(ZMapAppContext app_context)
{
  gboolean result = FALSE ;

  if (!(result = zMapRemoteControlRequest(app_context->remote_controller, app_context->peer_unique_id,
					  "hi there", 0)))
    zMapLogCritical("%s", "Cannot initialise zmap remote control interface.") ;

  return result ;
}





/* 
 *                Internal functions
 */


static gboolean requestHandlerCB(ZMapRemoteControlCallWithReplyFunc remote_reply_func, void *remote_reply_data,
				 void *request, void *user_data)
{
  gboolean result = TRUE ;
  char *test_data = "Test reply from ZMap." ;

  /* this is where we need to process the zmap commands perhaps passing on the remote_reply_func
   * and data to be called from a callback for an asynch. request.
   * 
   * For now we just call straight back for testing.... */
  result = (remote_reply_func)(remote_reply_data, test_data, strlen(test_data) + 1) ;


  return result ;
}


static gboolean replyHandlerCB(void *request, void *user_data)
{
  gboolean result = TRUE ;




  return result ;
}




static gboolean timeoutHandlerFunc(void *user_data)
{
  gboolean result = FALSE ;




  return result ;
}




