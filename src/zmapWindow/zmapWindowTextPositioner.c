/*  File: zmapWindowTextPositioner.c
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
 * Description:
 *
 * TODO: destroy functions
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Feb 24 14:14 2011 (edgrif)
 * Created: Thu Jan 18 16:19:10 2007 (rds)
 * CVS info:   $Id: zmapWindowTextPositioner.c,v 1.13 2011-03-14 11:35:18 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowTextFeature.h>


enum
  {
    MAXIMUM_ITERATIONS = 3
  };

typedef struct
{
  int point1;
  int point2;
}TextCanvasSpanStruct, *TextCanvasSpan;

typedef struct
{
  double x1;
  double x2;
}TextPointStruct, *TextPoint;

/* -------------------------- */

/* The main object structs... */

/* The parent object and only external */
typedef struct _ZMapWindowTextPositionerStruct
{
  FooCanvasGroup *container_overlay;
  int maximum_iterations;
  double col_min, col_max;
  GList *groups;
}ZMapWindowTextPositionerStruct;

/* the positioner has a list of groups... */
typedef  struct
{
  int list_length;
  GList *text_objects;
  double col_min, col_max;
}TextPositionerGroupStruct, *TextPositionerGroup;

/* each group a list of entries which = a line of text. */
typedef struct
{
  FooCanvasItem *text;
  TextPointStruct original_point;
  GQuark locus_name;
}TextItemStruct, *TextItem;

/* -------------------------- */

/* structs for all the g_list_foreach goings on. */
typedef struct
{
  GList *list_item;
  double line_height, min, max;
  gboolean center;
}UnOverlapItemsDataStruct, *UnOverlapItemsData;

typedef struct
{
  TextPositionerGroup ignore;
  TextCanvasSpan span;
}FindOverlapGroupStruct, *FindOverlapGroup;

typedef struct
{
  GList *merged_groups;
  ZMapWindowTextPositioner positioner;
}MergeDataStruct, *MergeData;

typedef struct
{
  FooCanvasGroup  *container_overlay;
  FooCanvasPoints *line_points;
}DrawOverlayLineDataStruct, *DrawOverlayLineData;

static double getCenterPoint(double p1, double p2);
static void getItemCanvasSpan(TextItem text_item,
                              TextCanvasSpanStruct *x_out,
                              TextCanvasSpanStruct *y_out);
static TextItem itemCreate(FooCanvasItem *text);
static TextPositionerGroup groupCreate(void);
static void groupSpan(TextPositionerGroup group,
                      TextCanvasSpanStruct *xspan_out,
                      TextCanvasSpanStruct *yspan_out);
static void groupOriginalSpan(TextPositionerGroup group,
                              double *x1, double *y1,
                              double *x2, double *y2);
#ifdef RDS_UNUSED_FUNCTION
static double groupOriginalMedian(TextPositionerGroup group);
#endif /* RDS_UNUSED_FUNCTION */
static gint match_group_overlapping(gconstpointer group_data, gconstpointer find_data);
static TextPositionerGroup getOverlappingGroup(ZMapWindowTextPositioner positioner, TextCanvasSpan span, TextPositionerGroup ignore);
static gint order_text_objects(gconstpointer a, gconstpointer b);
static void groupAddItem(TextPositionerGroup group, TextItem text);
static gboolean check_any_groups_overlap(ZMapWindowTextPositioner positioner);
static void unoverlap_items(gpointer text_item_data, gpointer user_data);
static void center_items(gpointer text_item_data, gpointer user_data);
static void unoverlap_groups(gpointer group_data, gpointer user_data);
static void draw_line_for_item(gpointer text_item_data, gpointer points_data);
static void draw_lines_for_groups(gpointer group_data, gpointer user_data);
static void insert_elsewhere(gpointer text_item_data, gpointer group_data);
static void merge_groups(TextPositionerGroup group_merger, TextPositionerGroup mergee);
static void fetch_merge_lists(gpointer list_data, gpointer merge_data);


static gboolean debug_positioner_G = FALSE;


/*!
 *
 * Simply create a positioner.
 * No args, returns a positioner!
 *
 * @return              A New ZMapWindowTextPositioner
 ***************************************************** */
ZMapWindowTextPositioner zmapWindowTextPositionerCreate(double column_min, double column_max)
{
  ZMapWindowTextPositioner positioner;

  if(!(positioner = g_new0(ZMapWindowTextPositionerStruct, 1)))
    {
      zMapAssertNotReached();
    }

  positioner->maximum_iterations = MAXIMUM_ITERATIONS;
  positioner->col_min = column_min;
  positioner->col_max = column_max;

  if(debug_positioner_G)
    printf(" ** positioner created, column extremes %f %f\n", column_min, column_max);

  return positioner;
}

/*!
 *
 * Add a text item to the positioner.
 *
 * @param              A ZMapWindowTextPositioner
 * @param              FooCanvasItem * to add [FOO_IS_CANVAS_TEXT(item) == TRUE]
 * @return             void
 **************************************************** */
void zmapWindowTextPositionerAddItem(ZMapWindowTextPositioner positioner,
                                     FooCanvasItem *item)
{
  TextPositionerGroup group;
  TextCanvasSpanStruct span = {0,0};
  TextItem text;

  text = itemCreate(item);

  getItemCanvasSpan(text, NULL, &span);

  if(debug_positioner_G)
    printf("adding text with span: %d %d ...", span.point1, span.point2);

  if(!(group = getOverlappingGroup(positioner, &span, NULL)))
    {
      if(debug_positioner_G)
        printf(" creating new group ...");
      group = groupCreate();
      group->col_min = positioner->col_min;
      group->col_max = positioner->col_max;
      positioner->groups = g_list_append(positioner->groups, group);
    }

  groupAddItem(group, text);

  if(!positioner->container_overlay)
    {
      ZMapWindowContainerGroup container_parent;
      if((container_parent = zmapWindowContainerCanvasItemGetContainer(item)))
        positioner->container_overlay = (FooCanvasGroup *)zmapWindowContainerGetOverlay(container_parent);
    }

  if(debug_positioner_G)
    printf("item added \n");

  return ;
}

/*!
 *
 * Unoverlap all text items the positioner knows about
 *
 * @param                      A ZMapWindowTextPositioner
 * @param                      boolean, whether to draw lines from
 *                             original to unoverlapped position
 * @return                     void
 *
 ***************************************************  */
void zmapWindowTextPositionerUnOverlap(ZMapWindowTextPositioner positioner,
                                       gboolean draw_lines)
{
  int i = 0;

  if(debug_positioner_G)
    printf("%s begins\n", __PRETTY_FUNCTION__);

  do
    {
      g_list_foreach(positioner->groups, unoverlap_groups, NULL);
      i++;
    }
  while(check_any_groups_overlap(positioner) && i < positioner->maximum_iterations);

  if(debug_positioner_G)
    printf("%s ends\n", __PRETTY_FUNCTION__);

  if(draw_lines)
    {
      DrawOverlayLineDataStruct draw_data = {NULL};

      draw_data.line_points = foo_canvas_points_new(2);
      draw_data.container_overlay = positioner->container_overlay;

      g_list_foreach(positioner->groups, draw_lines_for_groups, &draw_data);

      foo_canvas_points_free(draw_data.line_points);
    }

  return ;
}

/*!
 *
 * Destroy a positioner, cleaning up all its own structures
 *
 * @param                        A ZMapWindowTextPositioner
 * @return                       void
 *
 ***************************************************  */
void zmapWindowTextPositionerDestroy(ZMapWindowTextPositioner positioner)
{
  /* clean up properly here soon... */
  g_free(positioner);
  return ;
}



/* INTERNALS */


static double getCenterPoint(double p1, double p2)
{
  return (p1 + ((p2 - p1 + 1) / 2));
}

/*
 * Fill in the structs with the _canvas_ coords of an item
 *
 ***************************************************  */
static void getItemCanvasSpan(TextItem text_item,
                              TextCanvasSpanStruct *x_out,
                              TextCanvasSpanStruct *y_out)
{
  FooCanvas *canvas = NULL;
  double x1, x2, y1, y2;
  int cx1 = 0, cx2 = 0, cy1 = 0, cy2 = 0;

  my_foo_canvas_item_get_world_bounds(text_item->text, &x1, &y1, &x2, &y2);

  canvas = FOO_CANVAS((FOO_CANVAS_ITEM(text_item->text))->canvas);

  foo_canvas_w2c(text_item->text->canvas, x1, y1, &cx1, &cy1);
  foo_canvas_w2c(text_item->text->canvas, x2, y2, &cx2, &cy2);

  if(x_out)
    {
      x_out->point1 = cx1;
      x_out->point2 = cx2;
    }
  if(y_out)
    {
      y_out->point1 = cy1;
      y_out->point2 = cy2;
    }

  return ;
}

/*
 * Create a TextItem from a FooCanvasItem [FOO_IS_CANVAS_TEXT(text) == TRUE]
 *
 ***************************************************  */
static TextItem itemCreate(FooCanvasItem *text)
{
  TextItem item;
  double x1, x2, y1, y2;

  //zMapAssert(FOO_IS_CANVAS_TEXT(text));
  if(!ZMAP_IS_WINDOW_TEXT_FEATURE(text))
    g_warning("Error");

  if(!(item = g_new0(TextItemStruct, 1)))
    zMapAssertNotReached();

  foo_canvas_item_get_bounds(text,&x1, &y1, &x2, &y2);

  /* FOO_CANVAS_TEXT_PLACE_WEST or whatever it is*/
  item->original_point.x1 = 0.0; /* was x1, but this causes endless movement east of the line ???  */
  item->original_point.x2 = y1; /* getCenterPoint(y1, y2); */

  item->text = text;

  {
    ZMapFeature feature = NULL;

    /* THIS CODE ATTEMPTS TO FUDGE THE ISSUE OF WHEN SOMETHING IS A LOCUS.... */
    if ((feature = zmapWindowItemGetFeature(text)) && feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
      {
	item->locus_name = feature->feature.transcript.locus_id ;
      }
  }

  return item;
}

/*
 * Simply create a TextPositionerGroup
 *
 ***************************************************  */
static TextPositionerGroup groupCreate(void)
{
  TextPositionerGroup group;

  if(!(group = g_new0(TextPositionerGroupStruct, 1)))
    zMapAssertNotReached();

  return group;
}

/*
 * Calculate the _canvas_ span of a group
 *
 ***************************************************  */
static void groupSpan(TextPositionerGroup group,
                      TextCanvasSpanStruct *xspan_out,
                      TextCanvasSpanStruct *yspan_out)
{
  GList *list;
  TextItem item;
  TextCanvasSpanStruct x = {0,0}, y = {0,0};

  zMapAssert(xspan_out && yspan_out);

  if((list = (g_list_first(group->text_objects))) && (item = (TextItem)(list->data)))
    {
      getItemCanvasSpan(item, &x, &y);
      xspan_out->point1 = x.point1;
      yspan_out->point1 = y.point1;
    }

  if((list = (g_list_last(group->text_objects))) && (item = (TextItem)(list->data)))
    {
      getItemCanvasSpan(item, &x, &y);
      xspan_out->point2 = x.point2;
      yspan_out->point2 = y.point2;
    }

  return ;
}

/*
 * Calculate the _item_ span of a group
 *
 * N.B. This is between center points in y and
 *      between 0 and 0 in x!
 ***************************************************  */
static void groupOriginalSpan(TextPositionerGroup group,
                              double *x1, double *y1,
                              double *x2, double *y2)
{
  GList *list;
  TextItem item;

  if((list = (g_list_first(group->text_objects))) && (item = (TextItem)(list->data)))
    {
      if(x1)
        *x1 = item->original_point.x1;
      if(y1)
        *y1 = item->original_point.x2;
    }

  if((list = (g_list_last(group->text_objects))) && (item = (TextItem)(list->data)))
    {
      if(x2)
        *x2 = item->original_point.x1;
      if(y2)
        *y2 = item->original_point.x2;
    }

  return ;
}

#ifdef RDS_UNUSED_FUNCTION
/*
 * Get the median item's center point. (unused!)
 ***************************************************  */
static double groupOriginalMedian(TextPositionerGroup group)
{
  TextItem item;
  double median, tmp;
  int idx;

  if(group->list_length % 2)
    {
      /* odd use middle (n + 1) / 2 */
      idx = (group->list_length - 1) / 2; /* zero based index */

      if((item = (TextItem)(g_list_nth_data(group->text_objects, idx))))
        {
          median = item->original_point.x2;
        }
    }
  else
    {
      /* even use middle two... n / 2 & (n / 2) + 1 */
      idx = (group->list_length / 2);
      /* get the second of the two */
      if((item = (TextItem)(g_list_nth_data(group->text_objects, idx))))
        {
          tmp = item->original_point.x2;
        }

      idx--;                    /* swap to zero based to get the first*/
      if((item = (TextItem)(g_list_nth_data(group->text_objects, idx))))
        {
          median = getCenterPoint(item->original_point.x2, tmp);
        }
    }

  return median;
}
#endif /* RDS_UNUSED_FUNCTION */

/*
 * g_list_foreach GFunc
 *  - matches a group that overlaps (canvas coords) find_data->span
 *  - will ignore groups matching find_data->ignore
 *
 ***************************************************  */
static gint match_group_overlapping(gconstpointer group_data, gconstpointer find_data)
{
  TextPositionerGroup group = (TextPositionerGroup)group_data;
  FindOverlapGroup find = (FindOverlapGroup)find_data;
  TextCanvasSpanStruct xspan = {0.0, 0.0}, yspan = {0.0, 0.0};
  TextCanvasSpan fspan = find->span;
  gint match = -1;

  if(group != find->ignore)
    {
      groupSpan(group, &xspan, &yspan);

      if((!(fspan->point2 < yspan.point1) && !(fspan->point1 > yspan.point2)))
        {
          if((fspan->point1 < yspan.point1) || (fspan->point2 > yspan.point2))
            {
              match = 0;
            }
          match = 0;
        }
    }

  return match;
}

/*
 * Find a group that overlaps supplied span,
 * but isn't == ignore param
 *
 * @param                A ZMapWindowTextPositioner
 * @param                span to overlap
 * @param                group to ignore
 * @return               group which matches or NULL if none match
 *
 ***************************************************  */
static TextPositionerGroup getOverlappingGroup(ZMapWindowTextPositioner positioner, TextCanvasSpan span, TextPositionerGroup ignore)
{
  TextPositionerGroup group = NULL;
  FindOverlapGroupStruct data = {NULL};
  GList *group_list;

  data.ignore = ignore;
  data.span   = span;

  if((group_list = g_list_find_custom(positioner->groups, &data, match_group_overlapping)))
    {
      group = (TextPositionerGroup)(group_list->data);
    }

  return group;
}

/*
 * g_list_insert_sorted GCompareFunc
 *
 * @return   negative value if a < b;
 *           zero           if a = b;
 *           positive value if a > b;
 ***************************************************  */
static gint order_text_objects(gconstpointer a, gconstpointer b)
{
  TextItem a_item = (TextItem)a,
    b_item        = (TextItem)b;
  gint order = 0;

  if(a_item->original_point.x2 < b_item->original_point.x2)
    order = -1;
  else if(a_item->original_point.x2 > b_item->original_point.x2)
    order = 1;
  else
    order = 0;

  return order;
}


/*
 * Add a TextItem to a TextPositionerGroup,
 * inserting so that the list remains sorted.
 *
 ***************************************************  */
static void groupAddItem(TextPositionerGroup group, TextItem text)
{
  zMapAssert(group && text);

  group->text_objects = g_list_insert_sorted(group->text_objects, text, order_text_objects);

  group->list_length++;

  return ;
}

/*
 * Check the positioner to see if any of it's groups overlap
 *
 * @param             A ZMapWindowTextPositioner
 * @param             boolean TRUE if overlaps still exist
 *
 ***************************************************  */
static gboolean check_any_groups_overlap(ZMapWindowTextPositioner positioner)
{
  gboolean overlapping = FALSE;
  MergeDataStruct merge_inout = {NULL};

  merge_inout.positioner = positioner;
  /* In here we try to merge any groups which overlap */
  if(debug_positioner_G)
    printf("fetching merge_lists from list with length %d\n", g_list_length(positioner->groups));

  g_list_foreach(positioner->groups, fetch_merge_lists, &merge_inout);

  if(debug_positioner_G)
    printf("done fetching\n");

  if(merge_inout.merged_groups)
    {
      positioner->groups = g_list_concat(positioner->groups, merge_inout.merged_groups);
      overlapping = TRUE;
    }

  return overlapping;
}

/*
 * g_list_foreach GFunc
 * - process a group's items to unoverlap them.
 * - simple algorithm to move each member of the sorted (y coord) list
 *   down the page (increasing coords) until it no longer overlaps.
 *   Current max coord is recorded on each iteration.
 *
 ***************************************************  */
#warning MH17:this function is totally insane
/* to process an ordered list we call a g_list_foreach function and pass in a pointer to a data structure with a pointer to the 'current item' in the list ??
*/
static void unoverlap_items(gpointer text_item_data, gpointer user_data)
{
  UnOverlapItemsData full_data = (UnOverlapItemsData)user_data;
  TextItem text_item = (TextItem)text_item_data;
  double y1, y2, overlap, reduce, tmp;
  GList *prev;

  /* Test we're not the first. */
  if((prev = g_list_previous(full_data->list_item)))
    {
      foo_canvas_item_get_bounds(text_item->text,  NULL, &y1, NULL, &y2);
      overlap = (full_data->max) - y1 + 1;

      if(debug_positioner_G)
        printf("  text[%s] (%f), min = %f, max = %f, y1 = %f, overlap = %f\n",
               g_quark_to_string(text_item->locus_name),
               text_item->original_point.x2,
               full_data->min,
               full_data->max,
               y1, overlap);

      if(overlap != 0.0)
        {
          /* Only move in y axis. */
          foo_canvas_item_move(text_item->text, 0.0, overlap);
          full_data->center = TRUE;
        }

      full_data->max += full_data->line_height;
    }
  else
    {
      /* first needs to setup for the following */
      foo_canvas_item_get_bounds(text_item->text,  NULL, &y1, NULL, &y2);

      if(getCenterPoint(y1, y2) != (tmp = text_item->original_point.x2))
        {
          if(debug_positioner_G)
            printf("current %f target %f\n", getCenterPoint(y1, y2), tmp);

          foo_canvas_item_move(text_item->text, 0.0, tmp - getCenterPoint(y1, y2));
          foo_canvas_item_get_bounds(text_item->text,  NULL, &y1, NULL, &y2);
        }

      /* We reduce the actual extent here to pack in more text that is still readable. */
      reduce = (y2 - y1 + 1) * 0.13; /* reduce by 26% (2 * 13) */

      full_data->line_height = (y2 - y1 + 1) - (reduce * 2);
      full_data->min         = y1 + reduce;
      full_data->max         = y2 - reduce;


      if(debug_positioner_G)
        printf("  FIRST: [%s] min = %f, max = %f, line = %f\n",
             g_quark_to_string(text_item->locus_name),
             full_data->min, full_data->max,
             full_data->line_height);
    }

  full_data->list_item = g_list_next(full_data->list_item);

  return ;
}

/*
 * g_list_foreach GFunc
 * - the unoverlap items function ONLY move the items down the page (increase)
 *   so we have to re-center the group by moving all the items back up the page.
 * - here we do that moving..
 *
 ***************************************************  */
static void center_items(gpointer text_item_data, gpointer user_data)
{
  double *data = (double *)user_data;
  TextItem text_item = (TextItem)text_item_data;

  /* Only move in y axis. */
  foo_canvas_item_move(text_item->text, 0.0, *data);

  return ;
}

/*
 * g_list_foreach GFunc
 * - do all that is required to unoverlap the items in a list of groups
 * - foreach group we call the functions on their list of items
 ***************************************************  */
static void unoverlap_groups(gpointer group_data, gpointer user_data)
{
  TextPositionerGroup group = (TextPositionerGroup)group_data;
  UnOverlapItemsDataStruct full_data = {NULL};
  double real = 0.0, original = 0.0, size, y1, y2, real_max;
  int both = 0;

  /* Set up the data needed by the child foreach */
  full_data.list_item   = g_list_first(group->text_objects);
  full_data.center      = FALSE;
  full_data.line_height =
    full_data.min       =
    full_data.max       = 0.0;

  if(debug_positioner_G)
    printf("starting a new group\n");

  /* unoverlap the items of this group... */
  g_list_foreach(group->text_objects, unoverlap_items, &full_data);

  /* Now we need to re-center this group by moving each of the items */
  if(full_data.center)
    {
      /*
       * MH17: NOTE it should be relatively simple to invent an algorithm that only needs one pass
       * (limiting iterations could be a way of admitting that the alorgithm is not deterministic
       * and may fail that famous halting problem they knew about in the late 1940's)
       * - the text (locus names) is in order (or can be)
       * - calculate the space needed to display the text (per group) and how much spare there is
       * - position the groups at one end
       * - start at one end and add gaps from the spare space allocation  to centre the groups nicely
       */

      /*
       * Using the median item's center point seemed like a good idea to make
       * the positioning look a little better.  However, it just means more
       * iterations to unoverlap everything...
       *
       * original = groupOriginalMedian(group);
       */
      real_max = full_data.max - full_data.line_height;
      real     = getCenterPoint(full_data.min, real_max);
      groupOriginalSpan(group, NULL, &y1, NULL, &y2);
      original = getCenterPoint(y1, y2);
      size     = original - real;

      /* here we're checking if the text is going to be drawn outside the visible canvas area.
       * for this we use the supplied column min and max, - & + (rsp.), the text height (the border). */
      if(real_max > (group->col_max + full_data.line_height))
        {
          both++;
          size -= (real_max - (group->col_max + full_data.line_height));
          if(debug_positioner_G)
            printf(" ** real_max %f > %f col_max\n", real_max, group->col_max + full_data.line_height);
        }
      if(full_data.min < (group->col_min - full_data.line_height))
        {
          both++;
          size += ((group->col_min - full_data.line_height) - full_data.min);
          if(debug_positioner_G)
            printf(" ** real_min %f < %f col_min\n", full_data.min, group->col_min - full_data.line_height);
        }

      if(both == 2)
        zMapLogWarning("%s", "Text is out of bounds in both directions...");

      if(debug_positioner_G && full_data.center)
        printf(" ** current center %f, original %f, difference %f\n", real, original, size);

      /* call the next foreach... */
      g_list_foreach(group->text_objects, center_items, &size);
    }

  if(debug_positioner_G)
    printf("finished group\n");

  return ;
}

/*
 * g_list_foreach GFunc
 * - draw a line to mark from the original position of the text to
 *   the un overlapped position.
 ***************************************************  */
static void draw_line_for_item(gpointer text_item_data, gpointer draw_data)
{
  TextItem text_item = (TextItem)text_item_data;
  DrawOverlayLineData draw = (DrawOverlayLineData)draw_data;
  FooCanvasPoints *points;
  FooCanvasItem *line;
  double right = 10.0;

  points = draw->line_points;

  if(draw->container_overlay)
    {
      if((line = g_object_get_data(G_OBJECT(text_item->text), ZMAPWINDOWTEXT_ITEM_TO_LINE)))
        {
          gtk_object_destroy(GTK_OBJECT(line));
          line = NULL;
        }

      if(!line)
        {
          foo_canvas_item_get_bounds(text_item->text,
                                     &(points->coords[0]), &(points->coords[1]),
                                     &(points->coords[2]), &(points->coords[3]));
          /* only move right the once.  needs a better method really... */
          if(points->coords[0] == 0.0)
            {
              foo_canvas_item_move(text_item->text, right, 0.0);
              foo_canvas_item_get_bounds(text_item->text,
                                         &(points->coords[0]), &(points->coords[1]),
                                         &(points->coords[2]), &(points->coords[3]));
            }
#if MH17_DEBUG_NAV_FOOBAR
{
ZMapFeatureAny  feature_any = zmapWindowItemGetFeatureAny(text_item->text);
char * name = zMapFeatureName(feature_any);

printf("text postion %s/%p/%s at %f -> %f\n",g_quark_to_string(text_item->locus_name), text_item->text, name, points->coords[1],points->coords[3]);

if(!strcmp(name,"CCDS44517.1"))
{
      extern FooCanvasItem *text_foo;
      text_foo = text_item->text;
}

}
#endif
          points->coords[3] = getCenterPoint(points->coords[1], points->coords[3]);

          points->coords[0] = text_item->original_point.x1;
          points->coords[1] = text_item->original_point.x2;
          points->coords[2] = right;

          line = foo_canvas_item_new(FOO_CANVAS_GROUP(draw->container_overlay),
                                     FOO_TYPE_CANVAS_LINE,
                                     "points",       points,
                                     "width_pixels", 1,
                                     NULL);
          g_object_set_data(G_OBJECT(text_item->text), ZMAPWINDOWTEXT_ITEM_TO_LINE, line);
        }
    }

  return ;
}

/*
 * g_list_foreach GFunc
 * - call draw_line_for_item
 ***************************************************  */
static void draw_lines_for_groups(gpointer group_data, gpointer user_data)
{
  TextPositionerGroup group = (TextPositionerGroup)group_data;

  if(group->list_length > 1)
    g_list_foreach(group->text_objects, draw_line_for_item, user_data);

  return ;
}

/*
 * g_list_foreach GFunc
 * - simply call groupAddItem.
 ***************************************************  */
static void insert_elsewhere(gpointer text_item_data, gpointer group_data)
{
  TextItem text_item = (TextItem)text_item_data;
  TextPositionerGroup group = (TextPositionerGroup)group_data;

  groupAddItem(group, text_item);

  return;
}

/*
 * Merge TextItems from mergee into group_merger.  This will ensure the
 * lists are kept sorted. N.B. mergee is freed!
 ***************************************************  */
static void merge_groups(TextPositionerGroup group_merger, TextPositionerGroup mergee)
{
  g_list_foreach(mergee->text_objects, insert_elsewhere, group_merger);

  g_list_free(mergee->text_objects);

  g_free(mergee);

  return ;
}


/*
 * g_list_foreach GFunc
 * - Merge overlapping groups into single groups
 ***************************************************  */
static void fetch_merge_lists(gpointer list_data, gpointer merge_data)
{
  MergeData merge = (MergeData)merge_data;
  TextPositionerGroup group = (TextPositionerGroup)list_data;
  TextPositionerGroup overlap_group;
  TextCanvasSpanStruct span   = {0,0}, unused = {};
  FindOverlapGroupStruct data = {NULL};
  GList *group_list;

  groupSpan(group, &unused, &span);

  if(debug_positioner_G)
    printf("    checking group with span %d %d ...", span.point1, span.point2);

  data.ignore = group;
  data.span   = &span;

  /* check current list for an overlapping group. */
  if((overlap_group = getOverlappingGroup(merge->positioner, &span, group)))
    {
      /* overlap_group is ignored in this block. It will get it's turn
       * in the else if block on one of the next iterations of this
       * foreach */

      /* remove current group from groups list */
      merge->positioner->groups = g_list_remove(merge->positioner->groups, group);

      if(debug_positioner_G)
        printf(" found overlap in positioner ...");

      /* check the already merged list */
      if((group_list = g_list_find_custom(merge->merged_groups, &data, match_group_overlapping)))
        {
          if(debug_positioner_G)
            printf(" and in already merged list, merging ...");
          /* merge into the group in the merged_groups list */
          merge_groups((TextPositionerGroup)(group_list->data), group);
        }
      else
        {
          if(debug_positioner_G)
            printf(" adding to merged list");
          /* Just append to the list of merged_groups */
          merge->merged_groups = g_list_append(merge->merged_groups, group);
        }
    }
  else if((group_list = g_list_find_custom(merge->merged_groups, &data, match_group_overlapping)))
    {
      /* overlapping group... (from merged_groups) */
      overlap_group = (TextPositionerGroup)(group_list->data);

      if(debug_positioner_G)
        printf(" found overlap in merged list, merging ...");

      /* remove from groups list */
      merge->positioner->groups = g_list_remove(merge->positioner->groups, group);

      /* merge the two into the one present int he merged_groups list */
      merge_groups(overlap_group, group);
    }

  if(debug_positioner_G)
    printf("\n");

  return ;
}

