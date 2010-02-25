/*  File: zmapWindowCanvasItem_I.h
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
 * Last edited: Feb 16 09:53 2010 (edgrif)
 * Created: Wed Dec  3 08:38:10 2008 (rds)
 * CVS info:   $Id: zmapWindowCanvasItem_I.h,v 1.7 2010-02-25 14:14:20 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_CANVAS_ITEM_I_H
#define ZMAP_WINDOW_CANVAS_ITEM_I_H

#include <ZMap/zmapBase.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindow_P.h>



typedef enum
  {
    WINDOW_ITEM_BACKGROUND,
    WINDOW_ITEM_UNDERLAY,
    WINDOW_ITEM_OVERLAY,
    WINDOW_ITEM_COUNT
  } ZMapWindowCanvasItemType ;

/* This class is basically a foocanvas group, and might well be one... */
/* If ZMAP_USE_WINDOW_CANVAS_ITEM is defined FooCanvasGroup will be used... */

typedef struct _zmapWindowCanvasItemClassStruct
{
  FooCanvasGroupClass __parent__; /*!< \extends FooCanvasGroupClass  */

  /* long items is really class level. */
  ZMapWindowLongItems long_items;

  GdkBitmap *fill_stipple;


  /* methods */

  /* We want to use foo_canvas_item_new and have default items created.  These
   * might not be the same for all our items... */
  void (* post_create)(ZMapWindowCanvasItem window_canvas_item) ;

  ZMapFeatureTypeStyle (* get_style)(ZMapWindowCanvasItem window_canvas_item) ;

  /* Returns item bounds in _world_ coords. */
  void (*get_bounds)(ZMapWindowCanvasItem window_canvas_item,
				  double top, double bottom,
				  double left, double right) ;

  /* This might be a no-op for some... */
  FooCanvasItem *(* add_interval)(ZMapWindowCanvasItem window_canvas_item,
				  ZMapFeatureSubPartSpan sub_feature, /* can be NULL */
				  double top, double bottom,
				  double left, double right) ;

  void (* set_colour)(ZMapWindowCanvasItem   window_canvas_item,
		      FooCanvasItem         *interval,
		      ZMapFeatureSubPartSpan sub_feature,
		      ZMapStyleColourType    colour_type,
		      GdkColor              *default_fill_gdk) ;

  /*   ????????????????? is this just a predeclared struct type problem ???? if so we can solve it... */
#ifdef CATCH_22
  ZMapWindowCanvasItem (*fetch_parent)(FooCanvasItem *any_child);
#endif /* CATCH_22 */

  /* Ability to check all subitems... */
  gboolean (* check_data)(ZMapWindowCanvasItem window_canvas_item, GError **error) ;

  /* clear items... */
  void (* clear)(ZMapWindowCanvasItem window_canvas_item) ;

} zmapWindowCanvasItemClassStruct ;




typedef struct _zmapWindowCanvasItemStruct
{
  FooCanvasGroup __parent__;	/*!< \extends FooCanvasGroup  */

  ZMapFeature feature;		/*!< \property The Feature that this Canvas Item represents  */


  /* These items (underlay etc.) are separate from the group and need to be mapped,
   * realized and drawn by us. */
  FooCanvasItem *items[WINDOW_ITEM_COUNT];

  FooCanvasItem *mark_item;


  unsigned int interval_type;

  unsigned int auto_resize_background : 1;

  unsigned int debug : 1;

} zmapWindowCanvasItemStruct;


#endif /* ZMAP_WINDOW_CANVAS_ITEM_I_H */
