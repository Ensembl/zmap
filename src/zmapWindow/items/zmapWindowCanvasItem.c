/*  File: zmapWindowCanvasItem.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the base functions of a zmap canvas item
 *              which is derived from a foocanvas item.
 *
 * Exported functions: See zmapWindowCanvas.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>


#include <string.h>		/* memcpy */
#include <zmapWindowCanvas.h>
#include <zmapWindowCanvasItem_I.h>


typedef enum
  {
    WINDOW_CANVAS_ITEM_0,		/* zero == invalid property id */
    WINDOW_CANVAS_ITEM_INTERVAL_TYPE,
    WINDOW_CANVAS_ITEM_FEATURE,
    WINDOW_CANVAS_ITEM_USER_HIDDEN,
    WINDOW_CANVAS_ITEM_CODE_HIDDEN,
    WINDOW_CANVAS_ITEM_DEBUG,
  } WindowCanvasItemType ;


/* Some convenience stuff */
#define GCI_UPDATE_MASK (FOO_CANVAS_UPDATE_REQUESTED | FOO_CANVAS_UPDATE_DEEP)
#define GCI_EPSILON 1e-18

#ifdef RDS_NEVER_INCLUDE

#define DEBUG_BACKGROUND_BOX

#define DEBUG_ITEM_MARK

#endif



typedef struct
{
  ZMapWindowCanvasItem parent;
  ZMapFeature          feature;
  ZMapFeatureSubPartSpan sub_feature;
  ZMapStyleColourType  style_colour_type;
  int 			colour_flags;
  GdkColor            *default_fill_colour;
  GdkColor            *border_colour;
  ZMapWindowCanvasItemClass klass;
}EachIntervalDataStruct, *EachIntervalData;


static void zmap_window_canvas_item_class_init  (ZMapWindowCanvasItemClass class);
static void zmap_window_canvas_item_init        (ZMapWindowCanvasItem      group);


static void zmap_window_canvas_item_destroy     (GtkObject *gtkobject);


//static void zmap_window_canvas_item_post_create(ZMapWindowCanvasItem canvas_item);
static void zmap_window_canvas_item_set_colour(ZMapWindowCanvasItem   canvas_item,
					       FooCanvasItem         *interval,
					       ZMapFeature 	      feature,
					       ZMapFeatureSubPartSpan unused,
					       ZMapStyleColourType    colour_type,
					       int 				colour_flags,
					       GdkColor              *default_fill,
                                     GdkColor              *border);

static ZMapFeatureTypeStyle zmap_window_canvas_item_get_style(ZMapWindowCanvasItem canvas_item);




/* A couple of accessories */
static double window_canvas_item_invoke_point (FooCanvasItem *item,
					       double x, double y,
					       int cx, int cy,
					       FooCanvasItem **actual_item);


static void window_canvas_invoke_set_colours(gpointer list_data, gpointer user_data);





/* Globals.... */

static FooCanvasItemClass *group_parent_class_G;

static gboolean debug_point_method_G = FALSE;


/*!
 * \brief Get the GType for ZMapWindowCanvasItem
 *
 * \details Registers the ZMapWindowCanvasItem class if necessary, and returns the type ID
 *          associated to it.
 *
 * \return  The type ID of the ZMapWindowCanvasItem class.
 **/

GType zMapWindowCanvasItemGetType (void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowCanvasItemClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_canvas_item_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowCanvasItem),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_canvas_item_init,
	  NULL
	};

      group_type = g_type_register_static (foo_canvas_item_get_type (),
					   ZMAP_WINDOW_CANVAS_ITEM_NAME,
					   &group_info,
					   0);
  }

  return group_type;
}



/*!
 * \brief   Function to access the ZMapFeature
 * \details Although a little surprising, NULL maybe returned legitimately,
 *          as not all ZMapWindowCanvasItem subclasses require features...
 *          It is architected this way so that we can have _all_ the items
 *          on the canvas below ZMapWindowContainerFeatureSet extend
 *          ZMapWindowCanvasItem .
 * \param   canvas_item The owning ZMapWindowCanvasItem
 * \return  The ZMapFeature or NULL. NULL maybe returned legitimately!
 */
ZMapFeature zMapWindowCanvasItemGetFeature(FooCanvasItem *any_item)
{
  ZMapWindowCanvasItem canvas_item ;
  ZMapFeature feature = NULL ;

  if (ZMAP_IS_CANVAS_ITEM(any_item))
    canvas_item = ZMAP_CANVAS_ITEM(any_item) ;
  else
    canvas_item = zMapWindowCanvasItemIntervalGetObject(any_item) ;

  if (canvas_item)
    feature = canvas_item->feature ;

  return feature ;
}






ZMapFeatureSubPartSpan zMapWindowCanvasItemIntervalGetData(FooCanvasItem *item)
{
  ZMapFeatureSubPartSpan sub_feature = NULL;

  	if(ZMAP_IS_WINDOW_FEATURESET_ITEM(item))
	{
		  sub_feature =
  			zMapWindowCanvasFeaturesetGetSubPartSpan(item , zMapWindowCanvasItemGetFeature(item) ,0, 0);
	}
#if !ZWCI_AS_FOO
	else
	{
		sub_feature = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA);
	}
#endif

  return sub_feature;
}

FooCanvasItem *zMapWindowCanvasItemGetInterval(ZMapWindowCanvasItem canvas_item,
					       double x, double y,
					       ZMapFeatureSubPartSpan *sub_feature_out)
{
  FooCanvasItem *matching_interval = NULL;
  FooCanvasItem *item, *check;
  FooCanvas *canvas;
  double ix1, ix2, iy1, iy2;
  double wx1, wx2, wy1, wy2;
  double gx, gy, dist;
  int cx, cy;
  int small_enough;

  zMapLogReturnValIfFail(ZMAP_IS_CANVAS_ITEM(canvas_item), matching_interval);

  /* The background can be clipped by the long items code, so we
   * need to use the groups position as the background extends
   * this far really! */

  item   = (FooCanvasItem *)canvas_item;
  canvas = item->canvas;
  gx = x;
  gy = y;

  foo_canvas_item_w2i(item, &gx, &gy);

  foo_canvas_w2c(canvas, x, y, &cx, &cy);

  if(debug_point_method_G)
    printf("GetInterval like ->point(canvas_item=%p, gx=%f, gy=%f, cx=%d, cy=%d)\n", item, gx, gy, cx, cy);

  foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);
  wx1 = ix1;
  wy1 = iy1;
  wx2 = ix2;
  wy2 = iy2;
  foo_canvas_item_i2w(item, &wx1, &wy1);
  foo_canvas_item_i2w(item, &wx2, &wy2);

  if(debug_point_method_G)
    printf("ItemCoords(%f,%f,%f,%f), WorldCoords(%f,%f,%f,%f)\n",
	   ix1, iy1, ix2, iy2,
	   wx1, wy1, wx2, wy2);

	   /* mh17 NOTE this invoke function work with simple foo canvas items and groups */
  dist = window_canvas_item_invoke_point(item, gx, gy, cx, cy, &check);

  small_enough = (int)(dist * canvas->pixels_per_unit_x + 0.5);

  if(small_enough <= canvas->close_enough)
    {
      /* we'll continue! */

	matching_interval = item;
    }

  if(matching_interval == NULL)
    g_warning("No matching interval!");
  else if(sub_feature_out)
  {
  	if(ZMAP_IS_WINDOW_FEATURESET_ITEM(matching_interval))
  	{
  		/* returns a static data structure */
  		*sub_feature_out =
  			zMapWindowCanvasFeaturesetGetSubPartSpan(matching_interval, zMapWindowCanvasItemGetFeature(item) ,x,y);
  	}
  	else
  		*sub_feature_out = g_object_get_data(G_OBJECT(matching_interval), ITEM_SUBFEATURE_DATA);
  }

  return matching_interval;
}



/* for composite items (eg ZMapWindowGraphDensity)
 * set the pointed at feature to be the one nearest to cursor
 * We need this to stabilise the feature while processing a mouse click (item/feature select)
 * point used to set this but the cursor is still moving while we trundle thro the code
 * causing the feature to change with unpredictable results
 * write up in density.html when i get round to correcting it.
 */
gboolean zMapWindowCanvasItemSetFeature(ZMapWindowCanvasItem item, double x, double y)
{
	ZMapWindowCanvasItemClass class = ZMAP_CANVAS_ITEM_GET_CLASS(item);
	gboolean ret = FALSE;

	if(class->set_feature)
		ret = class->set_feature((FooCanvasItem *) item,x,y);

	return ret;
}


/* a pointless function created due to scope issues */
gboolean zMapWindowCanvasItemSetFeaturePointer(ZMapWindowCanvasItem item, ZMapFeature feature)
{
	int pop = 0;

	/* collpased features have 0 population, the one that was displayed has the total
	 * if we use window seacrh then select a collapsed feature we have to update this
	 * see zmapViewfeatureCollapse.c etc; scan for 'population' and 'collasped'
	 */

	if(item->feature)
		pop = item->feature->population;

	if(feature->flags.collapsed)
		feature->population = pop;

	item->feature = feature;
	return(TRUE);
}

gboolean zMapWindowCanvasItemSetStyle(ZMapWindowCanvasItem item, ZMapFeatureTypeStyle style)
{
	ZMapWindowCanvasItemClass class = ZMAP_CANVAS_ITEM_GET_CLASS(item);
	gboolean ret = FALSE;

	if(class->set_style)
		ret = class->set_style((FooCanvasItem *) item, style);

	return ret;
}

/* like foo but for a ZMap thingy */
gboolean zMapWindowCanvasItemShowHide(ZMapWindowCanvasItem item, gboolean show)
{
	ZMapWindowCanvasItemClass class = ZMAP_CANVAS_ITEM_GET_CLASS(item);
	gboolean ret = FALSE;

	if(class->showhide)
		ret = class->showhide((FooCanvasItem *) item, show);
	else
	{
		if(show)
			foo_canvas_item_show(FOO_CANVAS_ITEM(item));
		else
			foo_canvas_item_hide(FOO_CANVAS_ITEM(item));
	}

	return ret;

}




/* Get at parent... */
/* mh17 NOTE this is now a null function, unless we have naked foo items on the canvas */
ZMapWindowCanvasItem zMapWindowCanvasItemIntervalGetObject(FooCanvasItem *item)
{
  ZMapWindowCanvasItem canvas_item = NULL;

  if(ZMAP_IS_CANVAS_ITEM(item))
    canvas_item = ZMAP_CANVAS_ITEM(item);

  return canvas_item;
}





gboolean zMapWindowCanvasItemIsMasked(ZMapWindowCanvasItem item,gboolean andHidden)
{
      ZMapFeature feature;
      ZMapFeatureTypeStyle style;

      feature = item->feature;
      style = feature->style;

      if(style->mode == ZMAPSTYLE_MODE_ALIGNMENT && feature->feature.homol.flags.masked)
      {
            if(andHidden && feature->feature.homol.flags.displayed)
                  return FALSE;
            return TRUE;
      }
      return FALSE;
}



/* If item is the parent item then the whole feature is coloured, otherwise just the sub-item
 * is coloured... */
/* NOTE
 * this is a legacy interface, sub-feature is used for sequence features (recent mod)
 * now we don't have parent items, so we always do the whole
 * this relates to split alignment features and whole transcripts
 * which is a user requirement possibly implemented by fluke due the structure of the data given to ZMap
 */
void zMapWindowCanvasItemSetIntervalColours(FooCanvasItem *item, ZMapFeature feature, ZMapFeatureSubPartSpan sub_feature,
					    ZMapStyleColourType colour_type,
					    int colour_flags,
                                  GdkColor *default_fill_colour,
                                  GdkColor *border_colour)
{
  ZMapWindowCanvasItem canvas_item ;
  GList *item_list, dummy_item = {NULL} ;
  EachIntervalDataStruct interval_data = {NULL};

  zMapLogReturnIfFail(FOO_IS_CANVAS_ITEM(item)) ;

  canvas_item = zMapWindowCanvasItemIntervalGetObject(item) ;
  dummy_item.data = item ;
  item_list = &dummy_item ;

  interval_data.parent = canvas_item ;
  interval_data.feature = feature;
  interval_data.sub_feature = sub_feature;
  interval_data.style_colour_type = colour_type ;
  interval_data.colour_flags = colour_flags;
  interval_data.default_fill_colour = default_fill_colour ;
  interval_data.border_colour = border_colour ;
  interval_data.klass = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item) ;

  g_list_foreach(item_list, window_canvas_invoke_set_colours, &interval_data) ;

  return ;
}





/*
 *                Internals routines
 */







/*
 *                Internal routines.
 */


/* Class initialization function for ZMapWindowCanvasItemClass */
static void zmap_window_canvas_item_class_init (ZMapWindowCanvasItemClass window_class)
{
  GtkObjectClass *object_class;
  FooCanvasItemClass *item_class;
  GType canvas_item_type, parent_type;
  int make_clickable_bmp_height  = 4;
  int make_clickable_bmp_width   = 16;
  char make_clickable_bmp_bits[] =
    {
      0x00, 0x00,
      0x00, 0x00,
      0x00, 0x00,
      0x00, 0x00
    } ;

//  gobject_class = (GObjectClass *)window_class;
  object_class  = (GtkObjectClass *)window_class;
  item_class    = (FooCanvasItemClass *)window_class;

  canvas_item_type = g_type_from_name(ZMAP_WINDOW_CANVAS_ITEM_NAME);
  parent_type      = g_type_parent(canvas_item_type);

  group_parent_class_G = gtk_type_class(parent_type);


  object_class->destroy = zmap_window_canvas_item_destroy;


//  window_class->post_create  = zmap_window_canvas_item_post_create;
  window_class->set_colour   = zmap_window_canvas_item_set_colour;
  window_class->get_style    = zmap_window_canvas_item_get_style;

  window_class->fill_stipple = gdk_bitmap_create_from_data(NULL, &make_clickable_bmp_bits[0],
							   make_clickable_bmp_width,
							   make_clickable_bmp_height) ;

  /* init the stats fields. */
  zmapWindowItemStatsInit(&(window_class->stats), ZMAP_TYPE_CANVAS_ITEM) ;

  return ;
}


/* Object initialization function for ZMapWindowCanvasItem */
static void zmap_window_canvas_item_init (ZMapWindowCanvasItem canvas_item)
{
  ZMapWindowCanvasItemClass canvas_item_class ;

  canvas_item_class = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item) ;

  zmapWindowItemStatsIncr(&(canvas_item_class->stats)) ;

  return ;
}



/* Destroy handler for canvas groups */
static void zmap_window_canvas_item_destroy (GtkObject *gtkobject)
{
  ZMapWindowCanvasItem canvas_item;
  ZMapWindowCanvasItemClass canvas_item_class ;
//  int i;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("In %s %s()\n", __FILE__, __PRETTY_FUNCTION__) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  canvas_item = ZMAP_CANVAS_ITEM(gtkobject);
  canvas_item_class = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item) ;


  canvas_item->feature = NULL;

  if(GTK_OBJECT_CLASS (group_parent_class_G)->destroy)
    (GTK_OBJECT_CLASS (group_parent_class_G)->destroy)(GTK_OBJECT(gtkobject));

  zmapWindowItemStatsDecr(&(canvas_item_class->stats)) ;

  return ;
}

#ifdef NEVER_INCLUDE_DEBUG_EVENTS
static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	FooCanvasRE *rect;
	double ix1, ix2, iy1, iy2;
	double wx1, wx2, wy1, wy2;

	rect = FOO_CANVAS_RE(item);

	foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);

	wx1 = ix1;
	wx2 = ix2;
	wy1 = iy1;
	wy2 = iy2;

	foo_canvas_item_i2w(item, &wx1, &wy1);
	foo_canvas_item_i2w(item, &wx2, &wy2);

	printf("Scroll: (%f,%f)\n", item->canvas->scroll_y1, item->canvas->scroll_y2);
	printf("Rect: (%f,%f), Item: (%f,%f)\n", rect->y1, rect->y2, item->y1, item->y2);
	printf("Item: (%f,%f), World: (%f, %f)\n", iy1, iy2, wy1, wy2);

      }
      break;
    default:
      break;
    }
  return FALSE;
}
#endif /* NEVER_INCLUDE_DEBUG_EVENTS */




static ZMapFeatureTypeStyle zmap_window_canvas_item_get_style(ZMapWindowCanvasItem canvas_item)
{
  ZMapFeatureTypeStyle style = NULL;

  zMapLogReturnValIfFail(canvas_item != NULL, NULL);
  zMapLogReturnValIfFail(canvas_item->feature != NULL, NULL);

  style = canvas_item->feature->style;

  return style;
}





void zmapWindowCanvasItemGetColours(ZMapFeatureTypeStyle style, ZMapStrand strand, ZMapFrame frame,
                                    ZMapStyleColourType    colour_type,
                                    GdkColor **fill, GdkColor **draw, GdkColor **outline,
                                    GdkColor              *default_fill,
                                    GdkColor              *border)
{
      ZMapStyleParamId colour_target = STYLE_PROP_COLOURS;

      if (zMapStyleIsFrameSpecific(style))
      {
        switch (frame)
          {
          case ZMAPFRAME_0:
            colour_target = STYLE_PROP_FRAME0_COLOURS ;
            break ;
          case ZMAPFRAME_1:
            colour_target = STYLE_PROP_FRAME1_COLOURS ;
            break ;
          case ZMAPFRAME_2:
            colour_target = STYLE_PROP_FRAME2_COLOURS ;
            break ;
          default:
//            zMapAssertNotReached() ; no longer valid: frame specific style by eg for swissprot we display in one col on startup
            break ;
          }

        zMapStyleGetColours(style, colour_target, colour_type,
                        fill, draw, outline);
      }

      colour_target = STYLE_PROP_COLOURS;
      if (strand == ZMAPSTRAND_REVERSE && zMapStyleColourByStrand(style))
      {
        colour_target = STYLE_PROP_REV_COLOURS;
      }

      if (*fill == NULL && *draw == NULL && *outline == NULL)
            zMapStyleGetColours(style, colour_target, colour_type, fill, draw, outline);


  if (colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED)
    {
      if(default_fill)
            *fill = default_fill;
      if(border)
            *outline = border;
    }
}

static void zmap_window_canvas_item_set_colour(ZMapWindowCanvasItem   canvas_item,
					       FooCanvasItem         *interval,
					       ZMapFeature 		feature,
					       ZMapFeatureSubPartSpan unused,
					       ZMapStyleColourType    colour_type,
					       int 				colour_flags,
					       GdkColor              *default_fill,
                                     GdkColor              *border)

{
  ZMapFeatureTypeStyle style;
  GdkColor *draw = NULL, *fill = NULL, *outline = NULL;
  GType interval_type;

  zMapLogReturnIfFail(canvas_item != NULL);
  zMapLogReturnIfFail(interval    != NULL);
  zMapLogReturnIfFail(canvas_item->feature != NULL);

  if((style = (ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->get_style)(canvas_item)))
    {
      ZMapFrame frame;
      ZMapStrand strand;

      frame = zMapFeatureFrame(canvas_item->feature);
      strand = canvas_item->feature->strand;
      zmapWindowCanvasItemGetColours(style, strand, frame, colour_type, &fill, &draw, &outline, default_fill, border);
    }


  interval_type = G_OBJECT_TYPE(interval);


  if (g_type_is_a(interval_type, FOO_TYPE_CANVAS_LINE))
      // mh17: not used || g_type_is_a(interval_type, FOO_TYPE_CANVAS_LINE_GLYPH))
    {
      /* Using the fill colour seems a mis-nomer.... */

      foo_canvas_item_set(interval,
			  "fill_color_gdk", fill,
			  NULL);
    }
  else if (g_type_is_a(interval_type, FOO_TYPE_CANVAS_TEXT))
    {
      /* For text it seems to be more intuitive to use the "draw" colour for the text. */

      foo_canvas_item_set(interval,
			  "fill_color_gdk", draw,
			  NULL);
    }
  else if(g_type_is_a(interval_type, FOO_TYPE_CANVAS_RE)      ||
	  g_type_is_a(interval_type, FOO_TYPE_CANVAS_POLYGON)
		)
    {
      foo_canvas_item_set(interval,
			  "fill_color_gdk", fill,
			  NULL);

      if (outline)
	foo_canvas_item_set(interval,
			    "outline_color_gdk", outline,
			    NULL);
    }
  else
    {
      g_warning("Interval has unknown FooCanvasItem type '%s'.", g_type_name(interval_type));
    }

  return ;
}





/*
 * This routine invokes the point method of the item.
 * The arguments x, y should be in the parent item local coordinates.
 */

static double window_canvas_item_invoke_point (FooCanvasItem *item,
					       double x, double y,
					       int cx, int cy,
					       FooCanvasItem **actual_item)
{
  /* Calculate x & y in item local coordinates */

  if (FOO_CANVAS_ITEM_GET_CLASS (item)->point)
    return FOO_CANVAS_ITEM_GET_CLASS (item)->point (item, x, y, cx, cy, actual_item);

  return 1e18;
}


static void window_canvas_invoke_set_colours(gpointer list_data, gpointer user_data)
{
  FooCanvasItem *interval        = FOO_CANVAS_ITEM(list_data);
  EachIntervalData interval_data = (EachIntervalData)user_data;

  if(interval_data->feature)
    {
      ZMapFeatureSubPartSpanStruct local_struct;
      ZMapFeatureSubPartSpan sub_feature = NULL;


	/* new interface via calling stack */
	sub_feature = interval_data->sub_feature;

	/* if not provided (eg via window focus) the revert to the old */
	/* i'd rather get rid of these lurking pointers but I'm doing somehting else ATM */
	if(!sub_feature && !(sub_feature = g_object_get_data(G_OBJECT(interval), ITEM_SUBFEATURE_DATA)))
	{
	  sub_feature = &local_struct;
	  sub_feature->subpart = ZMAPFEATURE_SUBPART_INVALID;
	}

      if(interval_data->klass->set_colour)
	interval_data->klass->set_colour(interval_data->parent, interval, interval_data->feature, sub_feature,
					 interval_data->style_colour_type,
					 interval_data->colour_flags,
					 interval_data->default_fill_colour,
                               interval_data->border_colour);
    }

  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printBounds(FooCanvasItem *item, char *txt)
{
  double x1, y1, x2, y2 ;
  double world_x1, world_y1, world_x2, world_y2 ;

  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  world_x1 = x1 ;
  world_y1 = y1 ;
  world_x2 = x2 ;
  world_y2 = y2 ;

  foo_canvas_item_i2w(item, &world_x1, &world_y1) ;
  foo_canvas_item_i2w(item, &world_x2, &world_y2) ;

  printf("%s\n"
	 "\tposition of %p within %p:\tx1 = %g, y1 = %g, x2 = %g, y2 = %g\n"
	 "\t    world position of %p:\tx1 = %g, y1 = %g, x2 = %g, y2 = %g\n",
	 txt,
	 item, item->parent, x1, y1, x2, y2,
	 item, world_x1, world_y1, world_x2, world_y2) ;

  fflush(stdout) ;

  return ;
}
#endif



/*  LocalWords:  zmapWindowCanvasItem ZMap Griffiths
 */
