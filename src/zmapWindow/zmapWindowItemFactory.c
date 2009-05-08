 /*  File: zmapWindowItemFactory.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 *
 * Description: Factory-object based set of functions for drawing
 *              features.
 *
 * Exported functions: See zmapWindowItemFactory.h
 * HISTORY:
 * Last edited: May  8 15:35 2009 (rds)
 * Created: Mon Sep 25 09:09:52 2006 (rds)
 * CVS info:   $Id: zmapWindowItemFactory.c,v 1.59 2009-05-08 14:42:57 rds Exp $
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapStyle.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>
#include <zmapWindowItemFactory.h>
#include <zmapWindowItemTextFillColumn.h>

typedef struct _RunSetStruct *RunSet;

typedef FooCanvasItem * (*ZMapWindowFToIFactoryInternalMethod)(RunSet run_data, ZMapFeature feature,
                                                               double feature_offset,
                                                               double x1, double y1, double x2, double y2,
                                                               ZMapFeatureTypeStyle style);

typedef struct
{
  ZMapStyleMode type;

  ZMapWindowFToIFactoryInternalMethod method;
}ZMapWindowFToIFactoryMethodsStruct, *ZMapWindowFToIFactoryMethods;


typedef struct _ZMapWindowFToIFactoryStruct
{
  guint line_width;            /* window->config.feature_line_width */
  ZMapWindowFToIFactoryMethodsStruct *methods;
  ZMapWindowFToIFactoryMethodsStruct *basic_methods;
  ZMapWindowFToIFactoryMethodsStruct *text_methods;
  ZMapWindowFToIFactoryMethodsStruct *graph_methods;
  GHashTable                         *ftoi_hash;
  ZMapWindowLongItems                 long_items;
  ZMapWindowFToIFactoryProductionTeam user_funcs;
  gpointer                            user_data;
  double                              limits[4];
  double                              points[4];
  PangoFontDescription               *font_desc;
  double                              text_width;
  double                              text_height;
} ZMapWindowFToIFactoryStruct ;

typedef struct _RunSetStruct
{
  ZMapWindowFToIFactory factory;
  ZMapFeatureContext    context;
  ZMapFeatureAlignment  align;
  ZMapFeatureBlock      block;
  ZMapFeatureSet        set;
  ZMapFrame             frame;
  FooCanvasGroup       *container;
}RunSetStruct;


static void copyCheckMethodTable(const ZMapWindowFToIFactoryMethodsStruct  *table_in, 
                                 ZMapWindowFToIFactoryMethodsStruct       **table_out) ;

static void datalistRun(gpointer key, gpointer list_data, gpointer user_data);
inline 
static void callItemHandler(ZMapWindowFToIFactory                   factory,
                            FooCanvasItem            *new_item,
                            ZMapWindowItemFeatureType new_item_type,
                            ZMapFeature               full_feature,
                            ZMapWindowItemFeature     sub_feature,
                            double                    new_item_y1,
                            double                    new_item_y2);
inline
static void callTopItemHandler(ZMapWindowFToIFactory factory, 
                               ZMapFeatureContext context,
                               ZMapFeatureAlignment align,
                               ZMapFeatureBlock block,
                               ZMapFeatureSet set,
                               ZMapFeature feature,
                               ZMapWindowItemFeatureSetData set_data,
                               FooCanvasItem *top_item);

/* ZMapWindowFToIFactoryInternalMethod prototypes */
static FooCanvasItem *invalidFeature(RunSet run_data, ZMapFeature feature,
                                     double feature_offset, 
                                     double x1, double y1, double x2, double y2,
                                     ZMapFeatureTypeStyle style);
static FooCanvasItem *drawSimpleFeature(RunSet run_data, ZMapFeature feature,
                                        double feature_offset, 
					double x1, double y1, double x2, double y2,
                                        ZMapFeatureTypeStyle style);
static FooCanvasItem *drawGlyphFeature(RunSet run_data, ZMapFeature feature,
				       double feature_offset, 
				       double x1, double y1, double x2, double y2,
				       ZMapFeatureTypeStyle style);
static FooCanvasItem *drawAlignFeature(RunSet run_data, ZMapFeature feature,
                                       double feature_offset, 
                                       double x1, double y1, double x2, double y2,
                                       ZMapFeatureTypeStyle style);
static FooCanvasItem *drawTranscriptFeature(RunSet run_data, ZMapFeature feature,
                                            double feature_offset, 
                                            double x1, double y1, double x2, double y2,
                                            ZMapFeatureTypeStyle style);
static FooCanvasItem *drawSeqFeature(RunSet run_data, ZMapFeature feature,
                                     double feature_offset, 
                                     double x1, double y1, double x2, double y2,
                                     ZMapFeatureTypeStyle style);
static FooCanvasItem *drawPepFeature(RunSet run_data, ZMapFeature feature,
                                     double feature_offset, 
                                     double x1, double y1, double x2, double y2,
                                     ZMapFeatureTypeStyle style);
static FooCanvasItem *drawSimpleAsTextFeature(RunSet run_data, ZMapFeature feature,
                                              double feature_offset, 
                                              double x1, double y1, double x2, double y2,
                                              ZMapFeatureTypeStyle style);
static FooCanvasItem *drawSimpleGraphFeature(RunSet run_data, ZMapFeature feature,
					     double feature_offset, 
					     double x1, double y1, double x2, double y2,
					     ZMapFeatureTypeStyle style);
static FooCanvasItem *drawFullColumnTextFeature(RunSet run_data,  ZMapFeature feature,
                                                double feature_offset,
                                                double x1, double y1, double x2, double y2,
                                                ZMapFeatureTypeStyle style,
                                                int bases_per_char, char *column_text);

static void GapAlignBlockFromAdjacentBlocks(ZMapAlignBlock block_a, ZMapAlignBlock block_b, 
					    ZMapAlignBlockStruct *gap_span_out,
					    gboolean *t_indel_gt_1,
					    gboolean *q_indel_gt_1);

static void null_item_created(FooCanvasItem            *new_item,
                              ZMapWindowItemFeatureType new_item_type,
                              ZMapFeature               full_feature,
                              ZMapWindowItemFeature     sub_feature,
                              double                    new_item_y1,
                              double                    new_item_y2,
                              gpointer                  handler_data);
static gboolean null_top_item_created(FooCanvasItem *top_item,
                                      ZMapFeatureContext context,
                                      ZMapFeatureAlignment align,
                                      ZMapFeatureBlock block,
                                      ZMapFeatureSet set,
                                      ZMapFeature feature,
                                      gpointer handler_data);
static gboolean null_feature_size_request(ZMapFeature feature, 
                                          double *limits_array, 
                                          double *points_array_inout, 
                                          gpointer handler_data);


/*
 * Static function tables for drawing features in various ways.
 */

const static ZMapWindowFToIFactoryMethodsStruct factory_methods_G[] = {
  {ZMAPSTYLE_MODE_INVALID,      invalidFeature},
  {ZMAPSTYLE_MODE_BASIC,        drawSimpleFeature},
  {ZMAPSTYLE_MODE_ALIGNMENT,    drawAlignFeature},
  {ZMAPSTYLE_MODE_TRANSCRIPT,   drawTranscriptFeature},
  {ZMAPSTYLE_MODE_RAW_SEQUENCE, drawSeqFeature},
  {ZMAPSTYLE_MODE_PEP_SEQUENCE, drawPepFeature},
  {ZMAPSTYLE_MODE_TEXT,         drawSimpleAsTextFeature},
  {ZMAPSTYLE_MODE_GRAPH,        drawSimpleGraphFeature},
  {ZMAPSTYLE_MODE_GLYPH,        drawGlyphFeature},
  {-1,                          NULL}
};

const static ZMapWindowFToIFactoryMethodsStruct factory_text_methods_G[] = {
  {ZMAPSTYLE_MODE_INVALID,      invalidFeature},
  {ZMAPSTYLE_MODE_BASIC,        drawSimpleAsTextFeature},
  {ZMAPSTYLE_MODE_ALIGNMENT,    drawAlignFeature},
  {ZMAPSTYLE_MODE_TRANSCRIPT,   drawTranscriptFeature},
  {ZMAPSTYLE_MODE_RAW_SEQUENCE, drawSeqFeature},
  {ZMAPSTYLE_MODE_PEP_SEQUENCE, drawPepFeature},
  {ZMAPSTYLE_MODE_TEXT,         drawSimpleAsTextFeature},
  {ZMAPSTYLE_MODE_GRAPH,        drawSimpleGraphFeature},
  {ZMAPSTYLE_MODE_GLYPH,        drawGlyphFeature},
  {-1,                          NULL}
};

const static ZMapWindowFToIFactoryMethodsStruct factory_graph_methods_G[] = {
  {ZMAPSTYLE_MODE_INVALID,      invalidFeature},
  {ZMAPSTYLE_MODE_BASIC,        drawSimpleGraphFeature},
  {ZMAPSTYLE_MODE_ALIGNMENT,    drawSimpleGraphFeature},
  {ZMAPSTYLE_MODE_TRANSCRIPT,   drawSimpleGraphFeature},
  {ZMAPSTYLE_MODE_RAW_SEQUENCE, drawSimpleGraphFeature},
  {ZMAPSTYLE_MODE_PEP_SEQUENCE, drawSimpleGraphFeature},
  {ZMAPSTYLE_MODE_TEXT,         drawSimpleAsTextFeature},
  {ZMAPSTYLE_MODE_GRAPH,        drawSimpleGraphFeature},
  {ZMAPSTYLE_MODE_GLYPH,        drawGlyphFeature},
  {-1,                          NULL}
};

const static ZMapWindowFToIFactoryMethodsStruct factory_basic_methods_G[] = {
  {ZMAPSTYLE_MODE_INVALID,      invalidFeature},
  {ZMAPSTYLE_MODE_BASIC,        drawSimpleFeature},
  {ZMAPSTYLE_MODE_ALIGNMENT,    drawSimpleFeature},
  {ZMAPSTYLE_MODE_TRANSCRIPT,   drawSimpleFeature},
  {ZMAPSTYLE_MODE_RAW_SEQUENCE, drawSimpleFeature},
  {ZMAPSTYLE_MODE_PEP_SEQUENCE, drawSimpleFeature},
  {ZMAPSTYLE_MODE_TEXT,         drawSimpleAsTextFeature},
  {ZMAPSTYLE_MODE_GRAPH,        drawSimpleGraphFeature},
  {ZMAPSTYLE_MODE_GLYPH,        drawGlyphFeature},
  {-1,                          NULL}
};



ZMapWindowFToIFactory zmapWindowFToIFactoryOpen(GHashTable *feature_to_item_hash, 
                                                ZMapWindowLongItems long_items)
{
  ZMapWindowFToIFactory factory = NULL;
  ZMapWindowFToIFactoryProductionTeam user_funcs = NULL;

  if ((factory = g_new0(ZMapWindowFToIFactoryStruct, 1)) && 
     (user_funcs = g_new0(ZMapWindowFToIFactoryProductionTeamStruct, 1)))
    {
      factory->ftoi_hash  = feature_to_item_hash;
      factory->long_items = long_items;

      copyCheckMethodTable(&(factory_methods_G[0]), 
                           &(factory->methods));
      copyCheckMethodTable(&(factory_basic_methods_G[0]), 
                           &(factory->basic_methods));
      copyCheckMethodTable(&(factory_text_methods_G[0]), 
                           &(factory->text_methods));
      copyCheckMethodTable(&(factory_graph_methods_G[0]), 
                           &(factory->graph_methods));

      user_funcs->item_created = null_item_created;
      user_funcs->top_item_created = null_top_item_created;
      user_funcs->feature_size_request = null_feature_size_request;
      factory->user_funcs = user_funcs;
    }

  return factory;
}

void zmapWindowFToIFactorySetup(ZMapWindowFToIFactory factory, 
                                guint line_width, /* a config struct ? */
                                ZMapWindowFToIFactoryProductionTeam signal_handlers, 
                                gpointer handler_data)
{
  zMapAssert(signal_handlers);

  if(signal_handlers->item_created)
    factory->user_funcs->item_created = signal_handlers->item_created;

  if(signal_handlers->top_item_created)
    factory->user_funcs->top_item_created = signal_handlers->top_item_created;

  if(signal_handlers->feature_size_request)
    factory->user_funcs->feature_size_request = signal_handlers->feature_size_request;

  factory->user_data  = handler_data;
  factory->line_width = line_width;

  return ;
}

void zmapWindowFToIFactoryRunSet(ZMapWindowFToIFactory factory, 
                                 ZMapFeatureSet set, 
                                 FooCanvasGroup *container,
                                 ZMapFrame frame)
{
  RunSetStruct run_data = {NULL};

  run_data.factory = factory;
  /*
    run_data.container = canZMapWindowFToIFactoryDoThis_ShouldItBeAParameter();
  */
  run_data.container   = container;

  run_data.set     = set;
  run_data.context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)set, ZMAPFEATURE_STRUCT_CONTEXT);
  run_data.align   = (ZMapFeatureAlignment)zMapFeatureGetParentGroup((ZMapFeatureAny)set, ZMAPFEATURE_STRUCT_ALIGN);
  run_data.block   = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)set, ZMAPFEATURE_STRUCT_BLOCK);

  run_data.frame   = frame;

  g_hash_table_foreach(set->features, datalistRun, &run_data);

  return ;
}



FooCanvasItem *zmapWindowFToIFactoryRunSingle(ZMapWindowFToIFactory factory, 
                                              FooCanvasGroup       *parent_container,
                                              ZMapFeatureContext    context, 
                                              ZMapFeatureAlignment  align, 
                                              ZMapFeatureBlock      block,
                                              ZMapFeatureSet        set,
                                              ZMapFeature           feature)
{
  RunSetStruct run_data = {NULL};
  FooCanvasItem *item = NULL, *return_item = NULL;
  FooCanvasGroup *features_container = NULL;
  ZMapWindowItemFeatureSetData set_data = NULL;
  ZMapFeatureTypeStyle style = NULL;
  ZMapStyleMode style_mode = ZMAPSTYLE_MODE_INVALID ;
  gboolean no_points_in_block = TRUE;
  /* check here before they get called.  I'd prefer to only do this
   * once per factory rather than once per Run! */
  ZMapWindowStats parent_stats ;

  zMapAssert(factory->user_funcs && 
             factory->user_funcs->item_created &&
             factory->user_funcs->top_item_created);
#ifdef RDS_REMOVED
  parent_stats = g_object_get_data(G_OBJECT(parent_container), ITEM_FEATURE_STATS) ;
#endif
  set_data = g_object_get_data(G_OBJECT(parent_container), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;
  
  /* Get the styles table from the column and look for the features style.... */
  if (!(style = zmapWindowItemFeatureSetStyleFromID(set_data, feature->style_id)))
    {
      zMapAssertNotReached();
    }

  style_mode = zMapStyleGetMode(style) ;

  if (style_mode >= 0 && style_mode <= FACTORY_METHOD_COUNT)
    {
      double *limits = factory->limits;
      double *points = factory->points;
      ZMapWindowFToIFactoryMethods method = NULL;
      ZMapWindowFToIFactoryMethodsStruct *method_table = NULL;
      ZMapWindowStats stats ;
      
      stats = g_object_get_data(G_OBJECT(parent_container), ITEM_FEATURE_STATS) ;
#ifdef RDS_REMOVED
      zMapAssert(stats) ;
      
      zmapWindowStatsAddChild(stats, (ZMapFeatureAny)feature) ;
#endif
      features_container = zmapWindowContainerGetFeatures(parent_container) ;
  
      if (zMapStyleIsStrandSpecific(style)
          && (feature->strand == ZMAPSTRAND_REVERSE && !zMapStyleIsShowReverseStrand(style)))
        goto undrawn;

      /* Note: for object to _span_ "width" units, we start at zero and end at "width". */
      limits[0] = 0.0;
      limits[1] = block->block_to_sequence.q1;
      limits[2] = zMapStyleGetWidth(style);
      limits[3] = block->block_to_sequence.q2;

      /* Set these to this for normal main window use. */
      points[0] = 0.0;
      points[1] = feature->x1;
      points[2] = zMapStyleGetWidth(style);
      points[3] = feature->x2;

      /* inline static void normalSizeReq(ZMapFeature f, ... ){ return ; } */
      /* inlined this should be nothing most of the time, question is can it be inlined?? */
      if((no_points_in_block = (factory->user_funcs->feature_size_request)(feature, &limits[0], &points[0], factory->user_data)) == TRUE)
        goto undrawn;

      style_mode = zMapStyleGetMode(style) ;


      /* NOTE TO ROY: I have probably not got the best logic below...I am sure that we
       * have not handled/thought about all the combinations of style modes and feature
       * types that might occur so will need to revisit this.... */
      switch(style_mode)
        {
        case ZMAPSTYLE_MODE_BASIC:
        case ZMAPSTYLE_MODE_GLYPH:
	  {
	    ZMapWindowStatsBasic stats ;
#ifdef RDS_REMOVED
	    stats = zmapWindowStatsAddBasic(parent_stats, feature) ;
	    stats->features++ ;
	    stats->items++ ;
#endif
	    method_table = factory->basic_methods;

	    if(feature->flags.has_score)
	      zmapWindowGetPosFromScore(style, feature->score, &(points[0]), &(points[2])) ;

	    method = &(method_table[style_mode]);

	    break;
	  }

        case ZMAPSTYLE_MODE_ALIGNMENT:
	  {
	    ZMapWindowStatsAlign stats ;
#ifdef RDS_REMOVED
	    stats = zmapWindowStatsAddAlign(parent_stats, feature) ;
	    stats->total_matches++ ;
#endif
	    method_table = factory->methods;

	    if (feature->flags.has_score)
	      zmapWindowGetPosFromScore(style, feature->score, &(points[0]), &(points[2])) ;


	    if (!zMapStyleIsAlignGaps(style) || !(feature->feature.homol.align))
	      {
#ifdef RDS_REMOVED
		stats->ungapped_matches++;
		stats->ungapped_boxes++;
		stats->total_boxes++;
#endif
		method = &(method_table[ZMAPSTYLE_MODE_BASIC]);              
	      }
	    else
	      {
		if (feature->feature.homol.flags.perfect)
		  {
#ifdef RDS_REMOVED
		    stats->gapped_matches++;
		    stats->total_boxes  += feature->feature.homol.align->len ;
		    stats->gapped_boxes += feature->feature.homol.align->len ;
#endif
		  }
		else
		  {
#ifdef RDS_REMOVED
		    stats->not_perfect_gapped_matches++;
		    stats->total_boxes++;
		    stats->ungapped_boxes++;
		    stats->imperfect_boxes += feature->feature.homol.align->len;
#endif
		  }

		method = &(method_table[style_mode]);
	      }
	    
	    break;
	  }

        case ZMAPSTYLE_MODE_TEXT:
	  {
	    ZMapWindowStatsBasic stats ;
#ifdef RDS_REMOVED
	    stats = zmapWindowStatsAddBasic(parent_stats, feature) ;
	    stats->features++ ;
	    stats->items++ ;
#endif
	    method_table = factory->text_methods;

	    method = &(method_table[style_mode]);

	    break;
	  }

        case ZMAPSTYLE_MODE_GRAPH:
	  {
	    ZMapWindowStatsBasic stats ;
#ifdef RDS_REMOVED
	    stats = zmapWindowStatsAddBasic(parent_stats, feature) ;
	    stats->features++ ;
	    stats->items++ ;
#endif
	    method_table = factory->graph_methods;

	    method = &(method_table[style_mode]);

	    break;
	  }

        case ZMAPSTYLE_MODE_TRANSCRIPT:
	  {
	    ZMapWindowStatsTranscript stats ;
#ifdef RDS_REMOVED
	    stats = zmapWindowStatsAddTranscript(parent_stats, feature) ;
	    stats->transcripts++ ;

	    if (feature->feature.transcript.exons)
	      {
		stats->exons += feature->feature.transcript.exons->len ;
		stats->exon_boxes += feature->feature.transcript.exons->len ;
	      }

	    if (feature->feature.transcript.introns)
	      {
		/* There are two canvas items for every intron. */
		stats->introns += feature->feature.transcript.introns->len ;
		stats->intron_boxes += (feature->feature.transcript.introns->len * 2) ;
	      }

	    if (feature->feature.transcript.flags.cds)
	      {
		stats->cds++ ;
		stats->cds_boxes++ ;
	      }
#endif
	    method_table = factory->methods;
	    method = &(method_table[style_mode]);

	    break ;
	  }

	case ZMAPSTYLE_MODE_RAW_SEQUENCE:
	case ZMAPSTYLE_MODE_PEP_SEQUENCE:
	  {
	    method_table = factory->methods;

	    method = &(method_table[style_mode]);

	    break;
	  }

	default:
	  zMapAssertNotReached() ;
	  break;
	}

      
      run_data.factory   = factory;
      run_data.container = features_container;
      run_data.context   = context;
      run_data.align     = align;
      run_data.block     = block;
      run_data.set       = set;

      item   = ((method)->method)(&run_data, feature, limits[1],
				  points[0], points[1],
				  points[2], points[3],
				  style);

      if (item)
        {
          gboolean status = FALSE;
          
          g_object_set_data(G_OBJECT(item), ITEM_FEATURE_ITEM_STYLE, style) ;
          
          if(factory->ftoi_hash)
            status = zmapWindowFToIAddFeature(factory->ftoi_hash,
                                              align->unique_id, block->unique_id, 
                                              set->unique_id, 
                                              set_data->strand, 
                                              set_data->frame,
                                              feature->unique_id, item) ;

          status = (factory->user_funcs->top_item_created)(item, context, align, block, set, feature, factory->user_data);
          
          zMapAssert(status);

          return_item = item;
        }
    }
  else
    {
      zMapLogFatal("Feature %s is of unknown type: %d\n", 
		   (char *)g_quark_to_string(feature->original_id), feature->type) ;
    }

 undrawn:

  return return_item;
}


void zmapWindowFToIFactoryClose(ZMapWindowFToIFactory factory)
{
  factory->ftoi_hash  = NULL;
  factory->long_items = NULL;
  g_free(factory->user_funcs);
  factory->user_data  = NULL;
  /* font desc??? */
  /* factory->font_desc = NULL; */

  /* free the factory */
  g_free(factory);

  return ;
}



/*
 *                          INTERNAL
 */

static void copyCheckMethodTable(const ZMapWindowFToIFactoryMethodsStruct  *table_in, 
                                 ZMapWindowFToIFactoryMethodsStruct       **table_out)
{
  ZMapWindowFToIFactoryMethods methods = NULL;
  int i = 0;

  if(table_in && table_out && *table_out == NULL)
    {  
      /* copy factory_methods into factory->methods */
      methods = (ZMapWindowFToIFactoryMethods)table_in;
      while(methods && methods->method){ methods++; i++; } /* Get the size first! */
      
      /* Allocate */
      *table_out = g_new0(ZMapWindowFToIFactoryMethodsStruct, ++i);
      /* copy ... */
      memcpy(table_out[0], table_in, 
             sizeof(ZMapWindowFToIFactoryMethodsStruct) * i);
      methods = *table_out;
      i = 0;
      /* check that all is going to be ok with methods... */
      while(methods && methods->method)
        {
          if(!(methods->type == i))
            {
              zMapLogFatal("Bad Setup: expected %d, found %d\n", i, methods->type);
            }
          methods++;
          i++;
        }
    }
  else
    zMapAssertNotReached();

  return ;
}



static void datalistRun(gpointer key, gpointer list_data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)list_data;
  RunSet run_data = (RunSet)user_data;

  /* filter on frame! */
  if((run_data->frame != ZMAPFRAME_NONE) &&
     run_data->frame  != zmapWindowFeatureFrame(feature))
    return ;

  zmapWindowFToIFactoryRunSingle(run_data->factory, 
                                 run_data->container,
                                 run_data->context,
                                 run_data->align,
                                 run_data->block,
                                 run_data->set,
                                 feature);
  
  return ;
}

inline
static void callItemHandler(ZMapWindowFToIFactory     factory,
                            FooCanvasItem            *new_item,
                            ZMapWindowItemFeatureType new_item_type,
                            ZMapFeature               full_feature,
                            ZMapWindowItemFeature     sub_feature,
                            double                    new_item_y1,
                            double                    new_item_y2)
{
  /* what was the non signal part of attachDataToItem.  _ALL_ our canvas feature MUST have these */

  g_object_set_data(G_OBJECT(new_item), ITEM_FEATURE_TYPE, GINT_TO_POINTER(new_item_type)) ;
  g_object_set_data(G_OBJECT(new_item), ITEM_FEATURE_DATA, full_feature) ;


  if(sub_feature != NULL)
    g_object_set_data(G_OBJECT(new_item), ITEM_SUBFEATURE_DATA, sub_feature);
  else
    zMapAssert(new_item_type != ITEM_FEATURE_CHILD &&
               new_item_type != ITEM_FEATURE_BOUNDING_BOX);

  if(factory->long_items)
    zmapWindowLongItemCheck(factory->long_items, new_item, new_item_y1, new_item_y2);

  (factory->user_funcs->item_created)(new_item, new_item_type, full_feature, sub_feature, 
                                      new_item_y1, new_item_y2, factory->user_data);

  return ;
}

inline
static void callTopItemHandler(ZMapWindowFToIFactory factory, 
                               ZMapFeatureContext context,
                               ZMapFeatureAlignment align,
                               ZMapFeatureBlock block,
                               ZMapFeatureSet set,
                               ZMapFeature feature,
                               ZMapWindowItemFeatureSetData set_data,
                               FooCanvasItem *top_item)
{

  return ;
}

static FooCanvasItem *createParentGroup(FooCanvasGroup *parent,
					ZMapFeature feature,
					double y_origin)
{
  FooCanvasItem *group = NULL;
  /* this group is the parent of the entire transcript. */
  group = foo_canvas_item_new(FOO_CANVAS_GROUP(parent),
                              foo_canvas_group_get_type(),
                              "x", 0.0,
                              "y", y_origin,
                              NULL) ;
  group = FOO_CANVAS_ITEM(group) ;

  /* we probably need to set this I think, it means we will go to the group when we
   * navigate or whatever.... */
  g_object_set_data(G_OBJECT(group), ITEM_FEATURE_TYPE,
                    GINT_TO_POINTER(ITEM_FEATURE_PARENT)) ;
  g_object_set_data(G_OBJECT(group), ITEM_FEATURE_DATA, feature) ;

  return group ;
}

static double getWidthFromScore(ZMapFeatureTypeStyle style, double score)
{
  double tmp, width = 0.0 ;
  double fac, max_score, min_score ;

  width = zMapStyleGetWidth(style) ;
  min_score = zMapStyleGetMinScore(style) ;
  max_score = zMapStyleGetMaxScore(style) ;

  fac = width / (max_score - min_score) ;

  if (score <= min_score)
    tmp = 0 ;
  else if (score >= max_score) 
    tmp = width ;
  else 
    tmp = fac * (score - min_score) ;

  width = tmp ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (seg->data.f <= bc->meth->minScore) 
	x = 0 ;
      else if (seg->data.f >= bc->meth->maxScore) 
	x = bc->width ;
      else 
	x = fac * (seg->data.f - bc->meth->minScore) ;

      box = graphBoxStart() ;

      if (x > origin + 0.5 || x < origin - 0.5) 
	graphLine (bc->offset+origin, y, bc->offset+x, y) ;
      else if (x > origin)
	graphLine (bc->offset+origin-0.5, y, bc->offset+x, y) ;
      else
	graphLine (bc->offset+origin+0.5, y, bc->offset+x, y) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return width ;
}

static FooCanvasItem *drawSimpleFeature(RunSet run_data, ZMapFeature feature,
                                        double feature_offset,
					double x1, double y1, double x2, double y2,
                                        ZMapFeatureTypeStyle style)
{
  ZMapWindowFToIFactory factory = run_data->factory;
  FooCanvasGroup        *parent = run_data->container;
  FooCanvasItem *feature_item ;
  GdkColor *outline = NULL, *foreground = NULL, *background = NULL ;
  guint line_width;
  gboolean status ;

  line_width = factory->line_width;

  zmapWindowSeq2CanOffset(&y1, &y2, feature_offset) ;	    /* Make sure we cover the whole last base. */

  if(feature->strand == ZMAPSTRAND_REVERSE &&
     zMapStyleColourByStrand(style))
    status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_STRAND, ZMAPSTYLE_COLOURTYPE_NORMAL,
				 &background, &foreground, &outline) ;
  else
    status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
				 &background, &foreground, &outline) ;
  zMapAssert(status) ;

  feature_item = zMapDrawBox(parent,
			     x1, y1, x2, y2,
			     outline, background, line_width) ;

  callItemHandler(factory, feature_item,
                  ITEM_FEATURE_SIMPLE,
                  feature, NULL, y1, y2);


  return feature_item ;
}


static FooCanvasItem *drawGlyphFeature(RunSet run_data, ZMapFeature feature,
				       double feature_offset,
				       double x1, double y1, double x2, double y2,
				       ZMapFeatureTypeStyle style)
{
  ZMapWindowFToIFactory factory = run_data->factory;
  FooCanvasGroup        *parent = run_data->container;
  FooCanvasItem *feature_item ;
  guint line_width;

  line_width = factory->line_width;

  zmapWindowSeq2CanOffset(&y1, &y2, feature_offset) ;	    /* Make sure we cover the whole last base. */


  /* There will be other alternatives to splice once we add them to the the canvas. */
  if (feature->flags.has_boundary)
    {
      GdkColor *splice_background ;
      double width, origin, start ;
      double style_width, max_score, min_score ;
      ZMapDrawGlyphType glyph_type ;
      ZMapFrame frame ;
      gboolean status ;
      ZMapStyleColourTarget target ;

      frame = zmapWindowFeatureFrame(feature) ;

      switch (frame)
	{
	case ZMAPFRAME_0:
	  target = ZMAPSTYLE_COLOURTARGET_FRAME0 ;
	  break ;
	case ZMAPFRAME_1:
	  target = ZMAPSTYLE_COLOURTARGET_FRAME1 ;
	  break ;
	case ZMAPFRAME_2:
	  target = ZMAPSTYLE_COLOURTARGET_FRAME2 ;
	  break ;
	default:
	  zMapAssertNotReached() ;
	}

      status = zMapStyleGetColours(style, target, ZMAPSTYLE_COLOURTYPE_NORMAL, &splice_background, NULL, NULL) ;
      zMapAssert(status) ;

      if (feature->boundary_type == ZMAPBOUNDARY_5_SPLICE)
	{
	  glyph_type = ZMAPDRAW_GLYPH_DOWN_BRACKET ;
	}
      else
	{
	  glyph_type = ZMAPDRAW_GLYPH_UP_BRACKET ;
	}

      style_width = zMapStyleGetWidth(style) ;
      min_score = zMapStyleGetMinScore(style) ;
      max_score = zMapStyleGetMaxScore(style) ;

      if (min_score < 0 && 0 < max_score)
	origin = 0 ;
      else
	origin = style_width * (min_score / (max_score - min_score)) ;


      /* Adjust width to score....NOTE that to do acedb like stuff we really need to set an origin
	 and pass it in to this routine....*/
      width = getWidthFromScore(style, feature->score) ;

      x1 = 0.0;                 /* HACK */
      if (width > origin + 0.5 || width < origin - 0.5)
	{
	  start = x1 + origin ;
	  width = width - origin ;
	  /* graphLine (bc->offset+origin, y, bc->offset+x, y) ; */
	}
      else if (width > origin)
	{
	  /*graphLine (bc->offset+origin-0.5, y, bc->offset+x, y) ; */
	  start = x1 + origin - 0.5 ;
	  width = width - origin - 0.5 ;
	}
      else
	{
	  /* graphLine (bc->offset+origin+0.5, y, bc->offset+x, y) ; */
	  start = x1 + origin + 0.5 ;
	  width = width - origin + 0.5 ;
	}

      feature_item = zMapDrawGlyph(parent, start, (y2 + y1) * 0.5,
				   glyph_type,
				   splice_background, width, 2) ;
    }

  callItemHandler(factory, feature_item,
                  ITEM_FEATURE_SIMPLE,
                  feature, NULL, y1, y2);


  return feature_item ;
}




/* Reduce the number of variables in drawAlignFeature slightly */
typedef struct 
{
  gboolean colour_init;
  GdkColor perfect;
  GdkColor colinear;
  GdkColor noncolinear;
  char *perfect_colour;
  char *colinear_colour;
  char *noncolinear_colour;

} AlignColinearityColoursStruct;

/* Draws a series of boxes connected by centrally placed straight lines which represent the match
 * blocks given by the align array in an alignment feature.
 * 
 * NOTE: returns NULL if the align array is null, i.e. you must call drawSimpleFeature to draw
 * an alignment which has no gaps or just to draw it without gaps.
 *  */

/* Warning about actual data:
 * 
 * feature->homol.aligns[
 * {
 *   t1 = 575457,   q1 = 1043,
 *   t2 = 575457,   q2 = 1043,
 *   t_strand = +,  q_strand = +,
 * },
 * {
 *   t1 = 575461,   q1 = 1040,
 *   t2 = 575463,   q2 = 1042,
 *   t_strand = +,  q_strand = +,
 * },
 * {
 *   t1 = 575467,   q1 = 1033,
 *   t2 = 575470,   q2 = 1039,
 *   t_strand = +,  q_strand = +,
 * },
 * ]
 */

/* made me write GapAlignBlockFromAdjacentBlocks */
static FooCanvasItem *drawAlignFeature(RunSet run_data, ZMapFeature feature,
                                       double feature_offset,
                                       double x1, double y1, double x2, double y2,
                                       ZMapFeatureTypeStyle style)
{
  FooCanvasItem *feature_item = NULL ;
  ZMapWindowFToIFactory factory = run_data->factory;
  ZMapFeatureBlock        block = run_data->block;
  GdkColor *outline = NULL, *foreground = NULL, *background = NULL ;
  guint match_threshold = 0, line_width = 0;
  gboolean status ;
  static AlignColinearityColoursStruct c_colours = {
    FALSE, {0}, {0}, {0}, 
    ZMAP_WINDOW_MATCH_PERFECT,
    ZMAP_WINDOW_MATCH_COLINEAR,
    ZMAP_WINDOW_MATCH_NOTCOLINEAR
  };

  /* Get the colours, it's an error at this stage if none set we don't draw. */
  /* We could speed performance by caching these colours and only changing them when the style
   * quark changes. */
  status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
			       &background, &foreground, &outline) ;
  zMapAssert(status) ;

  /* If we are here, style should be ok for display. */

  /* Fill in the static colinearity colours struct... */
  if (!c_colours.colour_init)
    {
      gboolean colinear_colours_from_style = FALSE;

      if(colinear_colours_from_style)
	{
	  /* These will _never_ parse. Write, if this is required... */
	  c_colours.perfect_colour     = "ZMapStyle->perfect_colour";
	  c_colours.colinear_colour    = "ZMapStyle->perfect_colour";
	  c_colours.noncolinear_colour = "ZMapStyle->perfect_colour";
	}

      gdk_color_parse(c_colours.perfect_colour, &c_colours.perfect) ;
      gdk_color_parse(c_colours.colinear_colour, &c_colours.colinear) ;
      gdk_color_parse(c_colours.noncolinear_colour, &c_colours.noncolinear) ;

      c_colours.colour_init = TRUE ;
    }

  /* Are we drawing gapped alignments? get the match threshold */
  zMapStyleGetGappedAligns(style, &match_threshold) ;

  /* line width from the factory? Should this be from the style? */
  line_width = factory->line_width;

  if (feature->feature.homol.align)
    {
      FooCanvasGroup *feature_group  = NULL;
      ZMapAlignBlock first_match_block = NULL, prev_match_block;
      double limits[]       = {0.0, 0.0, 0.0, 0.0};
      double points_inout[] = {0.0, 0.0, 0.0, 0.0};
      double feature_start, feature_end;
      double first_item_top    = 0.0;
      double first_item_bottom = 0.0;
      double prev_item_bottom;
      double line_x_axis = ((x2 - x1) / 2) + x1;
      double offset ;
      guint i = 0;		/* Get First Block */

      limits[1] = block->block_to_sequence.q1;
      limits[3] = block->block_to_sequence.q2;

      feature_start = feature->x1;
      feature_end   = feature->x2; /* I want this function to find these values */
      zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;
      feature_item  = createParentGroup(run_data->container, feature, feature_start);
      feature_group = FOO_CANVAS_GROUP(feature_item);

      /* Calculate total offset for for subparts of the feature. */
      offset = feature_start + feature_offset ;

      /* Algorithm here is:
       * - Get First Block (first)
       * Loop
       *   - Get Next Block (this)
       *   - Calculate Gap
       *   - Draw Gap _if_ there is one
       *   - Draw Block (this)
       * - Draw First Block (first)
       *
       * Sadly, it only really avoids a if(prev_match_block) in the loop and
       * doesn't cure any issue with the the gap lines overlapping the
       * match blocks (this requires 2 loops or a change to the way we draw all features). 
       */

      first_match_block = prev_match_block =
	&(g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i));

      points_inout[1] = first_match_block->t1;
      points_inout[3] = first_match_block->t2;
      
      /* Do a size request to check we need not be clipped */
      (factory->user_funcs->feature_size_request)(feature, &limits[0], 
						  &points_inout[0], 
						  factory->user_data);
      first_item_top    = points_inout[1];
      first_item_bottom = points_inout[3];

      zmapWindowSeq2CanOffset(&first_item_top, &first_item_bottom, offset) ;

      prev_item_bottom  = first_item_bottom;


      /* Do any more we have... */
      for (i = 1; prev_match_block && i < feature->feature.homol.align->len ; i++)  
        {
          ZMapWindowItemFeature box_data, gap_data, align_data ;
	  ZMapAlignBlockStruct gap_block, *match_block;
          FooCanvasItem *gap_line, *gap_line_box, *align_box;
          double item_top, item_bottom;
	  int q_indel, t_indel;
	  gboolean expect_problems = TRUE;
	  gboolean t_indel_set, q_indel_set;
          
          match_block = &(g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i));
	  points_inout[1] = match_block->t1;
          points_inout[3] = match_block->t2;

	  /* Do a size request to check we need not be clipped */
          if((factory->user_funcs->feature_size_request)(feature, &limits[0], &points_inout[0], factory->user_data))
	    continue;

          item_top    = points_inout[1];
          item_bottom = points_inout[3];

          zmapWindowSeq2CanOffset(&item_top, &item_bottom, offset) ;

	  /* Check the strandedness of prev and current
	   * align_spans. This should be removed when Ed finishes
	   * the correct dumping in the sgifaceserver. */
	  if(expect_problems)
	    {
	      
	      /* Problems expected are due to some services
	       * providing data to us which relies on the order of
	       * the coordinates to determine strand.  This is all
	       * well and good for everything _except_ single
	       * bases. We probably don't do enough to trap this 
	       * elsewhere, by the time we are here, we should just
	       * be drawing... */
	    
	      if(match_block->t_strand != prev_match_block->t_strand &&
		 match_block->t1 == match_block->t2)
		match_block->t_strand = prev_match_block->t_strand;

	      if(match_block->q_strand != prev_match_block->q_strand &&
		 match_block->q1 == match_block->q2)
		match_block->q_strand = prev_match_block->q_strand;

	      /* Sadly we need to check _again_ as the prev block
	       * could be the one with the single base match.
	       * Only a problem for the _first_ one I think...sigh... */
	      /* Anyway we set the prev one's strand to match this 
	       * one, rather than vice versa as above. */
	      if(match_block->t_strand != prev_match_block->t_strand &&
		 prev_match_block->t1 == prev_match_block->t2)
		prev_match_block->t_strand = match_block->t_strand;
	      
	      if(match_block->q_strand != prev_match_block->q_strand &&
		 prev_match_block->q1 == prev_match_block->q2)
		prev_match_block->q_strand = match_block->q_strand;
	    }
	      
	  

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* THIS NEEDS MORE THOUGHT......WE CAN'T JUST ASSERT HERE, WE EITHER NEED
	   * TO CHUCK OUT THE DATA AT AN EARLIER STAGE (E.G. GFF PARSER) OR HANDLE
	   * IT AT THIS STAGE...BOMBING OUT IS NO GOOD..... */


	  /* These may or may not be good to assert... */
	  /* I don't see any reason why someone shouldn't supply
	   * data that will break these, but I'm assuming the hit
	   * would actually be a separate HSP at that point.
	   * These are _after_ the above "expect_problems" code!
	   */
	  zMapAssert(match_block->t_strand == prev_match_block->t_strand);
	  zMapAssert(match_block->q_strand == prev_match_block->q_strand);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  
	  /* Get the Gap AlignBlock from the adjacent Match AlignBlocks. */
	  GapAlignBlockFromAdjacentBlocks(prev_match_block, match_block, &gap_block, &t_indel_set, &q_indel_set);
	  
	  /* Null this... */
	  prev_match_block = NULL;

	  t_indel = gap_block.t2 - gap_block.t1;
	  q_indel = gap_block.q2 - gap_block.q1;
	    
	  if(t_indel_set)
	    {
	      GdkColor *line_colour = &c_colours.perfect ;
	      
	      box_data          = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	      box_data->subpart = ZMAPFEATURE_SUBPART_GAP ;
	      box_data->start   = gap_block.t1;
	      box_data->end     = gap_block.t2;
	      box_data->query_start = 0;
	      box_data->query_end   = 0;

	      if(q_indel_set)
		{
		  box_data->query_start = gap_block.q1;
		  box_data->query_end   = gap_block.q2;
		}

	      if(q_indel - 1 > match_threshold)
		line_colour = &c_colours.colinear;

	      gap_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	      memcpy(gap_data, box_data, sizeof(ZMapWindowItemFeatureStruct));

	      gap_line_box = zMapDrawBox(feature_group, 
					 x1, prev_item_bottom,
					 x2, item_top,
					 NULL, NULL, line_width);

	      callItemHandler(factory, gap_line_box, 
			      ITEM_FEATURE_BOUNDING_BOX, 
			      feature, box_data, 
			      prev_item_bottom, item_top);

	      gap_line = zMapDrawLine(feature_group,
				      line_x_axis, prev_item_bottom, 
				      line_x_axis, item_top,
				      line_colour, line_width);

	      callItemHandler(factory, gap_line, 
			      ITEM_FEATURE_CHILD, 
			      feature, gap_data, 
			      prev_item_bottom, item_top);

	      gap_data->twin_item = gap_line_box;
	      box_data->twin_item = gap_line;
	    }


          align_data              = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	  align_data->subpart     = ZMAPFEATURE_SUBPART_MATCH ;
          align_data->start       = match_block->t1 ;
          align_data->end         = match_block->t2 ;
          align_data->query_start = match_block->q1 ;
          align_data->query_end   = match_block->q2 ;

	  align_box = zMapDrawBox(feature_group, x1, item_top, x2, item_bottom, 
				  outline, background, line_width);

	  callItemHandler(factory, align_box, 
			  ITEM_FEATURE_CHILD, 
			  feature, align_data, 
			  item_top, item_bottom);

	  /* Save this block for next iteration... Incl. the
	   * Seq2CanOffset of the Bottom of this drawn align box */
	  prev_match_block = match_block;
	  prev_item_bottom = item_bottom;
        }

      if(first_match_block)
	{
	  FooCanvasItem *align_box;
	  ZMapWindowItemFeature align_data;

	  align_data              = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	  align_data->subpart     = ZMAPFEATURE_SUBPART_MATCH ;
	  align_data->start       = first_match_block->t1 ;
	  align_data->end         = first_match_block->t2 ;
	  align_data->query_start = first_match_block->q1 ;
	  align_data->query_end   = first_match_block->q2 ;
	  
	  align_box = zMapDrawBox(feature_group, 
				  x1, first_item_top, 
				  x2, first_item_bottom, 
				  outline, background, 
				  line_width);
	  
	  callItemHandler(factory, align_box, 
			  ITEM_FEATURE_CHILD, 
			  feature, align_data, 
			  first_item_top, first_item_bottom);
	}
    }
    
  return feature_item ;
}

static void drawFeatureExon(ZMapWindowFToIFactory factory, 
                            FooCanvasItem *parent,
                            ZMapFeature feature, 
                            ZMapSpan exon_span,
                            ZMapFeatureTypeStyle style,
                            int left, int right,
                            int top, int bottom, int offset,
                            int cds_start, int cds_end,
                            int line_width,
                            GdkColor *background,
                            GdkColor *foreground,
                            GdkColor *outline,
                            gboolean has_cds,
                            GdkColor *cds_fill,
                            GdkColor *cds_draw,
                            GdkColor *cds_border)
{
  ZMapWindowItemFeature exon_data_ptr, tmp_data_ptr; 
  FooCanvasItem *exon_box, *utr_box;
  GdkColor *exon_fill, *exon_outline;
  int non_start, non_end ;
  
  /* create a ZMapWindowItemFeature for the exon, this will be attached later */
  tmp_data_ptr = exon_data_ptr = g_new0(ZMapWindowItemFeatureStruct, 1) ;
  /* tmp_data_ptr == exon_data_ptr for original box in case 
   * cds_start > exon_start && cds_end < exon_end  */
  exon_data_ptr->start   = exon_span->x1;
  exon_data_ptr->end     = exon_span->x2;
  exon_data_ptr->subpart = ZMAPFEATURE_SUBPART_EXON ;
  
  /* If any of the cds is in this exon we colour the whole exon as a cds and then
   * overlay non-cds parts. For the cds part we use the foreground colour, else it's the background. 
   * If background is null though outline will need to be set to what background would be and 
   * background should then = NULL */
  if (has_cds)
    {
      if ((cds_start > bottom) || (cds_end < top))
        {
          has_cds = FALSE;      /* This exon is not a exon of the CDS */

	  exon_fill = background ;
	  exon_outline = outline ;
        }
      else
        {
          /* At least some part of the exon is CDS... */
          exon_data_ptr->subpart = ZMAPFEATURE_SUBPART_EXON_CDS ;
          
	  exon_fill = cds_fill ;
	  exon_outline = cds_border ;
        }
    }
  else
    {
      exon_fill = background ;
      exon_outline = outline ;
    }

  exon_box = zMapDrawSSPolygon(parent, 
                               (zMapStyleIsDirectionalEnd(style) ? ZMAP_POLYGON_POINTING : ZMAP_POLYGON_SQUARE), 
                               left, right,
                               top, bottom,
                               exon_outline, 
                               exon_fill,
                               line_width,
                               feature->strand);
  callItemHandler(factory, exon_box, ITEM_FEATURE_CHILD, feature, exon_data_ptr, top, bottom);
  
  if (has_cds)
    {
      utr_box = NULL;

      /* Watch out for the conditions on the start/end here... */
      if ((cds_start > top) && (cds_start <= bottom))
        {
          utr_box = zMapDrawAnnotatePolygon(exon_box,
                                            (feature->strand == ZMAPSTRAND_REVERSE ? 
                                             ZMAP_ANNOTATE_UTR_LAST
                                             : ZMAP_ANNOTATE_UTR_FIRST),
                                            outline,
                                            background,
                                            cds_start,
                                            line_width,
                                            feature->strand) ;
          
          if (utr_box)
            {
              non_start = top ;
              non_end = top + (cds_start - top) - 1 ;
              
              /* set the cds box's exon_data_ptr while we still have ptr 2 it */
              /* Without this the cds part retains the coords of the FULL exon! */
              tmp_data_ptr->start = non_end + offset + 1 ;
              
              exon_data_ptr        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
              exon_data_ptr->subpart = ZMAPFEATURE_SUBPART_EXON ;
              exon_data_ptr->start = non_start + offset ;
              exon_data_ptr->end   = non_end + offset ;
              
              callItemHandler(factory, utr_box, ITEM_FEATURE_CHILD, feature, exon_data_ptr, non_start, non_end);
            }
        }
      if ((cds_end >= top) && (cds_end < bottom))
        {
          utr_box = zMapDrawAnnotatePolygon(exon_box,
                                            (feature->strand == ZMAPSTRAND_REVERSE ? 
                                             ZMAP_ANNOTATE_UTR_FIRST
                                             : ZMAP_ANNOTATE_UTR_LAST),
                                            outline,
                                            background,
                                            cds_end,
                                            line_width,
                                            feature->strand) ;
          
          if (utr_box)
            {
              non_start = bottom - (bottom - cds_end) - 1 ;
              non_end = bottom ;
              
              /* set the cds box's exon_data_ptr while we still have ptr 2 it */
              /* Without this the cds part retains the coords of the FULL exon! */
              tmp_data_ptr->end = non_start + offset - 1 ;
              
              exon_data_ptr = g_new0(ZMapWindowItemFeatureStruct, 1) ;
              exon_data_ptr->subpart = ZMAPFEATURE_SUBPART_EXON ;
              exon_data_ptr->start = non_start + offset ;
              exon_data_ptr->end   = non_end + offset - 1 ; /* - 1 because non_end
                                                           calculated from bottom
                                                           which covers the last base. */
              
              callItemHandler(factory, utr_box, ITEM_FEATURE_CHILD, feature, exon_data_ptr, non_start, non_end);
            }
        }
    }

  return;
}

static FooCanvasItem *drawTranscriptFeature(RunSet run_data,  ZMapFeature feature,
                                            double feature_offset,
                                            double x1, double y1, double x2, double y2,
                                            ZMapFeatureTypeStyle style)
{
  ZMapWindowFToIFactory factory = run_data->factory;
  /*  ZMapFeatureBlock        block = run_data->block; */
  FooCanvasGroup        *parent = run_data->container;
  FooCanvasItem *feature_item ;
  gboolean has_cds = FALSE;
  gboolean status ;
  GdkColor *outline = NULL, *foreground = NULL, *background = NULL ;
  GdkColor *cds_border = NULL, *cds_draw = NULL, *cds_fill = NULL ;
  guint line_width;
  GdkColor *intron_fill = NULL;
  int i ;
  double offset ;


  if (feature->feature.transcript.flags.cds)
    has_cds = TRUE ;


  /* NOT SURE THIS SHOULD BE AN ASSERT HERE...BETTER TO DEAL WITH THIS BY NOT DRAWING THIS COL
   * AT ALL...perhaps that's what the assert is about ??? */
  status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
			       &background, &foreground, &outline) ;
  zMapAssert(status) ;


  if (has_cds)
    {
      /* cds will default to normal colours if its own colours are not set. */
      if (!(status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_CDS, ZMAPSTYLE_COLOURTYPE_NORMAL,
					 &cds_fill, &cds_draw, &cds_border)))
	{
	  cds_fill = background ;
	  cds_draw = foreground ;
	  cds_border = outline ;

	  zMapLogWarning("Feature \"%s\" of feature set \"%s\" has a CDS but it's style, \"%s\","
			 "has no CDS colours set.",
			 g_quark_to_string(feature->original_id),
			 g_quark_to_string(feature->parent->original_id),
			 zMapStyleGetName(style)) ;
	}
    }




  line_width = factory->line_width ;

  intron_fill = outline ;

  
  if (feature->feature.transcript.introns && feature->feature.transcript.exons)
    {
      double feature_start, feature_end;
      double cds_start = 0.0, cds_end = 0.0 ; /* cds_start < cds_end */
      FooCanvasGroup *feature_group ;
      double *limits        = factory->limits;
      double points_inout[] = {0.0, 0.0, 0.0, 0.0};

      feature_start = feature->x1;
      feature_end   = feature->x2; /* I want this function to find these values */

      zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;

      feature_item  = createParentGroup(parent, feature, feature_start);
      feature_group = FOO_CANVAS_GROUP(feature_item);

      /* Calculate total offset for subparts of the feature. */
      /* We could look @ foo_canavs_group->ypos, but locks us 2 the vertical! */
      offset = feature_start + feature_offset ;

      /* Set up the cds coords if there is one. */
      if (feature->feature.transcript.flags.cds)
        {
          cds_start = points_inout[1] = feature->feature.transcript.cds_start ;
          cds_end   = points_inout[3] = feature->feature.transcript.cds_end ;
          zMapAssert(cds_start < cds_end);
          
          /* I think this is correct...???? */
          if (!((factory->user_funcs->feature_size_request)(feature, &limits[0], 
                                                           &points_inout[0], 
                                                           factory->user_data)))
            {
              cds_start = points_inout[1];
              cds_end   = points_inout[3];
              zmapWindowSeq2CanOffset(&cds_start, &cds_end, offset) ;
            }

        }

      /* first we draw the introns, then the exons.  Introns will have an invisible
       * box around them to allow the user to click there and get a reaction.
       * It's a bit hacky but the bounding box has the same coords stored on it as
       * the intron...this needs tidying up because its error prone, better if they
       * had a surrounding group.....e.g. what happens when we free ?? */
      if (feature->feature.transcript.introns)
	{
          float line_width = 1.5 ;
          GdkColor *line_fill = intron_fill;
          double dimension = 1.5 ;

	  for (i = 0 ; i < feature->feature.transcript.introns->len ; i++)  
	    {
	      FooCanvasItem *intron_box, *intron_line ;
	      double left, right, top, bottom ;
	      ZMapWindowItemFeature box_data, intron_data ;
	      ZMapSpan intron_span ;

	      intron_span = &g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i) ;

	      left   = x1;
	      right  = x2 - (double)line_width ;
              top    = points_inout[1] = intron_span->x1;
              bottom = points_inout[3] = intron_span->x2;

              if((factory->user_funcs->feature_size_request)(feature, &limits[0], &points_inout[0], factory->user_data))
                continue;

              top    = points_inout[1];
              bottom = points_inout[3];

              box_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
              box_data->start = intron_span->x1;
              box_data->end   = intron_span->x2;

              intron_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	      intron_data->subpart = ZMAPFEATURE_SUBPART_INTRON ;
              intron_data->start = intron_span->x1;
              intron_data->end   = intron_span->x2;

              zmapWindowSeq2CanOffset(&top, &bottom, offset);
              intron_box = zMapDrawSSPolygon(feature_item, 
                                             ZMAP_POLYGON_SQUARE, 
                                             left, right,
                                             top, bottom,
                                             NULL, NULL,
					     line_width,
                                             feature->strand);

              callItemHandler(factory, intron_box, ITEM_FEATURE_BOUNDING_BOX, feature, box_data, top, bottom);

              /* IF we are part of CDS we need to set intron_fill to foreground */
              /* introns will be completely contained */
#ifdef RDS_FMAP_DOESNT_DO_THIS
              if(has_cds && 
                 ((cds_start < top) && (cds_end >= bottom)) )
                line_fill = foreground;
              else
                line_fill = intron_fill;
#endif
              intron_line = zMapDrawAnnotatePolygon(intron_box,
                                                    ZMAP_ANNOTATE_INTRON, 
                                                    NULL,
                                                    line_fill,
						    dimension,
                                                    line_width,
                                                    feature->strand);
              callItemHandler(factory, intron_line, ITEM_FEATURE_CHILD, feature, intron_data, top, bottom);
                  
              intron_data->twin_item = intron_box;
              box_data->twin_item    = intron_line;
              
            }

	}


      if (feature->feature.transcript.exons)
	{

	  for (i = 0; i < feature->feature.transcript.exons->len; i++)
	    {
	      ZMapSpan exon_span ;
	      double top, bottom ;

	      exon_span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);

              top    = points_inout[1] = exon_span->x1;
              bottom = points_inout[3] = exon_span->x2;

              if ((factory->user_funcs->feature_size_request)(feature, &limits[0],
							      &points_inout[0], factory->user_data))
                continue;

              top    = points_inout[1];
              bottom = points_inout[3];

              zmapWindowSeq2CanOffset(&top, &bottom, offset) ;

              drawFeatureExon(factory, feature_item, feature, 
                              exon_span, style, 
                              x1, x2, top, bottom, offset,
                              cds_start, cds_end, line_width, 
                              background, foreground, outline,
			      has_cds, cds_fill, cds_draw, cds_border);
            }
	}
    }
  else
    {
      ZMapSpanStruct exon_span = {0,0};
      double top, bottom, feature_start;
      double *limits        = factory->limits;
      double points_inout[] = {0.0, 0.0, 0.0, 0.0};
      double cds_start = 0.0, cds_end = 0.0; /* cds_start < cds_end */
      gboolean has_cds = FALSE;
      float line_width = 1.5 ;

      feature_start = exon_span.x1 = feature->x1;
      exon_span.x2  = feature->x2;
      feature_item  = createParentGroup(parent, feature, feature_start);

      offset = feature_start + feature_offset ;
      top    = points_inout[1] = exon_span.x1;
      bottom = points_inout[3] = exon_span.x2;
      
      if (!(factory->user_funcs->feature_size_request)(feature, &limits[0],
                                                      &points_inout[0], factory->user_data))
        {
      
	  top    = points_inout[1];
	  bottom = points_inout[3];
      
	  zmapWindowSeq2CanOffset(&top, &bottom, offset) ;

	  /* Set up the cds coords if there is one. */
	  if(feature->feature.transcript.flags.cds)
	    {
	      cds_start = points_inout[1] = feature->feature.transcript.cds_start ;
	      cds_end   = points_inout[3] = feature->feature.transcript.cds_end ;
	      zMapAssert(cds_start < cds_end);
	      has_cds   = TRUE;
          
	      /* I think this is correct...???? */
	      if(!((factory->user_funcs->feature_size_request)(feature, &limits[0], 
							       &points_inout[0], 
							       factory->user_data)))
		{
		  cds_start = points_inout[1];
		  cds_end   = points_inout[3];
		  zmapWindowSeq2CanOffset(&cds_start, &cds_end, offset) ;
		}
	      
	    }

      
	  /* This is a single exon transcript.  We can't just get away
	   * with using the simple feature code.. Single exons may have a
	   * cds */

	  /* need to sort out top and bottom! */
	  drawFeatureExon(factory, feature_item, feature, 
			  &exon_span, style,
			  x1, x2, top, bottom, offset,
			  cds_start, cds_end, line_width, 
			  background, foreground, outline,
			  has_cds, cds_fill, cds_draw, cds_border);
        }
    }
  
  return feature_item ;
}

static FooCanvasItem *drawSeqFeature(RunSet run_data,  ZMapFeature feature,
                                     double feature_offset,
                                     double x1, double y1, double x2, double y2,
                                     ZMapFeatureTypeStyle style)
{
  ZMapFeatureBlock feature_block = run_data->block;
  FooCanvasItem  *text_item_parent = NULL;

  if(!feature_block->sequence.sequence)
    {
      zMapLogWarning("%s", "Trying to draw a seq feature, but there's no sequence!");
      return NULL;
    }

  text_item_parent = drawFullColumnTextFeature(run_data, feature, feature_offset,
                                               x1, y1, x2, y2, style, 
                                               1, NULL);

  return text_item_parent;
}

static FooCanvasItem *drawPepFeature(RunSet run_data,  ZMapFeature feature,
                                     double feature_offset,
                                     double x1, double y1, double x2, double y2,
                                     ZMapFeatureTypeStyle style)
{
  FooCanvasItem *text_item_parent;

  text_item_parent = drawFullColumnTextFeature(run_data, feature, feature_offset, 
                                               x1, y1, x2, y2, style,
                                               3, feature->description);

  return text_item_parent;
}


static gint canvas_allocate_protein_cb(FooCanvasItem   *item,
				       ZMapTextDrawData draw_data,
				       gint             max_width,
				       gint             max_buffer_size,
				       gpointer         user_data)
{
  int buffer_size = max_buffer_size;
  int char_per_line, line_count;
  int cx1, cy1, cx2, cy2, cx, cy; /* canvas coords of scroll region and item */
  int table;
  int width, height;
  double world_range, raw_chars_per_line, raw_lines;
  int real_chars_per_line, canvas_range;
  int real_lines, real_lines_space;

  /* A bit I don't like. Handles the top/bottom border. */
  if(draw_data->y1 < 0.0)
    {
      double t = 0.0 - draw_data->y1;
      draw_data->y1  = 0.0;
      draw_data->y2 -= t;
    }
  /* scroll region and text position to canvas coords */
  foo_canvas_w2c(item->canvas, draw_data->x1, draw_data->y1, &cx1, &cy1);
  foo_canvas_w2c(item->canvas, draw_data->x2, draw_data->y2, &cx2, &cy2);
  foo_canvas_w2c(item->canvas, draw_data->wx, draw_data->wy, &cx,  &cy);

  /* We need to know the extents of a character */
  width  = draw_data->table.ch_width;
  height = draw_data->table.ch_height;

  /* world & canvas range to work out rough chars per line */
  world_range        = (draw_data->y2 - draw_data->y1 + 1) / 3.0;
  canvas_range       = (cy2 - cy1 + 1);
  raw_chars_per_line = ((world_range * height) / canvas_range);
  
  /* round up raw to get real. */
  if((double)(real_chars_per_line = (int)raw_chars_per_line) < raw_chars_per_line)
    real_chars_per_line++;
  

  /* how many lines? */
  raw_lines = world_range / real_chars_per_line;
  
  /* round up raw to get real */
  if((double)(real_lines = (int)raw_lines) < raw_lines)
    real_lines++;
  
  /* How much space do real_lines takes up? */
  real_lines_space = real_lines * height;
  
  if(real_lines_space > canvas_range)
    {
      /* Ooops... Too much space, try one less */
      real_lines--;
      real_lines_space = real_lines * height;
    }
  
  /* Make sure we fill the space... */
  if(real_lines_space < canvas_range)
    {
      double spacing_dbl = canvas_range;
      double delta_factor = 0.15;
      int spacing;
      spacing_dbl -= (real_lines * height);
      spacing_dbl /= real_lines;
      
      /* need a fudge factor here! We want to round up if we're
       * within delta factor of next integer */
      
      if(((double)(spacing = (int)spacing_dbl) < spacing_dbl) &&
	 ((spacing_dbl + delta_factor) > (double)(spacing + 1)))
	spacing_dbl = (double)(spacing + 1);
      
      draw_data->table.spacing = (int)(spacing_dbl * PANGO_SCALE);
      draw_data->table.spacing = spacing_dbl;
    }
  
  /* Now we've set the number of lines we have */
  line_count    = real_lines;
  /* Not too wide! Default! */
  char_per_line = max_width;
  /* Record this so we can do index calculations later */
  draw_data->table.untruncated_width = real_chars_per_line;

  if(real_chars_per_line <= char_per_line)
    {
      char_per_line = real_chars_per_line;
      draw_data->table.truncated = FALSE;
    }
  else
    draw_data->table.truncated = TRUE; /* truncated to max_width. */

  /* Record the table dimensions. */
  draw_data->table.width  = char_per_line;
  draw_data->table.height = line_count;
  /* which has table number of cells */
  table = char_per_line * line_count;

  /* We absolutely must not be bigger than buffer_size. */
  buffer_size = MIN(buffer_size, table);

  return buffer_size;
}

static gint canvas_allocate_dna_cb(FooCanvasItem   *item,
				   ZMapTextDrawData draw_data,
				   gint             max_width,
				   gint             buffer_size,
				   gpointer         user_data)
{

  buffer_size = foo_canvas_zmap_text_calculate_zoom_buffer_size(item, draw_data, buffer_size);

  return buffer_size;
}

static gint canvas_fetch_feature_text_cb(FooCanvasItem *text_item, 
					 ZMapTextDrawData draw_data,
					 char *buffer_in_out,
					 gint  buffer_size,
					 gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)user_data;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)user_data;
  char *buffer_ptr = buffer_in_out;
  char *seq = NULL, *seq_ptr;
  int itr, start;
  int table_size;
  int untruncated_size;
  int bases;
  gboolean if_last = FALSE, else_last = FALSE, debug = FALSE;
  gboolean report_table_size = FALSE;

  table_size       = draw_data->table.width * draw_data->table.height;
  bases            = draw_data->table.untruncated_width;
  untruncated_size = draw_data->table.height * bases;

  memset(&buffer_in_out[table_size], 0, buffer_size - table_size);

  if(report_table_size)
    printf("table size = %d\n", table_size);

  start   = (int)(draw_data->wy);

  if (feature->type == ZMAPSTYLE_MODE_RAW_SEQUENCE)
    {
      seq_ptr = feature->feature.sequence.sequence + start;
    }
  else if(feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
    {
      int protein_start;
      /* Get the protein index that the world y corresponds to. */
      protein_start = (int)(draw_data->wy) - feature->x1;
      protein_start = (int)(protein_start / 3);

      seq_ptr  = feature->feature.sequence.sequence;
      seq_ptr += protein_start;
    }
  else
    seq_ptr = feature->description;

  if(debug)
    printf("123456789|123456789|123456789|123456789|\n");

  /* make sure we include the end of the last row. */
  table_size++;

  for(itr = 0; itr < table_size; itr++, buffer_ptr++, seq_ptr++)
    {
      if(draw_data->table.truncated && 
	 (buffer_ptr != buffer_in_out) &&
	 ((itr) % (draw_data->table.width)) == 0)
	{
	  int e;

	  seq_ptr += (bases - draw_data->table.width);

	  if(debug)
	    printf("\t%d...\n", (bases - draw_data->table.width));

	  buffer_ptr-=3;	/* rewind. */

	  for(e = 0; e<3; e++, buffer_ptr++)
	    {
	      *buffer_ptr = '.';
	    }

	  if_last   = TRUE;
	  else_last = FALSE;

	  /* need to do what we've missed doing in else... */
	  *buffer_ptr = *seq_ptr;
	  if(debug)
	    printf("%c", *seq_ptr);
	}
      else
	{
	  if(debug)
	    printf("%c", *seq_ptr);

	  *buffer_ptr = *seq_ptr;
	  if_last   = FALSE;
	  else_last = TRUE;
	}
    }

  /* because of table_size++ above. */
  *(--buffer_ptr) = '\0';

  if(debug)
    printf("\n");

  if(seq)
    g_free(seq);

  return buffer_ptr - buffer_in_out;
}


static gboolean item_to_char_cell_coords2(FooCanvasPoints **points_out,
					  FooCanvasItem    *subject, 
					  gpointer          user_data)
{
  FooCanvasItem *overlay_item;
  FooCanvasGroup *overlay_group, *container;
  ZMapFeature subject_feature, overlay_feature;
  FooCanvasPoints *points;
  double first[ITEMTEXT_CHAR_BOUND_COUNT], last[ITEMTEXT_CHAR_BOUND_COUNT];
  int index1, index2, tmp;
  gboolean first_found, last_found;
  gboolean do_overlay = FALSE;
  gboolean redraw = FALSE;

  if(FOO_IS_CANVAS_ITEM(user_data) &&
     FOO_IS_CANVAS_GROUP(user_data))
    {
      overlay_group = FOO_CANVAS_GROUP(user_data);
      overlay_item  = FOO_CANVAS_ITEM(overlay_group->item_list->data);

      container = zmapWindowContainerGetParentContainerFromItem(overlay_item);

      do_overlay = (gboolean)(FOO_CANVAS_ITEM(container)->object.flags & FOO_CANVAS_ITEM_VISIBLE);
    }

  if(do_overlay)
    {
      ZMapWindowItemFeatureType feature_type;
      ZMapFrame subject_frame, overlay_frame;

      subject_feature  = g_object_get_data(G_OBJECT(subject),
					   ITEM_FEATURE_DATA);
      overlay_feature  = g_object_get_data(G_OBJECT(overlay_group), 
					   ITEM_FEATURE_DATA); 
      feature_type     = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(subject),
							   ITEM_FEATURE_TYPE));

      index1 = subject_feature->x1;
      index2 = subject_feature->x2;
      do_overlay = FALSE;

      if(feature_type == ITEM_FEATURE_CHILD)
	{
	  ZMapWindowItemFeature sub_feature_data;

	  sub_feature_data = g_object_get_data(G_OBJECT(subject),
					       ITEM_SUBFEATURE_DATA);
	  switch(sub_feature_data->subpart)
	    {
	    case ZMAPFEATURE_SUBPART_EXON:
	    case ZMAPFEATURE_SUBPART_EXON_CDS:
	      {
		gboolean select_only_matching_frame = FALSE;
		index1 = sub_feature_data->start;
		index2 = sub_feature_data->end;
		do_overlay = TRUE;

		if(overlay_feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
		  {
		    if(sub_feature_data->subpart == ZMAPFEATURE_SUBPART_EXON_CDS)
		      {
			subject_frame = zMapFeatureTranscriptFrame(subject_feature);
			subject_frame = zMapFeatureSubPartFrame(subject_feature, index1);
		      }
		    else
		      subject_frame = zMapFeatureFrame(subject_feature);

		    overlay_frame = zMapFeatureFrame(overlay_feature);

		    if(select_only_matching_frame &&
		       subject_frame != overlay_frame)
		      do_overlay = FALSE;
		    else
		      {
			int cds_index1, cds_index2;
			int end_phase = 0, start_phase = 0;

			/* We can't highlight the phase codons accurately... */
			
			/* I'm fairly sure what we need is zMapFeatureTranslationStartCoord(feature) */

			/* These come back 1 based. */
			zMapFeatureWorld2CDS(subject_feature, index1, index2,
					     &cds_index1, &cds_index2);
			/* Coord space: 1 2 3 | 4 5 6 | 7 8 9 | . . . */
			/* Exon coords: ^         ^ ^       ^         */
			/* Start phase: 1-1%3=0 - - 6-1%3=2 (dashes=missing bases) */

			/* missing start bases, need to skip forward to get codon start */
			start_phase = (cds_index1 - 1) % 3;
			/* missing end bases, need to skip backward to get full codon */
			end_phase   = (cds_index2 % 3);
#ifdef RDS_DONT_INCLUDE			
			printf("indices are %d %d\n", index1, index2);
			printf("phases are %d %d\n", start_phase, end_phase);
#endif
			if(start_phase != 0)
			  index1 += 3 - start_phase;
			if(end_phase != 0)
			  index2 -= end_phase;
#ifdef RDS_DONT_INCLUDE
			printf("indices are %d %d\n", index1, index2);
#endif
		      }
		  }
	      }
	      break;
	    case ZMAPFEATURE_SUBPART_MATCH:
	      {
		index1 = sub_feature_data->start;
		index2 = sub_feature_data->end;
		do_overlay = TRUE;

		if(overlay_feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
		  {
		    subject_frame = zMapFeatureFrame(subject_feature);
		    overlay_frame = zMapFeatureFrame(overlay_feature);
		    if(subject_frame != overlay_frame)
		      do_overlay = FALSE;
		  }
	      }
	      break;
	    default:
	      break;
	    }
	}
      else if(feature_type == ITEM_FEATURE_SIMPLE)
	{
	  switch(subject_feature->type)
	    {
	    case ZMAPSTYLE_MODE_ALIGNMENT:
	      do_overlay = TRUE;
	      if(overlay_feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
		{
		  subject_frame = zMapFeatureFrame(subject_feature);
		  overlay_frame = zMapFeatureFrame(overlay_feature);
		  if(subject_frame != overlay_frame)
		    do_overlay = FALSE;
		}
	      break;
	    case ZMAPSTYLE_MODE_GRAPH:
	    case ZMAPSTYLE_MODE_TEXT:
	    case ZMAPSTYLE_MODE_BASIC:
	      do_overlay = TRUE;
	      break;
	    default:
	      break;
	    }
	}
    }

  if(do_overlay)
    {
      /* SWAP MACRO? */
      if(index1 > index2){ tmp = index1; index1 = index2; index2 = tmp; }

      /* From the text indices, get the bounds of that char */
      first_found = zmapWindowItemTextIndex2Item(overlay_item, index1, &first[0]);
      last_found  = zmapWindowItemTextIndex2Item(overlay_item, index2, &last[0]);
      
      if(first_found && last_found)
	{
	  double minx, maxx;
	  foo_canvas_item_get_bounds(overlay_item, &minx, NULL, &maxx, NULL);

	  points = foo_canvas_points_new(8);

	  zmapWindowItemTextOverlayFromCellBounds(points,
						  &first[0],
						  &last[0],
						  minx, maxx);
	  zmapWindowItemTextOverlayText2Overlay(overlay_item, points);
	  
	  /* set the points */
	  if(points_out)
	    {
	      *points_out = points;
	      /* record such */
	      redraw = TRUE;
	    }
	  else
	    foo_canvas_points_free(points);
	}
    }

  return redraw;
}

/* item_to_char_cell_coords: This is a ZMapWindowOverlaySizeRequestCB callback
 * The overlay code calls this function to request whether to draw an overlay
 * on a region and specifically where to draw it.  returns TRUE to make the 
 * draw and FALSE to not...
 * Logic:  The text context is used to find the coords for the first and last
 * characters of the dna corresponding to the start and end of the feature 
 * attached to the subject given.  TRUE is only returned if at least one of 
 * the start and end are not defaulted, i.e. shown.
 */
static FooCanvasItem *drawFullColumnTextFeature(RunSet run_data,  ZMapFeature feature,
                                                double feature_offset,
                                                double x1, double y1, double x2, double y2,
                                                ZMapFeatureTypeStyle style,
                                                int bases_per_char, char *column_text)
{
  gboolean status ;
  ZMapWindowFToIFactory  factory = run_data->factory;
  FooCanvasGroup         *parent = run_data->container;
  double feature_start, feature_end;
  FooCanvasItem  *prev_trans     = NULL;
  FooCanvasItem  *feature_parent = NULL;
  FooCanvasGroup *column_parent  = NULL;
  ZMapWindowItemFeature feature_data;
  FooCanvasItem *item;
  GdkColor *outline = NULL, *foreground = NULL, *background = NULL;
  ZMapWindowOverlay overlay_manager;
  FooCanvasZMapAllocateCB allocate_func_cb = NULL;
  FooCanvasZMapFetchTextCB fetch_text_func_cb = NULL;
  double new_x ;

  status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
			       &background, &foreground, &outline);
  zMapAssert(status) ;

  feature_start  = feature->x1;
  feature_end    = feature->x2;

  zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;

  /* bump the feature BEFORE drawing */
  if(parent->item_list_end && (prev_trans = FOO_CANVAS_ITEM(parent->item_list_end->data)))
    {
      foo_canvas_item_get_bounds(prev_trans, NULL, NULL, &new_x, NULL);

      new_x += zMapStyleGetBumpWidth(style) ;
    }
  else
    new_x = 0.0 ;

  feature_parent = foo_canvas_item_new(FOO_CANVAS_GROUP(parent),
				       foo_canvas_float_group_get_type(),
				       "x", new_x,
				       "min-x", new_x,
				       "y", 0.0,
				       "min-y", 0.0,
				       NULL) ;

  g_object_set_data(G_OBJECT(feature_parent), ITEM_FEATURE_TYPE,
                    GINT_TO_POINTER(ITEM_FEATURE_PARENT)) ;
  g_object_set_data(G_OBJECT(feature_parent), ITEM_FEATURE_DATA, feature) ;

  my_foo_canvas_item_goto(feature_parent, &new_x, NULL);

  column_parent = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(parent)) ;

  if (!factory->font_desc)
    zMapFoocanvasGetTextDimensions(FOO_CANVAS_ITEM(feature_parent)->canvas, 
                                   &(factory->font_desc), 
                                   &(factory->text_width),
                                   &(factory->text_height));

  if(feature->type == ZMAPSTYLE_MODE_RAW_SEQUENCE)
    {
      allocate_func_cb   = canvas_allocate_dna_cb;
      fetch_text_func_cb = canvas_fetch_feature_text_cb;
    }
  else if(feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
    {
      allocate_func_cb   = canvas_allocate_protein_cb;
      fetch_text_func_cb = canvas_fetch_feature_text_cb;
    }

  if((item = foo_canvas_item_new(FOO_CANVAS_GROUP(feature_parent),
				 foo_canvas_zmap_text_get_type(),
				 "x",               0.0,
				 "y",               feature_start,
				 "anchor",          GTK_ANCHOR_NW,
				 "font_desc",       factory->font_desc,
				 "full-width",      30.0,
				 "wrap-mode",       PANGO_WRAP_CHAR,
				 "fill_color_gdk",  foreground,
				 "allocate_func",   allocate_func_cb,
				 "fetch_text_func", fetch_text_func_cb,
				 "callback_data",   feature,
				 NULL)))
    {
      feature_data = g_new0(ZMapWindowItemFeatureStruct, 1);
      callItemHandler(factory, item, ITEM_FEATURE_CHILD, feature, feature_data, 0.0, 10.0);
      g_object_set_data(G_OBJECT(item), ITEM_FEATURE_ITEM_STYLE, style);
    }
  

  /* This is attached to the column parent so needs updating each time
   * and doesn't get destroyed with the feature unlike the text
   * context */
  if((overlay_manager = g_object_get_data(G_OBJECT(column_parent), ITEM_FEATURE_OVERLAY_DATA)))
    {
      //zmapWindowOverlaySetLimitItem(overlay_manager, NULL);
      zmapWindowOverlaySetLimitItem(overlay_manager, feature_parent);
      zmapWindowOverlayUpdate(overlay_manager);
      zmapWindowOverlaySetSizeRequestor(overlay_manager, item_to_char_cell_coords2, feature_parent);
    }

  return feature_parent;
}

static FooCanvasItem *drawSimpleAsTextFeature(RunSet run_data, ZMapFeature feature,
                                              double feature_offset,
                                              double x1, double y1, double x2, double y2,
                                              ZMapFeatureTypeStyle style)
{
  ZMapWindowFToIFactory factory = run_data->factory;
  FooCanvasGroup        *parent = run_data->container;
  FooCanvasItem *item = NULL;
  char *text_string = NULL;
  char *text_colour = "black";
  
  text_string = (char *)g_quark_to_string(feature->locus_id);

  item = zMapDisplayText(parent, text_string, 
                         text_colour, 0.0, y1);
  
  callItemHandler(factory, item, ITEM_FEATURE_SIMPLE, feature, NULL, y1, y1);
    
  return item;
}




static FooCanvasItem *drawSimpleGraphFeature(RunSet run_data, ZMapFeature feature,
					     double feature_offset,
					     double x1, double y1, double x2, double y2,
					     ZMapFeatureTypeStyle style)
{
  ZMapWindowFToIFactory factory = run_data->factory ;
  gboolean status ;
  FooCanvasGroup *parent = run_data->container ;
  FooCanvasItem *feature_item ;
  GdkColor *outline = NULL, *foreground = NULL, *background = NULL ;
  guint line_width ;
  double numerator, denominator, dx ;
  double width, max_score, min_score ;

  width = zMapStyleGetWidth(style) ;
  min_score = zMapStyleGetMinScore(style) ;
  max_score = zMapStyleGetMaxScore(style) ;

  line_width = factory->line_width;

  zmapWindowSeq2CanOffset(&y1, &y2, feature_offset) ;	    /* Make sure we cover the whole last base. */

  status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
			       &background, &foreground, &outline) ;
  zMapAssert(status) ;

  numerator = feature->score - min_score ;
  denominator = max_score - min_score ;

  if (denominator == 0)				    /* catch div by zero */
    {
      if (numerator < 0)
	dx = 0 ;
      else if (numerator > 1)
	dx = 1 ;
    }
  else
    {
      dx = numerator / denominator ;
      if (dx < 0)
	dx = 0 ;
      if (dx > 1)
	dx = 1 ;
    }
  x1 = 0.0 + (width * zMapStyleBaseline(style)) ;
  x2 = 0.0 + (width * dx) ;

  /* If the baseline is not zero then we can end up with x2 being less than x1 so swop them for
   * drawing, perhaps the drawing code should take care of this. */
  if (x1 > x2)
    {
      double tmp ;

      tmp = x1 ;
      x1 = x2 ;
      x2 = tmp ;
    }

  feature_item = zMapDrawBox(parent,
			     x1, y1, x2, y2,
			     outline, background, line_width) ;

  callItemHandler(factory, feature_item,
                  ITEM_FEATURE_SIMPLE,
                  feature, NULL, y1, y2) ;

  return feature_item ;
}

/*!
 * \brief Accessory function for drawAlignFeature. 
 * 
 * The Gaps array from acedb and indeed any other source might not be sorted correctly.
 * Indeed I'm not sure it can be when the strands of the reference and the hit don't match.
 * Therefore we need a way to get the coords for the Gap from the Adjacent Matches in the
 * Gaps Array (Incorrectly named!).  The drawAlignFeature was doing this a couple of times
 * and incorrectly working out the actual indel in one if not both of the sequences. 
 * Something I'm guessing it's been doing since it was written.  This function provides a
 * single way to get the coords of the gap for both the reference and hit sequence.
 *
 * @param block_a        One of the Two Adjacent Match Blocks
 * @param block_b        One of the Two Adjacent Match Blocks
 * @param gap_block_out  Pointer to an empty struct to set values in.
 * @param t_indel_gt_1   pointer to boolean to notify indel in reference > 1 bp
 * @param q_indel_gt_1   pointer to boolean to notify indel in hit sequence > 1 bp
 *
 * @return void
 * 
 * N.B. if t_indel_gt_1 or q_indel_gt_1 return false the corresponding gap_block_out coords
 * will be equal to the adjacent match blocks
 *
 * N.B.B. The strand assertion in this function is ifdef'd out see rt
 * 79058.  And hence the same as the reason for ifdef'ing in calling
 * function (line ~ 1110)
 *
 */
/* feel free to optimise, it might need it... Way too many asserts... */
static void GapAlignBlockFromAdjacentBlocks(ZMapAlignBlock block_a, ZMapAlignBlock block_b, 
					    ZMapAlignBlockStruct *gap_span_out,
					    gboolean *t_indel_gt_1,
					    gboolean *q_indel_gt_1)
{
  if(gap_span_out)
    {
      int t_indel, q_indel, t_order = 0, q_order = 0;
      gboolean inconsistency_warning = FALSE, t = FALSE, q = FALSE;

      /* ordering == -1 when a < b, and == +1 when a > b. 
       * If it ends up as 0 they equal and this is BAD */
#ifdef NEEDS_THINKING_ABOUT___      
      /* Sanity first, insanity later */
      zMapAssert(block_a->t1 <= block_a->t2);
      zMapAssert(block_a->q1 <= block_a->q2);
      zMapAssert(block_b->t1 <= block_b->t2);
      zMapAssert(block_b->q1 <= block_b->q2);

      zMapAssert(block_a->q_strand == block_b->q_strand);
      zMapAssert(block_a->t_strand == block_b->t_strand);
#endif
      /* First copy across the strand to the gap span */
      gap_span_out->q_strand = block_a->q_strand;
      gap_span_out->t_strand = block_a->t_strand;


      /* get orders... */
      /* q1 < q2 _always_! so a->q2 < b->q1 means a < b and v.v.*/

      if(block_a->t2 < block_b->t1)
	{
	  t_order = -1;
	  gap_span_out->t1 = block_a->t2;
	  gap_span_out->t2 = block_b->t1;
	}
      else
	{
	  t_order =  1;
	  gap_span_out->t1 = block_b->t2;
	  gap_span_out->t2 = block_a->t1;
	}

      if(block_a->q2 < block_b->q1)
	{
	  q_order = -1;
	  gap_span_out->q1 = block_a->q2;
	  gap_span_out->q2 = block_b->q1;
	}
      else
	{
	  q_order =  1;
	  gap_span_out->q1 = block_b->q2;
	  gap_span_out->q2 = block_a->q1;
	}
#ifdef NEEDS_THINKING_ABOUT___
      zMapAssert(gap_span_out->t1 < gap_span_out->t2);
      zMapAssert(gap_span_out->q1 < gap_span_out->q2);
#endif
      /* What are the indels?  */
      t_indel = gap_span_out->t2 - gap_span_out->t1;
      q_indel = gap_span_out->q2 - gap_span_out->q1;

      /* If we've got an indel then the gap is actually + and - 1 from that.
       * We need to let our caller know what we know though.
       */
      if(t_indel > 1)
	{
	  (gap_span_out->t1)++;
	  (gap_span_out->t2)--;
	  t = TRUE;
	}

      if(q_indel > 1)
	{
	  (gap_span_out->q1)++;
	  (gap_span_out->q2)--;
	  q = TRUE;
	}

      if(t_indel_gt_1)
	*t_indel_gt_1 = t;
      if(q_indel_gt_1)
	*q_indel_gt_1 = q;
#ifdef NEEDS_THINKING_ABOUT___
      zMapAssert(gap_span_out->t1 <= gap_span_out->t2);
      zMapAssert(gap_span_out->q1 <= gap_span_out->q2);
#endif
      if(inconsistency_warning && 
	 q_order != t_order && 
	 gap_span_out->q_strand == gap_span_out->t_strand)
	{
	  zMapLogWarning("ZMapAlignBlocks have inconsistent order and strand\n"
			 "\tBlock A->t1 = %d,  Block B->t1 = %d\n"
			 "\tBlock A->t2 = %d,  Block B->t2 = %d\n"
			 "\tBlock A->t_strand = %s,  Block B->t_strand = %s\n"
			 "\tBlock A->q1 = %d,  Block B->q1 = %d\n"
			 "\tBlock A->q2 = %d,  Block B->q2 = %d\n"
			 "\tBlock A->q_strand = %s,  Block B->q_strand = %s",
			 block_a->t1, block_b->t1,
			 block_a->t2, block_b->t2,
			 zMapFeatureStrand2Str(block_a->t_strand),
			 zMapFeatureStrand2Str(block_b->t_strand),
			 block_a->q1, block_b->q1,
			 block_a->q2, block_b->q2,
			 zMapFeatureStrand2Str(block_a->q_strand), 
			 zMapFeatureStrand2Str(block_b->q_strand)
			 );
	}
    }

  return ;
}


static FooCanvasItem *invalidFeature(RunSet run_data,  ZMapFeature feature,
                                     double feature_offset,
                                     double x1, double y1, double x2, double y2,
                                     ZMapFeatureTypeStyle style)
{

  zMapAssertNotReached();

  return NULL;
}

static void null_item_created(FooCanvasItem            *new_item,
                              ZMapWindowItemFeatureType new_item_type,
                              ZMapFeature               full_feature,
                              ZMapWindowItemFeature     sub_feature,
                              double                    new_item_y1,
                              double                    new_item_y2,
                              gpointer                  handler_data)
{
  return ;
}

static gboolean null_top_item_created(FooCanvasItem *top_item,
                                      ZMapFeatureContext context,
                                      ZMapFeatureAlignment align,
                                      ZMapFeatureBlock block,
                                      ZMapFeatureSet set,
                                      ZMapFeature feature,
                                      gpointer handler_data)
{
  return TRUE;
}

static gboolean null_feature_size_request(ZMapFeature feature, 
                                          double *limits_array, 
                                          double *points_array_inout, 
                                          gpointer handler_data)
{
  return FALSE;
}

