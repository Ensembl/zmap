/*  File: zmapWindowContainerFeatureSet.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Last edited: Nov  6 17:59 2009 (edgrif)
 * Created: Wed Dec  3 08:21:03 2008 (rds)
 * CVS info:   $Id: zmapWindowContainerFeatureSet.h,v 1.8 2010-01-19 12:36:53 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_CONTAINER_FEATURESET_H
#define ZMAP_WINDOW_CONTAINER_FEATURESET_H

#include <glib-object.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapStyle.h>
#include <ZMap/zmapWindow.h>	/* ZMapWindow type */


#define ZMAP_WINDOW_CONTAINER_FEATURESET_NAME 	"ZMapWindowContainerFeatureSet"


#define ZMAP_TYPE_CONTAINER_FEATURESET           (zmapWindowContainerFeatureSetGetType())
#define ZMAP_CONTAINER_FEATURESET(obj)	         (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_FEATURESET, zmapWindowContainerFeatureSet))
#define ZMAP_CONTAINER_FEATURESET_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_FEATURESET, zmapWindowContainerFeatureSet const))
#define ZMAP_CONTAINER_FEATURESET_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_FEATURESET, zmapWindowContainerFeatureSetClass))
#define ZMAP_IS_CONTAINER_FEATURESET(obj)	 (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_FEATURESET))
#define ZMAP_CONTAINER_FEATURESET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_FEATURESET, zmapWindowContainerFeatureSetClass))


/* Instance */
typedef struct _zmapWindowContainerFeatureSetStruct  zmapWindowContainerFeatureSet, *ZMapWindowContainerFeatureSet ;


/* Class */
typedef struct _zmapWindowContainerFeatureSetClassStruct  zmapWindowContainerFeatureSetClass, *ZMapWindowContainerFeatureSetClass ;


/* Public funcs */
GType zmapWindowContainerFeatureSetGetType(void);

ZMapWindowContainerFeatureSet zmapWindowContainerFeatureSetAugment(ZMapWindowContainerFeatureSet container_set,
								   ZMapWindow window,
								   GQuark     align_id,
								   GQuark     block_id,
								   GQuark     feature_set_unique_id,
								   GQuark     feature_set_original_id, /* unused! */
								   GList     *style_list,
								   ZMapStrand strand,
								   ZMapFrame  frame);
gboolean zmapWindowContainerFeatureSetAttachFeatureSet(ZMapWindowContainerFeatureSet container_set,
						       ZMapFeatureSet feature_set_to_attach);
ZMapFeatureSet zmapWindowContainerFeatureSetRecoverFeatureSet(ZMapWindowContainerFeatureSet container_set);
#ifdef blah
ZMapWindowStats zmapWindowContainerFeatureSetRecoverStats(ZMapWindowContainerFeatureSet container_set);
#endif

/* Style lookup */
ZMapFeatureTypeStyle zmapWindowContainerFeatureSetStyleFromStyle(ZMapWindowContainerFeatureSet container_set,
								 ZMapFeatureTypeStyle         style2copy);
ZMapFeatureTypeStyle zmapWindowContainerFeatureSetStyleFromID(ZMapWindowContainerFeatureSet container_set,
							      GQuark                       style_unique_id);
ZMapFeatureTypeStyle zmapWindowContainerFeatureSetGetStyle(ZMapWindowContainerFeatureSet container_set,
							   ZMapFeature                  feature);
GQuark               zmapWindowContainerFeatureSetColumnDisplayName(ZMapWindowContainerFeatureSet container_set);

/* style properties for the whole column,  */
ZMapStrand zmapWindowContainerFeatureSetGetStrand(ZMapWindowContainerFeatureSet container_set);
ZMapFrame  zmapWindowContainerFeatureSetGetFrame (ZMapWindowContainerFeatureSet container_set);
double     zmapWindowContainerFeatureSetGetWidth(ZMapWindowContainerFeatureSet container_set);
double     zmapWindowContainerFeatureGetBumpSpacing(ZMapWindowContainerFeatureSet container_set);
gboolean   zmapWindowContainerFeatureSetGetMagValues(ZMapWindowContainerFeatureSet container_set, 
						     double *min_mag_out, double *max_mag_out);
ZMapStyleColumnDisplayState zmapWindowContainerFeatureSetGetDisplay(ZMapWindowContainerFeatureSet container_set);
void zmapWindowContainerFeatureSetDisplay(ZMapWindowContainerFeatureSet container_set,
					  ZMapStyleColumnDisplayState state);
gboolean zmapWindowContainerFeatureSetShowWhenEmpty(ZMapWindowContainerFeatureSet container_set);
ZMapStyle3FrameMode zmapWindowContainerFeatureSetGetFrameMode(ZMapWindowContainerFeatureSet container_set);
gboolean zmapWindowContainerFeatureSetIsFrameSpecific(ZMapWindowContainerFeatureSet container_set,
						      ZMapStyle3FrameMode         *frame_mode_out);

gboolean zmapWindowContainerFeatureSetIsStrandShown(ZMapWindowContainerFeatureSet container_set) ;

ZMapStyleBumpMode zmapWindowContainerFeatureSetGetBumpMode(ZMapWindowContainerFeatureSet container_set);
ZMapStyleBumpMode zmapWindowContainerFeatureSetGetDefaultBumpMode(ZMapWindowContainerFeatureSet container_set);
ZMapStyleBumpMode zmapWindowContainerFeatureSetResetBumpModes(ZMapWindowContainerFeatureSet container_set);
gboolean zmapWindowContainerFeatureSetJoinAligns(ZMapWindowContainerFeatureSet container_set, unsigned int *threshold);
gboolean zmapWindowContainerFeatureSetGetDeferred(ZMapWindowContainerFeatureSet container_set);

/* hidden stack code */
void zmapWindowContainerFeatureSetFeatureRemove(ZMapWindowContainerFeatureSet item_feature_set,
						ZMapFeature feature);
GList *zmapWindowContainerFeatureSetPopHiddenStack(ZMapWindowContainerFeatureSet container_set);
void zmapWindowContainerFeatureSetPushHiddenStack(ZMapWindowContainerFeatureSet container_set,
						  GList *hidden_items_list);

void zmapWindowContainerFeatureSetRemoveAllItems(ZMapWindowContainerFeatureSet container_set);

void zmapWindowContainerFeatureSetSortFeatures(ZMapWindowContainerFeatureSet container_set, 
					       gint direction);
                                     
void zMapWindowContainerFeatureSetMarkUnsorted(ZMapWindowContainerFeatureSet container_set);

/* Finished with this container */
ZMapWindowContainerFeatureSet zMapWindowContainerFeatureSetDestroy(ZMapWindowContainerFeatureSet canvas_item);

gboolean zmapWindowStyleListGetSetting(GList *list_of_styles,char *setting_name,GValue *value_in_out);

#endif /* ZMAP_WINDOW_CONTAINER_FEATURESET_H */
