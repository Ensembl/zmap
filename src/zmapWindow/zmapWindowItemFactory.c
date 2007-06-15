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
 * Last edited: Jun 14 17:47 2007 (rds)
 * Created: Mon Sep 25 09:09:52 2006 (rds)
 * CVS info:   $Id: zmapWindowItemFactory.c,v 1.32 2007-06-15 09:18:26 rds Exp $
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <string.h>
#include <ZMap/zmapUtils.h>
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
  ZMapFeatureType type;
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

  gboolean stats_allocated;
  ZMapWindowStats stats;
}ZMapWindowFToIFactoryStruct;

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

static void ZoomEventHandler(FooCanvasGroup *container, double zoom_factor, gpointer user_data);
static void ZoomDataDestroy(gpointer data);

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
static void TextDataDestroy(gpointer user_data);

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

static void drawTextFeatureWithIterator(ZMapWindowFToIFactory factory,
                                        FooCanvasGroup *parent,
                                        ZMapFeature feature,
                                        ZMapWindowItemTextIterator iterator, 
                                        char *text_input);


/*
 * Static function tables for drawing features in various ways.
 */

const static ZMapWindowFToIFactoryMethodsStruct factory_methods_G[] = {
  {ZMAPFEATURE_INVALID,      invalidFeature},
  {ZMAPFEATURE_BASIC,        drawSimpleFeature},
  {ZMAPFEATURE_ALIGNMENT,    drawAlignFeature},
  {ZMAPFEATURE_TRANSCRIPT,   drawTranscriptFeature},
  {ZMAPFEATURE_RAW_SEQUENCE, drawSeqFeature},
  {ZMAPFEATURE_PEP_SEQUENCE, drawPepFeature},
  {-1,                       NULL}
};

const static ZMapWindowFToIFactoryMethodsStruct factory_text_methods_G[] = {
  {ZMAPFEATURE_INVALID,      invalidFeature},
  {ZMAPFEATURE_BASIC,        drawSimpleAsTextFeature},
  {ZMAPFEATURE_ALIGNMENT,    drawAlignFeature},
  {ZMAPFEATURE_TRANSCRIPT,   drawTranscriptFeature},
  {ZMAPFEATURE_RAW_SEQUENCE, drawSeqFeature},
  {ZMAPFEATURE_PEP_SEQUENCE, drawPepFeature},
  {-1,                       NULL}
};

const static ZMapWindowFToIFactoryMethodsStruct factory_graph_methods_G[] = {
  {ZMAPFEATURE_INVALID,      invalidFeature},
  {ZMAPFEATURE_BASIC,        drawSimpleGraphFeature},
  {ZMAPFEATURE_ALIGNMENT,    drawSimpleGraphFeature},
  {ZMAPFEATURE_TRANSCRIPT,   drawSimpleGraphFeature},
  {ZMAPFEATURE_RAW_SEQUENCE, drawSimpleGraphFeature},
  {ZMAPFEATURE_PEP_SEQUENCE, drawSimpleGraphFeature},
  {-1,                       NULL}
};

const static ZMapWindowFToIFactoryMethodsStruct factory_basic_methods_G[] = {
  {ZMAPFEATURE_INVALID,      invalidFeature},
  {ZMAPFEATURE_BASIC,        drawSimpleFeature},
  {ZMAPFEATURE_ALIGNMENT,    drawSimpleFeature},
  {ZMAPFEATURE_TRANSCRIPT,   drawSimpleFeature},
  {ZMAPFEATURE_RAW_SEQUENCE, drawSimpleFeature},
  {ZMAPFEATURE_PEP_SEQUENCE, drawSimpleFeature},
  {-1,                       NULL}
};

static gboolean print_stats_on_close_G = FALSE;


ZMapWindowFToIFactory zmapWindowFToIFactoryOpen(GHashTable *feature_to_item_hash, 
                                                ZMapWindowLongItems long_items)
{
  ZMapWindowFToIFactory factory = NULL;
  ZMapWindowFToIFactoryProductionTeam user_funcs = NULL;

  if((factory = g_new0(ZMapWindowFToIFactoryStruct, 1)) && 
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
                                ZMapWindowStats stats,
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

  if(!stats)
    {
      factory->stats = g_new0(ZMapWindowStatsStruct, 1);
      factory->stats_allocated = TRUE;
    }
  else
    factory->stats = stats;

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
  int feature_type ;
  FooCanvasItem *item = NULL, *return_item = NULL;
  FooCanvasGroup *features_container = NULL;
  ZMapWindowItemFeatureSetData set_data = NULL;
  ZMapFeatureTypeStyle style = NULL;
  ZMapStyleMode style_mode = ZMAPSTYLE_MODE_INVALID ;
  GHashTable *style_table = NULL;
  gboolean no_points_in_block = TRUE;
  /* check here before they get called.  I'd prefer to only do this
   * once per factory rather than once per Run! */

  zMapAssert(factory->user_funcs && 
             factory->user_funcs->item_created &&
             factory->user_funcs->top_item_created);

  feature_type = (int)(feature->type) ;

  if (feature_type >=0 && feature_type <= FACTORY_METHOD_COUNT)
    {
      double *limits = factory->limits;
      double *points = factory->points;
      ZMapWindowFToIFactoryMethods method = NULL;
      ZMapWindowFToIFactoryMethodsStruct *method_table = NULL;

      features_container = zmapWindowContainerGetFeatures(parent_container) ;
      set_data = g_object_get_data(G_OBJECT(parent_container), ITEM_FEATURE_SET_DATA);
      zMapAssert(set_data);

      style_table = set_data->style_table;
      /* Get the styles table from the column and look for the features style.... */
      if (!(style = zmapWindowStyleTableFind(style_table, zMapStyleGetUniqueID(feature->style))))
        {
          style = zMapFeatureStyleCopy(feature->style) ;  
          zmapWindowStyleTableAdd(style_table, style) ;
        }


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
          method_table = factory->basic_methods;

          if(feature->flags.has_score)
            zmapWindowGetPosFromScore(style, feature->score, &(points[0]), &(points[2])) ;

          method = &(method_table[feature_type]);
          break;

        case ZMAPSTYLE_MODE_ALIGNMENT:
          method_table = factory->methods;

          if (feature->flags.has_score)
            zmapWindowGetPosFromScore(style, feature->score, &(points[0]), &(points[2])) ;

          factory->stats->total_matches++;

          if ((!feature->feature.homol.align) || (!zMapStyleIsAlignGaps(style)))
            {
              factory->stats->ungapped_matches++;
              factory->stats->ungapped_boxes++;
              factory->stats->total_boxes++;

              method = &(method_table[ZMAPFEATURE_BASIC]);              
            }
          else if (feature->feature.homol.align)
            {
              if(feature->feature.homol.flags.perfect)
                {
                  factory->stats->gapped_matches++;
                  factory->stats->total_boxes  += feature->feature.homol.align->len ;
                  factory->stats->gapped_boxes += feature->feature.homol.align->len ;

                  method = &(method_table[feature_type]);
                }
              else
                {
                  factory->stats->not_perfect_gapped_matches++;
                  factory->stats->total_boxes++;
                  factory->stats->ungapped_boxes++;
                  factory->stats->imperfect_boxes += feature->feature.homol.align->len;

                  method = &(method_table[ZMAPFEATURE_BASIC]);
                }
            }
          else
            zMapAssertNotReached();

          break;

        case ZMAPSTYLE_MODE_TEXT:
          method_table = factory->text_methods;
          break;

        case ZMAPSTYLE_MODE_GRAPH:
          method_table = factory->graph_methods;
          break;

        case ZMAPSTYLE_MODE_TRANSCRIPT:
          method_table = factory->methods;
	  method = &(method_table[feature_type]);
	  break ;


        default:
	  zMapAssertNotReached() ;
          break;
        }

      switch(feature_type)
        {
        case ZMAPFEATURE_TRANSCRIPT:
          break;

        case ZMAPFEATURE_ALIGNMENT:
          break;

        case ZMAPFEATURE_BASIC:
          method = &(method_table[feature_type]);
	  break ;

	case ZMAPFEATURE_RAW_SEQUENCE:
	case ZMAPFEATURE_PEP_SEQUENCE:
          method = &(method_table[feature_type]);
          break;

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
          
          g_object_set_data(G_OBJECT(item), ITEM_FEATURE_ITEM_STYLE, GINT_TO_POINTER(feature->style)) ;
          
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
  if(print_stats_on_close_G)
    zmapWindowStatsPrint(factory->stats);

  if(factory->stats_allocated)
    {
      g_free(factory->stats);
    }

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


static void ZoomEventHandler(FooCanvasGroup *container, double zoom_factor, gpointer user_data)
{
  ZMapWindowFToIFactory tmp_factory = NULL; /* A factory so we can redraw */
  ZMapWindowFToIFactory factory_input = (ZMapWindowFToIFactory)user_data;
  ZMapFeatureSet feature_set = NULL;
  FooCanvasGroup *container_features, *container_underlay;
  ZMapWindowItemFeatureSetData set_data;

  if(zmapWindowItemIsVisible(FOO_CANVAS_ITEM(container)) && 
     (set_data = (ZMapWindowItemFeatureSetData)g_object_get_data(G_OBJECT(container), ITEM_FEATURE_SET_DATA)))
    {
      tmp_factory = zmapWindowFToIFactoryOpen(factory_input->ftoi_hash, factory_input->long_items);
     
      feature_set = (ZMapFeatureSet)g_object_get_data(G_OBJECT(container), ITEM_FEATURE_DATA) ;
      /* Check it's actually a featureset */
      zMapAssert(feature_set && feature_set->struct_type == ZMAPFEATURE_STRUCT_FEATURESET) ;

      container_features = FOO_CANVAS_GROUP(zmapWindowContainerGetFeatures(container)) ;
      
      container_underlay = FOO_CANVAS_GROUP(zmapWindowContainerGetUnderlays(container));

      zmapWindowContainerPurge(container_features);
      zmapWindowContainerPurge(container_underlay);

      zmapWindowFToIFactorySetup(tmp_factory, factory_input->line_width,
                                 (factory_input->stats_allocated ? NULL : factory_input->stats),
                                 factory_input->user_funcs, 
                                 factory_input->user_data);
      
      zmapWindowFToIFactoryRunSet(tmp_factory, feature_set, container, set_data->frame);
      
      zmapWindowFToIFactoryClose(tmp_factory);
    }

  return ;
}

static void ZoomDataDestroy(gpointer data)
{
  ZMapWindowFToIFactory factory = (ZMapWindowFToIFactory)data;
  
  zmapWindowFToIFactoryClose(factory);

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



  line_width = factory->line_width;

  zmapWindowSeq2CanOffset(&y1, &y2, feature_offset) ;	    /* Make sure we cover the whole last base. */

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

      style_width = zMapStyleGetWidth(feature->style) ;
      min_score = zMapStyleGetMinScore(feature->style) ;
      max_score = zMapStyleGetMaxScore(feature->style) ;

      if (min_score < 0 && 0 < max_score)
	origin = 0 ;
      else
	origin = style_width * (min_score / (max_score - min_score)) ;


      /* Adjust width to score....NOTE that to do acedb like stuff we really need to set an origin
	 and pass it in to this routine....*/
      width = getWidthFromScore(feature->style, feature->score) ;

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
  else
    {
      gboolean status ;

      status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
				   &background, &foreground, &outline) ;
      zMapAssert(status) ;

      feature_item = zMapDrawBox(parent,
				 x1, y1, x2, y2,
				 outline, background, line_width) ;
    }

  callItemHandler(factory, feature_item,
                  ITEM_FEATURE_SIMPLE,
                  feature, NULL, y1, y2);

  return feature_item ;
}

static FooCanvasItem *drawAlignFeature(RunSet run_data, ZMapFeature feature,
                                       double feature_offset,
                                       double x1, double y1, double x2, double y2,
                                       ZMapFeatureTypeStyle style)
{
  FooCanvasItem *feature_item = NULL ;
  gboolean status ;
  ZMapWindowFToIFactory factory = run_data->factory;
  ZMapFeatureBlock        block = run_data->block;
  FooCanvasGroup        *parent = run_data->container;
  double offset ;
  int i ;
  GdkColor *outline = NULL, *foreground = NULL, *background = NULL ;
  guint line_width;
  static gboolean colour_init = FALSE ;
  static GdkColor perfect, colinear, noncolinear ;
  char *perfect_colour = ZMAP_WINDOW_MATCH_PERFECT ;
  char *colinear_colour = ZMAP_WINDOW_MATCH_COLINEAR ;
  char *noncolinear_colour = ZMAP_WINDOW_MATCH_NOTCOLINEAR ;
  unsigned int match_threshold = 0 ;


  /* Get the colours, it's an error at this stage if none set we don't draw. */
  /* We could speed performance by caching these colours and only changing them when the style
   * quark changes. */
  status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
			       &background, &foreground, &outline) ;
  zMapAssert(status) ;					    /* If we are here, style should be ok for display. */


  if (!colour_init)
    {
      gdk_color_parse(perfect_colour, &perfect) ;
      gdk_color_parse(colinear_colour, &colinear) ;
      gdk_color_parse(noncolinear_colour, &noncolinear) ;

      colour_init = TRUE ;
    }

  zMapStyleGetGappedAligns(style, &match_threshold) ;


  line_width = factory->line_width;

  if (feature->feature.homol.align)
    {
      double feature_start, feature_end;
      FooCanvasItem *lastBoxWeDrew   = NULL;
      FooCanvasGroup *feature_group  = NULL;
      ZMapAlignBlock prev_align_span = NULL;
      double dimension = 1.5 ;
      double limits[]       = {0.0, 0.0, 0.0, 0.0};
      double points_inout[] = {0.0, 0.0, 0.0, 0.0};

      limits[1] = block->block_to_sequence.q1;
      limits[3] = block->block_to_sequence.q2;

      feature_start = feature->x1;
      feature_end   = feature->x2; /* I want this function to find these values */
      zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;
      feature_item  = createParentGroup(parent, feature, feature_start);
      feature_group = FOO_CANVAS_GROUP(feature_item);

      /* Calculate total offset for for subparts of the feature. */
      offset = feature_start + feature_offset ;

      for (i = 0 ; i < feature->feature.homol.align->len ; i++)  
        {
          ZMapAlignBlock align_span;
          FooCanvasItem *align_box, *gap_line, *gap_line_box;
          double top, bottom, dummy = 0.0;
          ZMapWindowItemFeature box_data, gap_data, align_data ;
          
          align_span = &g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i) ;   
          top    = points_inout[1] = align_span->t1;
          bottom = points_inout[3] = align_span->t2;

          if((factory->user_funcs->feature_size_request)(feature, &limits[0], &points_inout[0], factory->user_data))
            continue;

          top    = points_inout[1];
          bottom = points_inout[3];

          align_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	  align_data->subpart = ZMAPFEATURE_SUBPART_MATCH ;
          align_data->start = align_span->t1 ;
          align_data->end   = align_span->t2 ;

          zmapWindowSeq2CanOffset(&top, &bottom, offset) ;

          if(prev_align_span)
            {
              int q_indel, t_indel;
              t_indel = align_span->t1 - prev_align_span->t2;
              /* For query the current and previous can be reversed
               * wrt target which is _always_ consecutive. */
              if((align_span->t_strand == ZMAPSTRAND_REVERSE && 
                  align_span->q_strand == ZMAPSTRAND_FORWARD) || 
                 (align_span->t_strand == ZMAPSTRAND_FORWARD &&
                  align_span->q_strand == ZMAPSTRAND_REVERSE))
                q_indel = prev_align_span->q1 - align_span->q2;
              else
                q_indel = align_span->q1 - prev_align_span->q2;
              
#ifdef RDS_THESE_NEED_A_LITTLE_BIT_OF_CHECKING
              /* I'm not 100% sure on some of this. */
                {
                  if(align_span->q_strand == ZMAPSTRAND_REVERSE)
                    printf("QR TF Find %s\n", g_quark_to_string(feature->unique_id));
                  else
                    printf("QF TR Find %s\n", g_quark_to_string(feature->unique_id));
                }
                {               /* Especially REVERSE REVERSE. */
                  if(align_span->t_strand == ZMAPSTRAND_REVERSE &&
                     align_span->q_strand == ZMAPSTRAND_REVERSE)
                    printf("QR TR Find %s\n", g_quark_to_string(feature->unique_id));
                }
#endif /* RDS_THESE_NEED_A_LITTLE_BIT_OF_CHECKING */

              if(q_indel >= t_indel) /* insertion in query, expand align and annotate the insertion */
                {
                  FooCanvasItem *tmp = NULL;
                  tmp = zMapDrawAnnotatePolygon(lastBoxWeDrew,
                                                ZMAP_ANNOTATE_EXTEND_ALIGN, 
                                                NULL,
                                                foreground,
                                                bottom,
                                                line_width,
                                                feature->strand);
                  /* we need to check here that the item hasn't been
                   * expanded to be too long! This will involve
                   * updating the rather than inserting as the check
                   * code currently does */
                  /* Really I want to display the correct width in the target here.
                   * Not sure on it's effect re the cap/join style.... 
                   * t_indel may always be 1, but possible that this is not the case, 
                   * code allows it not to be.
                   */
                  /* using bottom twice here is wrong */
                  if(tmp != NULL && tmp != lastBoxWeDrew)
                    callItemHandler(factory, tmp, ITEM_FEATURE_CHILD, feature, align_data, bottom, bottom);
                }
              else
                {
		  GdkColor *line_colour ;
		  int diff ;

		  /* Colour gap lines according to colinearity. */
		  diff = abs(prev_align_span->q2 - align_span->q1) - 1 ;
		  if (diff > match_threshold)
		    {
		      if (align_span->q1 < prev_align_span->q2)
			line_colour = &noncolinear ;
		      else
			line_colour = &colinear ;
		    }
		  else
		    line_colour = &perfect ;


                  lastBoxWeDrew = align_box = zMapDrawSSPolygon(feature_item, ZMAP_POLYGON_SQUARE,
								x1, x2,
								top, bottom,
								outline, background,
								line_width,
								feature->strand) ;
                  callItemHandler(factory, align_box, ITEM_FEATURE_CHILD, feature, align_data, y1, y2);

                  box_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
		  box_data->subpart = ZMAPFEATURE_SUBPART_GAP ;
                  box_data->start = align_span->t1 ;
                  box_data->end   = prev_align_span->t2 ;

                  gap_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
		  gap_data->subpart = ZMAPFEATURE_SUBPART_GAP ;
                  gap_data->start = align_span->t1 ;
                  gap_data->end   = prev_align_span->t2 ;

                  bottom = prev_align_span->t2;
                  zmapWindowSeq2CanOffset(&dummy, &bottom, offset);
                  gap_line_box = zMapDrawSSPolygon(feature_item, ZMAP_POLYGON_SQUARE,
                                                   x1, x2,
                                                   bottom, top,
                                                   NULL,NULL,
						   line_width,
                                                   feature->strand);
                  callItemHandler(factory, gap_line_box, ITEM_FEATURE_BOUNDING_BOX, feature, box_data, bottom, top);

                  gap_line = zMapDrawAnnotatePolygon(gap_line_box,
                                                     ZMAP_ANNOTATE_GAP, 
                                                     NULL,
                                                     line_colour,
						     dimension,
                                                     line_width,
						     feature->strand);
                  callItemHandler(factory, gap_line, ITEM_FEATURE_CHILD, feature, gap_data, bottom, top);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		  /* Why does it do this....???????????? it doesn't seem to have any good effect... */

                  foo_canvas_item_lower(gap_line, 2);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

                  gap_data->twin_item = gap_line_box;
                  box_data->twin_item = gap_line;
                }
            }
          else
            {
              /* Draw the align match box */
              lastBoxWeDrew =
                align_box = zMapDrawSSPolygon(feature_item, ZMAP_POLYGON_SQUARE,
                                              x1, x2,
                                              top, bottom,
                                              outline, background,
					      line_width,
                                              feature->strand);
              callItemHandler(factory, align_box, ITEM_FEATURE_CHILD, feature, align_data, top, bottom);
              /* Finished with the align match */
            }

          prev_align_span = align_span;
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
  


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (strcmp(g_quark_to_string(zMapStyleGetID(style)), "Known_CDS") == 0)
    printf("found it\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          if ((exon_fill = background) == NULL)
            exon_outline = outline;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  exon_fill = background ;
	  exon_outline = outline ;
        }
      else
        {
          /* At least some part of the exon is CDS... */
          exon_data_ptr->subpart = ZMAPFEATURE_SUBPART_EXON_CDS ;
          

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          if ((exon_fill = background) == NULL)
            exon_outline = foreground;
          else
            exon_fill = foreground;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  exon_fill = cds_fill ;
	  exon_outline = cds_border ;
        }
    }
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  else if ((exon_fill = background) == NULL)
    {
      exon_outline = outline;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
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


  status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
			       &background, &foreground, &outline) ;
  zMapAssert(status) ;


  if (feature->feature.transcript.flags.cds)
    has_cds = TRUE ;

  if (has_cds)
    {
      /* cds will default to normal colours if its own colours are not set. */
      status = zMapStyleGetColoursCDSDefault(style, &cds_fill, &cds_draw, &cds_border) ;
      zMapAssert(status) ;
    }


  line_width = factory->line_width ;



  if (background)
    intron_fill = background ;
  else
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
#warning RDS_FIX_THIS
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
                                               1, feature_block->sequence.sequence);

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
                                               3, feature->text);

  return text_item_parent;
}

static void drawTextFeatureWithIterator(ZMapWindowFToIFactory factory,
                                        FooCanvasGroup *parent,
                                        ZMapFeature feature,
                                        ZMapWindowItemTextIterator iterator, 
                                        char *text_input)
{
  ZMapWindowItemFeature feature_data;
  FooCanvasItem *item;
  int curr_idx = iterator->index_start;
  int char_count = iterator->truncate_at;
  GString *text_string = iterator->row_text;
  char min_ascii, max_ascii;
  char *text_to_display = NULL;
  char *elipsis = &(iterator->elipsis[0]);
  double x = 0.0;
  double y;
  int i, elipsis_len, last_pop_cell;
  GdkColor *text_color;


  min_ascii = ('a' < 'A' ? 'a' : 'A');
  min_ascii = ('*' < min_ascii ? '*' : min_ascii);
  max_ascii = ('z' > 'Z' ? 'z' : 'Z');
  max_ascii = ('*' > max_ascii ? '*' : max_ascii);
  elipsis_len = strlen(elipsis);

  g_string_truncate(text_string, 0);

  last_pop_cell = iterator->last_populated_cell + iterator->index_start;

  text_color = iterator->text_colour;

  for(i = 0; i < iterator->rows; i++)
    {
      if((curr_idx = ((i * iterator->cols) + iterator->index_start)) <= last_pop_cell)
        text_to_display = &text_input[curr_idx];
      else if((last_pop_cell - curr_idx) < iterator->truncate_at)
        {
          text_to_display = &text_input[curr_idx];
          char_count = last_pop_cell - curr_idx;
        }
      else
        {
          text_to_display = NULL;
        }

      /* attempt to catch this, but still not supremely robust */
      if(!text_to_display || 
         !*text_to_display ||
         text_to_display[0] > max_ascii || 
         text_to_display[0] < min_ascii)
        {
          text_to_display = "! input overflow !";
          char_count = strlen(text_to_display);
        }

      g_string_append_len(text_string, text_to_display, char_count);

      if(iterator->truncated && char_count == iterator->truncate_at)
        {
          g_string_append_len(text_string, elipsis, elipsis_len);
        }

      /* offset_start was index_start, but that's wrong which chars_per_base != 1 */
      /* if offset_start off by 1? */
      y = iterator->real_char_height * i + iterator->offset_start;

      if(0)
        printf("curr_idx = %d, y = %f\n", curr_idx, y);

      if((item = zMapDrawTextWithFont(parent, text_string->str, 
                                      factory->font_desc, x, y,
                                      text_color)))
        {
          feature_data = g_new0(ZMapWindowItemFeatureStruct, 1);
          callItemHandler(factory, item, ITEM_FEATURE_CHILD, feature, feature_data, 0.0, 10.0);
        }
      
      g_string_truncate(text_string, 0);
    }

  return ;
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

static gboolean item_to_char_cell_coords(FooCanvasPoints **points_out, FooCanvasItem *subject, gpointer user_data)
{
  FooCanvasItem *text_item = FOO_CANVAS_ITEM(user_data);
  gboolean redraw = TRUE;
  FooCanvasPoints *points = NULL;
  ZMapFeature feature;
  ZMapWindowItemFeature item_feature;
  ZMapWindowItemFeatureType item_type;
  ZMapWindowItemTextContext context;

  if((feature = g_object_get_data(G_OBJECT(subject), ITEM_FEATURE_DATA)))
    {
      double first[4], last[4];
      gboolean first_shown = FALSE, last_shown = FALSE;

      points = foo_canvas_points_new(8);

      if((item_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(subject), ITEM_FEATURE_TYPE))) &&
         (context = g_object_get_data(G_OBJECT(text_item), ITEM_FEATURE_TEXT_DATA)))
        {
          switch(item_type)
            {
            case ITEM_FEATURE_SIMPLE:
            case ITEM_FEATURE_PARENT:
              first_shown = zmapWindowItemTextIndexGetBounds(context, feature->x1, &first[0]);
              last_shown  = zmapWindowItemTextIndexGetBounds(context, feature->x2, &last[0]);
              break;
            case ITEM_FEATURE_CHILD:
              if((item_feature = g_object_get_data(G_OBJECT(subject), ITEM_SUBFEATURE_DATA)))
                {
                  first_shown = zmapWindowItemTextIndexGetBounds(context, item_feature->start, &first[0]);
                  last_shown  = zmapWindowItemTextIndexGetBounds(context, item_feature->end,   &last[0]);
                }
              break;
            case ITEM_FEATURE_GROUP:
            case ITEM_FEATURE_GROUP_BACKGROUND:
            case ITEM_FEATURE_BOUNDING_BOX:
            default:
              zMapAssertNotReached();
              break;
            } /* switch(item_type) */
  
          if(first_shown || last_shown)
            {
              double default_x_max, default_x_min;
              /* Set the defaults from the knowledge the ItemText code has */
              zmapWindowItemTextGetWidthLimitBounds(context, 
                                                    &default_x_min, 
                                                    &default_x_max);

              if(first[1] > last[1])
                zMapAssertNotReached();

              /* We use the first and last cell coords to 
               * build the coords for the polygon.  We
               * also set defaults for the 4 widest x coords
               * as we can't know from the cell position how 
               * wide the column is.  The exception to this
               * is dealt with below, for the case when we're 
               * only overlaying across part or all of one 
               * row, where we don't want to extend to the 
               * edge of the column.
               */
              points->coords[0]    = 
                points->coords[14] = first[0];
              points->coords[1]    = 
                points->coords[3]  = first[1];
              points->coords[2]    =
                points->coords[4]  = default_x_max;
              points->coords[5]    = 
                points->coords[7]  = last[1];
              points->coords[6]    = 
                points->coords[8]  = last[2];
              points->coords[9]    =
                points->coords[11] = last[3];
              points->coords[10]   = 
                points->coords[12] = default_x_min;
              points->coords[13]   =
                points->coords[15] = first[3];

              /* Do some fixing of the default values if we're only on one row */
              if(first[1] == last[1])
                points->coords[2]   =
                  points->coords[4] = last[2];

              if(first[3] == last[3])
                points->coords[10]   =
                  points->coords[12] = first[0];
            }
          else
            {
              foo_canvas_points_free(points);
              points = NULL;
            }
        } /* if((item_type = g_object_get_data(G_OBJECT(subject), ITEM_FEATURE_TYPE))) */
      else
        {
          foo_canvas_points_free(points);
          points = NULL;
          zMapLogWarning("%s", "No text item context attached");
        }
    } /* if((feature = g_object_get_data(G_OBJECT(subject), ITEM_FEATURE_DATA))) */
  else
    {
      zMapLogWarning("%s", "No feature attached!");
    }

  if(points_out && points)
    *points_out = points;
  else
    redraw = FALSE;

  return redraw;
}

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
  GdkColor *outline = NULL, *foreground = NULL, *background = NULL;
  ZMapWindowItemTextContext text_context;
  ZMapWindowOverlay overlay_manager;
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

  feature_parent = createParentGroup(parent, feature, feature_start);

  my_foo_canvas_item_goto(feature_parent, &new_x, NULL);

  /* -------------------------------------------------
   * outline = highlight outline colour...
   * background = hightlight background colour...
   * foreground = text colour... 
   * -------------------------------------------------
   */
  /* what is parent's CONTAINER_TYPE_KEY and what is it's CONTAINER_DATA->level */

  column_parent = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(parent)) ;

  if(!zmapWindowContainerIsChildRedrawRequired(column_parent))
    {
      ZMapWindowFToIFactory zoom_data = NULL;

      zoom_data = zmapWindowFToIFactoryOpen(factory->ftoi_hash, factory->long_items);

      zmapWindowFToIFactorySetup(zoom_data, factory->line_width, 
                                 NULL,
                                 factory->user_funcs, factory->user_data);

      /* ZoomEventHandler is of wrong type !!! */
      zmapWindowContainerSetZoomEventHandler(column_parent, ZoomEventHandler, 
                                             (gpointer)zoom_data, ZoomDataDestroy);
    }

  if(!factory->font_desc)
    zMapFoocanvasGetTextDimensions(FOO_CANVAS_ITEM(feature_parent)->canvas, 
                                   &(factory->font_desc), 
                                   &(factory->text_width),
                                   &(factory->text_height));

  if(column_text)
    {
      ZMapWindowItemTextEnvironmentStruct env = {0};
      ZMapWindowItemTextIterator iterator;

      env.feature        = feature;
      env.s2c_offset     = feature_offset;
      env.visible_y1     = y1;
      env.visible_y2     = y2;
      env.canvas         = FOO_CANVAS(FOO_CANVAS_ITEM(feature_parent)->canvas);
      env.bases_per_char = bases_per_char;

      text_context = zmapWindowItemTextContextCreate(foreground);

      /* This gets destroyed along with the feature_parent */
      g_object_set_data_full(G_OBJECT(feature_parent), ITEM_FEATURE_TEXT_DATA, 
                             text_context, TextDataDestroy);

      zmapWindowItemTextContextInitialise(text_context, &env);

      if((iterator = zmapWindowItemTextContextGetIterator(text_context)))
        drawTextFeatureWithIterator(factory, FOO_CANVAS_GROUP(feature_parent), feature, 
                                    iterator, column_text);

    }

  /* This is attached to the column parent so needs updating each time
   * and doesn't get destroyed with the feature unlike the text
   * context */
  if((overlay_manager = g_object_get_data(G_OBJECT(column_parent), "OVERLAY_MANAGER")))
    {
      //zmapWindowOverlaySetLimitItem(overlay_manager, NULL);
      zmapWindowOverlaySetLimitItem(overlay_manager, feature_parent);
      zmapWindowOverlayUpdate(overlay_manager);
      zmapWindowOverlaySetSizeRequestor(overlay_manager, item_to_char_cell_coords, feature_parent);
    }
  else if((overlay_manager = zmapWindowOverlayCreate(FOO_CANVAS_ITEM(column_parent), NULL)))
    {
      g_object_set_data(G_OBJECT(column_parent), "OVERLAY_MANAGER", overlay_manager);

      zmapWindowOverlaySetSizeRequestor(overlay_manager, item_to_char_cell_coords, feature_parent);

      /* looks like the perfect time to set the colour. We'll use the selected fill colour */
      status = zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, 
                                   ZMAPSTYLE_COLOURTYPE_SELECTED,
                                   &background, NULL, NULL);

      /* It's very likely that this gets changed later, especially with the monkeying
       * that goes on in the zmapWindowFocusMaskOverlay, but more likely with the 
       * desired behaviour when we've move to zmap styles in 
       * zmapWindowFocus.c:mask_in_overlay */
      zmapWindowOverlaySetGdkColorFromGdkColor(overlay_manager, background);
    }

  return feature_parent;
}

static void TextDataDestroy(gpointer user_data)
{ 
  ZMapWindowItemTextContext context = (ZMapWindowItemTextContext)user_data;

  zmapWindowItemTextContextDestroy(context);

  return ;
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

