/*  File: zmapWindowCanvasLocus.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 * Description:
 *
 * implements callback functions for FeaturesetItem locus features
 * locus features are single line (word?) things (the feature name with maybe a line for decoration)
 * unlike old style ZMWCI i called these locus not text as they are not generic text features
 * but instead a specific sequence region with a name
 *
 * of old there was talk of creating diff features externally for loci (i hear)
 * but here we simply have created a feature from the name and coords of the original, if it has a locus id
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.h>
#include <zmapWindowCanvasDraw.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasFeature_I.h>
#include <zmapWindowCanvasLocus_I.h>


static void zMapWindowCanvasLocusPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                              GdkDrawable *drawable, GdkEventExpose *expose) ;
static void zMapWindowCanvasLocusZoomSet(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable) ;
static void zMapWindowCanvasLocusFreeSet(ZMapWindowFeaturesetItem featureset) ;
static ZMapWindowCanvasFeature zMapWindowCanvasLocusAddFeature(ZMapWindowFeaturesetItem featureset,
                                                               ZMapFeature feature, double y1, double y2) ;
static double locusPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                         double item_x, double item_y, int cx, int cy,
                         double local_x, double local_y, double x_off) ;




/*
 *                    External interface routines.
 */


void zMapWindowCanvasLocusInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL };
  gpointer feature_funcs[CANVAS_FEATURE_FUNC_N_FUNC] = { NULL };

  funcs[FUNC_PAINT] = (void *)zMapWindowCanvasLocusPaintFeature ;
  funcs[FUNC_ZOOM] = (void *)zMapWindowCanvasLocusZoomSet ;
  funcs[FUNC_FREE] = (void *)zMapWindowCanvasLocusFreeSet ;
  funcs[FUNC_ADD] = (void *)zMapWindowCanvasLocusAddFeature ;
  funcs[FUNC_POINT] = (void *)locusPoint ;

  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_LOCUS, funcs, sizeof(zmapWindowCanvasLocusSetStruct));

  zMapWindowCanvasFeatureSetSize(FEATURE_LOCUS, feature_funcs, sizeof(zmapWindowCanvasLocusStruct)) ;

  return ;
}


void zMapWindowCanvasLocusSetFilter(ZMapWindowFeaturesetItem featureset, GList * filter)
{
  ZMapWindowCanvasLocusSet lset = NULL ;

  zMapReturnIfFail(featureset) ;

  lset = (ZMapWindowCanvasLocusSet) featureset->opt ;

  lset->filter = filter;

  return ;
}





/*
 *                    Internal routines
 */



static void zmapWindowCanvasLocusGetPango(GdkDrawable *drawable,
                                          ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasLocusSet lset)
{

  zMapReturnIfFail(featureset) ;

  /* lazy evaluation of pango renderer */
  if(lset)
    {
      if(lset->pango.drawable && lset->pango.drawable != drawable)
        zmapWindowCanvasFeaturesetFreePango(&lset->pango);

      if(!lset->pango.renderer)
        {
          GdkColor *draw;

          zMapStyleGetColours(featureset->style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, NULL, &draw, NULL);

          zmapWindowCanvasFeaturesetInitPango(drawable, featureset,
                                              &lset->pango, ZMAP_ZOOM_FONT_FAMILY, ZMAP_ZOOM_FONT_SIZE, draw);
        }
    }

  return ;
}



static void zMapWindowCanvasLocusPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                              GdkDrawable *drawable, GdkEventExpose *expose)
{
  double x1,x2;
  FooCanvasItem *foo = (FooCanvasItem *) featureset;
  ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) feature;
  ZMapWindowCanvasLocusSet lset = NULL ;
  char *text;
  int len;
  int cx1, cy1, cx2, cy2;

  zMapReturnIfFail(featureset && feature) ;

  lset = (ZMapWindowCanvasLocusSet) featureset->opt;


  zmapWindowCanvasLocusGetPango(drawable, featureset, lset);

  x1 = featureset->dx + featureset->x_off;
  x2 = x1 + locus->x_off;

  text = (char *) g_quark_to_string(feature->feature->original_id);
  len = strlen(text);
  pango_layout_set_text (lset->pango.layout, text, len);
  locus->x_wid = len * lset->pango.text_width;

  /* need to get pixel coordinates for pango */
  // (dy = start plus block offset)
  //        foo_canvas_w2c (foo->canvas, x1, locus->ylocus - featureset->start + featureset->dy, &cx1, &cy1);
  //        foo_canvas_w2c (foo->canvas, x2, locus->ytext - featureset->start + featureset->dy, &cx2, &cy2);
  foo_canvas_w2c (foo->canvas, x1, locus->ylocus, &cx1, &cy1);
  foo_canvas_w2c (foo->canvas, x2, locus->ytext, &cx2, &cy2);

  zMap_draw_line(drawable, featureset, cx1, cy1, cx2, cy2);

  cy2 -= lset->pango.text_height / 2;                /* centre text on line */
  /* NOTE this is equivalent to using feature->y1 and converting to canvas coords */
  pango_renderer_draw_layout (lset->pango.renderer, lset->pango.layout,  cx2 * PANGO_SCALE , cy2 * PANGO_SCALE);

  return ;
}


/* using world coords shift locus names around on the canvas so they don't overlap
 * the theory is that we have arranged for there to be enough space
 * but if not they will still overlap
 *
 * unlike the previous version this is less optimal, but generally tends to put the names nearer the loci
 * so it looks better with the downside of some overlap it it's a bit crowded.
 * we get groups overlapping, could do zebra stripe colour coding....
 */
/*! \todo #warning this function need shaking down a bit */


static double deOverlap(GList *visible,int n_loci, double text_h, double start, double end)
{
  LocusGroup group_span = NULL;
  GList *groups = NULL;
  int n_groups = 0;
  GList *l = NULL ;
  double needed = 0.0, spare = 0.0;
  //        double overlap;
  //        double group_sep;
  //        double item_sep;
  double cur_y = 0.0;
  double longest = 0.0 ;

  zMapReturnValIfFail(visible, longest) ;

  visible = g_list_sort(visible, zMapWindowFeatureCmp);

  /* arrange into groups of overlapping loci (not names).
   * NOTE the groups do not overlap at all,but the locus names may
   */
  for(l = visible;l; l = l->next)
    {
      ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) l->data;

      if(!group_span || locus->ytext > group_span->y2)
        /* simple comparison assuming data is in order of start coord */
        {
          /* does not overlap: start a new group */
          /* first save the span */

          group_span = g_new0(LocusGroupStruct,1);
          groups = g_list_append(groups,group_span);        /* append: keep in order */

          n_groups++;
          group_span->id = n_groups;

          /* store feature coords for sorting into groups */
          /* we create groups of loci that overlap as locus features
           * this is the only way to have stable groups as text itens
           * can overlap differently according to the window height
           * and NOTE it has more relevance biologically
           */
          group_span->y1 = locus->feature.feature->x1;
          group_span->y2 = locus->feature.feature->x2;
        }

      locus->group = group_span;

      if(locus->feature.feature->x2 > group_span->y2)
        group_span->y2 = locus->feature.feature->x2;

      group_span->height += text_h;
      //printf("locus %s in group %d %.1f,%.1f (%.1f)\n",g_quark_to_string(locus->feature.feature->original_id), group_span->id, group_span->y1, group_span->y2, group_span->height);

      needed += text_h;
    }

  spare = end - start - needed;

  /* de-overlap names in each group, extending downwards but leaving groups in the same place */
  /* change group span into text extent not locus extent */
  for(l = visible, group_span = NULL;l; l = l->next)
    {
      ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) l->data;

      if(group_span == locus->group)
        {
          /* may need this space */
          //                        if(locus->ytext < cur_y)
          locus->ytext = cur_y;
        }
      else
        {
          group_span = locus->group;
          group_span->y1 = locus->ytext;
        }
      cur_y = locus->ytext + text_h;
      group_span->y2 = cur_y;
      group_span->height = group_span->y2 - group_span->y1;
      //printf("locus %s/%d @ %.1f(%.1f)\n",g_quark_to_string(locus->feature.feature->original_id), group_span->id, locus->ytext, group_span->height);
    }

  /* de-overlap each group moving upwards  */
  for(l = g_list_last(groups), cur_y = end; l; l = l->prev)
    {
      group_span = (LocusGroup) l->data;

      if(cur_y - group_span->y2 > spare)
        {
          //                        group_span->y1 += spare;
        }

      /* as we extended loci downwards we can shift upwards, there will be space */
      if(group_span->y2 > cur_y)
        {
          double shift = group_span->y2 - cur_y;

          if(shift > 0.0)                /* we overlap the end or the following group */
            {
              group_span->y1 -= shift;
              group_span->y2 -= shift;
            }

          shift = start - group_span->y1;        /* but in case there is no spare space force to be on screen */
          if(shift > 0.0)
            {
              group_span->y1 += shift;
              group_span->y2 += shift;
            }

        }
      cur_y = group_span->y1;
      //printf("group %d @ %.1f\n",group_span->id,cur_y);
    }

  /* now move the loci to where the groups are */
  /* and set the feature extent so they paint */
  for(l = visible, group_span = NULL;l; l = l->next)
    {
      ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) l->data;

      if(group_span != locus->group)
        {
          group_span = locus->group;
          cur_y = group_span->y1;
        }

      locus->ytext = cur_y;

      locus->feature.y1 = cur_y - text_h / 2;
      locus->feature.y2 = cur_y + text_h / 2;

      if(locus->ylocus < locus->feature.y1)
        locus->feature.y1 = locus->ylocus;
      if(locus->ylocus > locus->feature.y2)
        locus->feature.y2 = locus->ylocus;

      if(locus->feature.y2 - locus->feature.y1 > longest)
        longest = locus->feature.y2 - locus->feature.y1;
      //printf("locus %s/%d @ %.1f (%.1f %.1f %.1f)\n",g_quark_to_string(locus->feature.feature->original_id), group_span->id, cur_y, locus->ylocus, locus->feature.y1, locus->feature.y2);

      cur_y += text_h;

    }

  if(groups)
    g_list_free(groups);

  g_list_free(visible);

  return (longest + 1) ;
}



static gboolean locusFeatureIsFiltered(GList *filters, char * locus)
{
  GList * l = NULL ;
  char *prefix;

  zMapReturnValIfFail(filters, FALSE) ;

  for(l = filters; l ; l = l->next)
    {
      prefix = (char *) l->data;
      if(!g_ascii_strncasecmp(locus, prefix,strlen(prefix)))
        return TRUE;
    }

  return FALSE;
}



/* de-overlap and hide features if necessary,
 * NOTE that we never change the zoom,
 * but we'll always get called once for display (NOTE before creating the index)
 * need to be sure we get called if features are added or deleted
 * or if the selection of which to display changes
 * or the window is resized
 */
static void zMapWindowCanvasLocusZoomSet(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
  //        ZMapSkipList sl;
  GList *l = NULL ;
  double len = 0.0, width = 0.0, f_width = 0.0 ;
  char *text = NULL ;
  int n_loci = 0;
  GList *visible = NULL;
  double span = 0.0 ;
  //        FooCanvasItem *foo = (FooCanvasItem *) featureset;

  zMapReturnIfFail(featureset) ;


  f_width = featureset->width ;
  span = featureset->end - featureset->start + 1.0;

  ZMapWindowCanvasLocusSet lset = (ZMapWindowCanvasLocusSet) featureset->opt;

  //printf("locus zoom\n");
  zmapWindowCanvasLocusGetPango(drawable, featureset, lset);

  featureset->width = 0.0;

  lset->text_h = lset->pango.text_height / featureset->zoom;        /* can't use foo_canvas_c2w as it does a scroll offset */


  /* but normally we get called before the index is created */
  for(l = featureset->features; l ; l = l->next)
    {
      ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) l->data;

      locus->ylocus = locus->feature.feature->x1;
      locus->ytext = locus->feature.feature->x1;
      locus->x_off = ZMAP_LOCUS_LINE_WIDTH;

      text = (char *) g_quark_to_string(locus->feature.feature->original_id);
      len = strlen(text);

      if(locusFeatureIsFiltered(lset->filter, text))
        {
          locus->feature.flags |= FEATURE_HIDDEN | FEATURE_HIDE_FILTER;
        }
      else
        {
          /* make visible */
          locus->feature.flags &= ~FEATURE_HIDE_FILTER;
          if(!(locus->feature.flags & FEATURE_HIDE_REASON))
            locus->feature.flags &= ~FEATURE_HIDDEN;

          /* expand the column if needed */
          width = locus->x_off + len * lset->pango.text_width;
          if(width > featureset->width)
            featureset->width = width;
          n_loci++;
          visible = g_list_prepend(visible,(gpointer) locus);
          //printf("visible locus %s\n",g_quark_to_string(locus->feature.feature->original_id));
        }
    }
  /* do more filtering if there are too many loci */

  if(n_loci * lset->text_h > span)
    {
      //                zMapWarning("Need to filter some more %d %.1f %.1f",n_loci, lset->text_h, span);
    }

  if(f_width != featureset->width)
    {
      FooCanvasItem * foo = (FooCanvasItem *) featureset;

      zMapWindowRequestReposition(foo);

      /* we only know the width after drawing, so we need anotehr expose */
      foo_canvas_update_now(foo->canvas);
      foo_canvas_item_request_redraw(foo->canvas->root);
    }



  /* de-overlap the loci left over
   * this will move features around and expand their extents
   * after this function the featureset code will create the index
   */
  if(n_loci > 1)
    featureset->longest = deOverlap(visible, n_loci, lset->text_h, featureset->start, featureset->end);
}



static void zMapWindowCanvasLocusFreeSet(ZMapWindowFeaturesetItem featureset)
{
  ZMapWindowCanvasLocusSet lset = NULL ;

  zMapReturnIfFail(featureset) ;
  lset = (ZMapWindowCanvasLocusSet) featureset->opt;

  if(lset)
    zmapWindowCanvasFeaturesetFreePango(&lset->pango);
}


static ZMapWindowCanvasFeature zMapWindowCanvasLocusAddFeature(ZMapWindowFeaturesetItem featureset,
                                                               ZMapFeature feature, double y1, double y2)
{
  ZMapWindowCanvasFeature feat = NULL ;

  zMapReturnValIfFail(featureset, feat);

  feat = zMapWindowFeaturesetAddFeature(featureset, feature, y1, y2);
  featureset->recalculate_zoom = TRUE;
  //featureset->zoom = 0.0;                /* force recalc of de-overlap: gb10 shouldn't be necessary now we have the recalculate_zoom flag */
  /* force display at all */
  //        printf("added locus feature %s\n",g_quark_to_string(feature->original_id));

  return feat;
}



/* the Canvas feature extends around the text and the line pointing at the start of the locus
 * but we want to click on the text part only
 * the CanvasFeatureset needs the visible extent and point has to use the text
 */
static double locusPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                         double item_x, double item_y, int cx, int cy,
                         double local_x, double local_y, double x_off)
{
  double best = 1e36;
  ZMapWindowCanvasLocus locus = NULL ;
  ZMapWindowCanvasLocusSet lset = NULL ;

  zMapReturnValIfFail(gs && fi, best) ;
  locus = (ZMapWindowCanvasLocus) gs;
  lset = (ZMapWindowCanvasLocusSet) fi->opt;

  /* function interface looks too complex, let's just use the feature info */
  double ytext = locus->ytext - lset->text_h / 2 - fi->dy + 1;

  if(item_y >= ytext && item_y <= (ytext + lset->text_h))
    {
      double x1, x2;

      x1 = locus->x_off;
      x2 = x1 + locus->x_wid;

      if(item_x >= x1 && item_x <= x2)
        best = 0.0;
    }
  //printf("locus point: %.1f %s (%.1f, %.1f) -> %.1f\n", item_y, g_quark_to_string(gs->feature->unique_id), ytext, ytext + lset->text_h, best > 0.0 ? 1.0 : 0.0);

  return best;
}


