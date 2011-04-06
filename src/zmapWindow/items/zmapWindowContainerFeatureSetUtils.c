/*  File: zmapWindowCollectionFeature.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * Description: Functions to handle various functions of column sets,
 *              e.g. adding colinear lines to alignments.
 *
 * Exported functions: zmapWindowContainerFeatureSet.h
 * HISTORY:
 * Last edited: May 24 12:05 2010 (edgrif)
 * Created: Wed Dec  3 10:02:22 2008 (rds)
 * CVS info:   $Id: zmapWindowContainerFeatureSetUtils.c,v 1.13 2011-04-06 13:43:51 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>





#include <math.h>
#include <glib.h>


/* WHY IS THIS NEEDED HERE ???? */
#include <zmapWindowCanvasItem_I.h>

#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerFeatureSet_I.h>


#define DEBUG_PINK_COLLECTION_FEATURE_BACKGROUND



typedef enum {FIRST_MATCH, LAST_MATCH} MatchType ;


typedef struct
{
  ZMapWindowCanvasItem parent;
  FooCanvasGroup *new_parent;
  double x, y;
} ParentPositionStruct, *ParentPosition;


typedef struct
{
  ZMapWindowContainerFeatureSet feature_set ;
  FooCanvasItem       *previous;
  ZMapFeature          prev_feature;
  GdkColor             perfect;
  GdkColor             colinear;
  GdkColor             non_colinear;
  double               x;
  double               block_offset;
  ZMapFeatureCompareFunc compare_func;
  gpointer               compare_data;
} ColinearMarkerDataStruct, *ColinearMarkerData ;




/* Accessory functions, some from foo-canvas.c */
static double get_glyph_mid_point(FooCanvasItem *item);
static void add_colinear_lines_and_markers(gpointer data, gpointer user_data);
static void markMatchIfIncomplete(ZMapWindowContainerFeatureSet feature_set,
				  ZMapStrand ref_strand, MatchType match_type,
				  FooCanvasItem *item, ZMapFeature feature,int block_offset) ;
static gboolean fragments_splice(char *fragment_a, char *fragment_b, gboolean reversed);
//static void process_feature(ZMapFeature prev_feature);
static void destroyListAndData(GList *item_list) ;
static void itemDestroyCB(gpointer data, gpointer user_data);



/* Local globals. */




/*
 *                           External interface
 */



void zMapWindowContainerFeatureSetAddColinearMarkers(ZMapWindowContainerFeatureSet feature_set,
						     GList *feature_list,
						     ZMapFeatureCompareFunc compare_func,
						     gpointer compare_data, int block_offset)
{
  ColinearMarkerDataStruct colinear_data = {NULL};
  ZMapFeatureTypeStyle style;
  double x2;
  char *perfect_colour     = ZMAP_WINDOW_MATCH_PERFECT ;
  char *colinear_colour    = ZMAP_WINDOW_MATCH_COLINEAR ;
  char *noncolinear_colour = ZMAP_WINDOW_MATCH_NOTCOLINEAR ;
  FooCanvasItem *current;

  zMapAssert(feature_set && feature_list && compare_func) ;

  colinear_data.feature_set = feature_set ;

  colinear_data.previous     = NULL; // FOO_CANVAS_ITEM(feature_list->data);
  colinear_data.prev_feature = NULL; // ZMAP_CANVAS_ITEM(colinear_data.previous)->feature;

  colinear_data.compare_func = compare_func;
  colinear_data.compare_data = compare_data;

  current = FOO_CANVAS_ITEM(feature_list->data);
  style = (ZMAP_CANVAS_ITEM_GET_CLASS(current)->get_style)(ZMAP_CANVAS_ITEM(current)) ;

  x2 = zMapStyleGetWidth(style);

  colinear_data.x = (x2 * 0.5);

  colinear_data.block_offset = block_offset;

  gdk_color_parse(perfect_colour,     &colinear_data.perfect) ;
  gdk_color_parse(colinear_colour,    &colinear_data.colinear) ;
  gdk_color_parse(noncolinear_colour, &colinear_data.non_colinear) ;

      /* process each item and then the end of list to pick up th elast homology marker*/
  g_list_foreach(feature_list, add_colinear_lines_and_markers, &colinear_data);
  add_colinear_lines_and_markers(NULL,&colinear_data);

  return ;
}



void zMapWindowContainerFeatureSetRemoveSubFeatures(ZMapWindowContainerFeatureSet container)
{

  if (container->colinear_markers)
    {
      destroyListAndData(container->colinear_markers) ;
      container->colinear_markers = NULL ;
    }

  if (container->incomplete_markers)
    {
      destroyListAndData(container->incomplete_markers) ;
      container->incomplete_markers = NULL ;
    }

  if (container->splice_markers)
    {
      destroyListAndData(container->splice_markers) ;
      container->splice_markers = NULL ;
    }

  return ;
}






/*
 *                    Internal routines.
 */









#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* WATCH OUT...THE REMOVE SUB FEATURES NEEDS TO BE ADDED TO THE FEATURE SET DESTROY !! */

static void zmap_window_collection_feature_destroy     (GObject *object)
{
  GObjectClass *object_class;

  zMapWindowCollectionFeatureRemoveSubFeatures(ZMAP_CANVAS_ITEM(object), TRUE, TRUE);

  object_class = (GObjectClass *)canvas_parent_class_G;

  if(object_class->dispose)
    (object_class->dispose)(object);

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



int add_nc_splice_markers(ZMapWindowContainerFeatureSet feature_set, int block_offset,
      ZMapWindowCanvasItem curr_item, ZMapWindowCanvasItem prev_item,
      ZMapFeature curr_feature,ZMapFeature prev_feature)
{
      char *prev, *curr;
      gboolean prev_reversed, curr_reversed;
      int added = 0;

      prev_reversed = prev_feature->strand == ZMAPSTRAND_REVERSE;
      curr_reversed = curr_feature->strand == ZMAPSTRAND_REVERSE;

#if 0
zMapLogWarning("splice %s -> %s", g_quark_to_string(prev_feature->unique_id), g_quark_to_string(curr_feature->unique_id));
#endif

      /* MH17 NOTE
       *
       * for reverse strand we need to splice backwards
       * logically we could has a mixed series
       * we do get duplicate series and these can be reversed
       * we assume any reversal is a series break
       * and do not attempt to splice inverted exons
       */

          // 3' end of exon: get 1 base  + 2 from intron
      prev = zMapFeatureGetDNA((ZMapFeatureAny)prev_feature,
                         prev_feature->x2,
                         prev_feature->x2 + 2,
                         prev_reversed);

            // 5' end of exon: get 2 bases from intron
      curr = zMapFeatureGetDNA((ZMapFeatureAny)curr_feature,
                         curr_feature->x1 - 2,
                         curr_feature->x1,
                         curr_reversed);

      if ((prev_reversed == curr_reversed) && !fragments_splice(prev, curr, prev_reversed))
      {
        FooCanvasGroup *parent;
        ZMapFeatureTypeStyle style;

        parent = FOO_CANVAS_GROUP(zmapWindowContainerGetOverlay(ZMAP_CONTAINER_GROUP(feature_set))) ;

        style = curr_feature->style;

        if (style)
          {
            /* do sub-feature bit or not needed now ? */
            style = curr_feature->style->sub_style[ZMAPSTYLE_SUB_FEATURE_NON_CONCENCUS_SPLICE];
              if (style)
                {
                  ZMapWindowGlyphItem glyph ;
                  double x_coord;

                  x_coord = get_glyph_mid_point((FooCanvasItem *) curr_item);

                  glyph = zMapWindowGlyphItemCreate(parent, style, prev_reversed? 5 : 3,
                                          x_coord, prev_feature->x2 - block_offset + 1, 0, FALSE) ;

                  /* Record the item so we can delete it later. */
                  if(glyph)
                  {
                        feature_set->splice_markers = g_list_append(feature_set->splice_markers, glyph) ;
                        added++;
                  }

                  glyph = zMapWindowGlyphItemCreate(parent, style, prev_reversed? 3 : 5,
                                          x_coord, curr_feature->x1 - block_offset, 0, FALSE) ;

                  /* Record the item so we can delete it later. */
                  if(glyph)
                  {
                        feature_set->splice_markers = g_list_append(feature_set->splice_markers, glyph) ;
                        added++;
                  }
                }
            }
      }

      /* free */
      if(prev)
      g_free(prev);
      if(curr)
      g_free(curr);
      return added;
}

static void add_colinear_lines_and_markers(gpointer data, gpointer user_data)
{
  ColinearMarkerData colinear_data = (ColinearMarkerData)user_data;
  ZMapWindowContainerFeatureSet feature_set ;
  FooCanvasItem *current = FOO_CANVAS_ITEM(data);
  FooCanvasItem *previous;
  ZMapFeature prev_feature = NULL, curr_feature = NULL;
  GdkColor *draw_colour;
  FooCanvasPoints line_points;
  double coords[4], y1, y2;
  ColinearityType colinearity = 0;

  feature_set = colinear_data->feature_set ;

  /* NOTE current and previous can be NULL if we are at the start or end of the list */
  previous = colinear_data->previous;
  colinear_data->previous = current;

  prev_feature = colinear_data->prev_feature;
  if(current)
      curr_feature = ZMAP_CANVAS_ITEM(current)->feature;
  colinear_data->prev_feature = curr_feature;

  if ((colinearity = (colinear_data->compare_func)(prev_feature, curr_feature,
						   colinear_data->compare_data)) == COLINEAR_INVALID)
    {
            /* must be start or end of a sequence */
            MatchType prev_match = LAST_MATCH;
            MatchType curr_match = FIRST_MATCH;
            ZMapStrand ref_strand ;

            /* add end homology marker to previous if present */
            if(previous)
            {
              ref_strand = prev_feature->strand ;
              if(ref_strand ==ZMAPSTRAND_REVERSE)
                  prev_match = FIRST_MATCH;
              markMatchIfIncomplete(feature_set, ref_strand, prev_match, previous, prev_feature, colinear_data->block_offset) ;
            }

            /* add start homology marker to current if present */
            if(current)
            {
              ref_strand = curr_feature->strand ;
              if(ref_strand ==ZMAPSTRAND_REVERSE)
                  curr_match = LAST_MATCH;
              markMatchIfIncomplete(feature_set, ref_strand, curr_match, current, curr_feature, colinear_data->block_offset) ;
            }
    }
  else      /* we must have two features and they are in the same series */
    {
      FooCanvasGroup *canvas_group;
      FooCanvasItem *colinear_line;
      double px1, py1, px2, py2, cx1, cy1, cx2, cy2 ;
      double mid_x ;
      FooCanvasGroup *parent ;

      if (colinearity == COLINEAR_NOT)
	draw_colour = &colinear_data->non_colinear ;
      else if (colinearity == COLINEAR_IMPERFECT)
	draw_colour = &colinear_data->colinear ;
      else
	draw_colour = &colinear_data->perfect ;

      foo_canvas_item_get_bounds(previous, &px1, &py1, &px2, &py2) ;
      foo_canvas_item_get_bounds(current,  &cx1, &cy1, &cx2, &cy2) ;

      canvas_group = (FooCanvasGroup *)feature_set ;

      y1 = floor(py2);
      y2 = ceil (cy1);

      y1 = prev_feature->x2 - colinear_data->block_offset;
      y2 = curr_feature->x1 - colinear_data->block_offset;

      mid_x = get_glyph_mid_point(previous) ;

      coords[0] = mid_x ;
      coords[1] = y1 ;
      coords[2] = mid_x ;
      coords[3] = y2 ;

      line_points.coords     = coords;
      line_points.num_points = 2;
      line_points.ref_count  = 1;

      parent = FOO_CANVAS_GROUP(zmapWindowContainerGetUnderlay(ZMAP_CONTAINER_GROUP(feature_set))) ;

      colinear_line = foo_canvas_item_new(parent,
					  foo_canvas_line_get_type(),
					  "width_pixels",   1,
					  "points",         &line_points,
					  "fill_color_gdk", draw_colour,
					  NULL);


      /* mh17: not sure if this is valid/safe..
       * can't find any definition anywhere of what map and realize signify,
       * can only see that they set the MAPPED and REALIZED flags
       * However, this causes the underlay and overlay features to draw as required.
       * Q: would CollectionFeature be better coded as another layer of CanvasItemGroup?
       * then we'd only have one lot of code to debug
       */
      // see also gylphs
      // these don't work!
      if(!(colinear_line->object.flags & FOO_CANVAS_ITEM_REALIZED))
	FOO_CANVAS_ITEM_GET_CLASS(colinear_line)->realize (colinear_line);
      if(!(colinear_line->object.flags & FOO_CANVAS_ITEM_MAPPED))
	FOO_CANVAS_ITEM_GET_CLASS(colinear_line)->map (colinear_line);



      /* THIS SEEMS TO CAUSE A CRASH.... */
      {
	FooCanvasItem *new ;

	new = zmapWindowLongItemCheckPointFull(colinear_line, &line_points, 0.0, 0.0, 0.0, 0.0) ;

	if (new != colinear_line)
	  {
	    colinear_line = new ;
	  }


	/* Record the item so we can delete it later. */
	feature_set->colinear_markers = g_list_append(feature_set->colinear_markers, colinear_line) ;


      add_nc_splice_markers(feature_set, colinear_data->block_offset, (ZMapWindowCanvasItem) current, (ZMapWindowCanvasItem) previous, curr_feature, prev_feature);

      }

    }

  return ;
}



/* Mark a first or last feature in a series of alignment features as incomplete.
 * The first feature is incomplete if it's start > 1, the last is incomplete
 * if it's end < homol_length.
 *
 * Code is complicated by markers/coord testing needing to be reversed for
 * alignments to reverse strand. */
static void markMatchIfIncomplete(ZMapWindowContainerFeatureSet feature_set,
				  ZMapStrand ref_strand, MatchType match_type,
				  FooCanvasItem *item, ZMapFeature feature,int block_offset)
{
  int start, end ;

  if (match_type == FIRST_MATCH)
    {
      start = 1 ;
      end = feature->feature.homol.y1 ;
    }
  else
    {
      start = feature->feature.homol.y2 ;
      end = feature->feature.homol.length ;
    }

  if (start < end)
    {
      double x_coord, y_coord ;
      int diff;
      ZMapFeatureTypeStyle style, sub_style = NULL;
      FooCanvasItem *foo;
//      GQuark id;
      FooCanvasGroup *parent;

      style = feature->style;

      // if homology is configured and we have the style...
      sub_style = feature->style->sub_style[ZMAPSTYLE_SUB_FEATURE_HOMOLOGY];

      // otherwise (eg style from ACEDB) if legacy switched on then invent it
      if(!sub_style)
        sub_style = zMapStyleLegacyStyle((char *) zmapStyleSubFeature2ExactStr(ZMAPSTYLE_SUB_FEATURE_HOMOLOGY));

      if(sub_style)
        {
	  x_coord = get_glyph_mid_point(item) ;

	  if ((match_type == FIRST_MATCH && ref_strand == ZMAPSTRAND_FORWARD)
	      || (match_type == LAST_MATCH && ref_strand == ZMAPSTRAND_REVERSE))
	    {
      	      y_coord = feature->x1 - block_offset;
	    }
	  else
	    {
      	      y_coord = feature->x2 - block_offset + 1;
	    }

	  diff = end - start;

	  parent = FOO_CANVAS_GROUP(zmapWindowContainerGetOverlay(ZMAP_CONTAINER_GROUP(feature_set))) ;

	  foo = FOO_CANVAS_ITEM(zMapWindowGlyphItemCreate(parent,
							  sub_style, (match_type == FIRST_MATCH) ? 5 : 3,
							  x_coord,y_coord,diff, FALSE));

	  /* Record the item so we can delete it later. */
        if(foo)
	      feature_set->incomplete_markers = g_list_append(feature_set->incomplete_markers, foo) ;

	  /* mh17: not sure if this is valid/safe..we're not using the layers that have been set up so carefully
	   * can't find any definition anywhere of what map and realize signify,
	   * can only see that they set the MAPPED and REALIZED flags
	   * However, this causes the underlay and overlay features to draw as required.
	   * Q: would CollectionFeature be better coded as another layer of CanvasItemGroup?
	   * then we'd only have one lot of code to debug
	   */
	  // see also colinear lines
	  // these don't work!
	  if(foo)
            {
	      if(!(foo->object.flags & FOO_CANVAS_ITEM_REALIZED))
		FOO_CANVAS_ITEM_GET_CLASS(foo)->realize (foo);
	      if(!(foo->object.flags & FOO_CANVAS_ITEM_MAPPED))
		FOO_CANVAS_ITEM_GET_CLASS(foo)->map (foo);
            }
        }
    }

  return ;
}



/*
James says:

These are the rules I implemented in the ExonCanvas for flagging good splice sites:

1.)  Canonical splice donor sites are:

    Exon|Intron
        |gt
       g|gc

2.)  Canonical splice acceptor site is:

    Intron|Exon
        ag|
*/

/*
These can be configured in/out via styles:
sub-features=non-concensus-splice:nc-splice-glyph
*/

static gboolean fragments_splice(char *fragment_a, char *fragment_b, gboolean reversed)
{
  gboolean splice = FALSE;
  char spliceosome[7];

    // NB: DNA always reaches us as lower case, see zmapUtils/zmapDNA.c
  if(!fragment_a || !fragment_b)
    return(splice);

  if(reversed)    /* bases have already been complemented & order reversed */
  {
      spliceosome[0] = fragment_b[2];
      spliceosome[1] = fragment_b[1];
      spliceosome[2] = fragment_b[0];
      spliceosome[3] = fragment_a[0];
      spliceosome[4] = fragment_a[1];
      spliceosome[5] = '\0';
  }
  else
  {
      spliceosome[0] = fragment_a[0];
      spliceosome[1] = fragment_a[1];
      spliceosome[2] = fragment_a[2];
      spliceosome[3] = fragment_b[0];
      spliceosome[4] = fragment_b[1];
      spliceosome[5] = '\0';
  }

  if(!g_ascii_strncasecmp(fragment_b, "AG",2))
    {
      if(!g_ascii_strncasecmp(&spliceosome[1],"GT",2) || !g_ascii_strncasecmp(&spliceosome[0],"GGC",3))
           splice = TRUE;
    }
#if 0
zMapLogWarning("nc splice %s %d = %d",spliceosome,reversed,splice);
#endif
  return splice;
}




/* Note that however we do this calculation in the end where an item is an even number
 * of pixels across we end up jumping one side of the other of the true middle. */
static double get_glyph_mid_point(FooCanvasItem *item)
{
  double mid_x = 0.0 ;
  double x1, x2, y1, y2 ;

  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  mid_x = x1 + (((x2 - x1) - 1) * 0.5) ;

  return mid_x ;
}


/* Two small functions to free lists of canvas items. */
static void destroyListAndData(GList *item_list)
{
  g_list_foreach(item_list, itemDestroyCB, NULL) ;

  g_list_free(item_list) ;

  return ;
}

static void itemDestroyCB(gpointer data, gpointer user_data_unused)
{
  gtk_object_destroy(GTK_OBJECT(data)) ;

  return ;
}



