/*  File: zmapWindowItemFeatureSet.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 * Last edited: Apr 27 11:45 2009 (edgrif)
 * Created: Fri Feb  6 15:32:46 2009 (rds)
 * CVS info:   $Id: zmapWindowItemFeatureSet.h,v 1.9 2009-04-28 14:33:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_WINDOW_ITEM_FEATURE_SET_H__
#define __ZMAP_WINDOW_ITEM_FEATURE_SET_H__

#include <glib.h>
#include <glib-object.h>


#define ZMAP_TYPE_WINDOW_ITEM_FEATURE_SET           (zmapWindowItemFeatureSetGetType())
#define ZMAP_WINDOW_ITEM_FEATURE_SET(obj)	    (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_ITEM_FEATURE_SET, zmapWindowItemFeatureSetData))
#define ZMAP_WINDOW_ITEM_FEATURE_SET_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_ITEM_FEATURE_SET, zmapWindowItemFeatureSetData const))
#define ZMAP_WINDOW_ITEM_FEATURE_SET_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_ITEM_FEATURE_SET, zmapWindowItemFeatureSetDataClass))
#define ZMAP_IS_WINDOW_ITEM_FEATURE_SET(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_ITEM_FEATURE_SET) || zmap_g_return_moan(__FILE__, __LINE__) )
#define ZMAP_WINDOW_ITEM_FEATURE_SET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_ITEM_FEATURE_SET, zmapWindowItemFeatureSetDataClass))


/* Instance */
typedef struct _zmapWindowItemFeatureSetDataStruct      zmapWindowItemFeatureSetData,      *ZMapWindowItemFeatureSetData;

/* Class */
typedef struct _zmapWindowItemFeatureSetDataClassStruct zmapWindowItemFeatureSetDataClass, *ZMapWindowItemFeatureSetDataClass ;


gboolean zmap_g_return_moan(char *file, int line);


/* Public funcs */
GType zmapWindowItemFeatureSetGetType(void);


ZMapWindowItemFeatureSetData zmapWindowItemFeatureSetCreate(ZMapWindow window,
							    FooCanvasGroup *column_container,
							    GQuark feature_set_unique_id,
							    GQuark feature_set_original_id,
                                                            GList *style_list,
                                                            ZMapStrand strand,
                                                            ZMapFrame frame);

void zmapWindowItemFeatureSetAttachFeatureSet(ZMapWindowItemFeatureSetData set_data,
					      ZMapFeatureSet feature_set_to_attach);
ZMapFeatureSet zmapWindowItemFeatureSetRecoverFeatureSet(ZMapWindowItemFeatureSetData set_data);
//ZMapWindowStats zmapWindowItemFeatureSetRecoverStats(ZMapWindowItemFeatureSetData set_data);
ZMapFeatureTypeStyle zmapWindowItemFeatureSetStyleFromStyle(ZMapWindowItemFeatureSetData set_data,
							    ZMapFeatureTypeStyle         style2copy);
ZMapFeatureTypeStyle zmapWindowItemFeatureSetStyleFromID(ZMapWindowItemFeatureSetData set_data,
							 GQuark                       style_unique_id);
GQuark zmapWindowItemFeatureSetColumnDisplayName(ZMapWindowItemFeatureSetData set_data);
ZMapWindow zmapWindowItemFeatureSetGetWindow(ZMapWindowItemFeatureSetData set_data);
ZMapStrand zmapWindowItemFeatureSetGetStrand(ZMapWindowItemFeatureSetData set_data);
ZMapFrame  zmapWindowItemFeatureSetGetFrame (ZMapWindowItemFeatureSetData set_data);

ZMapFeatureTypeStyle zmapWindowItemFeatureSetGetStyle(ZMapWindowItemFeatureSetData set_data,
						      ZMapFeature                  feature);


double zmapWindowItemFeatureSetGetWidth(ZMapWindowItemFeatureSetData set_data);
double zmapWindowItemFeatureGetBumpSpacing(ZMapWindowItemFeatureSetData set_data);

gboolean zmapWindowItemFeatureSetGetMagValues(ZMapWindowItemFeatureSetData set_data, 
					      double *min_mag_out, double *max_mag_out);

ZMapStyleColumnDisplayState zmapWindowItemFeatureSetGetDisplay(ZMapWindowItemFeatureSetData set_data);
void zmapWindowItemFeatureSetDisplay(ZMapWindowItemFeatureSetData set_data, ZMapStyleColumnDisplayState state);

gboolean zmapWindowItemFeatureSetShowWhenEmpty(ZMapWindowItemFeatureSetData set_data);
ZMapStyle3FrameMode  zmapWindowItemFeatureSetGetFrameMode(ZMapWindowItemFeatureSetData set_data);
gboolean zmapWindowItemFeatureSetIsFrameSpecific(ZMapWindowItemFeatureSetData set_data,
						 ZMapStyle3FrameMode         *frame_mode_out);
gboolean zmapWindowItemFeatureSetIsStrandSpecific(ZMapWindowItemFeatureSetData set_data);
ZMapStyleBumpMode zmapWindowItemFeatureSetGetBumpMode(ZMapWindowItemFeatureSetData set_data);
ZMapStyleBumpMode zmapWindowItemFeatureSetGetDefaultBumpMode(ZMapWindowItemFeatureSetData set_data);
ZMapStyleBumpMode zmapWindowItemFeatureSetResetBumpModes(ZMapWindowItemFeatureSetData set_data);
gboolean zmapWindowItemFeatureSetJoinAligns(ZMapWindowItemFeatureSetData set_data, unsigned int *threshold);
gboolean zmapWindowItemFeatureSetGetDeferred(ZMapWindowItemFeatureSetData set_data);
void zmapWindowItemFeatureSetFeatureRemove(ZMapWindowItemFeatureSetData item_feature_set,
					   ZMapFeature                  feature);

ZMapWindowItemFeatureSetData zmapWindowItemFeatureSetDestroy(ZMapWindowItemFeatureSetData item_feature_set);



#endif /* __ZMAP_WINDOW_ITEM_FEATURE_SET_H__ */
