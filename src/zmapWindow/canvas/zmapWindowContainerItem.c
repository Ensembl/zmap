/*  File: zmapWindowOldItems.c
 *  Author: ed (edgrif@sanger.ac.uk)
 *  Copyright (c) 2013: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Contains functions from Roy Storey's rewrite of the
 *              original drawing code. Roy's code was all in
 *              zmapWindow/items and has now been subsumed/replaced
 *              by Malcolm Hinsley's rewrite contained in
 *              zmapWindow/canvas.
 *              The intention is to gradually remove all of Roy's
 *              original code but for now it is in this file.
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <zmapWindowCanvasItem_I.h>



static void zmap_window_canvas_item_class_init  (ZMapWindowCanvasItemClass class);
static void zmap_window_canvas_item_init        (ZMapWindowCanvasItem      group);
static ZMapFeatureTypeStyle zmap_window_canvas_item_get_style(ZMapWindowCanvasItem canvas_item) ;
static void zmap_window_canvas_item_destroy (GtkObject *gtkobject) ;



static FooCanvasItemClass *group_parent_class_G;




/* Get the GType for ZMapWindowCanvasItem
 *
 * Registers the ZMapWindowCanvasItem class if necessary, and returns the type ID
 * associated to it.
 *
 * The type ID of the ZMapWindowCanvasItem class. */
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



/* Class initialization function for ZMapWindowCanvasItemClass */
static void zmap_window_canvas_item_class_init (ZMapWindowCanvasItemClass window_class)
{
  //  GtkObjectClass *object_class;
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


static ZMapFeatureTypeStyle zmap_window_canvas_item_get_style(ZMapWindowCanvasItem canvas_item)
{
  ZMapFeatureTypeStyle style = NULL;

  zMapLogReturnValIfFail(canvas_item != NULL, NULL);
  zMapLogReturnValIfFail(canvas_item->feature != NULL, NULL);

  style = *canvas_item->feature->style;

  return style;
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

  /* NOTE FooCanavsItems call destroy not destroy, which is in a Gobject  but not a GTK Object */
  if (G_OBJECT_CLASS (group_parent_class_G)->dispose)
    (G_OBJECT_CLASS (group_parent_class_G)->dispose)(G_OBJECT(gtkobject));

  zmapWindowItemStatsDecr(&(canvas_item_class->stats)) ;

  return ;
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



