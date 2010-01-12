/*  File: zmapWindowBasicFeature.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Last edited: Jul  3 17:07 2009 (rds)
 * Created: Wed Dec  3 10:02:22 2008 (rds)
 * CVS info:   $Id: zmapWindowBasicFeature.c,v 1.10 2010-01-12 09:17:28 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindowBasicFeature_I.h>
//#include <zmapWindow_P.h>	/* ITEM_FEATURE_DATA, ITEM_FEATURE_TYPE */

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
  static GType group_type = 0;
  
  if (!group_type) {
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
    
    group_type = g_type_register_static (zMapWindowCanvasItemGetType(),
					 ZMAP_WINDOW_BASIC_FEATURE_NAME,
					 &group_info,
					 0);
  }
  
  return group_type;
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
      char *fill = "white",*outline = "black";
      GdkColor gdk_fill,gdk_outline;
      GdkColor *pfill = &gdk_fill,*poutline = &gdk_outline;
      feature = basic->feature;
      style   = (ZMAP_CANVAS_ITEM_GET_CLASS(basic)->get_style)(basic);

      if(interval_type_from_feature_type)
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
	    /* where do we do the points calculation? */
	    /* Should it be here from the left, right, top, bottom cogreenords? */
	    /* Should the glyph code do it? */
	    /* intron/gaps are done at this level... */
	    int type = 0;
          int mode = zMapStyleGlyphMode(style);

          switch(mode)
          {
          case ZMAPSTYLE_GLYPH_SPLICE:          // hard coded on GF_Splice feature - should never be configured
                  if(feature->strand == ZMAPSTRAND_FORWARD)
                    type = ZMAP_GLYPH_ITEM_STYLE_TRIANGLE;
                  else 
                    type = ZMAP_GLYPH_ITEM_STYLE_TRIANGLE; // mh17: (sic)

                  fill = "blue";

                  break;

          case ZMAPSTYLE_GLYPH_MARKER:
                  type = zMapStyleGlyphType(style);
                  fill = "green";

                  break;
          }
          if(!zMapStyleGetColoursDefault(style,&pfill,NULL,&poutline))
            {
                  gdk_color_parse(fill,pfill);
                  gdk_color_parse(outline,poutline);
            }


	    basic->auto_resize_background = 1;

	    item = foo_canvas_item_new(FOO_CANVAS_GROUP(basic),
				ZMAP_TYPE_WINDOW_GLYPH_ITEM,
				"glyph_style", type,
				"x",           0.0,
				"y",           0.0,
				"width",       right,
				"height",      right,   // mh17: gross!.. right being the width of the column? so this means square??
				"line-width",  1,
				"join_style",  GDK_JOIN_BEVEL,
				"cap_style",   GDK_CAP_BUTT,
                        "fill_color_gdk", pfill,
                        "outline_color_gdk", poutline,
				NULL);
	  }
	  break;
	case ZMAP_WINDOW_BASIC_BOX:
	default:
	  {
	    basic->auto_resize_background = 1;
	    item = foo_canvas_item_new(FOO_CANVAS_GROUP(basic), 
				       FOO_TYPE_CANVAS_RECT,
				       "x1", left,  "y1", top,
				       "x2", right, "y2", bottom,
				       NULL);
	  }
	  break;
	}
    }

  return item;
}

/* object impl */
static void zmap_window_basic_feature_class_init  (ZMapWindowBasicFeatureClass basic_class)
{
  ZMapWindowCanvasItemClass canvas_class;
  GObjectClass *gobject_class;
  
  gobject_class = (GObjectClass *) basic_class;
  canvas_class  = (ZMapWindowCanvasItemClass)basic_class;

  gobject_class->set_property = zmap_window_basic_feature_set_property;
  gobject_class->get_property = zmap_window_basic_feature_get_property;
  

  canvas_class->add_interval = zmap_window_basic_feature_add_interval;
  canvas_class->check_data   = NULL;

  return ;
}

static void zmap_window_basic_feature_init        (ZMapWindowBasicFeature basic)
{

  return ;
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
#ifdef BASIC_REQUIRES_DESTROY
static void zmap_window_basic_feature_destroy     (GObject *object)
{

  return ;
}
#endif /* BASIC_REQUIRES_DESTROY */



