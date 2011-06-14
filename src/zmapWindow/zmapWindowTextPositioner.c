/*  File: zmapWindowTextPositioner.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  re-written by mh17
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
 * CVS info:   $Id: zmapWindowTextPositioner.c,v 1.16 2011-04-06 08:12:59 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowTextFeature.h>
#include <zmapWindow/zmapWindowNavigator_P.h>


typedef struct
{
  int x1;
  int x2;
}TextCanvasSpanStruct, *TextCanvasSpan;

typedef struct
{
  double x1;
  double x2;
}WorldSpanStruct, *WorldSpan;

typedef struct
{
      WorldSpanStruct span;
      TextCanvasSpanStruct text_span;
      int height;
} LocusGroupStruct, *LocusGroup;


/* The parent object and only external */
typedef struct _ZMapWindowTextPositionerStruct
{
  FooCanvasGroup *container_overlay;
  FooCanvas *canvas;
  WorldSpanStruct span_world;             /* extent of the column */
  TextCanvasSpanStruct span_canvas;       /* extent of the column */
  int n_groups;
  int n_items;
  GList *text_items;
  GList *groups;                      /* LocusGroupStruct of groups in order */
}ZMapWindowTextPositionerStruct;



typedef struct
{
  FooCanvasItem *text;
  WorldSpanStruct span_world;
  TextCanvasSpanStruct span_canvas;        /* extent of the text item */

  WorldSpanStruct locus_world;      /* locus feature extent;  used for calculating group overlap */

/*  TextCanvasSpanStruct locus_canvas;        not used */

  int group;
  GQuark locus_name;
} TextItemStruct, *TextItem;

static TextItem itemCreate(FooCanvasItem *foo);
static gint order_text_objects(gconstpointer a, gconstpointer b);

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

  positioner->span_world.x1 = column_min;
  positioner->span_world.x2 = column_max;

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
 /* MH17: each time this is called the text item is positioned with y1 at the start of the locus
  * see locus_gh_func() in windowNavigator.c
  */
void zmapWindowTextPositionerAddItem(ZMapWindowTextPositioner positioner,
                                     FooCanvasItem *item)
{
  TextItem text;

  if(!ZMAP_IS_WINDOW_TEXT_FEATURE(item))
    return;

  text = itemCreate(item);

      /* insert in order for ease of processing later */
  positioner->text_items = g_list_insert_sorted(positioner->text_items, text, order_text_objects);
  positioner->n_items++;

  if(!positioner->container_overlay)
    {
      ZMapWindowContainerGroup container_parent;
      if((container_parent = zmapWindowContainerCanvasItemGetContainer(item)))
        positioner->container_overlay = (FooCanvasGroup *)zmapWindowContainerGetOverlay(container_parent);

      positioner->canvas = item->canvas;
    }

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
      /*
       * the text_items list is in order of locus coord
       * scan the list to set coords sequentially from the top
       * and add group id's to all items based on overlap of locus_coord
       *
       * then scan again to add spaces between groups and centre locus names better
       * then scan again to move the canvas items
       */

  GList *l;
  LocusGroup group_span = NULL;
  int cur_y;
  int col_height;
  int col_spare;
  int group_sep,item_sep;
  int sep,item_adjust;
  int cur_group;
  GList *gs;
  int text_height;       /*in pixels */

  if(positioner->n_items < 2)
      return;

  positioner->n_groups = 0;

  zMapNavigatorWidgetGetCanvasTextSize(positioner->canvas, NULL, &text_height);
//printf("text size = %d\n",text_height);

  /* get canvas coords, NOTE we do not assume we start at 0 or 1 or anywhere */
  foo_canvas_w2c(positioner->canvas,0,positioner->span_world.x1,NULL,&positioner->span_canvas.x1);
  foo_canvas_w2c(positioner->canvas,0,positioner->span_world.x2,NULL,&positioner->span_canvas.x2);

  cur_y = positioner->span_canvas.x1;

      /* assign text items to groups of overlapping loci */
      /* and assign canvas text position from the top with no gaps */
  for(l = positioner->text_items;l; l = l->next)
  {
      TextItem ti = l->data;
      int height;

            /* we create groups of loci that overlap as locus features
             * this is the only way to have stable groups as text itens
             * can overlap differently according to the window height
             * and NOTE it has more relevance biologically
             */

      if(!group_span || ti->locus_world.x1 > group_span->span.x2)
            /* simple comparison assuming data is in order of start coord */
      {
            /* does not overlap: start a new group */
            /* first save the span */

            group_span = g_new0(LocusGroupStruct,1);
            positioner->groups = g_list_append(positioner->groups,group_span);

            positioner->n_groups++;

            /* store feature coords for sorting into groups */
            group_span->span.x1 = ti->locus_world.x1;
            group_span->span.x2 = ti->locus_world.x2;

            /* store text coords for positioning */
            group_span->text_span.x1 = ti->span_canvas.x1;
            group_span->text_span.x2 = ti->span_canvas.x2;
      }

      ti->group = positioner->n_groups;
      if(ti->locus_world.x2 > group_span->span.x2)
            group_span->span.x2 = ti->locus_world.x2;

//      height = ti->span_canvas.x2 - ti->span_canvas.x1;
      height = text_height;
      group_span->height += height;
//printf("%s %f / %f in group %d -> %d %d \n", g_quark_to_string(ti->locus_name),ti->locus_world.x1,ti->span_world.x1,ti->group,cur_y,height);

      ti->span_canvas.x1 = cur_y;
      cur_y += height;
      ti->span_canvas.x2 = cur_y;
  }

      /* add spaces between groups and items if possible */
  col_height = positioner->span_canvas.x2 - positioner->span_canvas.x1;
  col_spare = positioner->span_canvas.x2 - cur_y;

  item_sep = col_spare / positioner->n_items;
  if(item_sep > 0)
      item_sep = 0;
  col_spare -= item_sep * (positioner->n_items - 1);

  group_sep = col_spare / positioner->n_groups;
  if(group_sep > 8)
      group_sep = 8;

  col_spare -= group_sep * (positioner->n_groups - 1);

  for(gs = positioner->groups,sep = 0,cur_group = 0,cur_y = positioner->span_canvas.x1,l = positioner->text_items;l;l = l->next)
  {
      TextItem ti = l->data;
      double dx,dy;

      if(ti->group != cur_group)
      {
//printf("%s: group %d  (%d)\n",g_quark_to_string(ti->locus_name),ti->group,cur_group);
            if(cur_group)
                  sep += group_sep;
            if(gs)
            {
                  /* centre groups better, top ones get first dibs at the spare pixels */
                  LocusGroup gspan = (LocusGroup) gs->data;
                  int group_off;
                  int mid_canvas;   /* middle of group origionally */
                  int top_canvas;   /* top of group where we want it to be */

                  mid_canvas = (gspan->text_span.x1 + gspan->text_span.x2) / 2;
                  top_canvas = mid_canvas - gspan->height / 2;

//printf("group span %f %f, %d %d... %d %d %d %d %d\n",gspan->span.x1,gspan->span.x2,group_canvas.x1,group_canvas.x2, top_canvas, gspan->height, cur_y, sep,col_spare);

                  if(top_canvas > cur_y)
                  {
                        group_off = top_canvas - cur_y;
                        if(group_off > col_spare)
                              group_off = col_spare;
                        if(group_off > 0)
                        {
                              sep += group_off;
                              col_spare -= group_off;
                        }
                  }

                  gs = gs->next;
            }
            cur_group = ti->group;
      }
//zMapLogWarning("%s %d + %d = %d\n", g_quark_to_string(ti->locus_name),ti->span_canvas.x1,sep,ti->span_canvas.x1 + sep);


      /* move single items down to near their coordinate */

      /* NOTE this is not optimal, we're just handing out space on a first come first served basis
       * could be improved by first centring all the groups and then expanding them subject to
       * availability, but is it worth the effort?
       */
      foo_canvas_w2c(positioner->canvas,0,ti->span_world.x1,NULL,&item_adjust);
      item_adjust -= text_height;   /* should be half that, but somehow the text ends up heading south */
      if(item_adjust > ti->span_canvas.x1 + sep)
      {
            item_adjust -= ti->span_canvas.x1 + sep;
            if(item_adjust > col_spare)
                 item_adjust = col_spare;
            if(item_adjust > 0)
            {
                  sep += item_adjust;
                  col_spare -= item_adjust;
            }
      }

      ti->span_canvas.x1 += sep;
      ti->span_canvas.x2 += sep;


      cur_y = ti->span_canvas.x2;   /* where we are down to */

      sep += item_sep;  /* don't add item_sep before the first one */

      foo_canvas_c2w(ti->text->canvas,0,ti->span_canvas.x1,NULL,&dy);
      dx = 0;
      dy -= ti->span_world.x1;      /* get diff from original coordinate */

      /* draw a line if requested */
      if(draw_lines && positioner->container_overlay)
      {
            FooCanvasPoints *points = foo_canvas_points_new(2);
            FooCanvasItem *line;
            double right = 10.0;
            int mid;

            if((line = g_object_get_data(G_OBJECT(ti->text), ZMAPWINDOWTEXT_ITEM_TO_LINE)))
            {
#warning not ever called as this is done by the caller... duplicated code
//printf("repositionText destroys line unexpectedly\n");
                  gtk_object_destroy(GTK_OBJECT(line));
            }
            else
                  dx = right;       /* only move first time */

//            mid = ti->span_canvas.x1 + (ti->span_canvas.x2 - ti->span_canvas.x1 + 1) / 2;
            mid = ti->span_canvas.x1; // just point to the start
            foo_canvas_c2w(ti->text->canvas,0,mid,NULL,&points->coords[3]);

            points->coords[0] = 0.0;
            /* span world ios where the text would have been displyed ie at feature->x1
             * due to the navigator scaling factor we can't use the feature coords
             */
            points->coords[1] = ti->span_world.x1;
            points->coords[2] = right;
//zMapLogWarning("line %f %f -> %f %f\n",points->coords[0],points->coords[1],points->coords[2],points->coords[3]);

            line = foo_canvas_item_new(FOO_CANVAS_GROUP(positioner->container_overlay),
                                          FOO_TYPE_CANVAS_LINE,
                                          "points",       points,
                                          "width_pixels", 1,
                                          NULL);
            g_object_set_data(G_OBJECT(ti->text), ZMAPWINDOWTEXT_ITEM_TO_LINE, line);

            foo_canvas_points_free(points);
      }

      /* now move the canvas item */
//("move %s @  %d/%f by %f %f\n", g_quark_to_string(ti->locus_name),ti->span_canvas.x1,ti->span_world.x1,dx,dy);
      foo_canvas_item_move(ti->text, dx, dy);
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

  TextItem t;
  GList *l;
  LocusGroup group;

  for(l = positioner->text_items;l;)
  {
      t = (TextItem) l->data;
      g_free(t);
      l = g_list_delete_link(l,l);
  }

  for(l = positioner->groups;l;)
  {
      group = (LocusGroup) l->data;
      g_free(group);
      l = g_list_delete_link(l,l);
  }


  g_free(positioner);
  return ;
}


/*
 * Create a TextItem from a FooCanvasItem [FOO_IS_CANVAS_TEXT(text) == TRUE]
 *
 ***************************************************  */
static TextItem itemCreate(FooCanvasItem *foo)
{
  TextItem item;
  ZMapFeature feature = NULL;

  if(!(item = g_new0(TextItemStruct, 1)))
    zMapAssertNotReached();

  item->text = foo;
  foo_canvas_item_get_bounds(foo, NULL, &item->span_world.x1, NULL, &item->span_world.x2);
  foo_canvas_w2c(foo->canvas, 0, item->span_world.x1, NULL, &item->span_canvas.x1);
  foo_canvas_w2c(foo->canvas, 0, item->span_world.x2, NULL, &item->span_canvas.x2);

  if ((feature = zmapWindowItemGetFeature(foo)))
  {
      item->locus_name = feature->original_id;
      item->locus_world.x1 = feature->x1;       /* locus_world is where the feature is not the text */
      item->locus_world.x2 = feature->x2;

/*    foo_canvas_w2c(foo->canvas, 0, feature->x1, NULL, &item->locus_canvas.x1);
      foo_canvas_w2c(foo->canvas, 0, feature->x2, NULL, &item->locus_canvas.x2);
*/
  }

  return item;
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

  if(a_item->locus_world.x1 < b_item->locus_world.x1)
    order = -1;
  else if(a_item->locus_world.x1 > b_item->locus_world.x1)
    order = 1;
  else
    order = 0;

  return order;
}


