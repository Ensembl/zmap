/*  Last edited: 30 Oct 16"27 2012 (gb10) */
/*  File: zmapViewScratchColumn.c
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
 * Description: Handles the 'scratch' or 'edit' column, which allows users to
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


static void handBuiltInit(ZMapView zmap_view, ZMapFeatureSequenceMap sequence, ZMapFeatureContext context)
{
  ZMapFeatureSet featureset = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  ZMapFeatureSetDesc f2c;
  ZMapFeatureSource src;
  GList *list;
  ZMapFeatureColumn column;

  ZMapFeatureContextMap context_map = &zmap_view->context_map;

  if((style = zMapFindStyle(context_map->styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_HAND_BUILT_NAME))))
    {
      /* Create the featureset */
      featureset = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_HAND_BUILT_NAME, NULL);
      style = zMapFeatureStyleCopy(style);
      featureset->style = style ;

      /* Create the context, align and block, and add the featureset to it */
      if (!context)
        context = zmapViewCreateContext(zmap_view, NULL, featureset);

	/* set up featureset2_column and anything else needed */
      f2c = g_hash_table_lookup(context_map->featureset_2_column, GUINT_TO_POINTER(featureset->unique_id));
      if(!f2c)	/* these just accumulate  and should be removed from the hash table on clear */
	{
		f2c = g_new0(ZMapFeatureSetDescStruct,1);

		f2c->column_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_HAND_BUILT_NAME);
		f2c->column_ID = g_quark_from_string(ZMAP_FIXED_STYLE_HAND_BUILT_NAME);
		f2c->feature_src_ID = g_quark_from_string(ZMAP_FIXED_STYLE_HAND_BUILT_NAME);
		f2c->feature_set_text = ZMAP_FIXED_STYLE_HAND_BUILT_TEXT;
		g_hash_table_insert(context_map->featureset_2_column, GUINT_TO_POINTER(featureset->unique_id), f2c);
	}

      src = g_hash_table_lookup(context_map->source_2_sourcedata, GUINT_TO_POINTER(featureset->unique_id));
      if(!src)
	{
		src = g_new0(ZMapFeatureSourceStruct,1);
		src->source_id = f2c->feature_src_ID;
		src->source_text = g_quark_from_string(ZMAP_FIXED_STYLE_HAND_BUILT_TEXT);
		src->style_id = style->unique_id;
		src->maps_to = f2c->column_id;
		g_hash_table_insert(context_map->source_2_sourcedata, GUINT_TO_POINTER(featureset->unique_id), src);
	}

      list = g_hash_table_lookup(context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id));
      if(!list)
	{
		list = g_list_prepend(list,GUINT_TO_POINTER(src->style_id));
		g_hash_table_insert(context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id), list);
	}

      column = g_hash_table_lookup(context_map->columns,GUINT_TO_POINTER(f2c->column_id));
      if(!column)
	{
		column = g_new0(ZMapFeatureColumnStruct,1);
		column->unique_id = f2c->column_id;
		column->style_table = g_list_prepend(NULL, (gpointer)  style);
		/* the rest shoudl get filled in elsewhere */
		g_hash_table_insert(context_map->columns, GUINT_TO_POINTER(f2c->column_id), column);
	}
    }  
}


/*!
 * \brief Initialise the Scratch column
 *
 * \param[in] zmap_view
 * \param[in] sequence
 *
 * \return void
 */
void zmapViewScratchInit(ZMapView zmap_view, ZMapFeatureSequenceMap sequence, ZMapFeatureContext context_in, ZMapFeatureBlock block_in)
{
  ZMapFeatureSet scratch_featureset = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  ZMapFeatureSetDesc f2c;
  ZMapFeatureSource src;
  GList *list;
  ZMapFeatureColumn column;

  ZMapFeatureContextMap context_map = &zmap_view->context_map;
  ZMapFeatureContext context = context_in;
  ZMapFeatureBlock block = block_in;

  if((style = zMapFindStyle(context_map->styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME))))
    {
      /* Create the featureset */
      scratch_featureset = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_SCRATCH_NAME, NULL);
      style = zMapFeatureStyleCopy(style);
      scratch_featureset->style = style ;

      /* Create the context, align and block, and add the featureset to it */
      if (!context || !block)
        context = zmapViewCreateContext(zmap_view, NULL, scratch_featureset);
      else
        zMapFeatureBlockAddFeatureSet(block, scratch_featureset);
      
	/* set up featureset2_column and anything else needed */
      f2c = g_hash_table_lookup(context_map->featureset_2_column, GUINT_TO_POINTER(scratch_featureset->unique_id));
      if(!f2c)	/* these just accumulate  and should be removed from the hash table on clear */
	{
		f2c = g_new0(ZMapFeatureSetDescStruct,1);

		f2c->column_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME);
		f2c->column_ID = g_quark_from_string(ZMAP_FIXED_STYLE_SCRATCH_NAME);
		f2c->feature_src_ID = g_quark_from_string(ZMAP_FIXED_STYLE_SCRATCH_NAME);
		f2c->feature_set_text = ZMAP_FIXED_STYLE_SCRATCH_TEXT;
		g_hash_table_insert(context_map->featureset_2_column, GUINT_TO_POINTER(scratch_featureset->unique_id), f2c);
	}

      src = g_hash_table_lookup(context_map->source_2_sourcedata, GUINT_TO_POINTER(scratch_featureset->unique_id));
      if(!src)
	{
		src = g_new0(ZMapFeatureSourceStruct,1);
		src->source_id = f2c->feature_src_ID;
		src->source_text = g_quark_from_string(ZMAP_FIXED_STYLE_SCRATCH_TEXT);
		src->style_id = style->unique_id;
		src->maps_to = f2c->column_id;
		g_hash_table_insert(context_map->source_2_sourcedata, GUINT_TO_POINTER(scratch_featureset->unique_id), src);
	}

      list = g_hash_table_lookup(context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id));
      if(!list)
	{
		list = g_list_prepend(list,GUINT_TO_POINTER(src->style_id));
		g_hash_table_insert(context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id), list);
	}

      column = g_hash_table_lookup(context_map->columns,GUINT_TO_POINTER(f2c->column_id));
      if(!column)
	{
		column = g_new0(ZMapFeatureColumnStruct,1);
		column->unique_id = f2c->column_id;
		column->style_table = g_list_prepend(NULL, (gpointer)  style);
		/* the rest shoudl get filled in elsewhere */
		g_hash_table_insert(context_map->columns, GUINT_TO_POINTER(f2c->column_id), column);
	}

      /* Create an empty feature */
      ZMapFeature feature = zMapFeatureCreateEmpty() ;
      char *feature_name = "scratch_feature";
      
      zMapFeatureAddStandardData(feature, feature_name, feature_name,
                                 NULL, NULL,
                                 ZMAPSTYLE_MODE_TRANSCRIPT, &scratch_featureset->style,
                                 0, 0, FALSE, 0.0,
                                 ZMAPSTRAND_NONE);

      zMapFeatureTranscriptInit(feature);
      zMapFeatureAddTranscriptStartEnd(feature, FALSE, 0, FALSE);
      
      //zMapFeatureSequenceSetType(feature, ZMAPSEQUENCE_PEPTIDE);
      //zMapFeatureAddFrame(feature, ZMAPFRAME_NONE);
      
      zMapFeatureSetAddFeature(scratch_featureset, feature);      
    }  

  /* Also initialise the "hand_built" column. xace puts newly created
   * features in this column. */
  handBuiltInit(zmap_view, sequence, context);

  /* Merge our context into the view's context and view the diff context */
  if (!context_in)
    {
      ZMapFeatureContext diff_context = zmapViewMergeInContext(zmap_view, context);
      zmapViewDrawDiffContext(zmap_view, &diff_context, NULL);
    }
}


/*!
 * \brief Update any changes to the given featureset
 */
gboolean zmapViewScratchUpdateFeature(ZMapView zmap_view, 
                                      ZMapFeatureSequenceMap sequence,
                                      ZMapFeature feature,
                                      ZMapFeatureSet feature_set,
                                      ZMapFeatureContext context)
{

  
  GList *list_item = NULL;

  //  zmapViewResetWindows(zmap_view, FALSE);

  for (list_item = g_list_first(zmap_view->window_list); list_item; list_item = g_list_next(list_item))
    {
      ZMapViewWindow view_window = list_item->data ;
      zMapWindowFeatureReset(view_window->window, zmap_view->revcomped_features) ;
    }

  //zMapWindowNavigatorReset(zmap_view->navigator_window);  
  //zMapWindowNavigatorSetStrand(zmap_view->navigator_window, zmap_view->revcomped_features);
  //zMapWindowNavigatorDrawFeatures(zmap_view->navigator_window, zmap_view->features, zmap_view->context_map.styles);
  
  for (list_item = g_list_first(zmap_view->window_list); list_item; list_item = g_list_next(list_item))
    {
      ZMapViewWindow view_window = list_item->data ;
      zMapWindowFeatureRedraw(view_window->window, zmap_view->features, zmap_view->revcomped_features) ;
    }


//  if (feature->style)
//    {
//
//      ZMapFeatureBlock block = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_set, ZMAPFEATURE_STRUCT_BLOCK) ;      
//      ZMapFeatureBlock block_cp = (ZMapFeatureBlock)zMapFeatureAnyCopy((ZMapFeatureAny)block);
//      
//      ZMapFeatureAlignment align    = (ZMapFeatureAlignment)zMapFeatureGetParentGroup((ZMapFeatureAny)block, ZMAPFEATURE_STRUCT_ALIGN) ;
//      ZMapFeatureAlignment align_cp = (ZMapFeatureAlignment)zMapFeatureAnyCopy((ZMapFeatureAny)align);
//      
//      //ZMapFeatureContext context  = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)align, ZMAPFEATURE_STRUCT_CONTEXT) ;
//      ZMapFeatureContext context_cp = (ZMapFeatureContext)zMapFeatureAnyCopy((ZMapFeatureAny)context);
//
//      /* Now make a tree */
//      zMapFeatureContextAddAlignment(context_cp, align_cp, context->master_align == align);
//      zMapFeatureAlignmentAddBlock(align_cp, block_cp);
//      zMapFeatureBlockAddFeatureSet(block_cp, feature_set);
//
//      /* Now we have a context to merge. */
//      ZMapFeatureContext diff = NULL;
//      zMapFeatureContextMerge(&context, &context_cp, &diff,NULL);
//
//      /* Merge our context into the view's context and view the diff context */
//      //ZMapFeatureContext diff = zmapViewMergeInContext(zmap_view, context_cp);
//      if (diff)
//        zmapViewDrawDiffContext(zmap_view, &diff);
//
//    }
//  else
//    {
//      zMapLogWarning("Feature set '%s' has no style!",
//                     g_quark_to_string(feature_set->original_id));
//    }

  return TRUE;
}


/*!
 * \brief Remove the given feature from the scratch column
 */
gboolean zmapViewScratchRemoveFeature(ZMapView zmap_view, 
                                      ZMapFeatureSequenceMap sequence,
                                      ZMapFeature feature,
                                      ZMapFeatureSet feature_set,
                                      ZMapFeatureContext context)
{
  return zMapFeatureSetRemoveFeature(feature_set, feature);
}
