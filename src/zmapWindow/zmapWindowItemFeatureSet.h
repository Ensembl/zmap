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
 * Last edited: Feb  9 14:15 2009 (rds)
 * Created: Fri Feb  6 15:32:46 2009 (rds)
 * CVS info:   $Id: zmapWindowItemFeatureSet.h,v 1.1 2009-02-09 14:55:08 rds Exp $
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
#define ZMAP_IS_WINDOW_ITEM_FEATURE_SET(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_ITEM_FEATURE_SET))
#define ZMAP_WINDOW_ITEM_FEATURE_SET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_ITEM_FEATURE_SET, zmapWindowItemFeatureSetDataClass))


/* Instance */
typedef struct _zmapWindowItemFeatureSetDataStruct      zmapWindowItemFeatureSetData,      *ZMapWindowItemFeatureSetData;

/* Class */
typedef struct _zmapWindowItemFeatureSetDataClassStruct zmapWindowItemFeatureSetDataClass, *ZMapWindowItemFeatureSetDataClass ;





/* Public funcs */
GType zmapWindowItemFeatureSetGetType(void);


ZMapWindowItemFeatureSetData zmapWindowItemFeatureSetCreate(ZMapWindow window,
                                                            ZMapFeatureTypeStyle style,
                                                            ZMapStrand strand,
                                                            ZMapFrame frame);
ZMapFeatureTypeStyle zmapWindowItemFeatureSetGetStyle(ZMapWindowItemFeatureSetData set_data,
						      ZMapFeature                  feature);
ZMapFeatureTypeStyle zmapWindowItemFeatureSetColumnStyle(ZMapWindowItemFeatureSetData set_data);

double zmapWindowItemFeatureSetGetWidth(ZMapWindowItemFeatureSetData set_data);

gboolean zmapWindowItemFeatureSetGetMagValues(ZMapWindowItemFeatureSetData set_data, 
					      double *min_mag_out, double *max_mag_out);

ZMapStyleColumnDisplayState zmapWindowItemFeatureSetGetDisplay(ZMapWindowItemFeatureSetData set_data);
void zmapWindowItemFeatureSetDisplay(ZMapWindowItemFeatureSetData set_data, ZMapStyleColumnDisplayState state);

gboolean zmapWindowItemFeatureSetShowWhenEmpty(ZMapWindowItemFeatureSetData set_data);
gboolean zmapWindowItemFeatureSetIsFrameSensitive(ZMapWindowItemFeatureSetData set_data);
ZMapStyleOverlapMode zmapWindowItemFeatureSetGetOverlapMode(ZMapWindowItemFeatureSetData set_data);
ZMapStyleOverlapMode zmapWindowItemFeatureSetGetDefaultOverlapMode(ZMapWindowItemFeatureSetData set_data);

void zmapWindowItemFeatureSetFeatureRemove(ZMapWindowItemFeatureSetData item_feature_set,
					   ZMapFeature                  feature);

ZMapWindowItemFeatureSetData zmapWindowItemFeatureSetDestroy(ZMapWindowItemFeatureSetData item_feature_set);



#endif /* __ZMAP_WINDOW_ITEM_FEATURE_SET_H__ */
