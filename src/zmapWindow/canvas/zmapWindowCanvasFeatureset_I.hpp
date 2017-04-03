/*  File: zmapWindowCanvasFeatureset_I.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
 * Description: Private header for featuresets, a featureset
 *              is contained within a column.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_FEATURESET_ITEM_I_H
#define ZMAP_WINDOW_FEATURESET_ITEM_I_H

#include <glib.h>
#include <glib-object.h>

#include <libzmapfoocanvas/libfoocanvas.h>

#include <ZMap/zmapStyle.hpp>
#include <ZMap/zmapSkipList.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasFeatureset.hpp>

/* would love to get rid of canvas item... */
#include <zmapWindowCanvasItem_I.hpp>



/* Why is this public ???? */
/* enums for function type */
typedef enum
  {
    FUNC_SET_INIT,
    FUNC_PREPARE,
    FUNC_SET_PAINT,
    FUNC_PAINT,
    FUNC_FLUSH,
    FUNC_COLOUR,
    FUNC_STYLE,
    FUNC_PRE_ZOOM,
    FUNC_ZOOM,
    FUNC_INDEX,
    FUNC_FREE,
    FUNC_POINT,
    FUNC_ADD,
    FUNC_N_FUNC                                             /* Number of functions: must be last. */
  } zmapWindowCanvasFeatureItemFunc ;








/* Default feature colours, suitably boring to prompt user to set their own. */
#define CANVAS_DEFAULT_COLOUR_FILL   "grey"
#define CANVAS_DEFAULT_COLOUR_BORDER "black"
#define CANVAS_DEFAULT_COLOUR_DRAW   "white"



/* Enums ???? */
#define FEATURE_FOCUS_MASK	WINDOW_FOCUS_GROUP_FOCUSSED /* any focus flag will map to selected */
#define FEATURE_FOCUS_BLURRED	WINDOW_FOCUS_GROUP_BLURRED  /* eg masked */
#define FEATURE_FOCUS_BITMAP	(WINDOW_FOCUS_GROUP_BITMASK | WINDOW_FOCUS_DONT_USE) /* includes masking (EST) */

#define FEATURE_HIDDEN		0x0100                      /* not always false, set for hidden rather than
                                                               visible to make flag twiddling easier */
#define FEATURE_USER_HIDE	0x0200                      /* hidden by user request */
#define FEATURE_MARK_HIDE	0x0400                      /* hidden by bump from mark */
#define FEATURE_SUMMARISED	0x0800                      /* hidden by summarise */
#define FEATURE_MASK_HIDE	0x1000                      /* masked feature hidden by user */
#define FEATURE_HIDE_FILTER	0x2000                      /* filtered by score or something else eg locus prefix */
#define FEATURE_HIDE_COMPOSITE	0x4000                      /* squashed or collapsed */
#define FEATURE_HIDE_EXPAND	0x8000                      /* compressed feature got bumped */
#define FEATURE_HIDE_REASON	0xfe00                      /* NOTE: update this if you add a reason */

#define FEATURE_FOCUS_ID	WINDOW_FOCUS_ID

#define WCG_FILL_SET		1
#define WCG_OUTLINE_SET		2






/*
 * module generic text interface, initially used by sequence and locus feature types
 * is used to paint single line text things at given x,y
 * highlighting is done with coloured boxes behind the text
 */

typedef struct _zmapWindowCanvasPangoStruct
{
  GdkDrawable *drawable;		/* used to tell if a new window has been created and our pango is not valid */

  PangoRenderer *renderer;	/* we use one per column to draw each line seperatly */
  PangoContext *context;
  PangoLayout *layout;

  int text_height, text_width;
} zmapWindowCanvasPangoStruct ;


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




/* Oh goodness....what is all this for ?? */
#define N_FEAT_ALLOC      1000



typedef struct ZMapWindowFeaturesetItemClassStructType
{
  zmapWindowCanvasItemClass __parent__;

  GHashTable *featureset_items;         /* singleton canvas items per column, indexed by unique id */
  /* NOTE duplicated in container canvas item till we get rid of that */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  GList *feature_free_list[FEATURE_N_TYPE];
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* these are allocated for all columns, so it does not matter if we have a column with 1 feature */
  /* NOTE we have free lists foe each featuretype; this will waste only a few K of memory */
  size_t set_struct_size[FEATURE_N_TYPE];


  // THESE SHOULD ALL BE MOVED INTO THE FEATURESET STRUCT ITSELF SO THAT THEY CAN BE PER SCREEN.
  //
  /* Cached colourmaps/colours for drawing, provides default colours for all feature drawing. */
  GdkColormap *colour_map ;                                 /* This is a per-screen resource
                                                               so multi-screen operation would need multiple values. */

  gboolean colour_alloc ;                                   /* TRUE => colour allocation worked. */
  GdkColor fill_col ;                                           /* Fill/background colour. */
  GdkColor draw_col ;                                           /* Overlaid on fill colour. */
  GdkColor border_col ;                                         /* Surround/line colour. */

  /* Should we also have default select colours too ?? */



} ZMapWindowFeaturesetItemClassStruct ;



/* THIS IS A CHILD OF THE COLUMN GROUP CREATED ELSEWHERE....IN ESSENCE IT HOLDS THE
 * FEATURES THOUGH THESE ARE NO LONGER FOOCANVAS OBJECTS. THOUGH I DON'T THINK THERE
 * IS ONE OF THESE PER FEATURESET WITHIN A COLUMN....CHECK THIS THOUGH !
 *
 * NOTE this class/ structure is used for all types of columns
 * it is not inherited and various optional functions have been squeezed in
 * eg:
 * -- graph density feature get re-binned before display (and we have two lists of feature database)
 * -- gapped alignments are recalulated for display when zoom changes (extra data per feature, one list)
 */
typedef struct ZMapWindowFeaturesetItemStructType
{
  zmapWindowCanvasItemStruct __parent__ ;		/* itself derived from FooCanvasItem */

  /* id is a composite of column unique id and foo->canvas, group, name, layer. */
  GQuark id ;

  zmapWindowCanvasFeatureType type ;

  ZMapFeatureTypeStyle style ;				    /* Column style: NB could have several
							       featuresets mapped into this by virtualisation */
  ZMapFeatureTypeStyle featurestyle ;			    /* Current cached style for features */

  ZMapStrand strand ;
  ZMapFrame frame ;

  ZMapFeature point_feature ;                               /* set by cursor movement */
  ZMapWindowCanvasFeature point_canvas_feature ;            /* last clicked canvasfeature, set by select,
                                                               need for legacy code interface */

  ZMapWindowCanvasFeature last_added ;	/* necessary optimisation */
  /* NOTE
   * when we add a feature we wipe the display index (due to not having implemented the skip list fully)
   * which means that we can't look up the last added feature
   * which is what we need to do to set show/hide or colour
   * so we keep this pointer to find the last one added
   */

  /* Points to data that is needed on a per column basis and is assigned by the functions
   * that handle that data. */
  gpointer per_column_data ;


  /* SHOULD THE NEXT TWO FIELDS REALLY BE IN THE CONTAINERFEATURESET STRUCT ? */
  /* Glyph data, this is held separately because as well as glyph columns there are columns
   * e.g. alignments, that also need glyph data. */
  gpointer glyph_per_column_data ;

  gpointer opt ;                                            /* feature type optional set level data */


  /* Some stuff for handling zooming, not straight forward because we have to deal
   * with our canvas window being much smaller than our sequence making drawing/scrolling
   * difficult. */

  gboolean recalculate_zoom ;    /* gets set to true if the zoom has changed and we need to recalculate summary data */
  double zoom ;			/* current units per pixel */
  double bases_per_pixel ;

  GtkAdjustment *v_adjuster ;                               /* needed to calculate exposes. */

  /* stuff for column summarise */
  PixRect *col_cover ;    	/* temp. data structs */
#if SUMMARISE_STATS
  gint n_col_cover_show ;      /* stats for my own curiosity/ to justify the code */
  gint n_col_cover_hide ;
  gint n_col_cover_list ;
#endif


  double start,end ;
  double longest ;			/* feature y-coords extent of biggest feature */

  /* I'm not sure what this is ? */
  double x_off ;

  double width ;                 /* column width */
  double bump_width ;

  gboolean overlap ;		/* default is to assume features do, some styles imply that they do not (eg coverage/ heatmap) */
  double bump_overlap ;		/* calculated according length of compound features */

  gboolean bumped ;		/* using bumped X or not */
  ZMapStyleBumpMode bump_mode ;	/* if set */

  gint layer ;						    /* underlay features or overlay (flags) */

  gboolean draw_truncation_glyphs ;

  gboolean link_sideways ;	/* has complex features */
  gboolean linked_sideways ;	/* that have been constructed */

  /* OH GOSH....THIS DOESN'T LOOK GOOD..... */
  gboolean highlight_sideways ;/* temp bodge to allow old code to drive this */
  /* transcripts do alignments don't they get highlit by calling code */

  /* Some columns can be bumped to have sub-cols, this is a list of ZMapWindowCanvasSubCol
   * giving offsets, names etc for subcols. */
  GList *sub_col_list ;

  /* we add features to a simple list and create the index on demand when we get an expose */
  GList *features ;
  /* NOTE elsewhere we don't use GList as we get a 30% performance improvement
   * but we need to sort features so GList is more convenient */

  long n_features ;
  gboolean features_sorted ;				    /* by start coord */

  gboolean re_bin ;					    /* re-calculate bins/ features according to zoom */
  GList *display ;					    /* features for display */

  /* NOTE normally features are indexed into display_index
   * coverage data gets re-binned and new features stored in display which is then indexed
   * if we add new features then we re-create the index - new features are added to features
   * if display is not NULL then we have to free both lists on destroy
   */
  ZMapSkipList display_index ;

  // Used to cursor through canvasfeatures in the skiplist, reset to NULL when the skiplist is deleted.
  ZMapSkipList curr_item ;


  int set_index ;			/* for staggered columns (heatmaps) */

  /* graphics context for all contained features
   * each one has its own colours that we set on draw (eg for heatmaps)
   * foo_canvas_rect also has stipples but we don't use then ATM
   * it also pretends to do alpha but 'X doesn't do it'
   * crib foo-canvas-rect-elipse.c for inspiration
   */
  GdkGC *gc ;             	  /* GC for graphics output */

  gint clip_y1,clip_y2,clip_x1,clip_x2 ;		/* visble scroll region plus one pixel all round */

  double x ;				  /* x canvas coordinate of the featureset, used for column reposition */

  double dx, dy ;			  /* canvas offsets as calculated for paint */

  gpointer deferred ;		  /* buffer for deferred paints, eg constructed polyline */

  // Colour stuff....
  //
  GdkColormap *colour_map ;     

  // colinear colours, this is a per-colormap and therefore per-screen resource.
  ZMapCanvasDrawColinearColours colinear_colours ;

  /* Feature colours....Gosh...all seems a mish-mash....... */

  /* Bitfield ???? */
  gboolean fill_set ;                                       /* Is fill color set? */
  gboolean outline_set ;                                    /* Is outline color set? */
  gboolean background_set ;                                 /* Is background set ? */
  gboolean border_set ;                                     /* Is border set ? */
  gboolean splice_set ;

  gulong fill_colour;                                       /* Fill color, RGBA */
  gulong fill_pixel ;                                       /* Fill color */

  gulong outline_colour ;                                   /* Outline color, RGBA */
  gulong outline_pixel ;                                    /* Outline color */

  gulong background ;                                       /* eg for 3FT or strand separator */
  GdkBitmap *stipple ;

  gulong border ;                                           /* eg for navigator locator */

  gulong splice_colour ;                                    /* Fill color, RGBA */
  gulong splice_pixel ;                                     /* Fill color */



  double filter_value ;                                     /* active level, default 0.0 */
  int n_filtered ;
  gboolean enable_filter ;                                  /* has score in a feature and style allows it */



} ZMapWindowFeaturesetItemStruct ;





PixRect zmapWindowCanvasFeaturesetSummarise(PixRect pix,
					    ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature);
void zmapWindowCanvasFeaturesetSummariseFree(ZMapWindowFeaturesetItem featureset, PixRect pix);

gboolean zmapWindowCanvasFeaturesetFreeDisplayLists(ZMapWindowFeaturesetItem featureset_item_inout) ;

void zmapWindowFeaturesetS2Ccoords(double *start_inout, double *end_inout) ;
gboolean zmapWindowCanvasFeatureValid(ZMapWindowCanvasFeature feature) ;




#endif /* ZMAP_WINDOW_FEATURESET_ITEM_I_H */
