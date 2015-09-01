/*  File: zmapWindowContainerFeatureSet.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 * Description: Header for class representing a column in zmap.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_CONTAINER_FEATURESET_H
#define ZMAP_WINDOW_CONTAINER_FEATURESET_H

#include <glib-object.h>

#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapStyle.hpp>


/* This absolutely should NOT be in here..... */
#include <ZMap/zmapWindow.hpp>	/* ZMapWindow type */

/* Temp, make things compile.... */
#include <zmapWindowCanvasItem.hpp>


#define ZMAP_WINDOW_CONTAINER_FEATURESET_NAME 	"ZMapWindowContainerFeatureSet"
#define ZMAP_TYPE_CONTAINER_FEATURESET           (zmapWindowContainerFeatureSetGetType())
#define ZMAP_CONTAINER_FEATURESET(obj)                                  \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_FEATURESET, zmapWindowContainerFeatureSet))
#define ZMAP_CONTAINER_FEATURESET_CONST(obj)                            \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_FEATURESET, zmapWindowContainerFeatureSet const))
#define ZMAP_CONTAINER_FEATURESET_CLASS(klass)                          \
  (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_FEATURESET, zmapWindowContainerFeatureSetClass))
#define ZMAP_IS_CONTAINER_FEATURESET(obj)                               \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_FEATURESET))
#define ZMAP_CONTAINER_FEATURESET_GET_CLASS(obj)                        \
  (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_FEATURESET, zmapWindowContainerFeatureSetClass))


/* Instance */
typedef struct _zmapWindowContainerFeatureSetStruct  zmapWindowContainerFeatureSet, *ZMapWindowContainerFeatureSet ;


/* Class */
typedef struct _zmapWindowContainerFeatureSetClassStruct
zmapWindowContainerFeatureSetClass, *ZMapWindowContainerFeatureSetClass ;


typedef ColinearityType (*ZMapFeatureCompareFunc)(ZMapFeature feature_a, ZMapFeature feature_b, gpointer user_data);



/* What part of the feature will be used to filter. */
typedef enum
  {
    ZMAP_CANVAS_FILTER_INVALID,
    ZMAP_CANVAS_FILTER_NONE,                                /* None => undo any existing filter. */
    ZMAP_CANVAS_FILTER_PARTS,                               /* Find features with same matches or exons. */
    ZMAP_CANVAS_FILTER_GAPS,                                /* Find features with same gaps or introns. */
    ZMAP_CANVAS_FILTER_FEATURE                              /* Find features with same overall start/end. */
  } ZMapWindowContainerFilterType ;


/* What action to perform on filtered features. */
typedef enum
  {
    ZMAP_CANVAS_ACTION_INVALID,
    ZMAP_CANVAS_ACTION_HIGHLIGHT_SPLICE,                    /* Highlight matching splices. */
    ZMAP_CANVAS_ACTION_HIDE,                                /* Hide features with matches. */
    ZMAP_CANVAS_ACTION_SHOW,                                /* Show features with matches (=> hide non-matching). */
    ZMAP_CANVAS_ACTION_CREATE_NEW                           /* Create new features with matches. */
  } ZMapWindowContainerActionType ;


/* Which features to perform the action on, where "source" is the feature against which
 * other features are filtered. */
typedef enum
  {
    ZMAP_CANVAS_TARGET_INVALID,
    ZMAP_CANVAS_TARGET_NOT_SOURCE_FEATURES,                 /* Do not apply to source features. */
    ZMAP_CANVAS_TARGET_NOT_SOURCE_COLUMN,                   /* Do not apply to source column. */
    ZMAP_CANVAS_TARGET_ALL,                                 /* Apply to all features in all columns. */
  } ZMapWindowContainerTargetType ;


/* Return code from filtering...it seems important to be able to indicate to the user
 * whether nothing happened because the selected column is not "filter-sensitive"
 * (needs style resource "filter-sensitive" to be to TRUE for filtering)
 * or just because there were no matches. */
typedef enum
  {
    ZMAP_CANVAS_FILTER_FAILED,                              /* Serious error, see log file. */
    ZMAP_CANVAS_FILTER_NOT_SENSITIVE,                       /* The column is not filter-sensitive. */
    ZMAP_CANVAS_FILTER_NO_MATCHES,                          /* No features matched the filter. */
    ZMAP_CANVAS_FILTER_OK                                   /* Features were filtered. */
  } ZMapWindowContainerFilterRC ;



/* Public funcs */
GType zmapWindowContainerFeatureSetGetType(void);
FooCanvasItem *zmapWindowContainerFeatureSetFindCanvasColumn(ZMapWindowContainerGroup group,
                                                             GQuark align, GQuark block, GQuark set,
                                                             ZMapStrand strand, ZMapFrame frame) ;
void zmapWindowContainerFeatureSetAugment(ZMapWindowContainerFeatureSet container_set,
                                          ZMapWindow window,
                                          GQuark     align_id,
                                          GQuark     block_id,
                                          GQuark     feature_set_unique_id,
                                          GQuark     feature_set_original_id, /* unused! */
                                          ZMapFeatureTypeStyle style,
                                          ZMapStrand strand,
                                          ZMapFrame  frame);
gboolean zmapWindowContainerFeatureSetAttachFeatureSet(ZMapWindowContainerFeatureSet container_set,
						       ZMapFeatureSet feature_set_to_attach);

ZMapWindowFeaturesetItem zmapWindowContainerGetFeatureSetItem(ZMapWindowContainerFeatureSet container_set) ;

/* This function is kind of useless as there may be more than one feature set !!! */
ZMapFeatureSet zmapWindowContainerFeatureSetRecoverFeatureSet(ZMapWindowContainerFeatureSet container_set);

#ifdef blah
ZMapWindowStats zmapWindowContainerFeatureSetRecoverStats(ZMapWindowContainerFeatureSet container_set);
#endif

GList *zmapWindowContainerFeatureSetGetFeatureSets(ZMapWindowContainerFeatureSet container_set);

gboolean zmapWindowContainerHasFeaturesetItem(ZMapWindowContainerFeatureSet container);

/* Style lookup */
ZMapFeatureTypeStyle zMapWindowContainerFeatureSetGetStyle(ZMapWindowContainerFeatureSet container) ;
ZMapFeatureTypeStyle zmapWindowContainerFeatureSetStyleFromStyle(ZMapWindowContainerFeatureSet container_set,
								 ZMapFeatureTypeStyle         style2copy);
ZMapFeatureTypeStyle zmapWindowContainerFeatureSetStyleFromID(ZMapWindowContainerFeatureSet container_set,
							      GQuark                       style_unique_id);

char *zmapWindowContainerFeaturesetGetColumnName(ZMapWindowContainerFeatureSet container_set) ;
GQuark zmapWindowContainerFeaturesetGetColumnId(ZMapWindowContainerFeatureSet container_set) ;
char *zmapWindowContainerFeaturesetGetColumnUniqueName(ZMapWindowContainerFeatureSet container_set) ;
GQuark zmapWindowContainerFeaturesetGetColumnUniqueId(ZMapWindowContainerFeatureSet container_set) ;

/* style properties for the whole column,  */
ZMapStrand zmapWindowContainerFeatureSetGetStrand(ZMapWindowContainerFeatureSet container_set);
ZMapFrame zmapWindowContainerFeatureSetGetFrame(ZMapWindowContainerFeatureSet container_set);
double zmapWindowContainerFeatureSetGetWidth(ZMapWindowContainerFeatureSet container_set);
double zmapWindowContainerFeatureGetBumpSpacing(ZMapWindowContainerFeatureSet container_set);
gboolean zmapWindowContainerFeatureSetGetMagValues(ZMapWindowContainerFeatureSet container_set,
                                                   double *min_mag_out, double *max_mag_out);
ZMapStyleBumpMode zmapWindowContainerFeatureSetGetBumpUnmarked(ZMapWindowContainerFeatureSet container_set) ;
gboolean zMapWindowContainerFeatureSetSetBumpMode(ZMapWindowContainerFeatureSet container_set,
                                                  ZMapStyleBumpMode bump_mode);

ZMapStyleColumnDisplayState zmapWindowContainerFeatureSetGetDisplay(ZMapWindowContainerFeatureSet container_set);
void zmapWindowContainerFeatureSetSetDisplay(ZMapWindowContainerFeatureSet container_set,
					     ZMapStyleColumnDisplayState state,
                                             ZMapWindow window) ;

/* this one sets a style which is probably wrong.... */
void zmapWindowContainerFeatureSetDisplay(ZMapWindowContainerFeatureSet container_set,
					  ZMapStyleColumnDisplayState state);

ZMapWindow zMapWindowContainerFeatureSetGetWindow(ZMapWindowContainerFeatureSet container_set);

void zMapWindowContainerFeatureSetColumnHide(ZMapWindow window, GQuark column_id) ;
void zMapWindowContainerFeatureSetColumnShow(ZMapWindow window, GQuark column_id) ;

ZMapStyleBumpMode zMapWindowContainerFeatureSetGetContainerBumpMode(ZMapWindowContainerFeatureSet container_set);

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

void zMapWindowContainerFeatureSetShowHideMaskedFeatures(ZMapWindowContainerFeatureSet container,
                                                         gboolean show, gboolean set_colour);

gboolean zMapWindowContainerFeatureSetIsFilterable(ZMapWindowContainerFeatureSet container_set) ;
ZMapWindowContainerFilterType zMapWindowContainerFeatureSetGetSelectedType(ZMapWindowContainerFeatureSet container_set) ;
ZMapWindowContainerFilterType zMapWindowContainerFeatureSetGetFilterType(ZMapWindowContainerFeatureSet container_set) ;
ZMapWindowContainerActionType zMapWindowContainerFeatureSetGetActionType(ZMapWindowContainerFeatureSet container_set) ;
gboolean zMapWindowContainerFeatureSetIsCDSMatch(ZMapWindowContainerFeatureSet container_set) ;
ZMapWindowContainerFilterRC zMapWindowContainerFeatureSetFilterFeatures(ZMapWindowContainerFilterType selected_type,
                                                                        ZMapWindowContainerFilterType filter_type,
                                                                        ZMapWindowContainerActionType filter_action,
                                                                        ZMapWindowContainerTargetType filter_target,
                                                                        ZMapWindowContainerFeatureSet filter_column,
                                                                        GList *filter_features,
                                                                        ZMapWindowContainerFeatureSet target_column,
                                                                        int seq_start, int seq_end,
                                                                        gboolean cds_match) ;


/* hidden stack code */
void zmapWindowContainerFeatureSetFeatureRemove(ZMapWindowContainerFeatureSet item_feature_set,
						ZMapFeature feature);
GList *zmapWindowContainerFeatureSetPopHiddenStack(ZMapWindowContainerFeatureSet container_set);
void zmapWindowContainerFeatureSetPushHiddenStack(ZMapWindowContainerFeatureSet container_set,
						  GList *hidden_items_list);

void zmapWindowContainerFeatureSetRemoveAllItems(ZMapWindowContainerFeatureSet container_set);

void zmapWindowContainerFeatureSetSortFeatures(ZMapWindowContainerFeatureSet container_set,
					       gint direction);

gboolean zmapWindowContainerFeatureSetItemLowerToMiddle(ZMapWindowContainerFeatureSet container_set,
                                                        ZMapWindowCanvasItem item,int n_focus,int direction);

void zMapWindowContainerFeatureSetMarkUnsorted(ZMapWindowContainerFeatureSet container_set);

/* Finished with this container */
ZMapWindowContainerFeatureSet zMapWindowContainerFeatureSetDestroy(ZMapWindowContainerFeatureSet canvas_item);

gboolean zmapWindowStyleListGetSetting(GList *list_of_styles,char *setting_name,GValue *value_in_out);


void zMapWindowContainerFeatureSetRemoveSubFeatures(ZMapWindowContainerFeatureSet container_set) ;
void zMapWindowContainerFeatureSetAddColinearMarkers(ZMapWindowContainerFeatureSet container_set,
						     GList *feature_list,
						     ZMapFeatureCompareFunc compare_func,
						     gpointer compare_data, int block_offset) ;

void zMapWindowContainerFeatureSetAddIncompleteMarkers(ZMapWindowContainerFeatureSet container_set,
						       GList *feature_list, gboolean revcomped_features,
                                                       int block_offset) ;
void zMapWindowContainerFeatureSetAddSpliceMarkers(ZMapWindowContainerFeatureSet container_set,
                                                   GList *feature_list, int block_offset) ;


gboolean zMapWindowContainerFeatureSetHasHiddenBumpFeatures(FooCanvasItem *feature_item) ;
gboolean zMapWindowContainerFeatureSetIsUserHidden(FooCanvasItem *feature_item) ;
  gboolean zMapWindowContainerFeatureSetIsVisible(FooCanvasItem *feature_item) ;


#endif /* ZMAP_WINDOW_CONTAINER_FEATURESET_H */