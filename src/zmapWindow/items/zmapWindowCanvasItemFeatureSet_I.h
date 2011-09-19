/*  File: zmapWindowCanvasFeaturesetItem_I.h
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_CANVAS_FEATURESET_ITEM_I_H
#define ZMAP_WINDOW_CANVAS_FEATURESET_ITEM_I_H

#include <glib.h>
#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <zmapWindowCanvasItem_I.h>
#include <zmapWindowCanvasItemFeatureSet.h>
#include <ZMap/zmapStyle.h>





typedef struct _zmapWindowCanvasFeaturesetItemClassStruct
{
  zmapWindowCanvasItemClass __parent__;

	/* to save parent's original function */
  void (* canvas_item_set_colour)(ZMapWindowCanvasItem   window_canvas_item,
		      FooCanvasItem         *interval,
		      ZMapFeature		     feature,
		      ZMapFeatureSubPartSpan sub_feature,
		      ZMapStyleColourType    colour_type,
		      int 			     colour_flags,
		      GdkColor              *default_fill_gdk,
                  GdkColor              *border_gdk) ;


} zmapWindowCanvasFeaturesetItemClassStruct;


/*
 * this is a container for Featureset data masquerading as a ZMapWindowCanvasItem
 * and the prior canvas handling of item is subverted to move most of the compute into ZMap from the foo canvas
 */

typedef struct _zmapWindowCanvasFeaturesetItemStruct
{
  zmapWindowCanvasItem __parent__;


} zmapWindowCanvasFeaturesetItemStruct;



#endif /* ZMAP_WINDOW_Featureset_ITEM_I_H */
