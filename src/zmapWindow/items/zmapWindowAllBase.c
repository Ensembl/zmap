/*  File: zmapWindowAllBase.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2010: Genome Research Ltd.
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
 * Description: Contains functions required by all zmap item and
 *              group types. These are mostly utility functions
 *              like stats recording and so on.
 *
 * Exported functions: See zmapWindowAllBase.h
 * HISTORY:
 * Last edited: May 26 13:30 2010 (edgrif)
 * Created: Wed May 26 09:43:09 2010 (edgrif)
 * CVS info:   $Id: zmapWindowAllBase.c,v 1.1 2010-05-26 12:46:19 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <libfoocanvas/libfoocanvas.h>			    /* Used to get struct sizes/types for stats. */
#include <zmapWindowFeatures_I.h>
#include <zmapWindowContainers_I.h>

#include <zmapWindowAllBase.h>


/* Surprisingly I thought this would be typedef somewhere in the GType header but not so... */
typedef GType (*MyGTypeFunc)(void) ;

typedef struct MyGObjectInfoStructName
{
  MyGTypeFunc get_type_func ;
  GType obj_type ;
  char *obj_name ;
  unsigned int obj_size ;
} MyGObjectInfoStruct, *MyGObjectInfo ;



static MyGObjectInfo initObjectDescriptions(void) ;
static MyGObjectInfo findObjectInfo(GType type) ;


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


void zmapWindowItemStatsDecr(ZMapWindowItemStats stats)
{
  stats->curr-- ;

  return ;
}



/* We can maybe make this internal completely..... */
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



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (foo_type == FOO_TYPE_CANVAS_ITEM)
    {
      *name_out = "FOO_CANVAS_ITEM" ;
      *size_out = sizeof(FooCanvasItem) ;
    }
  if (foo_type == FOO_TYPE_CANVAS_GROUP)
    {
      *name_out = "FOO_CANVAS_GROUP" ;
      *size_out = sizeof(FooCanvasGroup) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_LINE)
    {
      *name_out = "FOO_CANVAS_LINE" ;
      *size_out = sizeof(FooCanvasLine) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_PIXBUF)
    {
      *name_out = "FOO_CANVAS_PIXBUF" ;
      *size_out = sizeof(FooCanvasPixbuf) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_POLYGON)
    {
      *name_out = "FOO_CANVAS_POLYGON" ;
      *size_out = sizeof(FooCanvasPolygon) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_RE)
    {
      *name_out = "FOO_CANVAS_RE" ;
      *size_out = sizeof(FooCanvasRE) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_TEXT)
    {
      *name_out = "FOO_CANVAS_TEXT" ;
      *size_out = sizeof(FooCanvasText) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_WIDGET)
    {
      *name_out = "FOO_CANVAS_WIDGET" ;
      *size_out = sizeof(FooCanvasWidget) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_FLOAT_GROUP)
    {
      *name_out = "FOO_CANVAS_FLOAT_GROUP" ;
      *size_out = sizeof(FooCanvasFloatGroup) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_LINE_GLYPH)
    {
      *name_out = "FOO_CANVAS_LINE_GLYPH" ;
      *size_out = sizeof(FooCanvasLineGlyph) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_ZMAP_TEXT)
    {
      *name_out = "FOO_CANVAS_ZMAP_TEXT" ;
      *size_out = sizeof(FooCanvasZMapText) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_LINE)
    {
      *name_out = "FOO_CANVAS_LINE" ;
      *size_out = sizeof(FooCanvasLine) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_LINE)
    {
      *name_out = "FOO_CANVAS_LINE" ;
      *size_out = sizeof(FooCanvasLine) ;
    }
  else if (foo_type == FOO_TYPE_CANVAS_LINE)
    {
      *name_out = "FOO_CANVAS_LINE" ;
      *size_out = sizeof(FooCanvasLine) ;
    }
  else
    {
      result = FALSE ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return object_data ;
}


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
      {zMapWindowAlignmentFeatureGetType, 0, NULL, sizeof(zmapWindowAlignmentFeature)},
      {zMapWindowAssemblyFeatureGetType, 0, NULL, sizeof(zmapWindowAssemblyFeature)},
      {zMapWindowBasicFeatureGetType, 0, NULL, sizeof(zmapWindowBasicFeature)},
      {zmapWindowContainerAlignmentGetType, 0, NULL, sizeof(zmapWindowContainerAlignment)},
      {zmapWindowContainerBlockGetType, 0, NULL, sizeof(zmapWindowContainerBlock)},
      {zmapWindowContainerFeaturesGetType, 0, NULL, sizeof(zmapWindowContainerFeatures)},
      {zmapWindowContainerContextGetType, 0, NULL, sizeof(zmapWindowContainerContext)},
      {zmapWindowContainerFeatureSetGetType, 0, NULL, sizeof(zmapWindowContainerFeatureSet)},
      {zmapWindowContainerStrandGetType, 0, NULL, sizeof(zmapWindowContainerStrand)},
      {zMapWindowGlyphItemGetType, 0, NULL, sizeof(zmapWindowGlyphItem)},
      {zMapWindowLongItemGetType, 0, NULL, sizeof(zmapWindowLongItem)},
      {zMapWindowSequenceFeatureGetType, 0, NULL, sizeof(zmapWindowSequenceFeature)},
      {zMapWindowTextFeatureGetType, 0, NULL, sizeof(zmapWindowTextFeature)},
      {zMapWindowTextItemGetType, 0, NULL, sizeof(zmapWindowTextItem)},
      {zMapWindowTranscriptFeatureGetType, 0, NULL, sizeof(zmapWindowTranscriptFeature)},

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



