/*  File: zmapWindowCanvasFeatureset_I.h
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

#ifndef ZMAP_WINDOW_FEATURESET_ITEM_I_H
#define ZMAP_WINDOW_FEATURESET_ITEM_I_H

#include <glib.h>
#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <zmapWindowCanvasItem_I.h>
#include <zmapWindowCanvasFeatureset.h>
#include <ZMap/zmapStyle.h>
#include <ZMap/zmapSkipList.h>




/*
 * minimal data struct to define a feature
 * handle boxes as y1,y2 + width
 */

typedef struct _zmapWindowCanvasFeatureStruct
{
	zmapWindowCanvasFeatureType type;
      ZMapFeature feature;

      double y1, y2;    	/* top, bottom of item (box or line) */
	double score;		/* determines feature width, gets re-calc'd on zoom */

	double width;
	double bump_offset;	/* for X coord */

	long flags;				/* non standard display option eg selected */
#define FEATURE_FOCUS_MASK	0xff		/* any focus flag will map to selected, this should really be defined by focus code but we are out of scope */
#define FEATURE_VISIBLE		0x100		/* not always true ! */
#define FEATURE_USER_HIDE	0x200		/* hidden by user request */
#define FEATURE_SUMMARISED	0x200		/* hidden by summarise */

#define FEATURE_FOCUS_ID	0xffff0000

} zmapWindowCanvasFeatureStruct;


ZMapWindowCanvasFeature zmapWindowCanvasFeatureAlloc(void);
void zmapWindowCanvasFeatureFree(gpointer thing);	/* is used as a callback, needs to be generic */

typedef struct _zmapWindowFeaturesetItemClassStruct
{
  zmapWindowCanvasItemClass __parent__;

  GHashTable *featureset_items;         /* singleton canvas items per column, indexed by unique id */
  GList *feature_free_list;
#define N_FEAT_ALLOC      1000
	/* these are allocated for all columns, so it does not matter if we have a column with 1 feature */

} zmapWindowFeaturesetItemClassStruct;



typedef struct _zmapWindowFeaturesetItemStruct
{
  FooCanvasItem __parent__;

  GQuark id;
  ZMapFeatureTypeStyle style;			/* column style: not valid if several featuresets mapped into this */
  ZMapFeatureTypeStyle featurestyle; 	/* current caches style for features */

  ZMapStrand strand;
  ZMapFrame frame;

  gboolean overlap;		/* default is to assume features do, some style imply that they do not (eg coverage/ heatmap) */

  double zoom;			/* current units per pixel */

  double start,end;
  double longest;			/* feature y-coords extent of biggest feature */

  GList *features;		/* we add features to a simple list and create the index on demand when we get an expose */
  int n_features;
  ZMapSkipList display_index;

  gboolean bumped;		/* using bumped X or not */
  ZMapStyleBumpMode bump_mode;	/* if set */

      /* graphics context for all contained features
       * each one has its own colours that we set on draw (eg for heatmaps)
       * foo_canvas_rect also has stipples but we don't use then ATM
       * it also pretends to do alpha but 'X doesn't do it'
       * crib foo-canvas-rect-elipse.c for inspiration
       */
  GdkGC *gc;             	  /* GC for graphics output */

  double dx,dy;			  /* canvas offsets as calculated for paint */
  gpointer deferred;		  /* buffer for deferred paints, eg constructed polyline */

  gulong fill_colour;            /* Fill color, RGBA */
  gulong outline_colour;         /* Outline color, RGBA */
  gulong fill_pixel;            /* Fill color */
  gulong outline_pixel;         /* Outline color */

  double width;                 /* Outline width */
  double bump_width;

      /* Configuration flags */
  gboolean fill_set;    	/* Is fill color set? */
  gboolean outline_set;	 	/* Is outline color set? */

  gboolean width_pixels;      /* Is outline width specified in pixels or units? */

  ZMapFeature point_feature;	/* set by cursor movement */

} zmapWindowFeaturesetItemStruct;




#endif /* ZMAP_WINDOW_FEATURESET_ITEM_I_H */
