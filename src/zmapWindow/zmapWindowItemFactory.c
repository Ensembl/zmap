 /*  File: zmapWindowItemFactory.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Factory-object based set of functions for drawing
 *              features.
 *
 * Exported functions: See zmapWindowItemFactory.h
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <string.h>
#include <gtk/gtk.h>

#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapStyle.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowItemFactory.h>

#include <zmapWindowCanvasItem.h>

#if 0
//#include <zmapWindowCanvasFeatureset.h>
#include <zmapWindowCanvasFeatureset_I.h>	// for debug

// temp include for timing test
#include <zmapWindowCanvasItem_I.h>
#endif

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
  guint line_width ;
  ZMapWindowFToIFactoryMethodsStruct *methods;
  GHashTable                         *ftoi_hash;
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
  ZMapWindowFeatureStack      feature_stack;
  ZMapFrame             frame;
  FooCanvasGroup       *container;
  FooCanvasItem        *canvas_item;
  GQuark                data_id;    /* used by composite objectes eg density plot */
} RunSetStruct ;


/* Noddy macro to calculate the index of sub-parts of features like exons, e.g. Exon 1, 2 etc. */
#define GET_SUBPART_INDEX(REVCOMP, FEATURE, SUBPART_ARRAY, ARRAY_INDEX)	\
  (REVCOMP \
   ? ((FEATURE)->strand == ZMAPSTRAND_FORWARD ? (SUBPART_ARRAY)->len - (ARRAY_INDEX) : (ARRAY_INDEX) + 1) \
   : ((FEATURE)->strand == ZMAPSTRAND_FORWARD ? (ARRAY_INDEX) + 1 : (SUBPART_ARRAY)->len - (ARRAY_INDEX)) )


static void datalistRun(gpointer key, gpointer list_data, gpointer user_data);



static gboolean null_top_item_created(FooCanvasItem *top_item,
                                      ZMapWindowFeatureStack feature_stack,
                                      gpointer handler_data);
static gboolean null_feature_size_request(ZMapFeature feature,
                                          double *limits_array,
                                          double *points_array_inout,
                                          gpointer handler_data);


static FooCanvasItem *drawFeaturesetFeature(RunSet run_data, ZMapFeature feature,
                              double feature_offset,
					double x1, double y1, double x2, double y2,
                              ZMapFeatureTypeStyle style);


/*
 * Static function tables for drawing features in various ways.
 */



ZMapWindowFToIFactory zmapWindowFToIFactoryOpen(GHashTable *feature_to_item_hash)
{
  ZMapWindowFToIFactory factory = NULL;
  ZMapWindowFToIFactoryProductionTeam user_funcs = NULL;

  if ((factory = g_new0(ZMapWindowFToIFactoryStruct, 1)) &&
     (user_funcs = g_new0(ZMapWindowFToIFactoryProductionTeamStruct, 1)))
    {
      factory->ftoi_hash  = feature_to_item_hash;

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

  if(signal_handlers->top_item_created)
    factory->user_funcs->top_item_created = signal_handlers->top_item_created;

  if(signal_handlers->feature_size_request)
    factory->user_funcs->feature_size_request = signal_handlers->feature_size_request;

  factory->user_data  = handler_data;
  factory->line_width = line_width;

  return ;
}


/* mh17: only called from windowNavigator(), no doubt here due to scope issues
 * but what scope issues? factory does not look static
 */

gboolean is_nav = FALSE;
void zmapWindowFToIFactoryRunSet(ZMapWindowFToIFactory factory,
                                 ZMapFeatureSet set,
                                 FooCanvasGroup *container,
                                 ZMapFrame frame)
{
  RunSetStruct run_data = {NULL};
  ZMapWindowFeatureStackStruct feature_stack;

  is_nav = TRUE;

  run_data.factory = factory;
  /*
    run_data.container = canZMapWindowFToIFactoryDoThis_ShouldItBeAParameter();
  */
  run_data.container   = container;

  run_data.feature_stack     = &feature_stack;
  feature_stack.feature = NULL;
  zmapGetFeatureStack(&feature_stack,set,NULL,frame);

  run_data.frame   = frame;

  g_hash_table_foreach(set->features, datalistRun, &run_data);

  is_nav = FALSE;
  return ;
}

#define MH17_REVCOMP_DEBUG	0

/* re-using a factory helper to scale a navigator coordinate */
/* NOTE all factory helpers must handle a NULL feature arg */
void getFactoriedCoordinates(ZMapWindowFToIFactory factory, int *y1, int *y2)
{
      double limits[4] = { 0 };
      double points[4] = { 0 };

      limits[1] = *y1;
      limits[3] = *y2;

      points[1] = *y1;
      points[3] = *y2;
      (factory->user_funcs->feature_size_request)(NULL,&limits[0], &points[0], factory->user_data);
      *y1 = (int) points[1];
      *y2 = (int) points[3];
}

FooCanvasItem *zmapWindowFToIFactoryRunSingle(ZMapWindowFToIFactory factory,
					      FooCanvasItem        *current_item,
                                              FooCanvasGroup       *parent_container,
                                              ZMapWindowFeatureStack     feature_stack)
{
  RunSetStruct run_data = {NULL};
  FooCanvasItem *item = NULL, *return_item = NULL;
  FooCanvasGroup *features_container = NULL;
  gboolean no_points_in_block = TRUE;
  ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet) parent_container;
  ZMapFeature feature = feature_stack->feature;

#if MH17_REVCOMP_DEBUG > 1
        zMapLogWarning("run_single ","");
#endif


  /* check here before they get called.  I'd prefer to only do this
   * once per factory rather than once per Run! */
  zMapAssert(factory->user_funcs && factory->user_funcs->top_item_created) ;


#ifndef RDS_REMOVED_STATS
//  ZMapWindowItemFeatureSetData set_data = NULL;
  ZMapWindowStats parent_stats ;

  parent_stats = g_object_get_data(G_OBJECT(parent_container), ITEM_FEATURE_STATS) ;

//  set_data = g_object_get_data(G_OBJECT(parent_container), ITEM_FEATURE_SET_DATA) ;
//  zMapAssert(set_data) ;
#endif

  if (!(feature->style))
    {
      zMapLogWarning("Feature %s cannot be drawn because it has no style.", (char *) g_quark_to_string(feature->original_id)) ;
    }
  else if (!zMapStyleHasDrawableMode(feature->style))
    {
      ZMapStyleMode style_mode ;

      style_mode = zMapStyleGetMode(feature->style) ;


      zMapLogWarning("Feature %s cannot be drawn because it has mode %s",
		     (char *)g_quark_to_string(feature->original_id), zMapStyleMode2ExactStr(style_mode)) ;
    }
  else
    {
      ZMapFeatureTypeStyle style ;
      ZMapStyleMode style_mode ;
      double *limits = &(factory->limits[0]);
      double *points = &(factory->points[0]);
      ZMapWindowStats stats ;

      style = feature->style ;
      style_mode = zMapStyleGetMode(style) ;


      stats = g_object_get_data(G_OBJECT(parent_container), ITEM_FEATURE_STATS) ;

#ifndef RDS_REMOVED_STATS
      zMapAssert(stats) ;

      zmapWindowStatsAddChild(stats, (ZMapFeatureAny)feature) ;
#endif


      features_container = (FooCanvasGroup *)zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)parent_container) ;

      if (zMapStyleIsStrandSpecific(style)
          && (feature->strand == ZMAPSTRAND_REVERSE && !zMapStyleIsShowReverseStrand(style)))
	{
#if MH17_REVCOMP_DEBUG > 2
	  printf("not reverse\n");
#endif
	  goto undrawn ;
	}

      /* Note: for object to _span_ "width" units, we start at zero and end at "width". */
      limits[0] = 0.0;
      limits[1] = (double)(feature_stack->block->block_to_sequence.block.x1);
      limits[2] = zMapStyleGetWidth(style);
      limits[3] = (double)(feature_stack->block->block_to_sequence.block.x2);

      /* Set these to this for normal main window use. */
      points[0] = 0.0;
      points[1] = feature->x1;
      points[2] = zMapStyleGetWidth(style);
      points[3] = feature->x2;


      /* inline static void normalSizeReq(ZMapFeature f, ... ){ return ; } */
      /* inlined this should be nothing most of the time, question is can it be inlined?? */

	/* mh17: NOTE this is very confusing
	 * the navigator used this function to say 'outside the block' when it really means 'the name is duplicated'
	 * incredible, despite knowing that 'size request' means 'is_duiplicated?' it still fools me
	 */
      if ((no_points_in_block = (factory->user_funcs->feature_size_request)(feature,
									    &limits[0], &points[0],
									    factory->user_data)) == TRUE)
	{
#if MH17_REVCOMP_DEBUG > 2
	  printf("no points in block\n");
#endif
	  goto undrawn ;
	}



      /* NOTE TO ROY: I have probably not got the best logic below...I am sure that we
       * have not handled/thought about all the combinations of style modes and feature
       * types that might occur so will need to revisit this.... */
      switch(style_mode)
        {
        case ZMAPSTYLE_MODE_BASIC:
        case ZMAPSTYLE_MODE_GLYPH:
	  {
#ifndef RDS_REMOVED_STATS
	    ZMapWindowStatsBasic stats ;
	    stats = zmapWindowStatsAddBasic(parent_stats, feature) ;
	    stats->features++ ;
	    stats->items++ ;
#endif
	    if(feature->flags.has_score)
	      zmapWindowGetPosFromScore(style, feature->score, &(points[0]), &(points[2])) ;


	    break;
	  }

        case ZMAPSTYLE_MODE_ALIGNMENT:
	  {
#ifndef RDS_REMOVED_STATS
	    ZMapWindowStatsAlign stats ;
	    stats = zmapWindowStatsAddAlign(parent_stats, feature) ;
	    stats->total_matches++ ;
#endif
	    if ((zMapStyleGetScoreMode(style) == ZMAPSCORE_WIDTH && feature->flags.has_score))
            zmapWindowGetPosFromScore(style, feature->score, &(points[0]), &(points[2])) ;
          else if(zMapStyleGetScoreMode(style) == ZMAPSCORE_PERCENT)
          {
            /* if is homol then must have percent, see gff2parser getHomolAttrs() */
            zmapWindowGetPosFromScore(style, feature->feature.homol.percent_id, &(points[0]), &(points[2])) ;
          }



	    if (!zMapStyleIsShowGaps(style) || !(feature->feature.homol.align))
	      {
#ifndef RDS_REMOVED_STATS
		stats->ungapped_matches++;
		stats->ungapped_boxes++;
		stats->total_boxes++;
#endif
	      }
	    else
	      {
		if (feature->feature.homol.flags.perfect)
		  {
#ifndef RDS_REMOVED_STATS
		    stats->gapped_matches++;
		    stats->total_boxes  += feature->feature.homol.align->len ;
		    stats->gapped_boxes += feature->feature.homol.align->len ;
#endif
		  }
		else
		  {
#ifndef RDS_REMOVED_STATS
		    stats->not_perfect_gapped_matches++;
		    stats->total_boxes++;
		    stats->ungapped_boxes++;
		    stats->imperfect_boxes += feature->feature.homol.align->len;
#endif
		  }

	      }

	    break;
	  }

        case ZMAPSTYLE_MODE_TEXT:
	  {
#ifdef RDS_REMOVED_STATS
	    ZMapWindowStatsBasic stats ;
	    stats = zmapWindowStatsAddBasic(parent_stats, feature) ;
	    stats->features++ ;
	    stats->items++ ;
#endif
	    break;
	  }

        case ZMAPSTYLE_MODE_GRAPH:
	  {
#ifdef RDS_REMOVED_STATS
	    ZMapWindowStatsBasic stats ;
	    stats = zmapWindowStatsAddBasic(parent_stats, feature) ;
	    stats->features++ ;
	    stats->items++ ;
#endif
	    break;
	  }


        case ZMAPSTYLE_MODE_TRANSCRIPT:
	  {
#ifdef RDS_REMOVED_STATS
	    ZMapWindowStatsTranscript stats ;
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
	    break ;
	  }

	case ZMAPSTYLE_MODE_SEQUENCE:
	  {
	    break;
	  }

	case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
	  {
	    break;
	  }

	default:
	  zMapAssertNotReached() ;
	  break;
	}


     {
            int offset,block_y1,block_y2;

            /*
             * we are drawing from diff_context which may be the result of a req from mark
             * in which case our block coords may be different from the canvas block
             * so we need to get the block start coord from the canvas block
             */

            /* get block via strand from container */
            ZMapWindowContainerBlock container_block = (ZMapWindowContainerBlock)
                  zmapWindowContainerUtilsItemGetParentLevel((FooCanvasItem *) container, ZMAPCONTAINER_LEVEL_BLOCK);
            ZMapFeatureBlock canvas_block;

            /* get the feature corresponding to tbe canvas block */
            zMapAssert(ZMAP_IS_CONTAINER_BLOCK(container_block));
            zmapWindowContainerGetFeatureAny( (ZMapWindowContainerGroup) container_block, (ZMapFeatureAny *) &canvas_block);

            /* get the block offset for the display context not the load request context */
            /* MH17: to handle the navigator as scaled we need to get the canvas item start coord
             * not the block feature start coord. for the normal window it should be the same
             */
            block_y1 = feature_stack->block->block_to_sequence.block.x1;
            block_y2 = feature_stack->block->block_to_sequence.block.x2;
// having taken the scaling factor out we don't need this
//            getFactoriedCoordinates(factory, &block_y1,&block_y2);
            offset =  block_y1;

            run_data.factory   = factory;
            run_data.container = features_container;
            run_data.feature_stack = feature_stack;
            run_data.canvas_item = current_item;

		item = drawFeaturesetFeature(&run_data, feature, offset,
     				  points[0], points[1],
				  points[2], points[3],
				  style);
      }

      /* if(!run_data.item_in && item) */
      if (item)
        {
          gboolean status = FALSE;
          ZMapFrame frame;
	  ZMapStrand strand;

	  frame  = zmapWindowContainerFeatureSetGetFrame((ZMapWindowContainerFeatureSet)parent_container);
	  strand = zmapWindowContainerFeatureSetGetStrand((ZMapWindowContainerFeatureSet)parent_container);

          if(factory->ftoi_hash)
	    {
#if MH17_REVCOMP_DEBUG > 1
if(is_nav)
            printf("FToIAddFeature %s %f,%f - %f,%f to %s\n",
            g_quark_to_string(feature->unique_id),
            points[1],points[0],points[3],points[2],
            g_quark_to_string(feature_stack->set->unique_id));
#endif

	      status = zmapWindowFToIAddFeature(factory->ftoi_hash,
						feature_stack->align->unique_id,
						feature_stack->block->unique_id,
						//zMapWindowGetFeaturesetContainerID(window,set->unique_id),
                                    feature_stack->set->unique_id,
						strand, frame,
						feature->unique_id, item, feature) ;
	    }
          else
          {
#if MH17_REVCOMP_DEBUG > 2
        printf("no hash\n");
#endif
          }

          status = (factory->user_funcs->top_item_created)(item, feature_stack, factory->user_data);

          zMapAssert(status);

          return_item = item;
        }
        else
        {
#if MH17_REVCOMP_DEBUG > 2
        printf("no item\n");
#endif
        }

    }

 undrawn:

  return return_item;
}


void zmapWindowFToIFactoryClose(ZMapWindowFToIFactory factory)
{
  factory->ftoi_hash  = NULL;
//  factory->long_items = NULL;
  g_free(factory->user_funcs);
  factory->user_data  = NULL;
  /* font desc??? */
  /* factory->font_desc = NULL; */

  /* free the factory */
  g_free(factory);

  return ;
}



static void datalistRun(gpointer key, gpointer list_data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)list_data;
  RunSet run_data = (RunSet)user_data;
  FooCanvasItem *foo;

  /* filter on frame! */
  if((run_data->frame != ZMAPFRAME_NONE) &&
     run_data->frame  != zmapWindowFeatureFrame(feature))
    return ;

  run_data->feature_stack->feature = feature;
  if(feature->style)	/* chicken */
  {
	if(zMapStyleIsStrandSpecific(feature->style))
		run_data->feature_stack->strand = zmapWindowFeatureStrand(NULL,feature);
	if(zMapStyleIsFrameSpecific(feature->style))
		run_data->feature_stack->frame = zmapWindowFeatureFrame(feature);
  }

  foo = zmapWindowFToIFactoryRunSingle(run_data->factory,
      run_data->canvas_item,
      run_data->container,
      run_data->feature_stack);

  return ;
}



static FooCanvasItem *drawFeaturesetFeature(RunSet run_data, ZMapFeature feature,
                              double feature_offset,
					double x1, double y1, double x2, double y2,
                              ZMapFeatureTypeStyle style)
{
  FooCanvasGroup        *parent = run_data->container;
  FooCanvasItem   *feature_item = NULL;
  ZMapWindowCanvasItem canvas_item;

      ZMapWindowContainerFeatureSet fset = (ZMapWindowContainerFeatureSet) run_data->container->item.parent;
      ZMapFeatureBlock block = run_data->feature_stack->block;


//       if(!run_data->feature_stack->id || zMapStyleIsStrandSpecific(style) || zMapStyleIsFrameSpecific(style))
      /* NOTE calling code calls zmapWindowDrawFeatureSet() for each frame but
       * expects it to shuffle features into the right stranded container
       * it's a half solution which caused a misunderstanding here.
       * ideally the calling code would have 6 column groups prepared and we'd do one scan of the featureset
       * that's a bit fiddly to sort now but it needs doing when containers get done
       * We'd like to set up the 6 CanvasFeaturesets once, not recalc every time
       * this would need 6 CanvasFeatureset id's to avoid the recalc every time
       * Perhaps the best way is to run 6 scans for all frame and strand combinations??
       */
#warning code needs restructuring around zmapWindowDrawFeatureSet()
      {
      	/* for frame spcecific data process_feature() in zmapWindowDrawFeatures.c extracts
      	 * all of one type at a time
      	 * so frame and strand are stable NOTE only frame, strand will wobble
      	 * we save the id here to optimise the code
      	 */
            GQuark col_id = zmapWindowContainerFeatureSetGetColumnId(fset);
            FooCanvasItem * foo = FOO_CANVAS_ITEM(fset);
            GQuark fset_id = run_data->feature_stack->set->unique_id;
            char strand = '+';
            char frame = '0';
		char *x;

            if(zMapStyleIsStrandSpecific(style) && feature->strand == ZMAPSTRAND_REVERSE)
            	strand = '-';
            if(run_data->feature_stack->frame != ZMAPFRAME_NONE)
            	frame += zmapWindowFeatureFrame(feature);

		/* see comment by zMapWindowGraphDensityItemGetDensityItem() */
		if(run_data->feature_stack->maps_to)
		{
			/* a virtual featureset for combing several source into one display item */
			fset_id = run_data->feature_stack->maps_to;
			x = g_strdup_printf("%p_%s_%s_%c%c", foo->canvas, g_quark_to_string(col_id), g_quark_to_string(fset_id),strand,frame);
		}
		else
		{
			/* a display column for combing one or several sources into one display item */
			x = g_strdup_printf("%p_%s_%c%c", foo->canvas, g_quark_to_string(col_id), strand,frame);
		}

            run_data->feature_stack->id = g_quark_from_string(x);
            g_free(x);
      }

            /* adds once per canvas+column+style, then returns that repeatedly */
            /* also adds an 'interval' foo canvas item which we need to look up */
      canvas_item = zMapWindowCanvasItemFeaturesetGetFeaturesetItem(parent, run_data->feature_stack->id,
            block->block_to_sequence.block.x1,block->block_to_sequence.block.x2, style,
            run_data->feature_stack->strand,run_data->feature_stack->frame,run_data->feature_stack->set_index);

      zMapAssert(canvas_item);

	/* NOTE the item hash used canvas _item->feature to set up a pointer to the feature
	 * so I changed FToIAddfeature to take the feature explicitly
	 * setting the feature here every time also fixes the problem but by fluke
	 */
     	zMapWindowCanvasItemSetFeaturePointer(canvas_item, feature);

	if(run_data->feature_stack->filter && (feature->flags.collapsed || feature->flags.squashed || feature->flags.joined))
	{
		/* collapsed items are not displayed as they contain no new information
		 * but they cam be searched for in the FToI hash
		 * so return the item that they got collapsed into
		 * if selected from the search they get assigned to the canvas item
		 * and the population copied in.
		 *
		 * NOTE calling code will need to set the feature in the hash as the composite feature
#warning need to set composite feature in lookup code
		 */

		return (FooCanvasItem *) canvas_item;
	}




	/* now we are outisde the normal ZMapWindowCanvasItem dogma */
	/* NOTE we know that graph items are not processed by another route
 	 * unlike alignments that get zMapWindowFeatureReplaced()
 	 */
 	/* NOTE normally AddInterval adds a foo canvas item and then runs another function to set the colour
  	 * we add a data struct and add colours to that in situ, as the feature has its style handy
  	 */

	/* NOTE as density items get re-binned on zoom and also get drawn in different ways
	 * (line, histogram, heatmap) we only set the item's score/width here
	 * not the x1,x2 coordinates
	 */
  	{
	      zMapWindowCanvasFeaturesetAddFeature((ZMapWindowFeaturesetItem) canvas_item, feature, y1, y2);
	}

      feature_item = (FooCanvasItem *)canvas_item;

	if(feature->population)		/* keep displayed iten for use by collapsed ones in FtoI hash */
		run_data->canvas_item = feature_item;

      return feature_item;
}





static gboolean null_top_item_created(FooCanvasItem *top_item,
                                      ZMapWindowFeatureStack feature_stack,
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

