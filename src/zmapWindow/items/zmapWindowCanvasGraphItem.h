/*  File: zmapWindowGraphItem.h
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

#ifndef ZMAP_WINDOW_GRAPH_DENSITY_H
#define ZMAP_WINDOW_GRAPH_DENSITY_H

#include <ZMap/zmap.h>

void zMapWindowCanvasGraphInit(void);


#if 0
/* Instance */
typedef struct _zmapWindowGraphDensityItemStruct  zmapWindowGraphDensityItem, *ZMapWindowGraphDensityItem ;


/* Class */
typedef struct _zmapWindowGraphDensityItemClassStruct  zmapWindowGraphDensityItemClass, *ZMapWindowGraphDensityItemClass ;


/* Public funcs */
GType zMapWindowGraphDensityItemGetType(void);


ZMapWindowCanvasItem zMapWindowGraphDensityItemGetDensityItem(FooCanvasGroup *parent, GQuark id, int start,int end, ZMapFeatureTypeStyle style, ZMapStrand strand, ZMapFrame frame, int index);
void zMapWindowGraphDensityAddItem(FooCanvasItem *foo, ZMapFeature feature, double dx, double y1, double y2);

void zmapWindowGraphDensityItemSetColour(ZMapWindowCanvasItem   item,
						      FooCanvasItem         *interval,
						      ZMapFeature			feature,
						      ZMapFeatureSubPartSpan sub_feature,
						      ZMapStyleColourType    colour_type,
							int colour_flags,
						      GdkColor              *fill,
                                          GdkColor              *border);

gboolean zMapWindowGraphDensityItemSetStyle(ZMapWindowGraphDensityItem di, ZMapFeatureTypeStyle style);
#endif

#endif /* ZMAP_WINDOW_GRAPH_DENSITY_H */
