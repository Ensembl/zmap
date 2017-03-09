/*  File: zmapViewCommand.c
 *  Author: ed (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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


