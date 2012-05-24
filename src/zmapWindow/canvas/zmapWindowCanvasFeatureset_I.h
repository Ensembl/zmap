/*  File: zmapWindowCanvasFeatureset_I.h
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
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
//#include <zmapWindowCanvasItem_I.h>
#include <ZMap/zmapWindow.h>
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
  GList *from;		/* the list node that holds the feature */
  /* refer to comment above zmapWindowCanvasFeatureset.c/zMapWindowFeaturesetItemRemoveFeature() */

  double y1, y2;    	/* top, bottom of item (box or line) */
  double score;		/* determines feature width */

  /* ideally these could be ints but the canvas works with doubles */
  double width;
  double bump_offset;	/* for X coord  (left hand side of sub column */

  int bump_col;		/* for calculating sub-col before working out width */

  long flags;				/* non standard display option eg selected */
#define FEATURE_FOCUS_MASK	WINDOW_FOCUS_GROUP_FOCUSSED		/* any focus flag will map to selected */
#define FEATURE_FOCUS_BLURRED	WINDOW_FOCUS_GROUP_BLURRED		/* eg masked */
#define FEATURE_FOCUS_BITMAP	(WINDOW_FOCUS_GROUP_BITMASK | WINDOW_FOCUS_DONT_USE)		/* includes masking (EST) */
#define FEATURE_HIDDEN		0x0100		/* not always false, set for hidden rather than visible to make flag twiddling easier */
#define FEATURE_USER_HIDE	0x0200		/* hidden by user request */
#define FEATURE_MARK_HIDE	0x0400		/* hidden by bump from mark */
#define FEATURE_SUMMARISED	0x0800		/* hidden by summarise */
#define FEATURE_MASK_HIDE	0x1000		/* masked feature hidden by user */
#define FEATURE_HIDE_FILTER	0x2000		/* filtered by score */
#define FEATURE_HIDE_COMPOSITE	0x4000	/* squashed or collapsed */
#define FEATURE_HIDE_EXPAND	0x8000		/* compressed feature got bumped */
#define FEATURE_HIDE_REASON	0xfe00		/* NOTE: update this if you add a reason */

#define FEATURE_FOCUS_ID	WINDOW_FOCUS_ID

#if 0
#define FEATURE_SQUASHED_START	0x10000
#define FEATURE_SQUASHED_END		0x20000
#define FEATURE_SQUASHED		0x30000

  GArray *gaps;					/* alternate gaps array for alignments if squashed */
#endif

  ZMapWindowCanvasFeature left,right;	/* for exons and alignments, NULL for simple features */

} zmapWindowCanvasFeatureStruct;



/* used for featureset summarise to prevent display of invisible features */
typedef struct _pixRect
{
  struct _pixRect *next,*prev;	/* avoid alloc and free overhead of GList
				 * we only expect relatively few of these but a high turnover
				 */

  ZMapWindowCanvasFeature feature;
  int y1,x2,y2;			/* we only need x2 as features are aligned centrally or to the left */
  //	int start;				/* we need to remember the real start as we trim the rect from the front */

#define PIX_LIST_DEBUG	0
#if PIX_LIST_DEBUG
  gboolean alloc;			/* for double alloc/free debug */
  int which;
#endif

} pixRect, *PixRect;    		/* think of a name not used elsewhere */


#define N_PIXRECT_ALLOC		20


PixRect zmapWindowCanvasFeaturesetSummarise(PixRect pix, ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature);
void zmapWindowCanvasFeaturesetSummariseFree(ZMapWindowFeaturesetItem featureset, PixRect pix);


typedef struct _zmapWindowFeaturesetItemClassStruct
{
//  zmapWindowCanvasItemClass __parent__;
  FooCanvasItemClass __parent__;

  GHashTable *featureset_items;         /* singleton canvas items per column, indexed by unique id */
  /* NOTE duplicated in container canvas item till we get rid of that */

  GList *feature_free_list[FEATURE_N_TYPE];
#define N_FEAT_ALLOC      1000
	/* these are allocated for all columns, so it does not matter if we have a column with 1 feature */
	/* NOTE we have free lists foe each featuretype; this will waste only a few K of memory */

  int struct_size[FEATURE_N_TYPE];
  int set_struct_size[FEATURE_N_TYPE];

} zmapWindowFeaturesetItemClassStruct;



/* NOTE this class/ structure is used for all types of columns
 * it is not inherited and various optional functions have been squeezed in
 * eg:
 * -- graph density feature get re-binned before display (and we have two lists of feature database)
 * -- gapped alignments are recalulated for display when zoom changes (extra data per feature, one list)
 */

typedef struct _zmapWindowFeaturesetItemStruct
{
  FooCanvasItem __parent__;

  FooCanvasItem *canvas_item;		/* containing ZMapWindowCanvasItem: these are a pain they do nothing and we can't get rid of them yet */

  GQuark id;
  ZMapFeatureTypeStyle style;				    /* column style: NB could have several
							       featuresets mapped into this by virtualisation */
  ZMapFeatureTypeStyle featurestyle;			    /* current cached style for features */

  zmapWindowCanvasFeatureType type;

  ZMapStrand strand;
  ZMapFrame frame;


  /* Points to data that is needed on a per column basis and is assigned by the functions
   * that handle that data. */
  gpointer per_column_data ;



  double zoom;			/* current units per pixel */
  double bases_per_pixel;

  /* stuff for column summarise */
  PixRect *col_cover;    	/* temp. data structs */
#if SUMMARISE_STATS
  gint n_col_cover_show;      /* stats for my own curiosity/ to justify the code */
  gint n_col_cover_hide;
  gint n_col_cover_list;
#endif

  double start,end;
  double longest;			/* feature y-coords extent of biggest feature */
  gboolean overlap;		/* default is to assume features do, some styles imply that they do not (eg coverage/ heatmap) */
  double bump_overlap;		/* calculated according length of compound features */

  gboolean link_sideways;	/* has complex features */
  gboolean linked_sideways;	/* that have been constructed */

  gboolean highlight_sideways;/* temp bodge to allow old code to drive this */
					/* transcripts do alignments don't they get highlit by calling code */

  GList *features;		/* we add features to a simple list and create the index on demand when we get an expose */
					/* NOTE elsewhere we don't use GList as we get a 30% performance improvement
					 * but we need to sort features so GList is more convenient */

  long n_features;
  gboolean features_sorted;	/* by start coord */

  gboolean re_bin;		/* re-calculate bins/ features according to zoom */
  GList *display;			/* features for display */
  /* NOTE normally features are indexed into display_index
   * coverage data gets re-binned and new features stored in display which is then indexed
   * if we add new features then we re-create the index - new features are added to features
   * if display is not NULL then we have to free both lists on destroy
   */
  ZMapSkipList display_index;

  gboolean bumped;		/* using bumped X or not */
  ZMapStyleBumpMode bump_mode;	/* if set */

  int set_index;			/* for staggered columns (heatmaps) */
  double x_off;

  /* graphics context for all contained features
   * each one has its own colours that we set on draw (eg for heatmaps)
   * foo_canvas_rect also has stipples but we don't use then ATM
   * it also pretends to do alpha but 'X doesn't do it'
   * crib foo-canvas-rect-elipse.c for inspiration
   */
  GdkGC *gc;             	  /* GC for graphics output */

  gint clip_y1,clip_y2,clip_x1,clip_x2;		/* visble scroll region plus one pixel all round */

  double dx,dy;			  /* canvas offsets as calculated for paint */
  gpointer deferred;		  /* buffer for deferred paints, eg constructed polyline */

  gulong fill_colour;           /* Fill color, RGBA */
  gulong outline_colour;        /* Outline color, RGBA */
  gulong fill_pixel;            /* Fill color */
  gulong outline_pixel;         /* Outline color */

  double width;                 /* column width */
  double bump_width;

  gboolean fill_set;    	/* Is fill color set? */
  gboolean outline_set;	 	/* Is outline color set? */

  ZMapFeature point_feature;	/* set by cursor movement */
  ZMapWindowCanvasFeature point_canvas_feature;		/* last clicked canvasfeature, set by select, need for legacy code interface */

  double filter_value;		/* active level, default 0.0 */
  int n_filtered;
  gboolean enable_filter;	/* has score in a feature and style allows it */

  gpointer opt;			/* feature type optional set level data */

} zmapWindowFeaturesetItemStruct;



void zmapWindowFeaturesetS2Ccoords(double *start_inout, double *end_inout) ;


/*
 * module generic text interface, initially used by sequence and locus feature types
 * is used to paint single line text things at given x,y
 * highlighting is done with coloured boxes behind the text
 */

typedef struct _zmapWindowCanvasPangoStruct
{
	PangoRenderer *renderer;	/* we use one per column to draw each line seperatly */
	PangoContext *context;
	PangoLayout *layout;

	int text_height, text_width;
}
zmapWindowCanvasPangoStruct;


#endif /* ZMAP_WINDOW_FEATURESET_ITEM_I_H */
