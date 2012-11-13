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
ZMapFeatureSet zmapWindowGetFeaturesetFromId(ZMapWindow window, GQuark set_id)
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
ZMapFeatureSet scratchGetFeatureset(ZMapWindow window)
{
  ZMapFeatureSet feature_set = NULL;

  GQuark column_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME);
  GList *fs_list = zMapFeatureGetColumnFeatureSets(window->context_map, column_id, TRUE);
  
  /* There should be one (and only one) featureset in the column */
  if (g_list_length(fs_list) == 1)
    {         
      GQuark set_id = (GQuark)(GPOINTER_TO_INT(fs_list->data));
      feature_set = zmapWindowGetFeaturesetFromId(window, set_id);
    }
  else
    {
      zMapWarning("Expected 1 featureset in column '%s' but found %d\n", ZMAP_FIXED_STYLE_SCRATCH_NAME, g_list_length(fs_list));
    }

  return feature_set;
}


/*! 
 * \brief Does the work to add/merge a feature to the scratch column
 */
void scratchAddFeature(ZMapFeatureSet feature_set, ZMapFeature feature)
{
  /* to do: merge with existing features */
  zMapFeatureSetAddFeature(feature_set, feature);
}


/*!
 * \brief Copy the given feature to the scratch column 
 */
void zmapWindowScratchCopyFeature(ZMapWindow window, ZMapFeature feature)
{
  if (feature)
    {
      ZMapFeature new_feature = (ZMapFeature)zMapFeatureAnyCopy((ZMapFeatureAny)feature);
      ZMapFeatureSet feature_set = scratchGetFeatureset(window);
      
      if (feature_set)
        scratchAddFeature(feature_set, new_feature);
    }
  else
    {
      zMapWarning("%s", "Error: no feature selected\n");
    }
}
