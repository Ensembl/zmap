/*  Last edited: Nov 09 14:25 2012 (gb10) */
/*  File: zmapWindowEditColumn.c
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the "scratch" or "edit" column in zmap. This
 *              is a column where the user can create and edit temporary 
 *              gene model objects.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindow_P.h>	/* ZMapWindow */
#include <zmapWindowScratch_P.h>


/*!
 * \brief Copy the given feature to the scratch column
 *
 * \param window
 * \param feature The new feature to be copied in
 * \param item The foo canvas item for the new feature
 * \param world_x The clicked x coord
 * \param world_y The clicked y coord
 * \param merge_subfeature If true, just merge the clicked subfeature, otherwise merge the whole feature
 */
void zmapWindowScratchCopyFeature(ZMapWindow window, 
                                  ZMapFeature feature, 
                                  FooCanvasItem *item, 
                                  const double world_x,
                                  const double world_y,
                                  const gboolean use_subfeature)
{
  if (window && feature)
    {
      /* Call back to the View to update all windows */
      ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
      ZMapWindowCallbackCommandScratch scratch_cmd = g_new0(ZMapWindowCallbackCommandScratchStruct, 1) ;
      
      /* Set up general command field for callback. */
      scratch_cmd->cmd = ZMAPWINDOW_CMD_COPYTOSCRATCH ;
      scratch_cmd->feature = feature;
      scratch_cmd->item = item;
      scratch_cmd->world_x = world_x;
      scratch_cmd->world_y = world_y;
      scratch_cmd->use_subfeature = use_subfeature;
      
      (*(window_cbs_G->command))(window, window->app_data, scratch_cmd) ;
    }
  else
    {
      zMapWarning("%s", "Error: no feature selected\n");
    }
}


/*!
 * \brief Clear the scratch column
 */
void zmapWindowScratchClear(ZMapWindow window)
{
  if (!window)
    return;

  /* Call the callback to the view to redraw everything */
  ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
  ZMapWindowCallbackCommandScratch scratch_cmd = g_new0(ZMapWindowCallbackCommandScratchStruct, 1) ;
  
  /* Set up general command field for callback. */
  scratch_cmd->cmd = ZMAPWINDOW_CMD_CLEARSCRATCH ;
  
  (*(window_cbs_G->command))(window, window->app_data, scratch_cmd) ;
}


/*!
 * \brief Undo the last copy into the scratch column
 */
void zmapWindowScratchUndo(ZMapWindow window)
{
  if (!window)
    return;

  /* Call the callback to the view to redraw everything */
  ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
  ZMapWindowCallbackCommandScratch scratch_cmd = g_new0(ZMapWindowCallbackCommandScratchStruct, 1) ;
  
  /* Set up general command field for callback. */
  scratch_cmd->cmd = ZMAPWINDOW_CMD_UNDOSCRATCH ;
  
  (*(window_cbs_G->command))(window, window->app_data, scratch_cmd) ;
}


/*!
 * \brief Redo the last operation
 */
void zmapWindowScratchRedo(ZMapWindow window)
{
  if (!window)
    return;

  /* Call the callback to the view to redraw everything */
  ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
  ZMapWindowCallbackCommandScratch scratch_cmd = g_new0(ZMapWindowCallbackCommandScratchStruct, 1) ;
  
  /* Set up general command field for callback. */
  scratch_cmd->cmd = ZMAPWINDOW_CMD_REDOSCRATCH ;
  
  (*(window_cbs_G->command))(window, window->app_data, scratch_cmd) ;
}

