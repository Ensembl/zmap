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
 * Last edited: Feb 26 16:23 2007 (edgrif)
 * Created: Mon Sep 25 09:09:52 2006 (rds)
 * CVS info:   $Id: zmapWindowItemFactory.c,v 1.23 2007-03-01 09:53:15 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>
#include <zmapWindowItemFactory.h>

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

static void datalistRun(GQuark key_id, gpointer list_data, gpointer user_data);
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

static gboolean getTextOnCanvasDimensions(FooCanvas *canvas, 
                                          PangoFontDescription **font_desc_out,
                                          double *width_out,
                                          double *height_out);



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

  g_datalist_foreach(&(set->features), datalistRun, &run_data);

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
  int feature_type = (int)(feature->type);
  FooCanvasItem *item = NULL, *return_item = NULL;
  FooCanvasGroup *features_container = NULL;
  ZMapWindowItemFeatureSetData set_data = NULL;
  ZMapFeatureTypeStyle style = NULL;
  ZMapStyleMode style_mode = ZMAPSTYLE_MODE_NONE;
  GHashTable *style_table = NULL;
  gboolean no_points_in_block = TRUE;
  /* check here before they get called.  I'd prefer to only do this
   * once per factory rather than once per Run! */

  zMapAssert(factory->user_funcs && 
             factory->user_funcs->item_created &&
             factory->user_funcs->top_item_created);

  if(feature_type >=0 && feature_type <= FACTORY_METHOD_COUNT)
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

      switch(style_mode)
        {
        case ZMAPSTYLE_MODE_BASIC:
          method_table = factory->basic_methods;
          break;
        case ZMAPSTYLE_MODE_TRANSCRIPT:
        case ZMAPSTYLE_MODE_ALIGNMENT:
          printf("SORT THIS OUT ROY\n");
          zMapAssertNotReached();
          break;
        case ZMAPSTYLE_MODE_TEXT:
          method_table = factory->text_methods;
          break;
        case ZMAPSTYLE_MODE_GRAPH:
          method_table = factory->graph_methods;
          break;
        case ZMAPSTYLE_MODE_NONE:
        default:
          method_table = factory->methods;
          break;
        }

      switch(feature_type)
        {
        case ZMAPFEATURE_TRANSCRIPT:
          if(!feature->feature.transcript.introns && !feature->feature.transcript.introns)
            method = &(method_table[ZMAPFEATURE_BASIC]);
          else
            method = &(method_table[feature_type]);
          break;
        case ZMAPFEATURE_ALIGNMENT:
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
          else if(feature->feature.homol.align)
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
        case ZMAPFEATURE_BASIC:
        default:
          if(feature->flags.has_score)
            zmapWindowGetPosFromScore(style, feature->score, &(points[0]), &(points[2])) ;

          method = &(method_table[feature_type]);
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
  FooCanvasGroup *container_features = NULL;
  ZMapWindowItemFeatureSetData set_data;

  if(zmapWindowItemIsVisible(FOO_CANVAS_ITEM(container)) && 
     (set_data = (ZMapWindowItemFeatureSetData)g_object_get_data(G_OBJECT(container), ITEM_FEATURE_SET_DATA)))
    {
      tmp_factory = zmapWindowFToIFactoryOpen(factory_input->ftoi_hash, factory_input->long_items);
     
      feature_set = (ZMapFeatureSet)g_object_get_data(G_OBJECT(container), ITEM_FEATURE_DATA) ;
      /* Check it's actually a featureset */
      zMapAssert(feature_set && feature_set->struct_type == ZMAPFEATURE_STRUCT_FEATURESET) ;

      container_features = FOO_CANVAS_GROUP(zmapWindowContainerGetFeatures(container)) ;

      zmapWindowContainerPurge(container_features);

      zmapWindowFToIFactorySetup(tmp_factory, factory_input->line_width,
                                 (factory_input->stats_allocated ? NULL : factory_input->stats),
                                 factory_input->user_funcs, 
                                 factory_input->user_data);
      
      zmapWindowFToIFactoryRunSet(tmp_factory, feature_set, container, set_data->frame);
      
      zmapWindowFToIFactoryClose(tmp_factory);
    }

  return ;
}

static void datalistRun(GQuark key_id, gpointer list_data, gpointer user_data)
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
  GdkColor *outline, *foreground, *background;
  guint line_width;

  line_width = factory->line_width;

  zmapWindowSeq2CanOffset(&y1, &y2, feature_offset) ;	    /* Make sure we cover the whole last base. */

  if (feature->flags.has_boundary)
    {
      static GdkColor splice_background ;
      double width, origin, start ;
      double style_width, max_score, min_score ;
      ZMapDrawGlyphType glyph_type ;
      char *colour ;
      ZMapFrame frame ;

      frame = zmapWindowFeatureFrame(feature) ;

      /* colouring is temporary until I get styles fixed up....then we need to allow stuff like
       * splice/frame specific colouring.... */
      switch (frame)
	{
	case ZMAPFRAME_0:
	  colour = "red" ;
	  break ;
	case ZMAPFRAME_1:
	  colour = "blue" ;
	  break ;
	case ZMAPFRAME_2:
	  colour = "green" ;
	  break ;
	default:
	  zMapAssertNotReached() ;
	}
      gdk_color_parse(colour, &splice_background) ;

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
				   &splice_background, width, 2) ;

    }
  else
    {
      zMapFeatureTypeGetColours(style, &background, &foreground, &outline);

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
  ZMapWindowFToIFactory factory = run_data->factory;
  ZMapFeatureBlock        block = run_data->block;
  FooCanvasGroup        *parent = run_data->container;
  FooCanvasItem *feature_item ;
  double offset ;
  int i ;
  GdkColor *outline, *foreground, *background;
  guint line_width;
  static gboolean colour_init = FALSE ;
  static GdkColor perfect, colinear, noncolinear ;
  char *perfect_colour = ZMAP_WINDOW_MATCH_PERFECT ;
  char *colinear_colour = ZMAP_WINDOW_MATCH_COLINEAR ;
  char *noncolinear_colour = ZMAP_WINDOW_MATCH_NOTCOLINEAR ;
  unsigned int match_threshold = 0 ;


  if (strcasecmp(g_quark_to_string(zMapStyleGetID(style)), "ditag_GIS_PET") == 0)
    printf("found it\n") ;


  if (!colour_init)
    {
      gdk_color_parse(perfect_colour, &perfect) ;
      gdk_color_parse(colinear_colour, &colinear) ;
      gdk_color_parse(noncolinear_colour, &noncolinear) ;

      colour_init = TRUE ;
    }

  zMapStyleGetGappedAligns(style, &match_threshold) ;


  zMapFeatureTypeGetColours(style, &background, &foreground, &outline);
  line_width = factory->line_width;

  if(feature->feature.homol.align)
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

static FooCanvasItem *drawTranscriptFeature(RunSet run_data,  ZMapFeature feature,
                                            double feature_offset,
                                            double x1, double y1, double x2, double y2,
                                            ZMapFeatureTypeStyle style)
{
  ZMapWindowFToIFactory factory = run_data->factory;
  /*  ZMapFeatureBlock        block = run_data->block; */
  FooCanvasGroup        *parent = run_data->container;
  FooCanvasItem *feature_item ;
  GdkColor *outline, *foreground, *background;
  guint line_width;
  GdkColor *intron_fill = NULL;
  int i ;
  double offset ;


  zMapFeatureTypeGetColours(style, &background, &foreground, &outline);
  line_width = factory->line_width;

  if((intron_fill = background) == NULL)
    intron_fill   = outline;
  
  /* If there are no exons/introns then just draw a simple box. Can happen for putative
   * transcripts or perhaps where user has a single exon object but does not give an exon,
   * only an overall extent. */
  if (feature->feature.transcript.introns && feature->feature.transcript.exons)
    {
      double feature_start, feature_end;
      double cds_start = 0.0, cds_end = 0.0; /* cds_start < cds_end */
      FooCanvasGroup *feature_group ;
      gboolean has_cds = FALSE;
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
          GdkColor *exon_fill = background, *exon_outline = outline;

	  for (i = 0; i < feature->feature.transcript.exons->len; i++)
	    {
	      ZMapSpan exon_span ;
	      FooCanvasItem *exon_box ;
	      ZMapWindowItemFeature exon_data ;
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

              exon_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
              exon_data->start = exon_span->x1;
	      exon_data->end = exon_span->x2;

	      /* If any of the cds is in this exon we colour the whole exon as a cds and then
	       * overlay non-cds parts. For the cds part we use the foreground colour, else it's the background. 
               * If background is null though outline will need to be set to what background would be and 
               * background should then = NULL */
	      exon_data->subpart = ZMAPFEATURE_SUBPART_EXON ;
              if (has_cds)
		{
		  if ((cds_start > bottom) || (cds_end < top))
		    {
		      if((exon_fill = background) == NULL)
			exon_outline = outline;
		    }
		  else
		    {
		      /* At least some part of the exon is CDS... */
		      exon_data->subpart = ZMAPFEATURE_SUBPART_EXON_CDS ;

		      if ((exon_fill = background) == NULL)
			exon_outline = foreground;
		      else
			exon_fill = foreground;
		    }
		}
              else if ((exon_fill = background) == NULL)
                {
                  exon_outline = outline;
                }

              exon_box = zMapDrawSSPolygon(feature_item, 
                                           (zMapStyleIsDirectionalEnd(style) ? ZMAP_POLYGON_POINTING : ZMAP_POLYGON_SQUARE), 
                                           x1, x2,
                                           top, bottom,
                                           exon_outline, 
                                           exon_fill,
					   line_width,
                                           feature->strand);
              callItemHandler(factory, exon_box, ITEM_FEATURE_CHILD, feature, exon_data, top, bottom);

              if (has_cds)
		{
		  FooCanvasItem *utr_box = NULL ;
		  int non_start, non_end ;

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

                          /* set the cds box's exon_data while we still have ptr 2 it */
                          /* Without this the cds part retains the coords of the FULL exon! */
                          exon_data->start = non_end + offset;

			  exon_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
			  exon_data->subpart = ZMAPFEATURE_SUBPART_EXON ;
			  exon_data->start = non_start + offset ;
			  exon_data->end   = non_end + offset ;

                          callItemHandler(factory, utr_box, ITEM_FEATURE_CHILD, feature, exon_data, non_start, non_end);
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
			  non_start = bottom - (bottom - cds_end) + 1 ;
			  non_end = bottom ;

                          /* set the cds box's exon_data while we still have ptr 2 it */
                          /* Without this the cds part retains the coords of the FULL exon! */
                          exon_data->end = non_start + offset;

			  exon_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
			  exon_data->subpart = ZMAPFEATURE_SUBPART_EXON ;
			  exon_data->start = non_start + offset ;
			  exon_data->end   = non_end + offset - 1 ; /* - 1 because non_end
								       calculated from bottom
								       which covers the last base. */

                          callItemHandler(factory, utr_box, ITEM_FEATURE_CHILD, feature, exon_data, non_start, non_end);
			}
		    }
		}

            }
	}


    }

  return feature_item ;
}

static void ZoomDataDestroy(gpointer data)
{
  ZMapWindowFToIFactory factory = (ZMapWindowFToIFactory)data;
  
  zmapWindowFToIFactoryClose(factory);

  return ;
}

static void fuzzyTableDimensions(double region_range, double trunc_col, 
                                 double chars_per_base, int *row, int *col,
                                 double *height_inout, int *rcminusmod_out)
{
  double orig_height, final_height, fl_height, cl_height;
  double temp_rows, temp_cols, text_height, tmp_mod = 0.0;
  gboolean row_precedence = FALSE;
  int tmp;

  region_range--;
  region_range *= chars_per_base;

  orig_height = text_height = *height_inout * chars_per_base;
  temp_rows   = region_range / orig_height     ;
  fl_height   = region_range / (((tmp = floor(temp_rows)) > 0) ? tmp : 1);
  cl_height   = region_range / ceil(temp_rows) ;

  if(((region_range / temp_rows) > trunc_col))
    row_precedence = TRUE;
  
  if(row_precedence)            /* dot dot dot */
    {
      /* printf("row_precdence\n"); */
      if(cl_height < fl_height && ((fl_height - cl_height) < (orig_height / 8)))
        final_height = cl_height;
      else
        final_height = fl_height;

      temp_rows = floor(region_range / final_height);
      temp_cols = floor(final_height);
    }
  else                          /* column_precedence */
    {
      double tmp_rws, tmp_len, sos;
      /* printf("col_precdence\n"); */

      if((sos = orig_height - 0.5) <= floor(orig_height) && sos > 0.0)
        temp_cols = final_height = floor(orig_height);
      else
        temp_cols = final_height = ceil(orig_height);

      /*
      printf(" we're going to miss %f bases at the end if using %f cols\n", 
             (tmp_mod = fmod(region_range, final_height)), final_height);
      */

      if(final_height != 0.0 && tmp_mod != 0.0)
        {
          tmp_len = region_range + (final_height - fmod(region_range, final_height));
          tmp_rws = tmp_len / final_height;
          
          final_height = (region_range + 1) / tmp_rws;
          temp_rows    = tmp_rws;

          /* 
          printf("so modifying... tmp_len=%f, tmp_rws=%f, final_height=%f\n", 
                 tmp_len, tmp_rws, final_height);
          */
        }
      else if(final_height != 0.0)
        temp_rows = floor(region_range / final_height);

    }

  /* sanity */
  if(temp_rows > region_range)
    temp_rows = region_range;

#ifdef RDS_DONT_INCLUDE
  printf("rows=%f, height=%f, range=%f, would have been min rows=%f & height=%f, or orignal rows=%f & height=%f\n", 
         temp_rows, final_height, region_range,
         region_range / fl_height, fl_height,
         region_range / orig_height, orig_height);
#endif

  if(height_inout)
    *height_inout = text_height = final_height / chars_per_base;
  if(row)
    *row = (int)temp_rows;
  if(col)
    *col = (int)temp_cols;
  if(rcminusmod_out)
    *rcminusmod_out = (int)(temp_rows * temp_cols - tmp_mod);

  return ;
}



static ZMapDrawTextIterator zmapDrawTextIteratorBuild(double feature_start, double feature_end,
                                                      double feature_offset,
                                                      double scroll_start,  double scroll_end,
                                                      double text_width,    double text_height, 
                                                      char  *full_text,     int    bases_per_char,
                                                      gboolean numbered,
                                                      PangoFontDescription *font)
{
  int tmp;
  double column_width = 300.0, chars_per_base, feature_first_char, feature_first_base;
  ZMapDrawTextIterator iterator   = g_new0(ZMapDrawTextIteratorStruct, 1);

  column_width  /= bases_per_char;
  chars_per_base = 1.0 / bases_per_char;

  iterator->seq_start      = feature_start;
  iterator->seq_end        = feature_end;
  iterator->offset_start   = floor(scroll_start - feature_offset);

  feature_first_base  = floor(scroll_start) - feature_start;
  tmp                 = (int)feature_first_base % (int)bases_per_char;
  feature_first_char  = (feature_first_base - tmp) / bases_per_char;

  /* iterator->offset_start += tmp; */ /* Keep this in step with the bit above */

  iterator->x            = 0.0;
  iterator->y            = 0.0;

  iterator->char_width   = text_width;

  iterator->truncate_at  = floor(column_width / text_width);

  iterator->char_height = text_height;

  fuzzyTableDimensions((ceil(scroll_end) - floor(scroll_start) + 1.0), 
                       (double)iterator->truncate_at, 
                       chars_per_base,
                       &(iterator->rows), 
                       &(iterator->cols), 
                       &(iterator->char_height),
                       &(iterator->lastPopulatedCell));

  /* Not certain this column calculation is correct (chars_per_base bit) */
  if(iterator->cols > iterator->truncate_at)
    {
      iterator->truncated = TRUE;
      /* Just make the cols smaller and the number of rows bigger */
      iterator->cols *= chars_per_base;
    }
  else
    {
      iterator->truncate_at  = iterator->cols;
    }


  iterator->row_text   = g_string_sized_new(iterator->truncate_at);
  iterator->wrap_text  = full_text;
  /* make the index 0 based... */
  iterator->wrap_text += iterator->index_start = (int)feature_first_char - (bases_per_char == 1 ? 1 : 0);

  iterator->row_data   = g_new0(ZMapDrawTextRowDataStruct, iterator->rows);

  return iterator;
}


static void destroyIterator(ZMapDrawTextIterator iterator)
{
  zMapAssert(iterator);
  g_string_free(iterator->row_text, TRUE);
  iterator->row_data = NULL;
  g_free(iterator);

  return ;
}


static FooCanvasItem *drawSeqFeature(RunSet run_data,  ZMapFeature feature,
                                     double feature_offset,
                                     double x1, double y1, double x2, double y2,
                                     ZMapFeatureTypeStyle style)
{
  ZMapWindowFToIFactory  factory = run_data->factory;
  ZMapFeatureBlock feature_block = run_data->block;
  FooCanvasGroup         *parent = run_data->container;
  double feature_start, feature_end;
  FooCanvasItem  *feature_parent = NULL;
  FooCanvasGroup *column_parent  = NULL;
  ZMapWindowItemHighlighter hlght= NULL;
  GdkColor *outline, *foreground, *background;
  ZMapDrawTextIterator iterator  = NULL;
  double txt_height;
  int i;

  zMapFeatureTypeGetColours(style, &background, &foreground, &outline);

  feature_start  = feature->x1;
  feature_end    = feature->x2;

  zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;

  feature_parent = createParentGroup(parent, feature, feature_start);

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
                                 (factory->stats_allocated ? NULL : factory->stats),
                                 factory->user_funcs, factory->user_data);

      /* ZoomEventHandler is of WRONG type */
      zmapWindowContainerSetZoomEventHandler(column_parent, ZoomEventHandler, 
                                             (gpointer)zoom_data, ZoomDataDestroy);
    }
#ifdef RDS_BREAKING_STUFF
  if((hlght = zmapWindowItemTextHighlightCreateData(FOO_CANVAS_GROUP(feature_parent))))
    zmapWindowItemTextHighlightSetFullText(hlght, feature_block->sequence.sequence, FALSE);
#endif

  if(!factory->font_desc)
    getTextOnCanvasDimensions(FOO_CANVAS_ITEM(feature_parent)->canvas, 
                              &(factory->font_desc), 
                              &(factory->text_width),
                              &(factory->text_height));

  /* calculate text height in world coords! */
  txt_height = factory->text_height / (FOO_CANVAS(FOO_CANVAS_ITEM(feature_parent)->canvas)->pixels_per_unit_y);

  if(1)
    {
      iterator = zmapDrawTextIteratorBuild(feature_start, feature_end,
                                           feature_offset,
                                           y1, y2, 
                                           factory->text_width, txt_height, 
                                           feature_block->sequence.sequence, 1, 
                                           FALSE, factory->font_desc);
      
      iterator->foreground = foreground;
      iterator->background = background;
      iterator->outline    = outline;
      
#ifdef RDS_THESE_NEED_A_LITTLE_BIT_OF_CHECKING
      printf("drawTextWrappedInColumn: start=%f, end=%f, y1=%f, y2=%f\n",
             feature_start, feature_end, y1, y2);
#endif

      for(i = 0; i < iterator->rows; i++)
        {
          FooCanvasItem *item = NULL;
          iterator->iteration = i;
          
          if((item = zMapDrawRowOfText(FOO_CANVAS_GROUP(feature_parent), 
                                       factory->font_desc, 
                                       feature_block->sequence.sequence, 
                                       iterator)))
            {
              ZMapWindowItemFeature dna_data;
              dna_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
              callItemHandler(factory, item, ITEM_FEATURE_CHILD, feature, dna_data, 0.0, 10.0);
            }
        }
      
      destroyIterator(iterator);

    }

  return feature_parent;
}

static FooCanvasItem *drawPepFeature(RunSet run_data,  ZMapFeature feature,
                                     double feature_offset,
                                     double x1, double y1, double x2, double y2,
                                     ZMapFeatureTypeStyle style)
{
  ZMapWindowFToIFactory  factory = run_data->factory;
  /* ZMapFeatureBlock feature_block = run_data->block; */
  FooCanvasGroup         *parent = run_data->container;
  double feature_start, feature_end;
  FooCanvasItem  *prev_trans     = NULL;
  FooCanvasItem  *feature_parent = NULL;
  FooCanvasGroup *column_parent  = NULL;
  GdkColor *outline, *foreground, *background;
  ZMapDrawTextIterator iterator  = NULL;
  double txt_height, new_x;
  int i;

  zMapFeatureTypeGetColours(style, &background, &foreground, &outline);

  feature_start  = feature->x1;
  feature_end    = feature->x2;

  zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;

    /* bump the feature BEFORE drawing */
  if(parent->item_list_end && (prev_trans = FOO_CANVAS_ITEM(parent->item_list_end->data)))
    foo_canvas_item_get_bounds(prev_trans, NULL, NULL, &new_x, NULL);
  
  feature_parent = createParentGroup(parent, feature, feature_start);

  new_x += COLUMN_SPACING;
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

#ifdef RDS_THIS_BREAKS_STUFF__WHY
  if((hlght = zmapWindowItemTextHighlightCreateData(FOO_CANVAS_GROUP(feature_parent))))
    zmapWindowItemTextHighlightSetFullText(hlght, feature_block->sequence.sequence, FALSE);
#endif

  if(!factory->font_desc)
    getTextOnCanvasDimensions(FOO_CANVAS_ITEM(feature_parent)->canvas, 
                              &(factory->font_desc), 
                              &(factory->text_width),
                              &(factory->text_height));

  /* calculate text height in world coords! */
  txt_height = factory->text_height / (FOO_CANVAS(FOO_CANVAS_ITEM(feature_parent)->canvas)->pixels_per_unit_y);

  if(1)
    {
      iterator = zmapDrawTextIteratorBuild(feature_start, feature_end,
                                           feature_offset,
                                           y1, y2, 
                                           factory->text_width, txt_height, 
                                           feature->text, 3, 
                                           FALSE, factory->font_desc);
      
      iterator->foreground = foreground;
      iterator->background = background;
      iterator->outline    = outline;
      
#ifdef RDS_THESE_NEED_A_LITTLE_BIT_OF_CHECKING
      printf("drawTextWrappedInColumn: start=%f, end=%f, y1=%f, y2=%f\n",
             feature_start, feature_end, y1, y2);
#endif

      for(i = 0; i < iterator->rows; i++)
        {
          FooCanvasItem *item = NULL;
          iterator->iteration = i;
          
          if((item = zMapDrawRowOfText(FOO_CANVAS_GROUP(feature_parent), 
                                       factory->font_desc, 
                                       feature->text, 
                                       iterator)))
            {
              ZMapWindowItemFeature dna_data;
              dna_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
              callItemHandler(factory, item, ITEM_FEATURE_CHILD, feature, dna_data, 0.0, 10.0);
            }
        }
      
      destroyIterator(iterator);

    }

  return feature_parent;
}

static FooCanvasItem *invalidFeature(RunSet run_data,  ZMapFeature feature,
                                     double feature_offset,
                                     double x1, double y1, double x2, double y2,
                                     ZMapFeatureTypeStyle style)
{

  zMapAssertNotReached();

  return NULL;
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
  FooCanvasGroup *parent = run_data->container ;
  FooCanvasItem *feature_item ;
  GdkColor *outline, *foreground, *background ;
  guint line_width ;
  double numerator, denominator, dx ;
  double width, max_score, min_score ;

  width = zMapStyleGetWidth(style) ;
  min_score = zMapStyleGetMinScore(style) ;
  max_score = zMapStyleGetMaxScore(style) ;

  line_width = factory->line_width;

  zmapWindowSeq2CanOffset(&y1, &y2, feature_offset) ;	    /* Make sure we cover the whole last base. */

  zMapFeatureTypeGetColours(style, &background, &foreground, &outline) ;

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


static gboolean getTextOnCanvasDimensions(FooCanvas *canvas, 
                                          PangoFontDescription **font_desc_out,
                                          double *width_out,
                                          double *height_out)
{
  FooCanvasItem *tmp_item = NULL;
  FooCanvasGroup *root_grp = NULL;
  PangoFontDescription *font_desc = NULL;
  PangoLayout *layout = NULL;
  PangoFont *font = NULL;
  gboolean success = FALSE;
  double width, height;
  int  iwidth, iheight;
  width = height = 0.0;
  iwidth = iheight = 0;

  root_grp = foo_canvas_root(canvas);

  /*

  layout = gtk_widget_create_pango_layout(GTK_WIDGET (canvas), NULL);
  pango_layout_set_font_description (layout, font_desc);
  pango_layout_set_text(layout, text, -1);
  pango_layout_set_alignment(layout, align);
  pango_layout_get_pixel_size(layout, &iwidth, &iheight);

  */

  if(!zMapGUIGetFixedWidthFont(GTK_WIDGET(canvas), 
                               g_list_append(NULL, "Monospace"), 10, PANGO_WEIGHT_NORMAL,
                               &font, &font_desc))
    printf("Couldn't get fixed width font\n");
  else
    {
      tmp_item = foo_canvas_item_new(root_grp,
                                     foo_canvas_text_get_type(),
                                     "x",         0.0,
                                     "y",         0.0,
                                     "text",      "A",
                                     "font_desc", font_desc,
                                     NULL);
      layout = FOO_CANVAS_TEXT(tmp_item)->layout;
      pango_layout_get_pixel_size(layout, &iwidth, &iheight);
      width  = (double)iwidth;
      height = (double)iheight;

      gtk_object_destroy(GTK_OBJECT(tmp_item));
      success = TRUE;
    }

  if(width_out)
    *width_out  = width;
  if(height_out)
    *height_out = height;
  if(font_desc_out)
    *font_desc_out = font_desc;

  return success;
}
