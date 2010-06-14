/*  File: zmapWindowBasicFeature.c
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
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 26 12:54 2010 (edgrif)
 * Created: Wed Dec  3 10:02:22 2008 (rds)
 * CVS info:   $Id: zmapWindowBasicFeature.c,v 1.21 2010-06-14 15:40:17 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <zmapWindowBasicFeature_I.h>


enum
  {
    BASIC_INTERVAL_0,		/* invalid */
    BASIC_INTERVAL_TYPE
  };


static void zmap_window_basic_feature_class_init  (ZMapWindowBasicFeatureClass basic_class);
static void zmap_window_basic_feature_init        (ZMapWindowBasicFeature      group);
static void zmap_window_basic_feature_set_property(GObject               *object,
						   guint                  param_id,
						   const GValue          *value,
						   GParamSpec            *pspec);
static void zmap_window_basic_feature_get_property(GObject               *object,
						   guint                  param_id,
						   GValue                *value,
						   GParamSpec            *pspec);
#ifdef BASIC_REQUIRES_DESTROY
static void zmap_window_basic_feature_destroy     (GObject *object);
#endif /* BASIC_REQUIRES_DESTROY */

static FooCanvasItem *zmap_window_basic_feature_add_interval(ZMapWindowCanvasItem   basic,
							     ZMapFeatureSubPartSpan unused,
							     double top,  double bottom,
							     double left, double right);


GType zMapWindowBasicFeatureGetType(void)
{
  static GType group_type = 0 ;

  if (!group_type)
    {
      static const GTypeInfo group_info = {
	sizeof (zmapWindowBasicFeatureClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) zmap_window_basic_feature_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (zmapWindowBasicFeature),
	0,              /* n_preallocs */
	(GInstanceInitFunc) zmap_window_basic_feature_init


      };

      group_type = g_type_register_static(zMapWindowCanvasItemGetType(),
					  ZMAP_WINDOW_BASIC_FEATURE_NAME,
					  &group_info,
					  0) ;
    }

  return group_type;
}



static void zmap_window_basic_feature_class_init(ZMapWindowBasicFeatureClass basic_class)
{
  ZMapWindowCanvasItemClass canvas_class ;
  GObjectClass *gobject_class ;

  gobject_class = (GObjectClass *) basic_class;
  canvas_class  = (ZMapWindowCanvasItemClass)basic_class;

  gobject_class->set_property = zmap_window_basic_feature_set_property;
  gobject_class->get_property = zmap_window_basic_feature_get_property;

  canvas_class->add_interval = zmap_window_basic_feature_add_interval;
  canvas_class->check_data   = NULL;

  zmapWindowItemStatsInit(&(canvas_class->stats), ZMAP_TYPE_WINDOW_BASIC_FEATURE) ;
  zmapWindowItemStatsInit(&(basic_class->RE_stats), FOO_TYPE_CANVAS_RE) ;

  return ;
}

static void zmap_window_basic_feature_init        (ZMapWindowBasicFeature basic)
{

  return ;
}



static FooCanvasItem *zmap_window_basic_feature_add_interval(ZMapWindowCanvasItem   basic,
							     ZMapFeatureSubPartSpan unused,
							     double top,  double bottom,
							     double left, double right)
{
  FooCanvasItem *item = NULL;

  if(!(FOO_CANVAS_GROUP(basic)->item_list))
    {
      ZMapFeatureTypeStyle style;
      ZMapFeature feature;
      gboolean interval_type_from_feature_type = TRUE; /* for now */
      feature = basic->feature;


      if (interval_type_from_feature_type)
	{
	  switch(feature->type)
	    {
	    case ZMAPSTYLE_MODE_GLYPH:
	      basic->interval_type = ZMAP_WINDOW_BASIC_GLYPH;
	      break;
	    case ZMAPSTYLE_MODE_GRAPH:
	    default:
	      basic->interval_type = ZMAP_WINDOW_BASIC_BOX;
	      break;
	    }
	}

      switch(basic->interval_type)
	{
	case ZMAP_WINDOW_BASIC_GLYPH:
	  {
	    int which = 0;      // 5', 3' or generic
	    gboolean strand = FALSE;

	    if(feature->strand == ZMAPSTRAND_REVERSE)
	      strand = TRUE;

	    if (feature->flags.has_boundary)
	      {
		basic->auto_resize_background = 1;

		which = feature->boundary_type == ZMAPBOUNDARY_5_SPLICE ? 5 : 3;
	      }

            style   = (ZMAP_CANVAS_ITEM_GET_CLASS(basic)->get_style)(basic);
                  // style is as configured or retrofitted in zMapStyleMakeDrawable()
                  // x and y coords are relative to main feature set up in CanvasItemCreate()

      	      // style is as configured or retrofitted in zMapStyleMakeDrawable()
	            // x and y coords are relative to main feature set up in CanvasItemCreate()
            item = FOO_CANVAS_ITEM(zMapWindowGlyphItemCreate(FOO_CANVAS_GROUP(basic),
                  style, which, 0.0, 0.0, feature->score,strand));

      	      // colour should be set by caller, esp if style is frame specific

	    break;
	  }


	case ZMAP_WINDOW_BASIC_BOX:
	default:
	  {
	    basic->auto_resize_background = 1;

	    item = foo_canvas_item_new(FOO_CANVAS_GROUP(basic),
				       FOO_TYPE_CANVAS_RECT,
				       "x1", left,  "y1", top,
				       "x2", right, "y2", bottom,
				       NULL);

	    break;
	  }
	}
    }

  return item;
}

static void zmap_window_basic_feature_set_property(GObject               *object,
						   guint                  param_id,
						   const GValue          *value,
						   GParamSpec            *pspec)

{
  ZMapWindowCanvasItem canvas_item;
  ZMapWindowBasicFeature basic;

  g_return_if_fail(ZMAP_IS_WINDOW_BASIC_FEATURE(object));

  basic = ZMAP_WINDOW_BASIC_FEATURE(object);
  canvas_item = ZMAP_CANVAS_ITEM(object);

  switch(param_id)
    {
    case BASIC_INTERVAL_TYPE:
      {
	g_warning("how did you get here?");
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_basic_feature_get_property(GObject               *object,
						   guint                  param_id,
						   GValue                *value,
						   GParamSpec            *pspec)
{
  return ;
}


/* I DON'T WHY THIS IS COMMENTED OUT.... */
#ifdef BASIC_REQUIRES_DESTROY
static void zmap_window_basic_feature_destroy     (GObject *object)
{

  return ;
}
#endif /* BASIC_REQUIRES_DESTROY */



