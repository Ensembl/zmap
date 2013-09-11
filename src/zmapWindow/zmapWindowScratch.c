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


typedef struct _GetFeaturesetCBDataStruct
{
  GQuark set_id;
  ZMapFeatureSet featureset;
} GetFeaturesetCBDataStruct, *GetFeaturesetCBData;


/*!
 * \brief Callback called on every child in a FeatureAny.
 * 
 * For each featureset, compares its id to the id in the user
 * data and if it matches it sets the result in the user data.
 */
static ZMapFeatureContextExecuteStatus getFeaturesetFromIdCB(GQuark key,
                                                             gpointer data,
                                                             gpointer user_data,
                                                             char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  
  GetFeaturesetCBData cb_data = (GetFeaturesetCBData) user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURESET:
      if (feature_any->unique_id == cb_data->set_id)
        cb_data->featureset = (ZMapFeatureSet)feature_any;
      break;
      
    default:
      break;
    };
  
  return status;
}


/*!
 * \brief Find the featureset with the given id in the given window's context
 */
static ZMapFeatureSet getFeaturesetFromId(ZMapWindow window, GQuark set_id)
{
  GetFeaturesetCBDataStruct cb_data = { set_id, NULL };
  
  zMapFeatureContextExecute((ZMapFeatureAny)window->feature_context,
                            ZMAPFEATURE_STRUCT_FEATURESET,
                            getFeaturesetFromIdCB,
                            &cb_data);

  return cb_data.featureset ;
}


/*!
 * \brief Get the single featureset that resides in the scratch column
 *
 * \returns The ZMapFeatureSet, or NULL if there was a problem
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
          feature_set = getFeaturesetFromId(window, set_id);
        }
      else
        {
          zMapWarning("No featureset for column '%s'\n", ZMAP_FIXED_STYLE_SCRATCH_NAME);
        }
    }
  
  return feature_set;
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

