/*  File: zmapViewCommand.c
 *  Author: ed (edgrif@sanger.ac.uk)
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
 * Description: Implements processing of commands to be routed to
 *              ZMapWindows within this view, e.g. splice highlighting.
 *
 * Exported functions: See zmapView_P.h
 *-------------------------------------------------------------------
 */


#include <zmapView_P.hpp>


static void callWindowCommandCB(gpointer data, gpointer user_data) ;


/* 
 *                       Package routines.
 */


/* Interface needs to be broadened to allow operation on a single or all windows.
 * 
 * Note we just pass the command through...we just use view as a vehicle to pass
 * the command to all the windows....
 *  */
gboolean zmapViewPassCommandToAllWindows(ZMapView view, gpointer user_data)
{
  gboolean result = TRUE ;                                  /* HACK FOR NOW... */

  /* Need to sort out errors..... */

  g_list_foreach(view->window_list, callWindowCommandCB, user_data) ;


  return result ;
}





/*
 *                      Internal routines
 */


/* A GFunc() called for all windows in a view to execute a window command. */
static void callWindowCommandCB(gpointer data, gpointer user_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)data ;
  ZMapWindowCallbackCommandAny cmd_any = (ZMapWindowCallbackCommandAny)user_data ;
  ZMapWindow window = view_window->window ;
  char *err_msg = NULL ;

  zMapWindowExecuteCommand(window, cmd_any, &err_msg) ;

  return ;
}


