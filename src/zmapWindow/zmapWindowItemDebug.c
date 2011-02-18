/*  File: zmapWindowItemDebug.c
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Feb 14 09:11 2011 (edgrif)
 * Created: Tue Nov  6 16:33:44 2007 (rds)
 * CVS info:   $Id: zmapWindowItemDebug.c,v 1.6 2011-02-18 10:09:34 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindowContainers.h>
#include <zmapWindowCanvasItem_I.h>
#include <zmapWindowFeatures.h>
#include <zmapWindow_P.h>






#define STRING_SIZE 50



static void printGroup(FooCanvasGroup *group, int indent, GString *buf) ;
static void printItem(FooCanvasItem *item) ;
static gboolean get_container_type_as_string(FooCanvasItem *item, char **str_out) ;
static gboolean get_container_child_type_as_string(FooCanvasItem *item, char **str_out) ;
static gboolean get_item_type_as_string(FooCanvasItem *item, char **str_out) ;
static gboolean get_feature_type_as_string(FooCanvasItem *item, char **str_out) ;



/* We just dip into the foocanvas struct for some data, we use the interface calls where they
 * exist. */
void zmapWindowPrintCanvas(FooCanvas *canvas)
{
  double x1, y1, x2, y2 ;


  foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);

  printf("Canvas stats:\n\n") ;

  printf("Zoom x,y: %f, %f\n", canvas->pixels_per_unit_x, canvas->pixels_per_unit_y) ;

  printf("Scroll region bounds: %f -> %f,  %f -> %f\n", x1, x2, y1, y2) ;

  zmapWindowPrintGroups(canvas) ;

  return ;
}


void zmapWindowPrintGroups(FooCanvas *canvas)
{
  FooCanvasGroup *root ;
  int indent ;
  GString *buf ;

  printf("Groups:\n") ;

  root = foo_canvas_root(canvas) ;

  indent = 0 ;
  buf = g_string_sized_new(1000) ;

  printGroup(root, indent, buf) ;

  g_string_free(buf, TRUE) ;

  return ;
}


void zmapWindowItemDebugItemToString(FooCanvasItem *item, GString *string)
{
  gboolean has_feature = FALSE, is_container = FALSE ;
  char *str = NULL ;

  if ((is_container = get_container_type_as_string(item, &str)))
    g_string_append_printf(string, " %s", str) ;
  else if (get_item_type_as_string(item, &str))
    g_string_append_printf(string, " %s", str) ;

  if ((has_feature = get_feature_type_as_string(item, &str)))
    g_string_append_printf(string, " %s", str) ;

  if (has_feature)
    {
      ZMapFeatureAny feature_any;

      feature_any = zmapWindowItemGetFeatureAny(item);
      g_string_append_printf(string, " \"%s\"", (char *)g_quark_to_string(feature_any->unique_id));
    }

  if (is_container)
    {
      FooCanvasItem *container;

      container = FOO_CANVAS_ITEM( zmapWindowContainerCanvasItemGetContainer(item) );
      if (container != item)
	{
	  g_string_append_printf(string, "Parent Details... ");
	  zmapWindowItemDebugItemToString(container, string);
	}
    }

  return ;
}



/* Prints out an items coords in local coords, good for debugging.... */
void zmapWindowPrintLocalCoords(char *msg_prefix, FooCanvasItem *item)
{
  double x1, y1, x2, y2 ;

  /* Gets bounding box in parents coord system. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  printf("%s:\t%f,%f -> %f,%f\n",
	 (msg_prefix ? msg_prefix : ""),
	 x1, y1, x2, y2) ;


  return ;
}

/* Prints out an items coords in world coords, good for debugging.... */
void zmapWindowPrintItemCoords(FooCanvasItem *item)
{
  double x1, y1, x2, y2 ;

  /* Gets bounding box in parents coord system. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  printf("P %f, %f, %f, %f -> ", x1, y1, x2, y2) ;

  foo_canvas_item_i2w(item, &x1, &y1) ;
  foo_canvas_item_i2w(item, &x2, &y2) ;

  printf("W %f, %f, %f, %f\n", x1, y1, x2, y2) ;

  return ;
}


/* Converts given world coords to an items coord system and prints them. */
void zmapWindowPrintW2I(FooCanvasItem *item, char *text, double x1_in, double y1_in)
{
  double x1 = x1_in, y1 = y1_in;

  my_foo_canvas_item_w2i(item, &x1, &y1) ;

  if (!text)
    text = "Item" ;

  printf("%s -  world(%f, %f)  ->  item(%f, %f)\n", text, x1_in, y1_in, x1, y1) ;

  return ;
}

/* Converts coords in an items coord system into world coords and prints them. */
/* Prints out item coords position in world and its parents coords.... */
void zmapWindowPrintI2W(FooCanvasItem *item, char *text, double x1_in, double y1_in)
{
  double x1 = x1_in, y1 = y1_in;

  my_foo_canvas_item_i2w(item, &x1, &y1) ;

  if (!text)
    text = "Item" ;

  printf("%s -  item(%f, %f)  ->  world(%f, %f)\n", text, x1_in, y1_in, x1, y1) ;


  return ;
}



/* 
 *                  Internal functions
 */


/* Recursive function to print out all the child groups of the supplied group. */
static void printGroup(FooCanvasGroup *group, int indent, GString *buf)
{
  int i ;
  GList *list ;

  buf = g_string_set_size(buf, 0) ;

  for (i = 0 ; i < indent ; i++)
    printf("\t") ;


  /* Print this group. */
  if (ZMAP_IS_CONTAINER_GROUP(group) || ZMAP_IS_CANVAS_ITEM(group))
    {
      zmapWindowItemDebugItemToString((FooCanvasItem *)group, buf) ;
      printf("%s", buf->str) ;
    }
  else if (FOO_IS_CANVAS_GROUP(group))
    {
      printf("%s ", "FOOCANVAS_GROUP") ;
    }
  else if (FOO_IS_CANVAS_ITEM(group))
    {
      printf("%s ", "FOOCANVAS_ITEM") ;
    }
  else
    {
      printf("%s ", "**UNKNOWN ITEM TYPE**") ;
    }

  printItem(FOO_CANVAS_ITEM(group)) ;

  if (ZMAP_IS_CONTAINER_GROUP(group))
    {
      GList *item_list ;

      if ((item_list = group->item_list))
	{
	  do
	    {
	      char *str ;
	      FooCanvasGroup *sub_group = (FooCanvasGroup *)(item_list->data) ;
	      FooCanvasItem *sub_item = (FooCanvasItem *)(item_list->data) ;
	      

	      if (get_container_child_type_as_string(sub_item, &str))
		{
		  for (i = 0 ; i < indent ; i++)
		    printf("\t") ;
		  printf("  ") ;

		  printf("%s ", str) ;

		  printItem(FOO_CANVAS_ITEM(sub_group)) ;
		}
	      else
		{
		  if (FOO_IS_CANVAS_GROUP(sub_group))
		    printGroup(sub_group, indent + 1, buf) ;
		  else
		    printItem(FOO_CANVAS_ITEM(group)) ;
		}
	    }
	  while((item_list = item_list->next)) ;
	}
    }
  else if (ZMAP_IS_CANVAS_ITEM(group))
    {
      ZMapWindowCanvasItem canvas_item ;
      int i ;

      canvas_item = ZMAP_CANVAS_ITEM(group) ;

      for (i = 0 ; i < WINDOW_ITEM_COUNT ; ++i)
	{
	  char *str = NULL ;

	  if (canvas_item->items[i])
	    {
	      switch(i)
		{
		case WINDOW_ITEM_BACKGROUND:
		  str = "ITEM_BACKGROUND" ;
		  break ;
		case WINDOW_ITEM_UNDERLAY:
		  str = "ITEM_UNDERLAY" ;
		  break ;
		case WINDOW_ITEM_OVERLAY:
		  str = "ITEM_OVERLAY" ;
		  break ;
		default:
		  break ;
		}

	      if (str)
		{
		  int j ;

		  for (j = 0 ; j < indent ; j++)
		    printf("\t") ;
		  printf("\t") ;

		  printf("%s ", str) ;

		  printItem(canvas_item->items[i]) ;

		  str = NULL ;
		}
	    }
	}
    }
  else
    {
      /* We appear to have groups that have no children...is this empty cols ? */
      /* Print all the child groups of this group. */
      if ((list = g_list_first(group->item_list)))
	{
	  do
	    {
	      if (FOO_IS_CANVAS_GROUP(list->data))
		printGroup(FOO_CANVAS_GROUP(list->data), indent + 1, buf) ;
	    }
	  while ((list = g_list_next(list))) ;
	}
    }


  buf = g_string_set_size(buf, 0) ;

  return ;
}


static void printItem(FooCanvasItem *item)
{
  double x1, y1, x2, y2 ;

  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  if (FOO_IS_CANVAS_GROUP(item))
    {
      FooCanvasGroup *group = (FooCanvasGroup *)item ;

      printf("FOO_CANVAS_GROUP Parent->Group: %p -> %p    Pos: %f, %f    Bounds: %f -> %f,  %f -> %f\n",
	     group->item.parent, group, group->xpos, group->ypos, x1, x2, y1, y2) ;
    }
  else
    {
      printf("FOO_CANVAS_ITEM Parent->Item: %p -> %p    Bounds: %f -> %f,  %f -> %f\n",
	     item->parent, item, x1, x2, y1, y2) ;
    }

  return ;
}








static gboolean get_container_type_as_string(FooCanvasItem *item, char **str_out)
{
  gboolean has_type = TRUE ;

  if (ZMAP_IS_CONTAINER_CONTEXT(item))
    *str_out = "ZMAP_CONTAINER_CONTEXT" ;
  else if (ZMAP_IS_CONTAINER_ALIGNMENT(item))
    *str_out = "ZMAP_CONTAINER_ALIGNMENT" ;
  else if (ZMAP_IS_CONTAINER_BLOCK(item))
    *str_out = "ZMAP_CONTAINER_BLOCK" ;
  else if (ZMAP_IS_CONTAINER_STRAND(item))
    *str_out = "ZMAP_CONTAINER_STRAND" ;
  else if (ZMAP_IS_CONTAINER_FEATURESET(item))
    *str_out = "ZMAP_CONTAINER_FEATURESET" ;
  else
    has_type = FALSE ;

  return has_type ;
}


static gboolean get_container_child_type_as_string(FooCanvasItem *item, char **str_out)
{
  gboolean has_type = TRUE ;

  if (ZMAP_IS_CONTAINER_OVERLAY(item))
    *str_out = "ZMAP_CONTAINER_OVERLAY" ;
  else if (ZMAP_IS_CONTAINER_UNDERLAY(item))
    *str_out = "ZMAP_CONTAINER_UNDERLAY" ;
  else if (ZMAP_IS_CONTAINER_BACKGROUND(item))
    *str_out = "ZMAP_CONTAINER_BACKGROUND" ;
  else
    has_type = FALSE ;

  return has_type ;
}


static gboolean get_item_type_as_string(FooCanvasItem *item, char **str_out)
{
  gboolean has_type = TRUE ;

  if (ZMAP_IS_WINDOW_ALIGNMENT_FEATURE(item))
    *str_out = "ZMAP_WINDOW_ALIGNMENT_FEATURE" ;
  if (ZMAP_IS_WINDOW_ASSEMBLY_FEATURE(item))
    *str_out = "ZMAP_WINDOW_ASSEMBLY_FEATURE" ;
  if (ZMAP_IS_WINDOW_BASIC_FEATURE(item))
    *str_out = "ZMAP_WINDOW_BASIC_FEATURE" ;
  if (ZMAP_IS_WINDOW_GLYPH_ITEM(item))
    *str_out = "ZMAP_WINDOW_GLYPH_ITEM" ;
  if (ZMAP_IS_WINDOW_SEQUENCE_FEATURE(item))
    *str_out = "ZMAP_WINDOW_SEQUENCE_FEATURE" ;
  if (ZMAP_IS_WINDOW_TEXT_FEATURE(item))
    *str_out = "ZMAP_WINDOW_TEXT_FEATURE" ;
  if (ZMAP_IS_WINDOW_TEXT_ITEM(item))
    *str_out = "ZMAP_WINDOW_TEXT_ITEM" ;
  if (ZMAP_IS_WINDOW_TRANSCRIPT_FEATURE(item))
    *str_out = "ZMAP_WINDOW_TRANSCRIPT_FEATURE" ;
  else
    has_type = FALSE ;

  return has_type ;
}


static gboolean get_feature_type_as_string(FooCanvasItem *item, char **str_out)
{
  ZMapFeatureAny feature_any ;
  gboolean has_type = FALSE ;

  if ((feature_any = zmapWindowItemGetFeatureAny(item)))
    {
      has_type = TRUE ;

      switch(feature_any->struct_type)
        {
        case ZMAPFEATURE_STRUCT_CONTEXT:
          *str_out = "ZMapFeatureContext";
          break ;
        case ZMAPFEATURE_STRUCT_ALIGN:
          *str_out = "ZMapFeatureAlign" ;
          break ;
        case ZMAPFEATURE_STRUCT_BLOCK:
          *str_out = "ZMapFeatureBlock" ;
          break ;
        case ZMAPFEATURE_STRUCT_FEATURESET:
          *str_out = "ZMapFeatureSet" ;
          break ;
        case ZMAPFEATURE_STRUCT_FEATURE:
          *str_out = "ZMapFeature" ;
          break ;

        default:
          has_type = FALSE ;
          break ;
        }
    }

  return has_type ;
}



