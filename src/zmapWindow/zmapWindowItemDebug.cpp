/*  File: zmapWindowItemDebug.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description:
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapUtils.hpp>
#include <zmapWindowContainers.hpp>
#include <zmapWindow_P.hpp>


#define STRING_SIZE 50



static void printCanvasItems(FooCanvasItem *item, int indent, GString *buf) ;
static void printCanvasItem(FooCanvasItem *item, int ident, GString *buf) ;
static gboolean get_container_type_as_string(FooCanvasItem *item, const char **str_out) ;
static gboolean get_item_type_as_string(FooCanvasItem *item, const char **str_out) ;
static gboolean get_feature_type_as_string(FooCanvasItem *item, const char **str_out) ;
static GString *getItemCoords(GString *str, FooCanvasItem *item, gboolean local_only) ;


/* We just dip into the foocanvas struct for some data, we use the interface calls where they
 * exist. */
void zmapWindowPrintCanvas(FooCanvas *canvas)
{
  double x1, y1, x2, y2 ;


  foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);

  zMapDebugPrintf("%s", "\n\nCanvas stats:\n") ;

  zMapDebugPrintf("\nZoom x,y: %f, %f\n", canvas->pixels_per_unit_x, canvas->pixels_per_unit_y) ;

  zMapDebugPrintf("\nScroll region bounds: %.f -> %.f,  %.f -> %.f\n", x1, x2, y1, y2) ;

  zMapDebugPrintf("%s", "\nCanvas contents:\n") ;

  zmapWindowPrintGroups(canvas) ;

  return ;
}


void zmapWindowPrintGroups(FooCanvas *canvas)
{
  FooCanvasGroup *root ;
  int indent ;
  GString *buf ;

  zMapDebugPrintf("%s", "\nGroups:\n") ;

  root = foo_canvas_root(canvas) ;

  indent = 0 ;
  buf = g_string_sized_new(1000) ;

  printCanvasItems((FooCanvasItem *)root, indent, buf) ;

  g_string_free(buf, TRUE) ;

  return ;
}


void zmapWindowItemDebugItemToString(GString *string_arg, FooCanvasItem *item)
{
  gboolean has_feature = FALSE, is_container = FALSE ;
  const char *str = NULL ;

  if ((is_container = get_container_type_as_string(item, &str)))
    g_string_append_printf(string_arg, "%s", str) ;
  else if (get_item_type_as_string(item, &str))
    g_string_append_printf(string_arg, "%s", str) ;

  if ((has_feature = get_feature_type_as_string(item, &str)))
    g_string_append_printf(string_arg, " %s", str) ;

  if (has_feature)
    {
      ZMapFeatureAny feature_any;

      feature_any = zmapWindowItemGetFeatureAny(item);
      g_string_append_printf(string_arg, " \"%s\"", (char *)g_quark_to_string(feature_any->unique_id));
    }

  if (is_container)
    {
      FooCanvasItem *container;

      container = FOO_CANVAS_ITEM( zmapWindowContainerCanvasItemGetContainer(item) );
      if (container != item)
        {
          g_string_append_printf(string_arg, "Parent Details... ");
          zmapWindowItemDebugItemToString(string_arg, container);
        }
    }

  return ;
}



/* Prints out an items coords in local coords, good for debugging.... */
void zmapWindowPrintLocalCoords(char *msg_prefix, FooCanvasItem *item)
{
  GString *str ;

  str = g_string_sized_new(2048) ;

  str = getItemCoords(str, item, TRUE) ;

  zMapDebugPrintf("%s %s\n", msg_prefix, str->str) ;

  g_string_free(str, TRUE) ;

  return ;
}


char *zmapWindowItemCoordsText(FooCanvasItem *item)
{
  char *item_coords = NULL ;
  GString *str ;

  str = g_string_sized_new(2048) ;

  str = getItemCoords(str, item, FALSE) ;

  item_coords = g_string_free(str, FALSE) ;

  return item_coords ;
}



/* Prints out an items coords in world coords, good for debugging.... */
void zmapWindowPrintItemCoords(FooCanvasItem *item)
{
  GString *str ;

  str = g_string_sized_new(2048) ;

  str = getItemCoords(str, item, FALSE) ;

  zMapDebugPrintf("%s\n", str->str) ;

  g_string_free(str, TRUE) ;

  return ;
}


#if NOT_USED

/* Converts given world coords to an items coord system and prints them. */
void zmapWindowPrintW2I(FooCanvasItem *item, char *text, double x1_in, double y1_in)
{
  double x1 = x1_in, y1 = y1_in;

  my_foo_canvas_item_w2i(item, &x1, &y1) ;

  if (!text)
    text = "Item" ;

  zMapDebugPrintf("%s -  world(%.f, %.f)  ->  item(%.f, %.f)\n", text, x1_in, y1_in, x1, y1) ;

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

  zMapDebugPrintf("%s -  item(%.f, %.f)  ->  world(%.f, %.f)\n", text, x1_in, y1_in, x1, y1) ;


  return ;
}

#endif


/*
 *                  Internal functions
 */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Recursive function to print out all the child groups of the supplied group. */
static void printGroup(FooCanvasGroup *group, int indent, GString *buf)
{
  int i ;
  GList *glist ;
  GList *item_list ;


  buf = g_string_set_size(buf, 0) ;

  for (i = 0 ; i < indent ; i++)
    zMapDebugPrintfPlain("%s", "\t") ;


  /* Print this group. */
  if (ZMAP_IS_CONTAINER_GROUP(group) || ZMAP_IS_CANVAS_ITEM(group))
    {
      zmapWindowItemDebugItemToString(buf, (FooCanvasItem *)group) ;
      zMapDebugPrintfPlain("%s", buf->str) ;
    }
  else
    {
      if (FOO_IS_CANVAS_GROUP(group))
        {
          zMapDebugPrintfPlain("%s ", "FOOCANVAS_GROUP") ;
        }
      else if (FOO_IS_CANVAS_ITEM(group))
        {
          zMapDebugPrintfPlain("%s ", "FOOCANVAS_ITEM") ;
        }
      else
        {
          zMapDebugPrintfPlain("%s ", "**UNKNOWN ITEM TYPE**") ;
        }
    }
  printItem(FOO_CANVAS_ITEM(group), indent) ;


  if ((item_list = group->item_list))
    {
      do
        {
          FooCanvasGroup *sub_group = (FooCanvasGroup *)(item_list->data) ;

          if (FOO_IS_CANVAS_GROUP(sub_group))
            {
              printGroup(sub_group, indent + 1, buf) ;
            }
          else
            {
              printItem(FOO_CANVAS_ITEM(sub_group), indent) ;
            }
        }
      while((item_list = item_list->next)) ;
    }

  buf = g_string_set_size(buf, 0) ;

  return ;
}


static void printItem(FooCanvasItem *item, int indent)
{
  double x1, y1, x2, y2 ;
  double wx1, wy1, wx2, wy2 ;
  double cx1, cy1, cx2, cy2 ;
  int i ;

  // Get bounds of an item in its _parents_ coordinates (see next step)
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  // We have the items coords in parent space so find out where those _parent_ coords are in world
  // space.
  wx1 = x1 ; wy1 = y1 ; wx2 = x2 ; wy2 = y2 ;
  foo_canvas_item_i2w(item->parent, &wx1, &wy1) ;
  foo_canvas_item_i2w(item->parent, &wx2, &wy2) ;

  // Now find out where those world coords are in canvas coords
  cx1 = wx1 ; cy1 = wy1 ; cx2 = wx2 ; cy2 = wy2 ;
  foo_canvas_w2c_rect_d(item->canvas, &cx1, &cy1, &cx2, &cy2) ;


  if (FOO_IS_CANVAS_GROUP(item))
    {
      FooCanvasGroup *group = (FooCanvasGroup *)item ;

      zMapDebugPrintf("FOO_CANVAS_GROUP Parent->Group: %p -> %p    Pos: %.f, %.f"
                      "    Bounds: %.f -> %.f,  %.f -> %.f"
                      "    World bounds: %.f -> %.f,  %.f -> %.f"
                      "    Canvas bounds: %.f -> %.f,  %.f -> %.f"
                      "\n",
                      group->item.parent, group, group->xpos, group->ypos,
                      x1, x2, y1, y2,
                      wx1, wx2, wy1, wy2,
                      cx1, cx2, cy1, cy2) ;
    }
  else
    {
      zMapDebugPrintf("FOO_CANVAS_ITEM Parent->Item: %p -> %p    Bounds: %.f -> %.f,  %.f -> %.f    World bounds: %.f -> %.f,  %.f -> %.f\n",
                      item->parent, item, x1, x2, y1, y2, wx1, wx2, wy1, wy2) ;
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Recursive function to print out all the child groups of the supplied group. */
static void printCanvasItems(FooCanvasItem *item, int indent, GString *buf)
{
  int i ;

  buf = g_string_set_size(buf, 0) ;

  for (i = 0 ; i < indent ; i++)
    zMapDebugPrintfPlain("%s", "\t") ;

  /* Print this item. */
  printCanvasItem(item, indent, buf) ;

  if (FOO_IS_CANVAS_GROUP(item))
    {
      FooCanvasGroup *group = FOO_CANVAS_GROUP(item) ;

      if ((group->item_list))
        {
          GList *item_list = group->item_list ;

          do
            {
              FooCanvasItem *sub_item = (FooCanvasItem *)(item_list->data) ;

              printCanvasItems(sub_item, indent + 1, buf) ;
            }
          while((item_list = item_list->next)) ;
        }
    }

  return ;
}


static void printCanvasItem(FooCanvasItem *item, int indent, GString *buf)
{
  double x1, y1, x2, y2 ;
  double wx1, wy1, wx2, wy2 ;
  double cx1, cy1, cx2, cy2 ;

  buf = g_string_set_size(buf, 0) ;

  if (ZMAP_IS_CONTAINER_GROUP(item) || ZMAP_IS_CANVAS_ITEM(item))
    {
      zmapWindowItemDebugItemToString(buf, item) ;
      zMapDebugPrintfPlain("%s", buf->str) ;
    }
  else
    {
      if (FOO_IS_CANVAS_GROUP(item))
        {
          zMapDebugPrintfPlain("%s ", "FOOCANVAS_GROUP") ;
        }
      else if (FOO_IS_CANVAS_ITEM(item))
        {
          zMapDebugPrintfPlain("%s ", "FOOCANVAS_ITEM") ;
        }
      else
        {
          zMapDebugPrintfPlain("%s ", "**UNKNOWN ITEM TYPE**") ;
        }
    }

  buf = g_string_set_size(buf, 0) ;


  // Get bounds of an item in its _parents_ coordinates (see next step)
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  // We have the items coords in parent space so find out where those _parent_ coords are in world
  // space.
  wx1 = x1 ; wy1 = y1 ; wx2 = x2 ; wy2 = y2 ;
  foo_canvas_item_i2w(item->parent, &wx1, &wy1) ;
  foo_canvas_item_i2w(item->parent, &wx2, &wy2) ;

  // Now find out where those world coords are in canvas coords
  cx1 = wx1 ; cy1 = wy1 ; cx2 = wx2 ; cy2 = wy2 ;
  foo_canvas_w2c_rect_d(item->canvas, &cx1, &cy1, &cx2, &cy2) ;


  if (FOO_IS_CANVAS_GROUP(item))
    {
      FooCanvasGroup *group = (FooCanvasGroup *)item ;

      zMapDebugPrintfPlain("FOO_CANVAS_GROUP Parent->Group: %p -> %p    Pos: %.f, %.f"
                           "    Bounds: %.f -> %.f,  %.f -> %.f"
                           "    World bounds: %.f -> %.f,  %.f -> %.f"
                           "    Canvas bounds: %.f -> %.f,  %.f -> %.f"
                           "\n",
                           group->item.parent, group, group->xpos, group->ypos,
                           x1, x2, y1, y2,
                           wx1, wx2, wy1, wy2,
                           cx1, cx2, cy1, cy2) ;
    }
  else
    {
      zMapDebugPrintfPlain("FOO_CANVAS_ITEM Parent->Item: %p -> %p    Bounds: %.f -> %.f,  %.f -> %.f    World bounds: %.f -> %.f,  %.f -> %.f\n",
                           item->parent, item, x1, x2, y1, y2, wx1, wx2, wy1, wy2) ;
    }


  return ;
}












static gboolean get_container_type_as_string(FooCanvasItem *item, const char **str_out)
{
  gboolean has_type = TRUE ;

  if (ZMAP_IS_CONTAINER_CONTEXT(item))
    *str_out = "ZMAP_CONTAINER_CONTEXT" ;
  else if (ZMAP_IS_CONTAINER_ALIGNMENT(item))
    *str_out = "ZMAP_CONTAINER_ALIGNMENT" ;
  else if (ZMAP_IS_CONTAINER_BLOCK(item))
    *str_out = "ZMAP_CONTAINER_BLOCK" ;
  else if (ZMAP_IS_CONTAINER_FEATURESET(item))
    *str_out = "ZMAP_CONTAINER_FEATURESET" ;
  else
    has_type = FALSE ;

  return has_type ;
}




static gboolean get_item_type_as_string(FooCanvasItem *item, const char **str_out)
{
  gboolean has_type = TRUE ;

    has_type = FALSE ;

  return has_type ;
}


static gboolean get_feature_type_as_string(FooCanvasItem *item, const char **str_out)
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




static GString *getItemCoords(GString *str, FooCanvasItem *item, gboolean local_only)
{
  GString *result = str ;
  double x1, y1, x2, y2 ;

  /* Gets bounding box in parents coord system. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  g_string_append_printf(str, "item: %g, %g, %g, %g", x1, y1, x2, y2) ;

  if (!local_only)
    {
      foo_canvas_item_i2w(item, &x1, &y1) ;
      foo_canvas_item_i2w(item, &x2, &y2) ;

      g_string_append_printf(str, " -> World: %g, %g, %g, %g\n", x1, y1, x2, y2) ;
    }

  return result ;
}


