/*  File: zmapWindowAllBase.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Contains functions required by all zmap item and
 *              group types. These are mostly utility functions
 *              like stats recording and so on.
 *
 * Exported functions: See zmapWindowAllBase.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>


#include <zmapWindowCanvasItem_I.h>
#include <zmapWindowContainers_I.h>

#include <zmapWindowAllBase.h>


#define PRINT_ITEM_TYPE(INSTANCE) \
  printf("%s", g_type_name(G_TYPE_FROM_INSTANCE((INSTANCE))))


/* Surprisingly I thought this would be typedef somewhere in the GType header but not so... */
typedef GType (*MyGTypeFunc)(void) ;


typedef struct MyGObjectInfoStructName
{
  MyGTypeFunc get_type_func ;
  GType obj_type ;
  char *obj_name ;
  unsigned int obj_size ;
} MyGObjectInfoStruct, *MyGObjectInfo ;


static GType getItemType(FooCanvasItem *item, gboolean print) ;
static MyGObjectInfo initObjectDescriptions(void) ;
static MyGObjectInfo findObjectInfo(GType type) ;





/* Returns the most derived class type for an item, i.e. effectively this is the "true"
 * type of the object. */
GType zmapWindowItemTrueType(FooCanvasItem *item)
{
  GType item_type ;

  item_type = getItemType(item, FALSE) ;

  return item_type ;
}


/* Same as zmapWindowItemTrueType() but prints out a hierachy of the derived classes
 * starting with FooCanvasItem. */
GType zmapWindowItemTrueTypePrint(FooCanvasItem *item)
{
  GType item_type ;

  item_type = getItemType(item, TRUE) ;

  return item_type ;
}



/* The following are a set of simple functions to init, increment and decrement stats counters. */

void zmapWindowItemStatsInit(ZMapWindowItemStats stats, GType item_type)
{
  MyGObjectInfo object_data ;

  if ((object_data = findObjectInfo(item_type)))
    {
      stats->type = object_data->obj_type ;
      stats->name = object_data->obj_name ;
      stats->size = object_data->obj_size ;
      stats->total = stats->curr = 0 ;
    }

  return ;
}

void zmapWindowItemStatsIncr(ZMapWindowItemStats stats)
{
  stats->total++ ;
  stats->curr++ ;

  return ;
}

/* "total" counter is a cumulative total so is not decremented. */
void zmapWindowItemStatsDecr(ZMapWindowItemStats stats)
{
  stats->curr-- ;

  return ;
}





/*
 *                  Internal routines.
 */



/* Returns the most derived class type for an item, i.e. effectively this is the "true"
 * type of the object and optionally prints out the types as it goes.
 *
 * NOTE that for this to work correctly we rely on the objects description array to be correctly
 * ordered.
 */
static GType getItemType(FooCanvasItem *item, gboolean print)
{
  GType item_type = 0 ;					    /* Have assumed that zero
							       is not a valid type value. */
  GType type ;
  static MyGObjectInfo objects = NULL ;
  MyGObjectInfo tmp ;

  if (!objects)
    objects = initObjectDescriptions() ;

  type = G_TYPE_FROM_INSTANCE(item) ;

  tmp = objects ;
  while (tmp->get_type_func)
    {
      if ((G_TYPE_CHECK_INSTANCE_TYPE((item), tmp->get_type_func())))
	{
	  if (item_type)
	    printf(" -> ") ;

	  if (print)
	    printf("%s", tmp->obj_name) ;

	  item_type = type ;
	}

      tmp++ ;
    }

  if (print && !item_type)
    printf("unknown type !") ;

  printf("\n") ;

  return item_type ;
}



/* Find the array entry for a particular type. */
static MyGObjectInfo findObjectInfo(GType type)
{
  MyGObjectInfo object_data = NULL ;
  static MyGObjectInfo objects = NULL ;
  MyGObjectInfo tmp ;

  if (!objects)
    objects = initObjectDescriptions() ;

  tmp = objects ;
  while (tmp->get_type_func)
    {
      if (tmp->obj_type == type)
	{
	  object_data = tmp ;
	  break ;
	}

      tmp++ ;
    }

  return object_data ;
}


/* Fills in an array of structs which can be used for various operations on all the
 * items we use on the canvas in zmap.
 *
 * NOTE that the array is ordered so that for the getType/print functions the
 * subclassing order and final type will be correct, i.e. we have parent classes
 * followed by derived classes.
 *
 *  */
static MyGObjectInfo initObjectDescriptions(void)
{
  static MyGObjectInfoStruct object_descriptions[] =
    {
      /* Original foocanvas items */
      {foo_canvas_item_get_type, 0, NULL, sizeof(FooCanvasItem)},
      {foo_canvas_group_get_type, 0, NULL, sizeof(FooCanvasGroup)},
      {foo_canvas_line_get_type, 0, NULL, sizeof(FooCanvasLine)},
      {foo_canvas_pixbuf_get_type, 0, NULL, sizeof(FooCanvasPixbuf)},
      {foo_canvas_polygon_get_type, 0, NULL, sizeof(FooCanvasPolygon)},
      {foo_canvas_re_get_type, 0, NULL, sizeof(FooCanvasRE)},
      {foo_canvas_text_get_type, 0, NULL, sizeof(FooCanvasText)},
      {foo_canvas_widget_get_type, 0, NULL, sizeof(FooCanvasWidget)},

      /* zmap types added to foocanvas */
      {foo_canvas_float_group_get_type, 0, NULL, sizeof(FooCanvasFloatGroup)},
      {foo_canvas_line_glyph_get_type, 0, NULL, sizeof(FooCanvasLineGlyph)},
      {foo_canvas_zmap_text_get_type, 0, NULL, sizeof(FooCanvasZMapText)},

      /* zmap item types */
      {zMapWindowCanvasItemGetType, 0, NULL, sizeof(zmapWindowCanvasItem)},
      {zmapWindowContainerGroupGetType, 0, NULL, sizeof(zmapWindowContainerGroup)},
      {zmapWindowContainerAlignmentGetType, 0, NULL, sizeof(zmapWindowContainerAlignment)},
      {zmapWindowContainerBlockGetType, 0, NULL, sizeof(zmapWindowContainerBlock)},
     {zmapWindowContainerContextGetType, 0, NULL, sizeof(zmapWindowContainerContext)},
      {zmapWindowContainerFeatureSetGetType, 0, NULL, sizeof(zmapWindowContainerFeatureSet)},
     /* end of array. */
      {NULL, 0, NULL, 0}
    } ;
  MyGObjectInfo tmp ;

  tmp = object_descriptions ;

  while (tmp->get_type_func)
    {
      tmp->obj_type = (tmp->get_type_func)() ;

      tmp->obj_name = (char *)g_type_name(tmp->obj_type) ;

      tmp++ ;
    }

  return object_descriptions ;
}



