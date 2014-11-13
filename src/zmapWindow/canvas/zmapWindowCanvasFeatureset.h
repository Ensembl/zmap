/*  File: zmapWindowCanvasFeatureset.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_FEATURESET_H
#define ZMAP_WINDOW_FEATURESET_H

#include <libzmapfoocanvas/libfoocanvas.h>

#include <ZMap/zmapFeature.h>

#include <zmapWindowCanvasFeature.h>



#define ZMAP_WINDOW_FEATURESET_ITEM_NAME "ZMapWindowFeaturesetItem"

#define ZMAP_TYPE_WINDOW_FEATURESET_ITEM        (zMapWindowFeaturesetItemGetType())

#if GOBJ_CAST

#define ZMAP_WINDOW_FEATURESET_ITEM(obj)       ((ZMapWindowFeaturesetItem) obj)
#define ZMAP_WINDOW_FEATURESET_ITEM_CONST(obj) ((ZMapWindowFeaturesetItem const) obj)

#else

#define ZMAP_WINDOW_FEATURESET_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_FEATURESET_ITEM, ZMapWindowFeaturesetItem))
#define ZMAP_WINDOW_FEATURESET_ITEM_CONST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_FEATURESET_ITEM, ZMapWindowFeaturesetItem const))

#endif

#define ZMAP_WINDOW_FEATURESET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_FEATURESET_ITEM, zmapWindowFeaturesetItemClass))
#define ZMAP_IS_WINDOW_FEATURESET_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_FEATURESET_ITEM))
#define ZMAP_WINDOW_FEATURESET_ITEM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_FEATURESET_ITEM, zmapWindowFeaturesetItemClass))



/* Class */
typedef struct zmapWindowFeaturesetItemClassStructType  zmapWindowFeaturesetItemClass, *ZMapWindowFeaturesetItemClass ;

/* Instance */
typedef struct _zmapWindowFeaturesetItemStruct *ZMapWindowFeaturesetItem ;

typedef struct _zmapWindowCanvasPangoStruct *ZMapWindowCanvasPango;

typedef struct _zmapWindowCanvasGraphicsStruct *ZMapWindowCanvasGraphics;





/* Typedefs for per feature type function implementations (there should be others here.... */

typedef void (*ZMapWindowFeatureItemSetInitFunc)(ZMapWindowFeaturesetItem featureset) ;

typedef void (*ZMapWindowFeatureItemSetPaintFunc)(ZMapWindowFeaturesetItem featureset,
						  GdkDrawable *drawable, GdkEventExpose *expose) ;

typedef void (*ZMapWindowFeatureItemPreZoomFunc)(ZMapWindowFeaturesetItem featureset) ;
typedef void (*ZMapWindowFeatureItemZoomFunc)(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable) ;

typedef double (*ZMapWindowFeatureItemPointFunc)(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
						 double item_x, double item_y, int cx, int cy,
						 double local_x, double local_y, double x_off) ;

typedef void (*ZMapWindowFeatureFreeFunc)(ZMapWindowFeaturesetItem featureset) ;




typedef enum
  {
    ZMWCF_HIDE_INVALID,
    ZMWCF_HIDE_USER,			/* user hides feature */
    ZMWCF_HIDE_EXPAND			/* user expands/ contracts feature */

  } ZMapWindowCanvasFeaturesetHideType;



/* holds all data need to drive exotic bump modes */
typedef struct BumpFeaturesetStructName
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

} BumpFeaturesetStruct, *BumpFeatureset ;




GType zMapWindowFeaturesetItemGetType(void) ;

void zMapWindowCanvasFeatureSetSetFuncs(int featuretype, gpointer *funcs, int set_size) ;

gboolean zMapWindowCanvasIsFeatureSet(ZMapWindowFeaturesetItem fi) ;

GString *zMapWindowCanvasFeatureset2Txt(ZMapWindowFeaturesetItem featureset_item) ;

void zmapWindowCanvasFeaturesetInitPango(GdkDrawable *drawable,
                                         ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasPango pango,
                                         char *family, int size, GdkColor *draw);
void zmapWindowCanvasFeaturesetFreePango(ZMapWindowCanvasPango pango);

void zMapWindowCanvasFeaturesetExpose(ZMapWindowFeaturesetItem fi);
void zMapWindowCanvasFeaturesetRedraw(ZMapWindowFeaturesetItem fi, double zoom);
void zMapWindowCanvasItemFeaturesetSetVAdjust(ZMapWindowFeaturesetItem featureset, GtkAdjustment *v_adjuster) ;

ZMapWindowFeaturesetItem zMapWindowCanvasItemFeaturesetGetFeaturesetItem(FooCanvasGroup *parent, GQuark id,
                                                                         GtkAdjustment *v_adjuster,
                                                                         int start, int end,
                                                                         ZMapFeatureTypeStyle style,
                                                                         ZMapStrand strand, ZMapFrame frame,
                                                                         int index, guint layer);


guint zMapWindowCanvasFeaturesetGetId(ZMapWindowFeaturesetItem featureset);
GQuark zMapWindowCanvasFeaturesetGetSetIDAtPos(ZMapWindowFeaturesetItem fi, double event_x) ;


gboolean zMapWindowCanvasFeaturesetHasPointFeature(FooCanvasItem *item) ;
ZMapFeature zMapWindowCanvasFeaturesetGetPointFeature(ZMapWindowFeaturesetItem featureset_item) ;
gboolean zMapWindowCanvasFeaturesetUnsetPointFeature(FooCanvasItem *item) ;

ZMapWindowCanvasFeature zMapWindowCanvasFeaturesetGetPointFeatureItem(ZMapWindowFeaturesetItem featureset_item) ;


void zMapWindowCanvasFeaturesetSetWidth(ZMapWindowFeaturesetItem featureset, double width);
double zMapWindowCanvasFeaturesetGetWidth(ZMapWindowFeaturesetItem featureset);

double zMapWindowCanvasFeaturesetGetOffset(ZMapWindowFeaturesetItem featureset);

void zMapWindowCanvasFeaturesetSetSequence(ZMapWindowFeaturesetItem featureset, double y1, double y2);

void zMapWindowCanvasFeaturesetSetBackground(FooCanvasItem *foo, GdkColor *fill, GdkColor *outline);

guint zMapWindowCanvasFeaturesetGetLayer(ZMapWindowFeaturesetItem featureset);
void zMapWindowCanvasFeaturesetSetLayer(ZMapWindowFeaturesetItem featureset, guint layer);

void zMapWindowFeaturesetSetFeatureWidth(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasFeature feat);
int zMapWindowCanvasFeaturesetAddFeature(ZMapWindowFeaturesetItem featureset,
                                         ZMapFeature feature, double y1, double y2);

ZMapWindowCanvasFeature zMapWindowFeaturesetAddFeature(ZMapWindowFeaturesetItem featureset_item,
                                                       ZMapFeature feature, double y1, double y2);


int zMapWindowFeaturesetItemRemoveFeature(FooCanvasItem *foo, ZMapFeature feature);
int zMapWindowFeaturesetItemRemoveSet(FooCanvasItem *foo, ZMapFeatureSet featureset, gboolean destroy);





ZMapWindowCanvasGraphics zMapWindowFeaturesetAddGraphics(ZMapWindowFeaturesetItem featureset_item,
                                                         zmapWindowCanvasFeatureType type,
                                                         double x1, double y1, double x2, double y2,
                                                         GdkColor *fill, GdkColor *outline, char *text);
int zMapWindowFeaturesetRemoveGraphics(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasGraphics feat);


void zmapWindowCanvasFeaturesetDumpFeatures(ZMapWindowFeaturesetItem feature) ;

gboolean zMapWindowFeaturesetItemSetStyle(ZMapWindowFeaturesetItem di, ZMapFeatureTypeStyle style);
void zmapWindowFeaturesetItemShowHide(FooCanvasItem *foo,
                                      ZMapFeature feature, gboolean show, ZMapWindowCanvasFeaturesetHideType how);

void zmapWindowFeaturesetItemSetColour(FooCanvasItem *interval,
				       ZMapFeature 		feature,
				       ZMapFeatureSubPartSpan sub_feature,
				       ZMapStyleColourType    colour_type,
				       int colour_flags,
				       GdkColor              *fill,
				       GdkColor              *border);
guint32 zMap_gdk_color_to_rgba(GdkColor *color);
int zMapWindowCanvasFeaturesetGetColours(ZMapWindowFeaturesetItem featureset,
                                         ZMapWindowCanvasFeature feature, gulong *fill_pixel,gulong *outline_pixel);
gboolean zmapWindowFeaturesetAllocColour(ZMapWindowFeaturesetItemClass featureset_class,
                                         GdkColor *colour) ;
gboolean zmapWindowFeaturesetGetDefaultColours(ZMapWindowFeaturesetItem feature_set_item,
                                               GdkColor **fill, GdkColor **draw, GdkColor **border) ;
gboolean zmapWindowFeaturesetGetDefaultPixels(ZMapWindowFeaturesetItem feature_set_item,
                                              guint32 *fill, guint32 *draw, guint32 *border) ;
gulong zMapWindowCanvasFeatureGetHeatColour(gulong a, gulong b, double score);
gulong zMapWindowCanvasFeatureGetHeatPixel(gulong a, gulong b, double score);
gboolean zMapWindowCanvasFeaturesetGetSpliceColour(ZMapWindowFeaturesetItem featureset, gulong *splice_pixel) ;

void zMapWindowCanvasFeaturesetIndex(ZMapWindowFeaturesetItem fi);

gboolean zMapWindowCanvasFeaturesetGetSeqCoord(ZMapWindowFeaturesetItem featureset,
                                               gboolean set, double x, double y, long *start, long *end);
gboolean zMapCanvasFeaturesetSeq2World(ZMapWindowFeaturesetItem featureset,
                                       int seq_start, int seq_end, double *world_start_out, double *world_end_out) ;
void zMapWindowCanvasFeaturesetGetFeatureBounds(FooCanvasItem *foo,
                                                double *rootx1, double *rooty1, double *rootx2, double *rooty2);
GList *zMapWindowFeaturesetFindItemAndFeatures(FooCanvasItem **item, double y1, double y2, double x1, double x2);
GList *zMapWindowFeaturesetFindFeatures(ZMapWindowFeaturesetItem featureset, double y1, double y2) ;

void zMapWindowCanvasFeaturesetPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
					    GdkDrawable *drawable, GdkEventExpose *expose);
void zMapWindowCanvasFeaturesetPaintFlush(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
					  GdkDrawable *drawable, GdkEventExpose *expose);

void zMapWindowCanvasFeaturesetPreZoom(ZMapWindowFeaturesetItem featureset) ;
void zMapWindowCanvasFeaturesetZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable);
void zMapWindowCanvasFeaturesetSetZoomY(ZMapWindowFeaturesetItem fi, double zoom_y) ;
void zMapWindowCanvasFeaturesetSetZoomRecalc(ZMapWindowFeaturesetItem featureset, gboolean recalc) ;

gboolean zMapWindowCanvasFeaturesetBump(ZMapWindowFeaturesetItem item,
                                        ZMapStyleBumpMode bump_mode, int compress_mode, BumpFeatureset bump_data);

void zMapWindowCanvasFeaturesetShowHideMasked(FooCanvasItem *foo, gboolean show, gboolean set_colour);

void zMapWindowCanvasFeaturesetSetStipple(ZMapWindowFeaturesetItem featureset, GdkBitmap *stipple);


double zMapWindowCanvasFeatureGetWidthFromScore(ZMapFeatureTypeStyle style, double width, double score);
double zMapWindowCanvasFeatureGetNormalisedScore(ZMapFeatureTypeStyle style, double score);

double zMapWindowCanvasFeaturesetGetFilterValue(FooCanvasItem *foo);
int zMapWindowCanvasFeaturesetGetFilterCount(FooCanvasItem *foo);
int zMapWindowCanvasFeaturesetFilter(gpointer filter, double value, gboolean highlight_filtered_columns);
int zMapWindowFeaturesetItemGetNFiltered(FooCanvasItem *item);

void zMapWindowCanvasFeaturesetRequestReposition(FooCanvasItem *foo);


ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapWindowCanvasFeatureType2ExactStr, zmapWindowCanvasFeatureType) ;


#endif /* ZMAP_WINDOW_FEATURESET_H */
