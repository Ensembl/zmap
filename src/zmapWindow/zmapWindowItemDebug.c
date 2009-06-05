/*  File: zmapWindowItemDebug.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Last edited: Jun  3 23:26 2009 (rds)
 * Created: Tue Nov  6 16:33:44 2007 (rds)
 * CVS info:   $Id: zmapWindowItemDebug.c,v 1.2 2009-06-05 13:34:59 rds Exp $
 *-------------------------------------------------------------------
 */
#include <string.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <ZMap/zmapUtils.h>

#define STRING_SIZE 50
#ifdef NO_NEED
static gboolean get_container_type_as_string(FooCanvasItem *item, char *str_inout)
{
  ContainerType container_type = CONTAINER_INVALID;
  gboolean has_type = FALSE;

  if((container_type = GPOINTER_TO_INT( g_object_get_data(G_OBJECT(item), CONTAINER_TYPE_KEY) )))
    {
      char *col_text;
      int   len;
      switch(container_type)
        {
        case CONTAINER_ROOT:
          col_text = "ROOT";
          break;
        case CONTAINER_PARENT:
          col_text = "PARENT";
          break;
        case CONTAINER_OVERLAYS:
          col_text = "OVERLAYS";
          break;
        case CONTAINER_FEATURES:
          col_text = "FEATURES";
          break;
        case CONTAINER_BACKGROUND:
          col_text = "BACKGROUND";
          break;
        case CONTAINER_UNDERLAYS:
          col_text = "UNDERLAYS";
          break;
        default:
          col_text = "INVALID";
          break;
        }
      len = strlen(col_text);
      has_type = TRUE;
      memcpy(str_inout, col_text, len);
      str_inout[len] = '\0';
    }

  return has_type;
}

static gboolean get_item_feature_type_as_string(FooCanvasItem *item, char *str_inout)
{
  ZMapWindowItemFeatureType feature_type = ITEM_FEATURE_INVALID;
  gboolean has_type = FALSE;

  if((feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE))))
    {
      char *feature_text;
      int   len;
      switch(feature_type)
        {
        case ITEM_FEATURE_SIMPLE:
          feature_text = "SIMPLE";
          break;
        case ITEM_FEATURE_PARENT:
          feature_text = "PARENT";
          break;
        case ITEM_FEATURE_CHILD:
          feature_text = "CHILD";
          break;
        case ITEM_FEATURE_BOUNDING_BOX:
          feature_text = "BOUNDING_BOX";
          break;
        case ITEM_FEATURE_GROUP:
          feature_text = "GROUP";
          break;
        default:
          feature_text = "INVALID";
          break;
        }
      len = strlen(feature_text);
      has_type = TRUE;
      memcpy(str_inout, feature_text, len);
      str_inout[len] = '\0';      
    }

  return has_type;
}

static gboolean get_feature_type_as_string(FooCanvasItem *item, char *str_inout)
{
  ZMapFeatureAny feature_any;
  gboolean has_type = FALSE;

  if((feature_any = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA)))
    {
      char *text;
      int   len;
      switch(feature_any->struct_type)
        {
        case ZMAPFEATURE_STRUCT_CONTEXT:
          text = "CONTEXT";
          break;
        case ZMAPFEATURE_STRUCT_ALIGN:
          text = "ALIGN";
          break;
        case ZMAPFEATURE_STRUCT_BLOCK:
          text = "BLOCK";
          break;
        case ZMAPFEATURE_STRUCT_FEATURESET:
          text = "FEATURESET";
          break;
        case ZMAPFEATURE_STRUCT_FEATURE:
          text = "FEATURE";
          break;
        case ZMAPFEATURE_STRUCT_INVALID:
        default:
          zMapAssertNotReached();
          break;
        }
      len = strlen(text);
      has_type = TRUE;
      memcpy(str_inout, text, len);
      str_inout[len] = '\0';
    }

  return has_type;
}
#endif /* NO_NEED */
void zmapWindowItemDebugItemToString(FooCanvasItem *item, GString *string)
{
  gboolean has_feature = FALSE, is_container = FALSE;

#ifdef NO_NEED
  char tmp_string[STRING_SIZE] = {0};

  if((has_feature = get_feature_type_as_string(item, &tmp_string[0])))
    g_string_append_printf(string, "Feature Type = '%s' ",   &tmp_string[0]);

  if(get_item_feature_type_as_string(item, &tmp_string[0]))
    g_string_append_printf(string, "Item Type = '%s' ",      &tmp_string[0]);

  if((is_container = get_container_type_as_string(item, &tmp_string[0])))
    g_string_append_printf(string, "Container Type = '%s' ", &tmp_string[0]);

  if (g_object_get_data(G_OBJECT(item), "my_range_key"))
    g_string_append_printf(string, "Item is a mark item ") ;
#endif /* NO_NEED */

  if(has_feature)
    {
      ZMapFeatureAny feature_any;
      feature_any = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);
      g_string_append_printf(string, "Feature UID = '%s' ", (char *)g_quark_to_string(feature_any->unique_id));
    }

  if(is_container)
    {
      FooCanvasItem *container;
      container = FOO_CANVAS_ITEM( zmapWindowContainerCanvasItemGetContainer(item) );
      if(container != item)
	{
	  g_string_append_printf(string, "Parent Details... ");
	  zmapWindowItemDebugItemToString(container, string);
	}
    }

  return ;
}
