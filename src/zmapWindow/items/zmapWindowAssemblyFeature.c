/*  File: zmapWindowAssemblyFeature.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 11 14:35 2010 (edgrif)
 * Created: Wed Dec  3 10:02:22 2008 (rds)
 * CVS info:   $Id: zmapWindowAssemblyFeature.c,v 1.5 2010-05-24 14:10:44 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindowAssemblyFeature_I.h>

enum
  {
    ASSEMBLY_INTERVAL_0,		/* invalid */
    ASSEMBLY_INTERVAL_TYPE
  };


static void zmap_window_assembly_feature_class_init  (ZMapWindowAssemblyFeatureClass assembly_class);
static void zmap_window_assembly_feature_init        (ZMapWindowAssemblyFeature      group);
static void zmap_window_assembly_feature_set_property(GObject               *object,
						   guint                  param_id,
						   const GValue          *value,
						   GParamSpec            *pspec);
static void zmap_window_assembly_feature_get_property(GObject               *object,
						   guint                  param_id,
						   GValue                *value,
						   GParamSpec            *pspec);
#ifdef ASSEMBLY_REQUIRES_DESTROY
static void zmap_window_assembly_feature_destroy     (GtkObject *gtkobject);
#endif /* ASSEMBLY_REQUIRES_DESTROY */

static void zmap_window_assembly_feature_post_create(ZMapWindowCanvasItem canvas_item);

static ZMapWindowCanvasItemClass canvas_item_parent_class_G = NULL;

GType zMapWindowAssemblyFeatureGetType(void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowAssemblyFeatureClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_assembly_feature_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowAssemblyFeature),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_assembly_feature_init

	};

      group_type = g_type_register_static (zMapWindowCanvasItemGetType(),
					   ZMAP_WINDOW_ASSEMBLY_FEATURE_NAME,
					   &group_info,
					   0);
    }

  return group_type;
}


static FooCanvasItem *zmap_window_assembly_feature_add_interval(ZMapWindowCanvasItem   assembly,
								ZMapFeatureSubPartSpan sub_feature,
								double top,  double bottom,
								double left, double right)
{
  FooCanvasItem *item = NULL;

  /* they are all the same at the moment */
  switch(sub_feature->subpart)
    {
    case ZMAPFEATURE_SUBPART_GAP:
    case ZMAPFEATURE_SUBPART_MATCH:
    default:
      item = foo_canvas_item_new(FOO_CANVAS_GROUP(assembly),
				 FOO_TYPE_CANVAS_RECT,
				 "x1", left,  "y1", top,
				 "x2", right, "y2", bottom,
				 NULL);
      break;
    }

  return item;
}


static void zmap_window_assembly_feature_set_colour(ZMapWindowCanvasItem   assembly,
						    FooCanvasItem         *interval,
						    ZMapFeatureSubPartSpan sub_feature,
						    ZMapStyleColourType    colour_type,
						    GdkColor              *default_fill)
{
  GdkColor *background, *foreground, *outline, *fill;
  ZMapFeatureTypeStyle style;
  ZMapStyleParamId colour_target;

  g_return_if_fail(assembly != NULL);
  g_return_if_fail(interval != NULL);

  fill = outline = foreground = background = NULL;

  style = (ZMAP_CANVAS_ITEM_GET_CLASS(assembly)->get_style)(assembly);

  switch(sub_feature->subpart)
    {
    case ZMAPFEATURE_SUBPART_GAP:
      {
	colour_target = STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS;
      }
      break;
    case ZMAPFEATURE_SUBPART_MATCH:
    default:
      colour_target = STYLE_PROP_COLOURS;
      break;
    }

  if((zMapStyleGetColours(style, colour_target, colour_type,
			  &background, &foreground, &outline)))
    {
      if(colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED && default_fill)
	background = default_fill;

      foo_canvas_item_set(interval,
			  "fill_color_gdk",    background,
			  "outline_color_gdk", outline,
			  NULL);
    }

  return ;
}


/* object impl */
static void zmap_window_assembly_feature_class_init  (ZMapWindowAssemblyFeatureClass assembly_class)
{
  ZMapWindowCanvasItemClass canvas_class;
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) assembly_class;
  canvas_class  = (ZMapWindowCanvasItemClass)assembly_class;

  gobject_class->set_property = zmap_window_assembly_feature_set_property;
  gobject_class->get_property = zmap_window_assembly_feature_get_property;

  canvas_item_parent_class_G = (ZMapWindowCanvasItemClass)g_type_class_peek_parent(assembly_class);

  canvas_class->obj_size = sizeof(zmapWindowAssemblyFeatureStruct) ;
  canvas_class->obj_total = 0 ;

  canvas_class->add_interval = zmap_window_assembly_feature_add_interval;
  canvas_class->post_create  = zmap_window_assembly_feature_post_create;
  canvas_class->set_colour   = zmap_window_assembly_feature_set_colour;
  canvas_class->check_data   = NULL;

  return ;
}

static void zmap_window_assembly_feature_init        (ZMapWindowAssemblyFeature assembly)
{

  return ;
}

static void zmap_window_assembly_feature_set_property(GObject               *object,
						      guint                  param_id,
						      const GValue          *value,
						      GParamSpec            *pspec)

{
  ZMapWindowCanvasItem canvas_item;
  ZMapWindowAssemblyFeature assembly;

  g_return_if_fail(ZMAP_IS_WINDOW_ASSEMBLY_FEATURE(object));

  assembly = ZMAP_WINDOW_ASSEMBLY_FEATURE(object);
  canvas_item = ZMAP_CANVAS_ITEM(object);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_assembly_feature_get_property(GObject               *object,
						      guint                  param_id,
						      GValue                *value,
						      GParamSpec            *pspec)
{
  return ;
}

#ifdef ASSEMBLY_REQUIRES_DESTROY
static void zmap_window_assembly_feature_destroy     (GtkObject *gtkobject)
{

  return ;
}
#endif /* ASSEMBLY_REQUIRES_DESTROY */



static void zmap_window_assembly_feature_post_create(ZMapWindowCanvasItem canvas_item)
{

  if(canvas_item_parent_class_G->post_create)
    (canvas_item_parent_class_G->post_create)(canvas_item);

  return ;
}


