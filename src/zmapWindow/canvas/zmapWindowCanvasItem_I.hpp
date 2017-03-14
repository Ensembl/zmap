/*  File: zmapWindowCanvasItem_I.h
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
 * Description: Internals of the basic zmap canvas item class, all
 *              other item classes include this class.
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_CANVAS_ITEM_I_H
#define ZMAP_WINDOW_CANVAS_ITEM_I_H

#include <ZMap/zmapBase.hpp>
#include <zmapWindowAllBase.hpp>
#include <zmapWindowCanvasItem.hpp>
#include <zmapWindow_P.hpp>			/* Why do we need this, can we remove it ? */
//#include <ZMap/zmapWindow.h>			/* Why do we need this, can we remove it ? */
							/* it's for long items */





/* This class is basically a foocanvas group, and might well be one... */
/* If ZMAP_USE_WINDOW_CANVAS_ITEM is defined FooCanvasGroup will be used... */

typedef struct _zmapWindowCanvasItemClassStruct
{

  FooCanvasItemClass __parent__;			    /* extends FooCanvasItemClass  */


  GdkBitmap *fill_stipple;


  /* Object statistics */
  ZMapWindowItemStatsStruct stats ;

  /* methods */

  ZMapFeatureTypeStyle (* get_style)(ZMapWindowCanvasItem window_canvas_item) ;

  /* Returns item bounds in _world_ coords. */
  void (*get_bounds)(ZMapWindowCanvasItem window_canvas_item,
				  double top, double bottom,
				  double left, double right) ;

  void (* set_colour)(ZMapWindowCanvasItem   window_canvas_item,
		      FooCanvasItem         *interval,
		      ZMapFeature		     feature,
		      ZMapFeatureSubPart sub_feature,
		      ZMapStyleColourType    colour_type,
		      int 			     colour_flags,
		      GdkColor              *default_fill_gdk,
                  GdkColor              *border_gdk) ;

  gboolean (* set_feature) (FooCanvasItem *item, double x, double y);

  gboolean (* set_style) (FooCanvasItem *item, ZMapFeatureTypeStyle style);	/* used to bump_style a density column */

  gboolean (* showhide) (FooCanvasItem *item, gboolean show);

  /*   ????????????????? is this just a predeclared struct type problem ???? if so we can solve it... */
#ifdef CATCH_22
  ZMapWindowCanvasItem (*fetch_parent)(FooCanvasItem *any_child);
#endif /* CATCH_22 */


} zmapWindowCanvasItemClassStruct ;



/* Note that all feature items are now derived from this....and what is not good about this
 * is that every single one now has a FooCanvasGroup struct in it....agh....
 *
 *  */


typedef struct _zmapWindowCanvasItemStruct
{
  FooCanvasItem __parent__;				    /* extends FooCanvasItem  */

  ZMapFeature feature ;					    /* The Feature that this Canvas Item represents  */


  unsigned int interval_type ;

  /* Item flags. */
  unsigned int debug : 1 ;
  gboolean connected : 1 ;		/* to gtk signal handlers */

} zmapWindowCanvasItemStruct ;




#endif /* ZMAP_WINDOW_CANVAS_ITEM_I_H */
