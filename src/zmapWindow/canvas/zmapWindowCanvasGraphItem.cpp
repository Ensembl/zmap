/*  File: zmapWindowCanvasGraphItem.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: this module takes a series of bins and draws then in various formats
 *              (line, hiostogram, heatmap) but in each case the source data is the same if
 *              'density' mode is set then the source data is re-binned according to zoom level
 *              it adds a single (extended) foo_canvas item to the canvas for each set of data
 *              in a column.  It is possible to have several line graphs overlapping, but note
 *              that heatmaps and histograms would not look as good.
 *
 *              NOTE originally implemented as free standing canvas type
 *               and later merged into CanvasFeatureset.
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <math.h>
#include <string.h>

#include <ZMap/zmapUtilsFoo.hpp>
#include <ZMap/zmapUtilsLog.hpp>
#include <ZMap/zmapFeature.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasFeatureset.hpp>
#include <zmapWindowCanvasFeature_I.hpp>
#include <zmapWindowCanvasGraphItem_I.hpp>

/*
 * if we draw wiggle plots then instead of one GDK call per segment we cache points and draw a big
 * long poly-line. For filled graphs we just call the gdk draw polygon function instead.
 *
 * ideally this would be implemented in the featureset struct as extended for graphs but we don't
 * extend featuresets as the rest of the code would be more complex.  instead we have a single
 * cache here which works because we only ever display one column at a time.  and relies on
 * ..PaintFlush() being called
 *
 * If we ever created threads to paint columns then this needs to be moved into CanvasFeatureset
 * and alloc'd for graphs and freed on CFS_destroy()
 *
 * NOTE currently we do use callbacks but these are necessarily serialised by the Foo Canvas
 */

/* Current state of play of this code:
 *
 * Code was originally written by Malcolm and then a load of fixes applied by Ed.
 *
 * I (Ed) don't completely understand all of Malcolm's thinking and there are
 * probably some bugs remaining but I don't have the time to fix this any more.
 * The code seems robust and the colouring/filling work except that the fill code draws
 * shapes that are one short in height. This is probably related to the gdk
 * polygon function which must call an X Windows function, these functions
 * often have arcane semantics about which pixels they actually fill on the screen.
 *
 *  */


static void graphInit(ZMapWindowFeaturesetItem featureset) ;
static void graphGetFeatureExtent(ZMapWindowCanvasFeature feature, ZMapSpan span, double *width) ;
static void graphPaintFlush(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                            GdkDrawable *drawable, GdkEventExpose *expose);
static void graphPaintPrepare(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                              GdkDrawable *drawable, GdkEventExpose *expose) ;
static void graphPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                              GdkDrawable *drawable, GdkEventExpose *expose) ;
static void graphPreZoom(ZMapWindowFeaturesetItem featureset) ;
static void graphZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable) ;
static double graphPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                         double item_x, double item_y, int cx, int cy,
                         double local_x, double local_y, double x_off) ;

static GList *densityCalcBins(ZMapWindowFeaturesetItem di) ;
static void setColumnStyle(ZMapWindowFeaturesetItem featureset, ZMapFeatureTypeStyle feature_style) ;


/*
 *                        Globals
 */

static gboolean debug_G = FALSE ;                           /* Set TRUE for debug output. */



/*
 *                     External routines.
 *
 * Some of these are static but are directly called from zmapWindowCanvasFeature
 * as they are "methods".
 *
 */


/* Called by canvasFeature to set up method function pointers for graph features to be painted, zoomed etc. */
void zMapWindowCanvasGraphInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL } ;
  gpointer feature_funcs[CANVAS_FEATURE_FUNC_N_FUNC] = { NULL };

  funcs[FUNC_SET_INIT] = (void *)graphInit ;
  funcs[FUNC_PREPARE] = (void *)graphPaintPrepare ;
  funcs[FUNC_PAINT] = (void *)graphPaintFeature ;
  funcs[FUNC_FLUSH] = (void *)graphPaintFlush ;
  funcs[FUNC_PRE_ZOOM] = (void *)graphPreZoom ;
  funcs[FUNC_ZOOM] = (void *)graphZoom ;
  funcs[FUNC_POINT] = (void *)graphPoint ;

  /* And again encapsulation is broken..... */
  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_GRAPH, funcs, sizeof(ZMapWindowCanvasGraphStruct)) ;


  feature_funcs[CANVAS_FEATURE_FUNC_EXTENT] = (void *)graphGetFeatureExtent ;

  zMapWindowCanvasFeatureSetSize(FEATURE_GRAPH, feature_funcs, sizeof(zmapWindowCanvasFeatureStruct)) ;

  return ;
}



/* Initialise the graph object. */
static void graphInit(ZMapWindowFeaturesetItem featureset)
{
  ZMapWindowCanvasGraph graph_set = NULL ;
  GdkColor *draw_col = NULL, *fill_col = NULL, *border_col = NULL ;

  zMapReturnIfFail(featureset) ;
  graph_set = (ZMapWindowCanvasGraph)(featureset->opt) ;

  /* Set up default colours to draw with, these may be overwritten later by the features
   * colours. */
  if (zmapWindowFeaturesetGetDefaultColours(featureset, &fill_col, &draw_col, &border_col))
    {
      graph_set->fill_col = *fill_col ;
      graph_set->draw_col = *draw_col ;
      graph_set->border_col = *border_col ;
    }

  graph_set->last_gy = -1.0 ;

  return ;
}


/* Called before any zooming, this column needs to be recalculated with every zoom, this call sets
 * a flag to make sure this happens.  This is required because we need special processing on zoom
 * not just box/line resizing. */
static void graphPreZoom(ZMapWindowFeaturesetItem featureset)
{

  /* This breaks encapsulation by calling back to featureset which is not good... */
  zMapWindowCanvasFeaturesetSetZoomRecalc(featureset, TRUE) ;

  return ;
}



/* Recalculate the bins if the zoom level changes */
static void graphZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
  zMapReturnIfFail(featureset);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* unused currently. */
  ZMapWindowCanvasGraph graph_set = (ZMapWindowCanvasGraph)(featureset->opt) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* WHEN DOES featureset->featurestyle GET SET ????????? */
  /* OK...THIS ALL POINTS TO SOMETHING NOT QUITE RIGHT....AT THIS POINT WE ARE ZOOMING
   * WHICH REQUIRES STYLE ACCESS BUT featureset->featurestyle IS NOT SET....DUH... */

  /* need logic here to handle all this style crud and to get hold of a feature in the
   * the first place.... Can this in fact happen here....we need the feature style so
   * it should have been set ??????? */
  /* FIND OUT WHEN featureset->featurestyle GETS SET....POTENTIAL FOR SCREW UPS HERE...
   *  */




  if (featureset->featurestyle)
    {
      setColumnStyle(featureset, featureset->featurestyle) ;
    }

  /* HACK FOR NOW TO TRY AND GET IT WORKING.... */
  else if (featureset->style)
    {
      setColumnStyle(featureset, featureset->style) ;
    }


  else
    {
      /* AGH, CRASHES HERE WITH NO feature->style bexcause there is no feature.... */

      GList *feature_list ;
      ZMapWindowCanvasFeature feature_item ;
      ZMapFeature feature ;

      feature_list = featureset->features ;

      feature_item = (ZMapWindowCanvasFeature)(feature_list->data) ;

      feature = feature_item->feature ;


      setColumnStyle(featureset, *(feature->style)) ;
    }

  /* Some displays must be rebinned on zoom. */
  if (featureset->re_bin)
    {
      if (featureset->display_index)
        zmapWindowCanvasFeaturesetFreeDisplayLists(featureset) ;


      featureset->display = densityCalcBins(featureset) ;
    }

  /* will index display not features if display is set */
  if (!featureset->display_index)
    zMapWindowCanvasFeaturesetIndex(featureset) ;

  return ;
}


/* get sequence extent of compound alignment (for bumping) */
/* NB: canvas coords could overlap due to sub-pixel base pairs
 * which could give incorrect de-overlapping
 * that would be revealed on Zoom
 */
/*
 * we adjust the extent of the first CanvasAlignment to cover tham all
 * as the first one draws all the colinear lines
 */

/* Agh...is this used for line graphs...I guess it might be....check this....
 *
 *
 *  */
static void graphGetFeatureExtent(ZMapWindowCanvasFeature feature, ZMapSpan span, double *width)
{
  ZMapWindowCanvasFeature first = feature ;
  int last_y2 ;

  zMapReturnIfFail(feature && feature->feature) ;

  *width = feature->width ;
  span->x1 = feature->y1 ;
  span->x2 = last_y2 = feature->y2 ;


  /* Needs recoding to work differently for different graph styles...???? */

  /* if not joining up same name features they don't need to go in the same column */
  if(!zMapStyleIsUnique(*feature->feature->style))
    {
      while(first->left)
        {
          first = first->left ;

          if(first->width > *width)
            *width = first->width ;
        }

      while(feature->right)
        {
          feature = feature->right ;

          if(feature->width > *width)
            *width = feature->width ;
        }

      last_y2 = feature->y2 ;
    }


  if (zMapStyleGraphMode(*feature->feature->style) == ZMAPSTYLE_GRAPH_LINE)
    {
      feature->y2 = span->x2 = last_y2 ;
    }
  else if (zMapStyleGraphMode(*feature->feature->style) == ZMAPSTYLE_GRAPH_HISTOGRAM)
    {
      *width = zMapStyleGetWidth(*feature->feature->style) ;
    }


  return ;
}






/* There may not be a feature so that may mean graph stuff is not set up properly....?? */
static void graphPaintPrepare(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                              GdkDrawable *drawable, GdkEventExpose *expose)
{
  ZMapWindowCanvasGraph graph_set = NULL ;
  int cx2 = 0, cy2 = 0 ;                                    /* initialised for LINE mode */
  double x2 = 0, y2 = 0 ;
  FooCanvasItem *item = (FooCanvasItem *)featureset ;

  zMapReturnIfFail(featureset) ;
  graph_set = (ZMapWindowCanvasGraph)(featureset->opt) ;


  if (featureset->featurestyle)
    {
      zMapDebugPrint(debug_G, "FeatureSet  \"%s\", featureset style \"%s\",  feature style \"%s\"",
                     g_quark_to_string(featureset->id),
                     zMapStyleGetName(featureset->style),
                     zMapStyleGetName(featureset->featurestyle)) ;

      /* Need score stuff to be copied across.... */
      if (zMapStyleIsPropertySetId(featureset->featurestyle, STYLE_PROP_MIN_SCORE))
        {
          featureset->style->min_score = featureset->featurestyle->min_score ;
          zmapStyleSetIsSet(featureset->style, STYLE_PROP_MIN_SCORE) ;
        }

      if (zMapStyleIsPropertySetId(featureset->featurestyle, STYLE_PROP_MAX_SCORE))
        {
          featureset->style->max_score = featureset->featurestyle->max_score ;
          zmapStyleSetIsSet(featureset->style, STYLE_PROP_MAX_SCORE) ;
        }

      if (zMapStyleIsPropertySetId(featureset->featurestyle, STYLE_PROP_SCORE_SCALE))
        {
          featureset->style->score_scale = featureset->featurestyle->score_scale ;
          zmapStyleSetIsSet(featureset->style, STYLE_PROP_SCORE_SCALE) ;
        }
    }


  if (zMapStyleGraphMode(featureset->style) == ZMAPSTYLE_GRAPH_LINE)
    {
      double x_offset, y_offset ;

      /* THERE IS ANOTHER PROBLEM HERE TOO....THE COLUMN STYLE NEEDS TO REFLECT THE
       * FEATURESET STYLES...NONE OF THIS IS TOO GOOD ACTUALLY SINCE REALLY THE FEATURESET
       * STYLES SHOULD BE USED BY THE CANVAS FEATURESET CODE TOO !! */
      /* quite hacky....we need the graph fill mode which is in the feature style but
       * if we move right to the end of the sequence the featurestyle can be NULL because there
       * may not be a feature within the canvas range and then the code crashes so here
       * I copy across the fill mode into the column style to cache it and also the colour stuff. */
      if (featureset->featurestyle)
        {
          featureset->style->mode_data.graph.fill_flag = zMapStyleGraphFill(featureset->featurestyle) ;

          if (zMapStyleIsPropertySetId(featureset->featurestyle, STYLE_PROP_GRAPH_COLOURS))
            {
              /* We need a function to copy colours across..... */
              featureset->style->mode_data.graph.colours = featureset->featurestyle->mode_data.graph.colours ;
              zmapStyleSetIsSet(featureset->style, STYLE_PROP_GRAPH_COLOURS) ;
            }

          if (zMapStyleIsPropertySetId(featureset->style, STYLE_PROP_GRAPH_COLOURS))
            {
              GdkColor *draw_col = NULL, *fill_col = NULL, *border_col = NULL ;
              ZMapWindowFeaturesetItemClass featureset_class = ZMAP_WINDOW_FEATURESET_ITEM_GET_CLASS(featureset) ;

              zMapStyleGetColours(featureset->style, STYLE_PROP_GRAPH_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                                  &fill_col, &draw_col, &border_col) ;

              if (fill_col)
                {
                  graph_set->fill_col = *fill_col ;
                  zmapWindowFeaturesetAllocColour(featureset_class, &(graph_set->fill_col)) ;
                }
              if (draw_col)
                {
                  graph_set->draw_col = *draw_col ;
                  zmapWindowFeaturesetAllocColour(featureset_class, &(graph_set->draw_col)) ;
                }
              if (border_col)
                {
                  graph_set->border_col = *border_col ;
                  zmapWindowFeaturesetAllocColour(featureset_class, &(graph_set->border_col)) ;
                }
            }
        }

      x_offset = featureset->dx + featureset->x_off ;
      y_offset = featureset->start - featureset->dy ;

      graph_set->n_points = 0 ;


      if (feature)
        {
          /* add first point to join up to */
          if (zMapStyleGraphFill(featureset->style))
            {
              x2 = 0 ;
              y2 = feature->y1 ;

              foo_canvas_w2c(item->canvas,
                             x2 + x_offset,
                             y2 + y_offset,
                             &cx2, &cy2) ;

              cy2 = CANVAS_CLAMP_COORDS(cy2) ;

              graph_set->points[graph_set->n_points].x = cx2 ;
              graph_set->points[graph_set->n_points].y = cy2 ;
              graph_set->n_points++ ;
            }
          else
            {
              /* this is probably a paint in the middle of the window */
              /* and the line has been drawn previously, we need to paint over it exactly */
              x2 = featureset->style->mode_data.graph.baseline + feature->width - 1 ;
              y2 = (feature->y2 + feature->y1 + 1) / 2;

              foo_canvas_w2c(item->canvas,
                             x2 + x_offset,
                             y2 + y_offset,
                             &cx2, &cy2) ;

              cy2 = CANVAS_CLAMP_COORDS(cy2) ;

              graph_set->points[graph_set->n_points].x = cx2 ;
              graph_set->points[graph_set->n_points].y = cy2 ;
              graph_set->n_points++ ;

            }

          graph_set->last_gx = x2 ;
          graph_set->last_gy = feature->y2 ;
          graph_set->last_width = feature->width ;
        }
      else
        {
          if (zMapStyleGraphFill(featureset->style))
            {
              x2 = 0 ;
              y2 = featureset->start ;

              foo_canvas_w2c(item->canvas,
                             x2 + x_offset,
                             y2 + y_offset,
                             &cx2, &cy2) ;

              cy2 = CANVAS_CLAMP_COORDS(cy2) ;

              graph_set->points[graph_set->n_points].x = cx2 ;
              graph_set->points[graph_set->n_points].y = cy2 ;
              graph_set->n_points++ ;
            }


          /* trigger vertical bracket at the start */
          graph_set->last_gx = x2 ;
          graph_set->last_gy = featureset->start ;
          graph_set->last_width = 0 ;
        }
    }

  return ;
}



/* paint one feature, CanvasFeatureset handles focus highlight
 *
 * NOTE wiggle plots are drawn as poly-lines and get cached and painted when complete
 * NOTE lines have to be drawn out of the box at the edges */
static void graphPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                              GdkDrawable *drawable, GdkEventExpose *expose)

{
  ZMapWindowCanvasGraph graph_set = NULL ;
  FooCanvasItem *item = (FooCanvasItem *)featureset ;
  int cx2 = 0, cy2 = 0 ;                                    /* initialised for LINE mode */
  double x1,x2,y2 ;
  gboolean draw_box = TRUE ;                                /* else line */
  gulong ufill,outline ;
  int colours_set, fill_set, outline_set ;

  zMapReturnIfFail(featureset) ;
  graph_set = (ZMapWindowCanvasGraph)(featureset->opt) ;

  switch(zMapStyleGraphMode(featureset->style))
    {
    case ZMAPSTYLE_GRAPH_LINE:
      {
        double x_offset, y_offset ;
        int bin_gap ;

        if (debug_G && feature && feature->feature)
          zMapDebugPrint(debug_G, "Feature %s -\tbin: %.f, %.f\tfeature: %d, %d\tdiff: %.f, %.f",
                         g_quark_to_string(feature->feature->original_id),
                         feature->y1, feature->y2,
                         feature->feature->x1, feature->feature->x2,
                         feature->y1 - feature->feature->x1, feature->y2 - feature->feature->x2) ;

        draw_box = FALSE;

        x_offset = featureset->dx + featureset->x_off ;
        y_offset = featureset->start - featureset->dy ;

        /* output poly-line segemnts via a series of points
         * NOTE we start at the previous segment if any via the calling display code
         * so this caters for dangling lines above the start
         * and we stop after the end so this caters for dangling lines below the end
         */

        /* get width, y1, y2 and middle
         * our point is middle,width
         * and we join to the previous point if it's close in y
         * or back to baseline if y1 > prev.y2 in pixels, and then join with a vertical (add two points)
         */


        /* If the very first feature is right at the start of the sequence we need to ufill
         * in s is the very first feature we fudge it big gap is left at start of sequence...try this.... */
        if (feature->y1 == featureset->start)
          {
            x2 = featureset->style->mode_data.graph.baseline + feature->width - 1 ;
            y2 = feature->y1 ;

            foo_canvas_w2c(item->canvas,
                           x2 + x_offset,
                           y2 + y_offset,
                           &cx2, &cy2) ;

            cy2 = CANVAS_CLAMP_COORDS(cy2) ;

            graph_set->points[graph_set->n_points].x = cx2 ;
            graph_set->points[graph_set->n_points].y = cy2 ;
            graph_set->n_points++ ;
          }


        /* && fill_gaps_between_bins */
        bin_gap = feature->y1 - graph_set->last_gy ;

        if (bin_gap > 1 && (bin_gap > (featureset->bases_per_pixel * 2)))
          {
            double x_pos, y_pos ;
            int cx, cy ;

            x_pos = featureset->style->mode_data.graph.baseline + x_offset ;
            y_pos = graph_set->last_gy ;

            /* draw from last point back to base: add baseline point at previous feature y2 */
            foo_canvas_w2c(item->canvas,
                           x_pos,
                           y_pos + y_offset,
                           &cx, &cy) ;

            cy = CANVAS_CLAMP_COORDS(cy) ;

            graph_set->points[graph_set->n_points].x = cx;
            graph_set->points[graph_set->n_points].y = cy;
            graph_set->n_points++;

            /* add point at current feature y1, implies vertical if there's a previous point */
            foo_canvas_w2c(item->canvas,
                           x_pos,
                           feature->y1 - y_offset,
                           &cx, &cy) ;
            cy = CANVAS_CLAMP_COORDS(cy) ;

            graph_set->points[graph_set->n_points].x = cx;
            graph_set->points[graph_set->n_points].y = cy;
            graph_set->n_points++;
          }


        /* WHY'S IT  - 1  HERE...?? */
        x2 = featureset->style->mode_data.graph.baseline + feature->width - 1 ;
        y2 = (feature->y2 + feature->y1 + 1) / 2 ;

        foo_canvas_w2c(item->canvas,
                       x2 + x_offset,
                       y2 + y_offset,
                       &cx2, &cy2) ;

        cy2 = CANVAS_CLAMP_COORDS(cy2) ;

        graph_set->points[graph_set->n_points].x = cx2 ;
        graph_set->points[graph_set->n_points].y = cy2 ;

        /* Remember to record this point position. */
        graph_set->last_gx = x2 ;
        graph_set->last_gy = feature->y2 ;
        graph_set->last_width = feature->width ;

        (graph_set->n_points)++ ;

        if (graph_set->n_points >= N_POINTS)
          {
            graphPaintFlush(featureset, feature, drawable, expose) ;
          }

        break ;
      }

    case ZMAPSTYLE_GRAPH_HEATMAP:
      {
        /* colour between ufill and outline according to score */
        x1 = featureset->dx + featureset->x_off;
        x2 = x1 + featureset->width - 1;

        outline_set = FALSE;
        fill_set = TRUE;
        ufill = foo_canvas_get_color_pixel(item->canvas,
                                          zMapWindowCanvasFeatureGetHeatColour(featureset->fill_colour,
                                                                               featureset->outline_colour,
                                                                               feature->score)) ;
        break;
      }

    default:
    case ZMAPSTYLE_GRAPH_HISTOGRAM:
      {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* old code.... */

        x1 = featureset->dx + featureset->x_off;
        x2 = x1 + feature->width - 1;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

        x1 = featureset->dx + featureset->x_off; //  + (width * zMapStyleBaseline(di->style)) ;

        if (featureset->bumped)
          x1 += feature->bump_offset ;

        if (feature->width < 1)
          x2 = x1 ;
        else
          x2 = x1 + feature->width - 1;

        /* If the baseline is not zero then we can end up with x2 being less than x1
         * so we mst swap these round.
         */
        if(x1 > x2)
          zMapUtilsSwop(double, x1, x2) ;

        colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &ufill, &outline);
        fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
        outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

        break ;
      }
    }

  if (draw_box)
    {
      zMapCanvasFeaturesetDrawBoxMacro(featureset,
                                       x1, x2, feature->y1, feature->y2,
                                       drawable, fill_set, outline_set, ufill, outline) ;
    }

  return ;
}


static void graphPaintFlush(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                            GdkDrawable *drawable, GdkEventExpose *expose)
{
  ZMapWindowCanvasGraph graph_set = NULL;

  zMapReturnIfFail(featureset) ;
  graph_set = (ZMapWindowCanvasGraph)(featureset->opt) ;

  /* Currently only lines are drawn in this function. */
  if (zMapStyleGraphMode(featureset->style) == ZMAPSTYLE_GRAPH_LINE)
    {
      gboolean line_fill ;

      line_fill = zMapStyleGraphFill(featureset->style) ;

      if (graph_set->n_points < 1)              /* no features, draw baseline */
        {
          zmapWindowCanvasFeatureStruct dummy = { (zmapWindowCanvasFeatureType)0 };
          FooCanvasItem *foo = (FooCanvasItem *) featureset;

          foo_canvas_c2w (foo->canvas, 0, featureset->clip_y1, NULL, &dummy.y2);

          if (dummy.y2 < 0)
            dummy.y2 = 0 ;

          dummy.y1 = dummy.y2 - 1.0 ;
          dummy.width = 0.0;

          graphPaintFeature(featureset, &dummy, drawable, expose );
          graph_set->n_points -= 1;     /* remove this point and leave the downstream vertical */
        }


      if (graph_set->n_points >= 1)
        {
          GdkColor *graph_colour ;
          double end ;
          FooCanvasItem *foo = (FooCanvasItem *)featureset ;


          /* This code attempts to use the clip to calculate the last point...fails when
           * zoomed right out.... */
          foo_canvas_c2w(foo->canvas, 0, featureset->clip_y2, NULL, &end) ;


          /* THIS IS ALL PRETTY HOAKY...A NULL FEATURE SEEMS TO SIGNAL THE END OF THE
           * FEATURE SET....DUH....... */

          /* draw back to baseline from last point and add trailing line */
          if (!feature)
            {
              zmapWindowCanvasFeatureStruct dummy = {(zmapWindowCanvasFeatureType)0} ;

              if (end > featureset->end)
                {
                  /* For line fill add a last point at the _bottom_ of the graph. */
                  if (zMapStyleGraphFill(featureset->style))
                    {
                      FooCanvasItem *item = (FooCanvasItem *)featureset ;
                      double x2, y2, x_offset, y_offset ;
                      int cx2 = 0, cy2 = 0 ;             /* initialised for LINE mode */

                      x_offset = featureset->dx + featureset->x_off ;
                      y_offset = featureset->start - featureset->dy ;

                      x2 = 0 ;
                      y2 = graph_set->last_gy ;

                      foo_canvas_w2c(item->canvas,
                                     x2 + x_offset,
                                     y2 - y_offset,
                                     &cx2, &cy2) ;

                      cy2 = CANVAS_CLAMP_COORDS(cy2) ;

                      graph_set->points[graph_set->n_points].x = cx2 ;
                      graph_set->points[graph_set->n_points].y = cy2 ;
                      graph_set->n_points++ ;

                      x2 = 0 ;
                      y2 = featureset->end ;

                      foo_canvas_w2c(item->canvas,
                                     x2 + x_offset,
                                     y2 + y_offset,
                                     &cx2, &cy2) ;

                      cy2 = CANVAS_CLAMP_COORDS(cy2) ;

                      graph_set->points[graph_set->n_points].x = cx2 ;
                      graph_set->points[graph_set->n_points].y = cy2 ;
                      graph_set->n_points++ ;
                    }
                  else
                    {
                      dummy.y1 = featureset->end - 1 ;
                      dummy.y2 = featureset->end ;          /* + 1 to draw in last base span. */

                      dummy.width = graph_set->last_width ;

                      graphPaintFeature(featureset, &dummy, drawable, expose) ;

                      /* I DO NOT KNOW WHY THIS IS DONE AT ALL...... */
                      graph_set->n_points -= 1 ;        /* remove this point and leave the downstream
                                                           vertical */
                    }

                }
              else
                {
                  /* For line fill add a last point at the _bottom_ of the graph. */
                  if (zMapStyleGraphFill(featureset->style))
                    {
                      FooCanvasItem *item = (FooCanvasItem *)featureset ;
                      double x2, y2, x_offset, y_offset ;
                      int cx2 = 0, cy2 = 0 ;             /* initialised for LINE mode */

                      x_offset = featureset->dx + featureset->x_off ;
                      y_offset = featureset->dy - featureset->start ;

                      x2 = 0 ;
                      y2 = graph_set->last_gy ;

                      foo_canvas_w2c(item->canvas,
                                     x2 + x_offset,
                                     y2 + y_offset,
                                     &cx2, &cy2) ;

                      cy2 = CANVAS_CLAMP_COORDS(cy2) ;

                      graph_set->points[graph_set->n_points].x = cx2 ;
                      graph_set->points[graph_set->n_points].y = cy2 ;
                      graph_set->n_points++ ;


                    }
                  else
                    {
                      /* original code..... */

                      dummy.y1 = end ;
                      dummy.y1 += featureset->bases_per_pixel * 2 ;
                      dummy.y2 = dummy.y1 + 1.0 ;
                      dummy.width = 0.0 ;

                      graphPaintFeature(featureset, &dummy, drawable, expose) ;

                      /* I DO NOT KNOW WHY THIS IS DONE AT ALL...... */
                      graph_set->n_points -= 1 ;        /* remove this point and leave the downstream
                                                           vertical */

                    }
                }
            }
          else
            {
              /* there is a feature ! */

              /* For line fill add a last point at the _bottom_ of the graph. */
              if (zMapStyleGraphFill(featureset->style))
                {
                  FooCanvasItem *item = (FooCanvasItem *)featureset ;
                  double x2, y2, x_offset, y_offset ;
                  int cx2 = 0, cy2 = 0 ;             /* initialised for LINE mode */

                  x_offset = featureset->dx + featureset->x_off ;
                  y_offset = featureset->dy - featureset->start ;

                  x2 = 0 ;
                  y2 = end ;

                  foo_canvas_w2c(item->canvas,
                                 x2 + x_offset,
                                 y2 + y_offset,
                                 &cx2, &cy2) ;

                  cy2 = CANVAS_CLAMP_COORDS(cy2) ;

                  graph_set->points[graph_set->n_points].x = cx2 ;
                  graph_set->points[graph_set->n_points].y = cy2 ;
                  graph_set->n_points++ ;


                }
            }

          /* Draw the lines or filled lines, they are already clipped to the visible scroll region */

          /* This should all be inserted at the start and cached.... */
          if (zMapStyleGraphFill(featureset->style))
            {
              graph_colour = &(graph_set->fill_col) ;
            }
          else
            {
              graph_colour = &(graph_set->border_col) ;
            }

          gdk_gc_set_foreground(featureset->gc, graph_colour) ;

          if (line_fill)
            gdk_draw_polygon(drawable, featureset->gc, TRUE, graph_set->points, graph_set->n_points) ;
          else
            gdk_draw_lines(drawable, featureset->gc, graph_set->points, graph_set->n_points) ;


          /* I do not understand this bit....EG */
          if (feature)          /* interim flush: add last point at start */
            {
              graph_set->n_points-- ;
              graph_set->points[0].x = graph_set->points[graph_set->n_points].x ;
              graph_set->points[0].y = graph_set->points[graph_set->n_points].y ;
              graph_set->n_points = 1 ;
            }
          else
            {
              graph_set->n_points = 0;
            }
        }
    }

  return ;
}



/* Function to check if the given x,y coord is within a feature, this
 * function assumes the feature is box-like. */
static double graphPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                         double item_x, double item_y, int cx, int cy,
                         double local_x, double local_y, double x_off)
{
  double best = 1.0e36 ;
  double can_start, can_end ;


  /* Not sure how/why this should ever fail.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMapReturnValIfFail(gs && gs->feature && fi, best) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zMapReturnValIfFail(gs, best) ;
  zMapReturnValIfFail(gs->feature, best) ;
  zMapReturnValIfFail(fi, best) ;



  /* Get feature extent on display. */
  /* NOTE cannot use feature coords as transcript exons all point to the same feature */
  /* alignments have to implement a special function to handle bumped features
   *  - the first exon gets expanded to cover the whole */
  /* when we get upgraded to vulgar strings these can be like transcripts...
   * except that there's a performance problem due to volume */
  /* perhaps better to add  extra display/ search coords to ZMapWindowCancasFeature ?? */


  can_start = gs->feature->x1 ;
  can_end = gs->feature->x2 ;
  zmapWindowFeaturesetS2Ccoords(&can_start, &can_end) ;


  if (can_start <= local_y && can_end >= local_y)                            /* overlaps cursor */
    {
      double wx ;
      double left, right ;

      wx = x_off; // - (gs->width / 2) ;

      if (fi->bumped)
        wx += gs->bump_offset ;

      /* get coords within one pixel */
      left = wx - 1 ;                                            /* X coords are on fixed zoom, allow one pixel grace */
      right = wx + gs->width + 1 ;

      if (local_x > left && local_x < right)                            /* item contains cursor */
        {
          best = 0.0;
        }
    }

  return best ;
}






/*
 *                      Internal routines.
 */



/* create a new list of binned data derived from the real stuff
 *
 * bins are coerced to be at least one pixel across
 *
 * bin values are set as the max (ie most extreme, can be -ve) pro-rated value within the range
 * this is to highlight peaks without creating silly wide graphs
 *
 * we are aiming for the same graph at lower resolution, this point is most acute for heatmaps that
 * cannot be overlapped sensibly
 *
 * try not to split big bins into smaller ones, there's no min size in BP, but the source data
 * imposes a limit
 */
static GList *densityCalcBins(ZMapWindowFeaturesetItem featureset_item)
{
  GList *result = NULL ;
  double start,end;
  int seq_range;
  int n_bins;
  int bases_per_bin;
  int bin_start,bin_end;
  GList *src = NULL,*dest = NULL ;
  ZMapWindowCanvasFeature src_gs = NULL;                    /* the original features */
  ZMapWindowCanvasFeature bin_gs = NULL ;                   /* the re-binned features */
  double score = 0.0 ;
  double min_feat_score = 0.0, max_feat_score = 0.0 ;
  int min_bin ;
  gboolean fixed ;


  /* this is weird given that the feature style may not have been given for the column style yet.... */
  min_bin = zMapStyleDensityMinBin(featureset_item->style); /* min pixels per bin */
  fixed = zMapStyleDensityFixed(featureset_item->style);


  zMapDebugPrint(debug_G, "FeatureSet  \"%s\", featureset style \"%s\",  feature style \"%s\", Score scaling is: %s",
                 g_quark_to_string(featureset_item->id),
                 zMapStyleGetName(featureset_item->style),
                 ((featureset_item->featurestyle)
                  ? zMapStyleGetName(featureset_item->featurestyle) : "no feature style"),
                 zmapStyleScale2ExactStr(zMapStyleGetScoreScale(featureset_item->style))) ;


  if (!featureset_item->features_sorted)
    featureset_item->features = g_list_sort(featureset_item->features, zMapWindowFeatureCmp) ;

  featureset_item->features_sorted = TRUE ;

  if(!min_bin)
    min_bin = 4;

  start = featureset_item->start;
  end   = featureset_item->end;

  seq_range = (int) (end - start + 1);

  n_bins = (int) (seq_range * featureset_item->zoom / min_bin) ;

  bases_per_bin = seq_range / n_bins;
  if(bases_per_bin < 1) /* at high zoom we get many pixels per base */
    bases_per_bin = 1;

  for (bin_start = start, src = featureset_item->features, dest = NULL ;
       bin_start < end && src ;
       bin_start = bin_end + 1)
    {
      bin_end = bin_start + bases_per_bin - 1 ;             /* end can equal start */

      bin_gs = zMapWindowCanvasFeatureAlloc(featureset_item->type) ;

      bin_gs->y1 = bin_start;
      bin_gs->y2 = bin_end;
      bin_gs->score = 0.0;

      for (;src; src = src->next)
        {
          src_gs = (ZMapWindowCanvasFeature) src->data;


          zMapDebugPrint(debug_G, "bin_start, bin_end: %d, %d\tsrc_gs->y1, src_gs->y2: %f, %f",
                         bin_start, bin_end, src_gs->y1, src_gs->y2) ;


          if(src_gs->y2 < bin_start)
            continue;

          if(src_gs->y1 > bin_end)
            {
              /* jump fwds to next data at high zoom */
              if(src_gs->y1 > bin_end + bases_per_bin)
                {
                  bin_end = src_gs->y1;
                  /* bias to even boundaries */
                  bin_end -= (bin_end - bin_start + 1) % bases_per_bin;
                  bin_end--;    /* as it gets ioncremented by the loop */
                }
              break;
            }

          if(fixed && bin_gs->feature && src_gs->y2 > bin_end + bases_per_bin)
            {
              /* short bin(s) ahead of big bin: ouput bin less than min size
               * to avoid swallowing up details in even bigger bin.
               * worst case is 2x no of bins
               */
              bin_gs->y2 = bin_end = src_gs->y1 - 1;
              break;
            }

          if(!bin_gs->feature)
            {
              if(!fixed)
                {
                  bin_gs->y1 = src_gs->y1;              /* limit dest bin to extent of src */
                }
              bin_gs->feature = src_gs->feature;
            }



          /* this implicitly pro-rates any partial overlap of source bins */
          score = src_gs->score;

          if(fabs(score) > fabs(bin_gs->score))
            {
              bin_gs->score = score;
              bin_gs->feature = src_gs->feature;                /* can only have one... choose the biggest */
            }

          if(src_gs->y2 > bin_end)
            {
              /* src bin is bigger than dest bin: make dest bigger */
              if(!fixed)
                {
                  bin_gs->y2 = bin_end = src_gs->y2;
                }
              else if(src_gs->y2 > bin_end + bases_per_bin)
                {
                  bin_end = src_gs->y2; /* bias to the next one before the end */
                  bin_gs->y2 = bin_end;
                }
              break;
            }
        }


      if (bin_gs->score > 0)
        {
          if (bin_gs->feature->score < min_feat_score)
            min_feat_score = bin_gs->feature->score ;
          else if (bin_gs->feature->score > max_feat_score)
            max_feat_score = bin_gs->feature->score ;

          bin_gs->score = zMapWindowCanvasFeatureGetNormalisedScore(featureset_item->style, bin_gs->feature->score);

          bin_gs->width = featureset_item->width;

          if (featureset_item->style->mode_data.graph.mode != ZMAPSTYLE_GRAPH_HEATMAP)
            bin_gs->width = featureset_item->width * bin_gs->score;

          zMapDebugPrint(debug_G,
                         "Feature score: %f, normalised score: %f, featureset_item->width: %f, bin_gs->width: %f",
                         bin_gs->feature->score, bin_gs->score, featureset_item->width, bin_gs->width) ;


          if (!fixed && src_gs->y2 < bin_gs->y2) /* limit dest bin to extent of src */
            {
              bin_gs->y2 = src_gs->y2;
            }

          dest = g_list_prepend(dest, (gpointer)bin_gs) ;
        }

    }

  /* we could have been artisitic and constructed dest backwards */
  if (dest)
    result = g_list_reverse(dest) ;


  zMapDebugPrint(debug_G, "Min Feature score: %f, max Feat score: %f",
                 min_feat_score, max_feat_score) ;


  return result ;
}



/* THERE IS ANOTHER PROBLEM HERE TOO....THE COLUMN STYLE NEEDS TO REFLECT THE
 * FEATURESET STYLES...NONE OF THIS IS TOO GOOD ACTUALLY SINCE REALLY THE FEATURESET
 * STYLES SHOULD BE USED BY THE CANVAS FEATURESET CODE TOO !! */
/* quite hacky....we need the graph fill mode which is in the feature style but
 * if we move right to the end of the sequence the featurestyle can be NULL because there
 * may not be a feature within the canvas range and then the code crashes so here
 * I copy across the fill mode into the column style to cache it and also the colour stuff. */
static void setColumnStyle(ZMapWindowFeaturesetItem featureset, ZMapFeatureTypeStyle feature_style)
{
  ZMapWindowCanvasGraph graph_set = NULL ;

  zMapReturnIfFail(featureset && feature_style) ;
  graph_set = (ZMapWindowCanvasGraph)(featureset->opt) ;

  /* Although doing a style merge here is tempting it's not a good idea as some _column_ style
   * settings (e.g. width, bump etc) are made independently by other parts of the code
   * and would be overwritten by this action. */

  /* Settings for all graphs. */
  if (zMapStyleIsPropertySetId(feature_style, STYLE_PROP_MIN_SCORE))
    {
      featureset->style->min_score = feature_style->min_score ;
      zmapStyleSetIsSet(featureset->style, STYLE_PROP_MIN_SCORE) ;
    }

  if (zMapStyleIsPropertySetId(feature_style, STYLE_PROP_MAX_SCORE))
    {
      featureset->style->max_score = feature_style->max_score ;
      zmapStyleSetIsSet(featureset->style, STYLE_PROP_MAX_SCORE) ;
    }

  if (zMapStyleIsPropertySetId(feature_style, STYLE_PROP_SCORE_SCALE))
    {
      featureset->style->score_scale = feature_style->score_scale ;
      zmapStyleSetIsSet(featureset->style, STYLE_PROP_SCORE_SCALE) ;
    }


  /* Settings for line graphs. */
  featureset->style->mode_data.graph.fill_flag = zMapStyleGraphFill(feature_style) ;

  /* UM....WHAT ON EARTH WAS I DOING HERE...JUST TESTING...????
   * (sm23) I don't understand the details, but not initialising these
   * pointers was causing the crash. Now all is well with the graph
   * display.
   */
  if (zMapStyleIsPropertySetId(feature_style, STYLE_PROP_GRAPH_COLOURS))
    {
      GdkColor *fill_col = NULL, *draw_col = NULL, *border_col = NULL ;
      if (zMapStyleGetColours(feature_style, STYLE_PROP_GRAPH_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                              &fill_col, &draw_col, &border_col))
         zMapStyleSetColours(feature_style, STYLE_PROP_GRAPH_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                             fill_col, draw_col, border_col) ;
    }

  /* Allocate and cache the draw colours */
  if (zMapStyleIsPropertySetId(featureset->style, STYLE_PROP_GRAPH_COLOURS))
    {
      GdkColor *draw_col = NULL, *fill_col = NULL, *border_col = NULL ;
      ZMapWindowFeaturesetItemClass featureset_class = ZMAP_WINDOW_FEATURESET_ITEM_GET_CLASS(featureset) ;

      zMapStyleGetColours(featureset->style, STYLE_PROP_GRAPH_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                          &fill_col, &draw_col, &border_col) ;

      if (fill_col)
        {
          graph_set->fill_col = *fill_col ;
          zmapWindowFeaturesetAllocColour(featureset_class, &(graph_set->fill_col)) ;
        }
      if (draw_col)
        {
          graph_set->draw_col = *draw_col ;
          zmapWindowFeaturesetAllocColour(featureset_class, &(graph_set->draw_col)) ;
        }
      if (border_col)
        {
          graph_set->border_col = *border_col ;
          zmapWindowFeaturesetAllocColour(featureset_class, &(graph_set->border_col)) ;
        }
    }

  return ;
}







