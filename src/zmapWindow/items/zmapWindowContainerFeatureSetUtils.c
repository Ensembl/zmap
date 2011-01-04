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
 * CVS info:   $Id: zmapWindowContainerFeatureSetUtils.c,v 1.8 2011-01-04 11:10:23 mh17 Exp $
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
  ZMapFeatureCompareFunc compare_func;
  gpointer               compare_data;
} ColinearMarkerDataStruct, *ColinearMarkerData ;




/* Accessory functions, some from foo-canvas.c */
static double get_glyph_mid_point(FooCanvasItem *item);
static void add_colinear_lines(gpointer data, gpointer user_data);
static void markMatchIfIncomplete(ZMapWindowContainerFeatureSet feature_set,
				  ZMapStrand ref_strand, MatchType match_type,
				  FooCanvasItem *item, ZMapFeature feature) ;
static gboolean fragments_splice(char *fragment_a, char *fragment_b);
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
						     gpointer compare_data)
{
  ColinearMarkerDataStruct colinear_data = {NULL};
  ZMapFeatureTypeStyle style;
  double x2;
  char *perfect_colour     = ZMAP_WINDOW_MATCH_PERFECT ;
  char *colinear_colour    = ZMAP_WINDOW_MATCH_COLINEAR ;
  char *noncolinear_colour = ZMAP_WINDOW_MATCH_NOTCOLINEAR ;


  zMapAssert(feature_set && feature_list && compare_func) ;

  colinear_data.feature_set = feature_set ;

  colinear_data.previous     = FOO_CANVAS_ITEM(feature_list->data);
  colinear_data.prev_feature = ZMAP_CANVAS_ITEM(colinear_data.previous)->feature;

  colinear_data.compare_func = compare_func;
  colinear_data.compare_data = compare_data;

  style
    = (ZMAP_CANVAS_ITEM_GET_CLASS(colinear_data.previous)->get_style)(ZMAP_CANVAS_ITEM(colinear_data.previous)) ;

  x2 = zMapStyleGetWidth(style);

  colinear_data.x = (x2 * 0.5);

  gdk_color_parse(perfect_colour,     &colinear_data.perfect) ;
  gdk_color_parse(colinear_colour,    &colinear_data.colinear) ;
  gdk_color_parse(noncolinear_colour, &colinear_data.non_colinear) ;

  g_list_foreach(feature_list->next, add_colinear_lines, &colinear_data);

  return ;
}




void zMapWindowContainerFeatureSetAddIncompleteMarkers(ZMapWindowContainerFeatureSet feature_set,
						       GList *feature_list, gboolean revcomped_features)
{
  ZMapWindowCanvasItem canvas_item ;
  FooCanvasItem *first_item, *last_item ;
  ZMapFeature first_feature, last_feature ;
  ZMapStrand ref_strand ;

  /* Assume that match is forward so first match is first item, last match is last item. */
  first_item   = FOO_CANVAS_ITEM(feature_list->data);
  canvas_item  = ZMAP_CANVAS_ITEM(first_item);
  first_feature = canvas_item->feature;

  last_item = FOO_CANVAS_ITEM(g_list_last(feature_list)->data);
  canvas_item  = ZMAP_CANVAS_ITEM(last_item);
  last_feature = canvas_item->feature;

  /* Now we have a feature we can retrieve which strand of the reference sequence the
   * alignment is to. */
  ref_strand = first_feature->strand ;

  /* If the alignment is to the reverse strand of the reference sequence then swop first and last. */
  if (ref_strand == ZMAPSTRAND_REVERSE)
    {
      last_item   = FOO_CANVAS_ITEM(feature_list->data);
      canvas_item  = ZMAP_CANVAS_ITEM(last_item);
      last_feature = canvas_item->feature;

      first_item    = FOO_CANVAS_ITEM(g_list_last(feature_list)->data);
      canvas_item  = ZMAP_CANVAS_ITEM(first_item);
      first_feature = canvas_item->feature;
    }

  /* If either the first or last match features are truncated then add the incomplete marker. */
  markMatchIfIncomplete(feature_set, ref_strand, FIRST_MATCH, first_item, first_feature) ;

  markMatchIfIncomplete(feature_set, ref_strand, LAST_MATCH, last_item, last_feature) ;

  return ;
}



void zMapWindowContainerFeatureSetAddSpliceMarkers(ZMapWindowContainerFeatureSet feature_set, GList *feature_list)
{
  double x_coord;
  gboolean canonical ;
  ZMapFeatureTypeStyle style;
  GList *list;
  ZMapFeature prev_feature;
  ZMapFeature curr_feature;

  canonical = FALSE ;

//  if((prev_feature = zMapWindowCanvasItemGetFeature(feature_list->data)))
//    process_feature(prev_feature);
  prev_feature = zMapWindowCanvasItemGetFeature(feature_list->data);


  if((list = feature_list->next))
    x_coord = get_glyph_mid_point(list->data);

  while (list && prev_feature)
    {
      char *prev, *curr;
      gboolean prev_reversed, curr_reversed;

      curr_feature = zMapWindowCanvasItemGetFeature(list->data);

      prev_reversed = prev_feature->strand == ZMAPSTRAND_REVERSE;
      curr_reversed = curr_feature->strand == ZMAPSTRAND_REVERSE;

//      process_feature(curr_feature);

            // 3' end of exon: get 1 base  + 2 from intron
      prev = zMapFeatureGetDNA((ZMapFeatureAny)prev_feature,
			       prev_feature->x2,
			       prev_feature->x2 + 2,
			       prev_reversed);

            // 5' end of exon: get 2 bases from intron
      curr = zMapFeatureGetDNA((ZMapFeatureAny)curr_feature,
			       curr_feature->x1 - 2,
			       curr_feature->x1 - 1,
			       curr_reversed);

      if (prev && curr && fragments_splice(prev, curr))
	{
	  FooCanvasGroup *parent;
//	  GQuark id;

	  parent = FOO_CANVAS_GROUP(zmapWindowContainerGetOverlay(ZMAP_CONTAINER_GROUP(feature_set))) ;

        style = curr_feature->style;

	  if (style)
	    {
	      /* do sub-feature bit or not needed now ? */
            style = curr_feature->style->sub_style[ZMAPSTYLE_SUB_FEATURE_NON_CONCENCUS_SPLICE];
		  if (style)
		    {
		      ZMapWindowGlyphItem glyph ;

		      glyph = zMapWindowGlyphItemCreate(parent, style, 3,
							x_coord, prev_feature->x2, 0, FALSE) ;

		      /* Record the item so we can delete it later. */
                  if(glyph)
		            feature_set->splice_markers = g_list_append(feature_set->splice_markers, glyph) ;

		      glyph = zMapWindowGlyphItemCreate(parent, style, 5,
							x_coord, curr_feature->x1 - 1, 0, FALSE) ;

		      /* Record the item so we can delete it later. */
                  if(glyph)
		            feature_set->splice_markers = g_list_append(feature_set->splice_markers, glyph) ;
		    }
		}
	}

      /* free */
      if(prev)
	g_free(prev);
      if(curr)
	g_free(curr);

      /* move on. */
      prev_feature = curr_feature;

      list = list->next;
    }

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



static void add_colinear_lines(gpointer data, gpointer user_data)
{
  ColinearMarkerData colinear_data = (ColinearMarkerData)user_data;
  ZMapWindowContainerFeatureSet feature_set ;
  FooCanvasItem *current = FOO_CANVAS_ITEM(data);
  FooCanvasItem *previous;
  ZMapFeature prev_feature, curr_feature;
  GdkColor *draw_colour;
  FooCanvasPoints line_points;
  double coords[4], y1, y2;
  ColinearityType colinearity = 0;

  feature_set = colinear_data->feature_set ;

  previous = colinear_data->previous;
  colinear_data->previous = current;

  prev_feature = colinear_data->prev_feature;
  curr_feature = ZMAP_CANVAS_ITEM(current)->feature;
  colinear_data->prev_feature = curr_feature;

  if ((colinearity = (colinear_data->compare_func)(prev_feature, curr_feature,
						   colinear_data->compare_data)) != COLINEAR_INVALID)
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

      y1 = prev_feature->x2 - canvas_group->ypos;
      y2 = curr_feature->x1 - canvas_group->ypos - 1.0; /* Ext2Zero */

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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Record the item so we can delete it later. */
      feature_set->colinear_markers = g_list_append(feature_set->colinear_markers, colinear_line) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      // canvas_item->debug = 1;

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
				  FooCanvasItem *item, ZMapFeature feature)
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
      	      y_coord = feature->x1 - ((FooCanvasGroup *)feature_set)->ypos - 1.0 ; /* Ext2Zero */
	    }
	  else
	    {
      	      y_coord = feature->x2 - ((FooCanvasGroup *)feature_set)->ypos;
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

static gboolean fragments_splice(char *fragment_a, char *fragment_b)
{
  gboolean splice = FALSE;
  char spliceosome[6];

    // NB: DNA always reaches us as lower case, see zmapUtils/zmapDNA.c
  if(fragment_a && fragment_b)
    {
      spliceosome[0] = fragment_a[0];
      spliceosome[1] = fragment_a[1];
      spliceosome[2] = fragment_a[2];
      spliceosome[3] = fragment_b[0];
      spliceosome[4] = fragment_b[1];
      spliceosome[5] = '\0';

#define NEW_RULES 1
#if NEW_RULES
      if(!g_ascii_strcasecmp(fragment_b, "AG"))
        {
          if(!g_ascii_strcasecmp(&spliceosome[1], "GT") || !g_ascii_strcasecmp(&spliceosome[0], "GGC"))
	        splice = TRUE;
	  }
#else
      if(g_ascii_strcasecmp(&spliceosome[1], "GTAG") == 0)
      {
        splice = TRUE;
      }
      else if(g_ascii_strcasecmp(&spliceosome[1], "GCAG") == 0)
      {
        splice = TRUE;
      }
      else if(g_ascii_strcasecmp(&spliceosome[1], "ATAC") == 0)
      {
        splice = TRUE;
      }
#endif
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if(splice)
    {
      printf("splices: %s\n", &spliceosome[0]);
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return splice;
}


// mh17: this function does not do anything other than allocate some memory look at it and free it
// was it an experiment?
#if 0
static void process_feature(ZMapFeature prev_feature)
{
  int i;
  gboolean reversed;

  reversed = prev_feature->strand == ZMAPSTRAND_REVERSE;

  if(prev_feature->feature.homol.align &&
     prev_feature->feature.homol.align->len > 1)
    {
      ZMapAlignBlock prev_align, curr_align;
      prev_align = &(g_array_index(prev_feature->feature.homol.align,
				   ZMapAlignBlockStruct, 0));

      for(i = 1; i < prev_feature->feature.homol.align->len; i++)
	{
	  char *prev, *curr;
	  curr_align = &(g_array_index(prev_feature->feature.homol.align,
				       ZMapAlignBlockStruct, i));

	  if(prev_align->t2 + 4 < curr_align->t1)
	    {
	      prev = zMapFeatureGetDNA((ZMapFeatureAny)prev_feature,
				       prev_align->t2,
				       prev_align->t2 + 2,
				       reversed);
	      curr = zMapFeatureGetDNA((ZMapFeatureAny)prev_feature,
				       curr_align->t1 - 2,
				       curr_align->t1 - 1,
				       reversed);
	      if(prev && curr)
		fragments_splice(prev, curr);

	      if(prev)
		g_free(prev);
	      if(curr)
		g_free(curr);
	    }

	  prev_align = curr_align;
	}
    }

  return ;
}
#endif


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



