/*  File: zmapWindowGraphItem.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
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
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun  3 09:51 2009 (rds)
 * Created: Fri Jan 16 11:20:07 2009 (rds)
 * CVS info:   $Id: zmapWindowGlyphItem.c,v 1.14 2011-03-14 11:35:18 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>


/*
NOTE: was munged from BasicFeature
*/




#include <math.h>
#include <string.h>
#include <zmapWindowGraphItem_I.h>


static void zmap_window_graph_item_class_init  (ZMapWindowGraphItemClass graph_class);
static void zmap_window_graph_item_init        (ZMapWindowGraphItem      group);
static void zmap_window_graph_item_set_property(GObject               *object,
                                       guint                  param_id,
                                       const GValue          *value,
                                       GParamSpec            *pspec);
static void zmap_window_graph_item_get_property(GObject               *object,
                                       guint                  param_id,
                                       GValue                *value,
                                       GParamSpec            *pspec);

static void zmap_window_graph_item_destroy     (GObject *object);

static FooCanvasItem *zmap_window_graph_item_add_interval(ZMapWindowCanvasItem   graph,
                                               ZMapFeatureSubPartSpan unused,
                                               double top,  double bottom,
                                               double left, double right);


GType zMapWindowGraphItemGetType(void)
{
  static GType group_type = 0 ;

  if (!group_type)
    {
      static const GTypeInfo group_info = {
      sizeof (zmapWindowGraphItemClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) zmap_window_graph_item_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof (zmapWindowGraphItem),
      0,              /* n_preallocs */
      (GInstanceInitFunc) zmap_window_graph_item_init,
      NULL
      };

      group_type = g_type_register_static(zMapWindowCanvasItemGetType(),
                                ZMAP_WINDOW_GRAPH_ITEM_NAME,
                                &group_info,
                                0) ;
    }

  return group_type;
}



static void zmap_window_graph_item_class_init(ZMapWindowGraphItemClass graph_class)
{
  ZMapWindowCanvasItemClass canvas_class ;
  GObjectClass *gobject_class ;

  gobject_class = (GObjectClass *) graph_class;
  canvas_class  = (ZMapWindowCanvasItemClass)graph_class;

  gobject_class->set_property = zmap_window_graph_item_set_property;
  gobject_class->get_property = zmap_window_graph_item_get_property;

  gobject_class->dispose = zmap_window_graph_item_destroy;

  canvas_class->add_interval = zmap_window_graph_item_add_interval;
  canvas_class->check_data   = NULL;

  zmapWindowItemStatsInit(&(canvas_class->stats), ZMAP_TYPE_WINDOW_GRAPH_ITEM) ;

  return ;
}

static void zmap_window_graph_item_init        (ZMapWindowGraphItem graph)
{

  return ;
}



static FooCanvasItem *zmap_window_graph_item_add_interval(ZMapWindowCanvasItem   graph,
                                               ZMapFeatureSubPartSpan unused,
                                               double top,  double bottom,
                                               double left, double right)
{
  FooCanvasItem *item = NULL;

  ZMapFeature feature;
  feature = graph->feature;


  item = foo_canvas_item_new(FOO_CANVAS_GROUP(graph),
                               FOO_TYPE_CANVAS_RECT,
                               "x1", left,  "y1", top,
                               "x2", right, "y2", bottom,
                               NULL);

  return item;
}

static void zmap_window_graph_item_set_property(GObject               *object,
                                       guint                  param_id,
                                       const GValue          *value,
                                       GParamSpec            *pspec)

{
  ZMapWindowCanvasItem canvas_item;
  ZMapWindowGraphItem graph;

  g_return_if_fail(ZMAP_IS_WINDOW_GRAPH_ITEM(object));

  graph = ZMAP_WINDOW_GRAPH_ITEM(object);
  canvas_item = ZMAP_CANVAS_ITEM(object);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_graph_item_get_property(GObject               *object,
                                       guint                  param_id,
                                       GValue                *value,
                                       GParamSpec            *pspec)
{
  return ;
}


static void zmap_window_graph_item_destroy     (GObject *object)
{

  return ;
}



