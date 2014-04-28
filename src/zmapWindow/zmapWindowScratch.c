/*  File: zmapWindowScratch.c
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
 *  Copyright (c) 2012-2014: Genome Research Ltd.
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
 * Description: Implements the Annotation column in zmap. This is a 
 *              column where the user can create and edit temporary 
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
 * \brief Get the single featureset that resides in the scratch column
 *
 * \returns The ZMapFeatureSet, or NULL if the column doesn't exist
 */
ZMapFeatureSet zmapWindowScratchGetFeatureset(ZMapWindow window)
{
  ZMapFeatureSet feature_set = NULL;

  if (window && window->context_map)
    {
      GQuark column_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME);
      GList *fs_list = zMapFeatureGetColumnFeatureSets(window->context_map, column_id, TRUE);
  
      /* There should be one (and only one) featureset in the column */
      if (g_list_length(fs_list) > 0)
        {         
          GQuark set_id = (GQuark)(GPOINTER_TO_INT(fs_list->data));
          feature_set = zmapFeatureContextGetFeaturesetFromId(window->feature_context, set_id);
        }
    }
  
  return feature_set;
}


static void doScratchCallbackCommand(ZMapWindowCommandType command_type,
                                     ZMapWindow window,
                                     ZMapFeature feature, 
                                     FooCanvasItem *item, 
                                     const double world_x,
                                     const double world_y,
                                     const gboolean use_subfeature)
{
  gboolean ok = TRUE ;
  ZMapWindowCallbackCommandScratch scratch_cmd = g_new0(ZMapWindowCallbackCommandScratchStruct, 1) ;
 
  /* Set up general command field for callback. */
  scratch_cmd->cmd = command_type ;
  scratch_cmd->seq_start = 0;
  scratch_cmd->seq_end = 0;
  scratch_cmd->subpart = NULL;
  scratch_cmd->use_subfeature = use_subfeature ;

  if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT && !use_subfeature)
    {
      /* For alignment features, get all of the linked match blocks */
      scratch_cmd->features = zMapWindowCanvasAlignmentGetAllMatchBlocks(item) ;
    }
  else
    {
      /* Otherwise just pass the single given feature */
      scratch_cmd->features = g_list_append(scratch_cmd->features, feature) ;
    }

  if (feature->mode == ZMAPSTYLE_MODE_SEQUENCE)
    {
      /* For sequence features, get the sequence coord that was clicked */
      zMapWindowItemGetSeqCoord(item, TRUE, world_x, world_y, &scratch_cmd->seq_start, &scratch_cmd->seq_end) ;
    }

  /* If necessary, find the subfeature and set it in the data. What constitutes a
   * "subfeature" depends on the feature type: for transcripts, it's an intron/exon. For
   * alignments, it's a match block, which is actually the whole ZMapFeature (and the
   * "entire" feature therefore includes the linked match blocks from the same alignment).  */
  if (use_subfeature && feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      zMapWindowCanvasItemGetInterval((ZMapWindowCanvasItem)item, world_x, world_y, &scratch_cmd->subpart) ;
      
      if (!scratch_cmd->subpart)
        {
          ok = FALSE ;
          zMapWarning("Cannot find subfeature for feature '%s' at position %f,%f", 
                      g_quark_to_string(feature->original_id), world_x, world_y) ;
        }
    }

  if (ok)
    {
      /* Call back to the View to update all windows */
      ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
      (*(window_cbs_G->command))(window, window->app_data, scratch_cmd) ;
    }
  else
    {
      g_free(scratch_cmd) ;
      scratch_cmd = NULL ;
    }
}

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
      doScratchCallbackCommand(ZMAPWINDOW_CMD_COPYTOSCRATCH, 
                               window,
                               feature, 
                               item, 
                               world_x,
                               world_y,
                               use_subfeature) ;
    }
  else
    {
      zMapWarning("%s", "Error: no feature selected\n");
    }
}


/*!
 * \brief Delete the subfeature at the given feature's coords from the scratch column
 *
 * \param window
 * \param feature The new feature to be copied in
 * \param item The foo canvas item for the new feature
 * \param world_x The clicked x coord
 * \param world_y The clicked y coord
 * \param merge_subfeature If true, just merge the clicked subfeature, otherwise merge the whole feature
 */
void zmapWindowScratchDeleteFeature(ZMapWindow window, 
                                    ZMapFeature feature, 
                                    FooCanvasItem *item, 
                                    const double world_x,
                                    const double world_y,
                                    const gboolean use_subfeature)
{
  if (window && feature)
    {
      doScratchCallbackCommand(ZMAPWINDOW_CMD_DELETEFROMSCRATCH, 
                               window,
                               feature, 
                               item, 
                               world_x,
                               world_y,
                               use_subfeature) ;
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

