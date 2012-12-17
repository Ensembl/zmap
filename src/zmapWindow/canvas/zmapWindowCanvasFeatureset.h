/*  File: zmapWindowFeatureset.h
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
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_FEATURESET_H
#define ZMAP_WINDOW_FEATURESET_H

//#include <ZMap/zmapStyle.h>         // for shape struct and style data


#define ZMAP_WINDOW_FEATURESET_ITEM_NAME "ZMapWindowFeaturesetItem"

#define ZMAP_TYPE_WINDOW_FEATURESET_ITEM        (zMapWindowFeaturesetItemGetType())

#if GOBJ_CAST
#define ZMAP_WINDOW_FEATURESET_ITEM(obj)       ((ZMapWindowFeaturesetItem) obj)
#define ZMAP_WINDOW_FEATURESET_ITEM_CONST(obj) ((ZMapWindowFeaturesetItem const) obj)
#else
#define ZMAP_WINDOW_FEATURESET_ITEM(obj)	      (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_FEATURESET_ITEM, zmapWindowFeaturesetItem))
#define ZMAP_WINDOW_FEATURESET_ITEM_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_FEATURESET_ITEM, zmapWindowFeaturesetItem const))
#endif
#define ZMAP_WINDOW_FEATURESET_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_FEATURESET_ITEM, zmapWindowFeaturesetItemClass))
#define ZMAP_IS_WINDOW_FEATURESET_ITEM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_FEATURESET_ITEM))
#define ZMAP_WINDOW_FEATURESET_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_FEATURESET_ITEM, zmapWindowFeaturesetItemClass))





/* Instance */
typedef struct _zmapWindowFeaturesetItemStruct  zmapWindowFeaturesetItem, *ZMapWindowFeaturesetItem ;
typedef struct _zmapWindowCanvasFeatureStruct  zmapWindowCanvasFeature, *ZMapWindowCanvasFeature ;
typedef struct _zmapWindowCanvasPangoStruct   zmapWindowCanvasPango, *ZMapWindowCanvasPango;

typedef struct _zmapWindowCanvasGraphicsStruct zmapWindowCanvasGraphics, *ZMapWindowCanvasGraphics;


/* Class */
typedef struct _zmapWindowFeaturesetItemClassStruct  zmapWindowFeaturesetItemClass, *ZMapWindowFeaturesetItemClass ;


/* enums for function type */
typedef enum
  {
    FUNC_SET_INIT,
    FUNC_PREPARE,
    FUNC_SET_PAINT, FUNC_PAINT,
    FUNC_FLUSH, FUNC_EXTENT,
    FUNC_COLOUR,
    FUNC_STYLE, FUNC_ZOOM,
    FUNC_INDEX, FUNC_FREE,
    FUNC_POINT,
    FUNC_ADD, FUNC_SUBPART,
    FUNC_N_FUNC
  } zmapWindowCanvasFeatureFunc;


/* Typedefs for per feature type function implementations (there should be others here.... */

typedef void (*ZMapWindowFeatureItemSetInitFunc)(ZMapWindowFeaturesetItem featureset) ;

typedef void (*ZMapWindowFeatureItemSetPaintFunc)(ZMapWindowFeaturesetItem featureset,
						  GdkDrawable *drawable, GdkEventExpose *expose) ;

typedef void (*ZMapWindowFeatureItemZoomFunc)(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable) ;

typedef double (*ZMapWindowFeatureItemPointFunc)(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
						 double item_x, double item_y, int cx, int cy,
						 double local_x, double local_y, double x_off) ;

typedef void (*ZMapWindowFeatureFreeFunc)(ZMapWindowFeaturesetItem featureset) ;



/* enums for feature function lookup  (feature types) */
/* NOTE these are set by style mode but are defined separately as CanvasFeaturesets
 * do not initially handle all style modes */
/* see  zMapWindowFeaturesetAddItem() */
typedef enum
  {
    FEATURE_INVALID,

    /* genomic features */
    FEATURE_BASIC,
    FEATURE_GENOMIC = FEATURE_BASIC,

    FEATURE_ALIGN,
    FEATURE_TRANSCRIPT,

    FEATURE_SEQUENCE,
    FEATURE_ASSEMBLY,
    FEATURE_LOCUS,

    FEATURE_GRAPH,
    FEATURE_GLYPH,

    /* unadorned graphics primitives, NOTE that FEATURE_GRAPHICS is used in the code to test
     * for feature vs. graphic items. */
    FEATURE_GRAPHICS,		/* a catch-all for the featureset type */
    FEATURE_LINE,
    FEATURE_BOX,
    FEATURE_TEXT,

    FEATURE_N_TYPE
  } zmapWindowCanvasFeatureType;



/* Public funcs */
GType zMapWindowFeaturesetItemGetType(void);


void zMapWindowCanvasFeatureSetSetFuncs(int featuretype,gpointer *funcs, int feature_size, int set_size);

/* GDK wrappers to clip features */

int zMap_draw_line(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, gint cx1, gint cy1, gint cx2, gint cy2);
int zMap_draw_broken_line(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, gint cx1, gint cy1, gint cx2, gint cy2);
int zMap_draw_rect(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, gint cx1, gint cy1, gint cx2, gint cy2, gboolean fill);


void zmapWindowCanvasFeaturesetInitPango(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasPango pango, char *family, int size, GdkColor *draw);

void zmapWindowCanvasFeaturesetFreePango(ZMapWindowCanvasPango pango);

void zMapWindowCanvasFeaturesetExpose(ZMapWindowFeaturesetItem fi);
void zMapWindowCanvasFeaturesetRedraw(ZMapWindowFeaturesetItem fi, double zoom);


ZMapWindowCanvasItem zMapWindowCanvasItemFeaturesetGetFeaturesetItem(FooCanvasGroup *parent, GQuark id, int start,int end, ZMapFeatureTypeStyle style, ZMapStrand strand, ZMapFrame frame, int index, guint layer);

guint zMapWindowCanvasFeaturesetGetId(ZMapWindowFeaturesetItem featureset);

ZMapFeatureSubPartSpan zMapWindowCanvasFeaturesetGetSubPartSpan(FooCanvasItem *foo,ZMapFeature feature,double x,double y);

gboolean zMapWindowCanvasFeaturesetHasPointFeature(FooCanvasItem *item);

ZMapWindowCanvasFeature zmapWindowCanvasFeatureAlloc(zmapWindowCanvasFeatureType type);
void zmapWindowCanvasFeatureFree(gpointer thing);

void zMapWindowCanvasFeaturesetSetWidth(ZMapWindowFeaturesetItem featureset, double width);
double zMapWindowCanvasFeaturesetGetWidth(ZMapWindowFeaturesetItem featureset);

double zMapWindowCanvasFeaturesetGetOffset(ZMapWindowFeaturesetItem featureset);

void zMapWindowCanvasFeaturesetSetSequence(ZMapWindowFeaturesetItem featureset, double y1, double y2);

void zMapWindowCanvasFeaturesetSetBackground(FooCanvasItem *foo, GdkColor *fill, GdkColor *outline);

guint zMapWindowCanvasFeaturesetGetLayer(ZMapWindowFeaturesetItem featureset);
void zMapWindowCanvasFeaturesetSetLayer(ZMapWindowFeaturesetItem featureset, guint layer);

void zMapWindowFeaturesetSetFeatureWidth(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasFeature feat);
int zMapWindowCanvasFeaturesetAddFeature(ZMapWindowFeaturesetItem featureset, ZMapFeature feature, double y1, double y2);

ZMapWindowCanvasFeature zMapWindowFeaturesetAddFeature(ZMapWindowFeaturesetItem featureset_item, ZMapFeature feature, double y1, double y2);
int zMapWindowFeaturesetItemRemoveFeature(FooCanvasItem *foo, ZMapFeature feature);

int zMapWindowFeaturesetItemRemoveSet(FooCanvasItem *foo, ZMapFeatureSet featureset, gboolean destroy);

ZMapWindowCanvasGraphics zMapWindowFeaturesetAddGraphics(ZMapWindowFeaturesetItem featureset_item, zmapWindowCanvasFeatureType type, double x1, double y1, double x2, double y2, GdkColor *fill, GdkColor *outline, char *text);
int zMapWindowFeaturesetRemoveGraphics(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasGraphics feat);


void zmapWindowFeaturesetItemSetColour(   FooCanvasItem         *interval,
						      ZMapFeature 		feature,
						      ZMapFeatureSubPartSpan sub_feature,
						      ZMapStyleColourType    colour_type,
							int colour_flags,
						      GdkColor              *fill,
                                          GdkColor              *border);

gboolean zMapWindowFeaturesetItemSetStyle(ZMapWindowFeaturesetItem di, ZMapFeatureTypeStyle style);

typedef enum
{
	ZMWCF_HIDE_INVALID,
	ZMWCF_HIDE_USER,			/* user hides feature */
	ZMWCF_HIDE_EXPAND			/* user expands/ contracts feature */

} ZMapWindowCanvasFeaturesetHideType;

void zmapWindowFeaturesetItemShowHide(FooCanvasItem *foo, ZMapFeature feature, gboolean show, ZMapWindowCanvasFeaturesetHideType how);



guint32 zMap_gdk_color_to_rgba(GdkColor *color);

int zMapWindowCanvasFeaturesetGetColours(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, gulong *fill_pixel,gulong *outline_pixel);

void zMapWindowCanvasFeaturesetIndex(ZMapWindowFeaturesetItem fi);

gboolean zMapWindowCanvasFeaturesetGetSeqCoord(ZMapWindowFeaturesetItem featureset, gboolean set, double x, double y, long *start, long *end);

void zMapWindowCanvasFeaturesetGetFeatureBounds(FooCanvasItem *foo, double *rootx1, double *rooty1, double *rootx2, double *rooty2);



/* basic feature draw a box
 * defined as a macro for efficiency to avoid multple copies of cut and paste
 * otherwise would need 10 args which is silly
 * used by basc feature, alignments, graphs, maybe transcripts... and what else??
 *
 * NOTE x1 and x2 passed as args as normal features are centred but graphs maybe not
 */
#define zMapCanvasFeaturesetDrawBoxMacro(featureset, x1,x2, y1,y2,  drawable,fill_set,outline_set,fill,outline)\
{\
	FooCanvasItem *item = (FooCanvasItem *) featureset;\
	GdkColor c;\
      int cx1, cy1, cx2, cy2;\
\
		/* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */\
		/* NOTE CanvasFeature coords are the extent including decorations so we get coords from the feature */\
	foo_canvas_w2c (item->canvas, x1, y1 - featureset->start + featureset->dy, &cx1, &cy1);\
	foo_canvas_w2c (item->canvas, x2, y2 - featureset->start + featureset->dy + 1, &cx2, &cy2);\
      						/* + 1 to draw to the end of the last base */\
\
zMapAssert(x2 - x1 > 0 && x2 - x1 < 40);\
		/* NOTE that the gdk_draw_rectangle interface is a bit esoteric\
		 * and it doesn't like rectangles that have no depth\
		 */\
\
	if(fill_set && (!outline_set || (cy2 - cy1 > 1)))	/* fill will be visible */\
	{\
		c.pixel = fill;\
		gdk_gc_set_foreground (featureset->gc, &c);\
		zMap_draw_rect (drawable, featureset, cx1, cy1, cx2, cy2, TRUE);\
	}\
\
	if(outline_set)\
	{\
		c.pixel = outline;\
		gdk_gc_set_foreground (featureset->gc, &c);\
		zMap_draw_rect (drawable, featureset, cx1, cy1, cx2+1, cy2+1, FALSE); /* +1 due to gdk_draw_rect and zMap_draw_rect */\
	}\
}


void zMapWindowCanvasFeaturesetPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
					    GdkDrawable *drawable, GdkEventExpose *expose);
void zMapWindowCanvasFeaturesetPaintFlush(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
					  GdkDrawable *drawable, GdkEventExpose *expose);
gboolean zMapWindowCanvasFeaturesetGetFeatureExtent(ZMapWindowCanvasFeature feature, gboolean complex,
						    ZMapSpan span, double *width);
void zMapWindowCanvasFeaturesetZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable);


gint zMapFeatureNameCmp(gconstpointer a, gconstpointer b);
gint zMapFeatureCmp(gconstpointer a, gconstpointer b);

gulong zMapWindowCanvasFeatureGetHeatColour(gulong a, gulong b, double score);
gulong zMapWindowCanvasFeatureGetHeatPixel(gulong a, gulong b, double score);


/* holds all data need to drive exotic bump modes */
typedef struct
{
	double start,end;	/* eg mark */
	double spacing;	/* between sub columns */
	double offset;	/* current offset */
	double incr;	/* per sub column */
	double width;	/* max column width */
	GList *pos_list;	/* list of features in bumped sub-columns */
	ZMapSpanStruct span;	/* of current feature(s) */
	gboolean complex;
	gboolean mark_set;

	/* effciency stats */
	int features;
	int comps;
	int n_col;

} BumpFeaturesetStruct, *BumpFeatureset;

gboolean zMapWindowCanvasFeaturesetBump(ZMapWindowFeaturesetItem item, ZMapStyleBumpMode bump_mode, int compress_mode, BumpFeatureset bump_data);


void zMapWindowCanvasFeaturesetShowHideMasked(FooCanvasItem *foo, gboolean show, gboolean set_colour);

GList *zMapWindowFeaturesetItemFindFeatures(FooCanvasItem **item, double y1, double y2, double x1, double x2);


void zMapWindowCanvasFeaturesetSetStipple(ZMapWindowFeaturesetItem featureset, GdkBitmap *stipple);


double zMapWindowCanvasFeatureGetWidthFromScore(ZMapFeatureTypeStyle style, double width, double score);
double zMapWindowCanvasFeatureGetNormalisedScore(ZMapFeatureTypeStyle style, double score);


double zMapWindowCanvasFeaturesetGetFilterValue(FooCanvasItem *foo);
int zMapWindowCanvasFeaturesetGetFilterCount(FooCanvasItem *foo);
int zMapWindowCanvasFeaturesetFilter(gpointer filter, double value);


void zMapWindowCanvasFeaturesetRequestReposition(FooCanvasItem *foo);



#endif /* ZMAP_WINDOW_FEATURESET_H */
