/*  Last edited: 30 Oct 16"27 2012 (gb10) */
/*  File: zmapViewEditColumn.c
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
 * Description: Handles the 'edit' column, which allows users to
 *              create and edit temporary features
 *
 * Exported functions: see zmapView_P.h
 *
 *-------------------------------------------------------------------
 */

#include <glib.h>

#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <zmapView_P.h>


/*!
 * \brief Initialise the Edit column
 *
 * \param[in] zmap_view
 * \param[in] sequence
 *
 * \return void
 */
void zmapViewEditColumnInit(ZMapView zmap_view, ZMapFeatureSequenceMap sequence)
{
  const char *featureset_name = "scratch";
  ZMapFeatureContextMap context_map = &zmap_view->context_map;

  /* Create a featureset for the edit column */
  ZMapFeatureSet feature_set = zMapFeatureSetCreate((char*)featureset_name, NULL);

  /* Create the context, align and block, and add the featureset to it */
  ZMapFeatureContext context = zmapViewCreateContext(sequence, NULL, feature_set);

  /* Merge our context into the view's context and view the diff context */
  ZMapFeatureContext diff_context = zmapViewMergeInContext(zmap_view, context);
  zmapViewDrawDiffContext(zmap_view, &diff_context);
  
  /* Create the source and add it to the source_2_sourcedata hash table */
  GQuark fid = g_quark_from_string(featureset_name);
  ZMapFeatureSource src = g_new0(ZMapFeatureSourceStruct,1);
  src->source_id = fid;
  src->source_text = src->source_id;
  src->style_id = fid;
  g_hash_table_insert(context_map->source_2_sourcedata, GUINT_TO_POINTER(fid), src) ;

  /* Create the FeatureSetDesc and add it to the featureset_2_column hash table */
  ZMapFeatureSetDesc set_data = g_new0(ZMapFeatureSetDescStruct,1);
  set_data->column_id = fid;
  set_data->column_ID = fid;
  g_hash_table_insert(context_map->featureset_2_column,GUINT_TO_POINTER(fid), (gpointer) set_data);


#warning gb10: hack in a test feature
  ZMapFeature translation = zMapFeatureCreateEmpty() ;
  zMapFeatureAddStandardData(translation, featureset_name, featureset_name,
                             "scratch_seq", "sequence",
                             ZMAPSTYLE_MODE_SEQUENCE, &feature_set->style,
                             0, 500, FALSE, 0.0,
                             ZMAPSTRAND_NONE) ;
  
  zMapFeatureSequenceSetType(translation, ZMAPSEQUENCE_PEPTIDE) ;
  zMapFeatureAddFrame(translation, ZMAPFRAME_NONE) ;
  
  zMapFeatureSetAddFeature(feature_set, translation) ;
  
}
