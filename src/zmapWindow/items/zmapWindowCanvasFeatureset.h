/*  File: zmapWindowFeatureset.h
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


/* Class */
typedef struct _zmapWindowFeaturesetItemClassStruct  zmapWindowFeaturesetItemClass, *ZMapWindowFeaturesetItemClass ;


/* Public funcs */
GType zMapWindowFeaturesetItemGetType(void);


void zMapWindowCanvasFeatureSetSetFuncs(int featuretype,gpointer *funcs);


ZMapWindowCanvasItem zMapWindowFeaturesetItemGetFeaturesetItem(FooCanvasGroup *parent, GQuark id, int start,int end, ZMapFeatureTypeStyle style, ZMapStrand strand, ZMapFrame frame, int index);
void zMapWindowFeaturesetAddItem(FooCanvasItem *foo, ZMapFeature feature, double dx, double y1, double y2);

void zMapWindowFeaturesetAddItem(FooCanvasItem *foo, ZMapFeature feature, double dx, double y1, double y2);


void zmapWindowFeaturesetItemSetColour(ZMapWindowCanvasItem   item,
						      FooCanvasItem         *interval,
						      ZMapFeature 		feature,
						      ZMapFeatureSubPartSpan sub_feature,
						      ZMapStyleColourType    colour_type,
							int colour_flags,
						      GdkColor              *fill,
                                          GdkColor              *border);

gboolean zMapWindowFeaturesetItemSetStyle(ZMapWindowFeaturesetItem di, ZMapFeatureTypeStyle style);


guint32 zMap_gdk_color_to_rgba(GdkColor *color);

int zMapWindowCanvasFeaturesetGetColours(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, gulong *fill_pixel,gulong *outline_pixel);

/* enums for function type */
typedef enum { FUNC_PAINT, FUNC_FLUSH, FUNC_EXTENT, FUNC_COLOUR, FUNC_STYLE, FUNC_ZOOM, FUNC_N_FUNC } zmapWindowCanvasFeatureFunc;

/* enums for FEATUREtion lookup  (feature types) */
typedef enum { FEATURE_BASIC, FEATURE_GLYPH, FEATURE_ALIGN, FEATURE_TRANSCRIPT, FEATURE_N_TYPE } zmapWindowCanvasFeatureType;



/* holds all data need to drive exotic bump modes */
typedef struct
{
	double spacing;	/* between sub columns */
	double offset;	/* current offset */
	double incr;	/* per sub column */
	double width;	/* max column width */
	GList *pos_list;	/* list of features in bumped sub-columns */
	gboolean complex;

	/* effciency stats */
	int features;
	int comps;
	int n_col;

} BumpFeaturesetStruct, *BumpFeatureset;

gboolean zMapWindowCanvasFeaturesetBump(ZMapWindowCanvasItem item, ZMapStyleBumpMode bump_mode, ZMapWindowCompressMode compress_mode, BumpFeatureset bump_data);

#endif /* ZMAP_WINDOW_FEATURESET_H */
