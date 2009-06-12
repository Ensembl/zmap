/*  File: zmapWindowItem.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions to manipulate canvas items.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Jun 12 09:45 2009 (rds)
 * Created: Thu Sep  8 10:37:24 2005 (edgrif)
 * CVS info:   $Id: zmapWindowItem.c,v 1.114 2009-06-12 12:50:25 rds Exp $
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowItemTextFillColumn.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerFeatureSet_I.h>

/* Used to hold highlight information for the hightlight callback function. */
typedef struct
{
  ZMapWindow window ;
  ZMapWindowMark mark ;
  gboolean highlight ;
} HighlightStruct, *Highlight ;

typedef struct
{
  int start, end;
  FooCanvasItem *item;
}StartEndTextHighlightStruct, *StartEndTextHighlight;

typedef struct
{
  double x1, y1, x2, y2;
} AreaStruct;

typedef struct
{
  ZMapWindow window;
  FooCanvasGroup *block;
  int seq_x, seq_y;
  double wx1, wx2, wy1, wy2;
  gboolean result;
} get_item_at_workaround_struct, *get_item_at_workaround;

static gboolean simple_highlight_region(FooCanvasPoints **points_out, 
                                        FooCanvasItem    *subject, 
                                        gpointer          user_data);

static void highlightCB(gpointer data, gpointer user_data) ;
static void unhighlightCB(gpointer data, gpointer user_data) ;

static void highlightItem(ZMapWindow window, FooCanvasItem *item, gboolean highlight) ;


static gint sortByPositionCB(gconstpointer a, gconstpointer b) ;
static void extract_feature_from_item(gpointer list_data, gpointer user_data);

static void getVisibleCanvas(ZMapWindow window,
			     double *screenx1_out, double *screeny1_out,
			     double *screenx2_out, double *screeny2_out) ;

static FooCanvasItem *translation_item_from_block_frame(ZMapWindow window, char *column_name, 
							gboolean require_visible,
							ZMapFeatureBlock block, ZMapFrame frame) ;

static void fill_workaround_struct(ZMapWindowContainerGroup container, 
				   FooCanvasPoints         *points,
				   ZMapContainerLevelType   level,
				   gpointer                 user_data);

static gboolean areas_intersection(AreaStruct *area_1, AreaStruct *area_2, AreaStruct *intersect);
static gboolean areas_intersect_gt_threshold(AreaStruct *area_1, AreaStruct *area_2, double threshold);

#ifdef INTERSECTION_CODE
static gboolean foo_canvas_items_get_intersect(FooCanvasItem *i1, FooCanvasItem *i2, FooCanvasPoints **points_out);
static gboolean foo_canvas_items_intersect(FooCanvasItem *i1, FooCanvasItem *i2, double threshold);
#endif

/* This looks like something we will want to do often.... */
GList *zmapWindowItemSortByPostion(GList *feature_item_list)
{
  GList *sorted_list = NULL ;

  if (feature_item_list)
    sorted_list = g_list_sort(feature_item_list, sortByPositionCB) ;

  return sorted_list ;
}


gboolean zmapWindowItemGetStrandFrame(FooCanvasItem *item, ZMapStrand *set_strand, ZMapFrame *set_frame)
{                                               
  ZMapWindowContainerFeatureSet container;
  ZMapFeature feature ;
  gboolean result = FALSE ;

  /* Retrieve the feature item info from the canvas item. */
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature && feature->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;

  container = (ZMapWindowContainerFeatureSet)zmapWindowContainerCanvasItemGetContainer(item) ;

  *set_strand = container->strand ;
  *set_frame  = container->frame ;

  result = TRUE ;

  return result ;
}

GList *zmapWindowItemListToFeatureList(GList *item_list)
{
  GList *feature_list = NULL;

  g_list_foreach(item_list, extract_feature_from_item, &feature_list);

  return feature_list;
}


/* 
   (TYPE)zmapWindowItemGetFeatureAny(ITEM, STRUCT_TYPE)

#define zmapWindowItemGetFeatureSet(ITEM)   (ZMapFeatureContext)zmapWindowItemGetFeatureAnyType((ITEM), ZMAPFEATURE_STRUCT_CONTEXT)
#define zmapWindowItemGetFeatureAlign(ITEM) (ZMapFeatureAlign)zmapWindowItemGetFeatureAnyType((ITEM), ZMAPFEATURE_STRUCT_ALIGN)
#define zmapWindowItemGetFeatureBlock(ITEM) (ZMapFeatureBlock)zmapWindowItemGetFeatureAnyType((ITEM), ZMAPFEATURE_STRUCT_BLOCK)
#define zmapWindowItemGetFeatureSet(ITEM)   (ZMapFeatureSet)zmapWindowItemGetFeatureAnyType((ITEM), ZMAPFEATURE_STRUCT_FEATURESET)
#define zmapWindowItemGetFeature(ITEM)      (ZMapFeature)zmapWindowItemGetFeatureAnyType((ITEM), ZMAPFEATURE_STRUCT_FEATURE)
#define zmapWindowItemGetFeatureAny(ITEM)   zmapWindowItemGetFeatureAnyType((ITEM), -1)
 */
#ifdef RDS_IS_MACRO_TOO
ZMapFeatureAny zmapWindowItemGetFeatureAny(FooCanvasItem *item)
{
  ZMapFeatureAny feature_any;
  /* -1 means don't check */
  feature_any = zmapWindowItemGetFeatureAnyType(item, -1);

  return feature_any;
}
#endif

ZMapFeatureAny zmapWindowItemGetFeatureAnyType(FooCanvasItem *item, ZMapFeatureStructType expected_type)
{
  ZMapFeatureAny feature_any = NULL;

  if(ZMAP_IS_CONTAINER_GROUP(item))
    {
      zmapWindowContainerGetFeatureAny((ZMapWindowContainerGroup)item, &feature_any);
    }
  else if(ZMAP_IS_CANVAS_ITEM(item))
    {
      feature_any = (ZMapFeatureAny)zMapWindowCanvasItemGetFeature((ZMapWindowCanvasItem)item);
    }
  else
    {
      zMapLogMessage("Unexpected item [%s]", G_OBJECT_TYPE_NAME(item));
    }

  if(expected_type != -1)
    {
      if(feature_any && feature_any->struct_type != expected_type)
	{
	  zMapLogCritical("Unexpected feature type [%d] attached to item [%s]",
			  feature_any->struct_type, G_OBJECT_TYPE_NAME(item));
	  feature_any = NULL;
	}
    }

  return feature_any;
}

/*
 *                     Feature Item highlighting.... 
 */


/* Highlight a feature or list of related features (e.g. all hits for same query sequence).
 * 
 *  */
void zMapWindowHighlightObject(ZMapWindow window, FooCanvasItem *item,
			       gboolean replace_highlight_item, gboolean highlight_same_names)
{                                               
  zmapWindowHighlightObject(window, item, replace_highlight_item, highlight_same_names) ;

  return ;
}

typedef struct
{
  ZMapWindow window;
  gboolean multiple_select;
  gint highlighted;
  gint feature_count;
}HighlightContextStruct, *HighlightContext;

static ZMapFeatureContextExecuteStatus highlight_feature(GQuark key, gpointer data, gpointer user_data, char **error_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  HighlightContext highlight_data = (HighlightContext)user_data;
  ZMapFeature feature_in, feature_current;
  FooCanvasItem *feature_item;
  gboolean replace_highlight = TRUE;

  if(feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      feature_in = (ZMapFeature)feature_any;
      if(highlight_data->multiple_select || highlight_data->highlighted == 0)
        {
          replace_highlight = !(highlight_data->multiple_select);
          
          if((feature_item = zmapWindowFToIFindFeatureItem(highlight_data->window->context_to_item,
                                                           feature_in->strand, ZMAPFRAME_NONE,
                                                           feature_in)))
            {
              if(highlight_data->multiple_select)
                replace_highlight = !(zmapWindowFocusIsItemInHotColumn(highlight_data->window->focus, feature_item));
              
              feature_current = zmapWindowItemGetFeature(feature_item);
              zMapAssert(feature_current);

              if(feature_in->type == ZMAPSTYLE_MODE_TRANSCRIPT && 
                 feature_in->feature.transcript.exons->len > 0 &&
                 feature_in->feature.transcript.exons->len != feature_current->feature.transcript.exons->len)
                {
                  ZMapSpan span;
                  int i;
                  
                  /* need to do something to find sub feature items??? */
                  for(i = 0; i < feature_in->feature.transcript.exons->len; i++)
                    {
                      if(replace_highlight && i > 0)
                        replace_highlight = FALSE;

                      span = &g_array_index(feature_in->feature.transcript.exons, ZMapSpanStruct, i) ;

                      if((feature_item = zmapWindowFToIFindItemChild(highlight_data->window->context_to_item,
                                                                     feature_in->strand, ZMAPFRAME_NONE,
                                                                     feature_in, span->x1, span->x2)))
                        zmapWindowHighlightObject(highlight_data->window, feature_item,
						  replace_highlight, TRUE) ;
                    }
                  for(i = 0; i < feature_in->feature.transcript.introns->len; i++)
                    {
                      span = &g_array_index(feature_in->feature.transcript.introns, ZMapSpanStruct, i) ;

                      if((feature_item = zmapWindowFToIFindItemChild(highlight_data->window->context_to_item,
                                                                     feature_in->strand, ZMAPFRAME_NONE,
                                                                     feature_in, span->x1, span->x2)))
                        zmapWindowHighlightObject(highlight_data->window, feature_item,
						  replace_highlight, TRUE);
                    }

                  replace_highlight = !(highlight_data->multiple_select);
                }
              else
                /* we need to highlight the full feature */
                zmapWindowHighlightObject(highlight_data->window, feature_item, replace_highlight, TRUE);
              
              if(replace_highlight)
                highlight_data->highlighted = 0;
              else
                highlight_data->highlighted++;
            }
        }
      highlight_data->feature_count++;
    }

  return status;
}

void zMapWindowHighlightObjects(ZMapWindow window, ZMapFeatureContext context, gboolean multiple_select)
{
  HighlightContextStruct highlight_data = {NULL};

  highlight_data.window          = window;
  highlight_data.multiple_select = multiple_select;
  highlight_data.highlighted     = 
    highlight_data.feature_count = 0;

  zMapFeatureContextExecute((ZMapFeatureAny)context, ZMAPFEATURE_STRUCT_FEATURE,
                            highlight_feature, &highlight_data);

  return ;
}

void zmapWindowHighlightObject(ZMapWindow window, FooCanvasItem *item,
			       gboolean replace_highlight_item, gboolean highlight_same_names)
{                                               
  ZMapFeature feature ;
  ZMapWindowItemFeatureType item_feature_type ;
  GList *set_items ;
  FooCanvasItem *dna_item, *framed_3ft;

  /* Retrieve the feature item info from the canvas item. */
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),	ITEM_FEATURE_TYPE)) ;


  /* If any other feature(s) is currently in focus, revert it to its std colours */
  if (replace_highlight_item)
    zMapWindowUnHighlightFocusItems(window) ;


  /* For some types of feature we want to highlight all the ones with the same name in that column. */
  switch (feature->type)
    {
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
	if (highlight_same_names)
	  {
	    char *set_strand, *set_frame ;
	
	    set_strand = set_frame = "*" ;

	    set_items = zmapWindowFToIFindSameNameItems(window->context_to_item, set_strand, set_frame, feature) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    if (set_items)
	      zmapWindowFToIPrintList(set_items) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	    zmapWindowFocusAddItems(window->focus, set_items);

	    /* We set FALSE here so that the hot item is _not_ unfocussed as before. */
	    zmapWindowFocusSetHotItem(window->focus, item, FALSE) ;
	  }
	else
	  zmapWindowFocusAddItem(window->focus, item);

	break ;
      }
    default:
      {
	/* Try highlighting both the item and its column. */
        zmapWindowFocusAddItem(window->focus, item);
      }
      break ;
    }

  zmapWindowFocusClearOverlayManagers(window->focus);

  if((dna_item = zmapWindowItemGetDNAItem(window, item)))
    {
      ZMapWindowOverlay overlay_manager;
      FooCanvasGroup *container;

      container = (FooCanvasGroup *)zmapWindowContainerCanvasItemGetContainer(dna_item);

      if((overlay_manager = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_OVERLAY_DATA)))
        {
          zmapWindowOverlaySetLimitItem(overlay_manager, NULL);
          zmapWindowFocusAddOverlayManager(window->focus, overlay_manager);
        }
    }

  {
    int frame_itr;
    for(frame_itr = ZMAPFRAME_0; frame_itr < ZMAPFRAME_2 + 1; frame_itr++)
      {
	if((framed_3ft = zmapWindowItemGetTranslationItemFromItemFrame(window, item, frame_itr)))
	  {
	    ZMapWindowOverlay overlay_manager;
	    FooCanvasGroup *container;
	    
	    container = (FooCanvasGroup *)zmapWindowContainerCanvasItemGetContainer(framed_3ft);
	    
	    if((overlay_manager = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_OVERLAY_DATA)))
	      {
		zmapWindowOverlaySetLimitItem(overlay_manager, framed_3ft);

		zmapWindowOverlaySetSubTypeMask(overlay_manager, ZMAPFEATURE_SUBPART_EXON_CDS | ZMAPFEATURE_SUBPART_EXON | ZMAPFEATURE_SUBPART_MATCH);
		
		zmapWindowFocusAddOverlayManager(window->focus, overlay_manager);
	      }
	  }
      }
  }

  zMapWindowHighlightFocusItems(window);

  return ;
}

void zMapWindowHighlightFocusItems(ZMapWindow window)
{                                               
  FooCanvasItem *hot_item ;
  FooCanvasGroup *hot_column = NULL;

  /* If any other feature(s) is currently in focus, revert it to its std colours */
  if ((hot_column = zmapWindowFocusGetHotColumn(window->focus)))
    zmapHighlightColumn(window, hot_column) ;

  if ((hot_item = zmapWindowFocusGetHotItem(window->focus)))
    zmapWindowFocusForEachFocusItem(window->focus, highlightCB, window) ;

  return ;
}



void zMapWindowUnHighlightFocusItems(ZMapWindow window)
{                                               
  FooCanvasItem *hot_item ;
  FooCanvasGroup *hot_column ;

  /* If any other feature(s) is currently in focus, revert it to its std colours */
  if ((hot_column = zmapWindowFocusGetHotColumn(window->focus)))
    zmapUnHighlightColumn(window, hot_column) ;

  if ((hot_item = zmapWindowFocusGetHotItem(window->focus)))
    zmapWindowFocusForEachFocusItem(window->focus, unhighlightCB, window) ;

  if (hot_column || hot_item)
    zmapWindowFocusReset(window->focus) ;    

  return ;
}




/* highlight/unhiglight cols. */
void zmapHighlightColumn(ZMapWindow window, FooCanvasGroup *column)
{
  zmapWindowContainerGroupSetBackgroundColour(ZMAP_CONTAINER_GROUP(column), &(window->colour_column_highlight)) ;

  return ;
}


void zmapUnHighlightColumn(ZMapWindow window, FooCanvasGroup *column)
{
  zmapWindowContainerGroupResetBackgroundColour(ZMAP_CONTAINER_GROUP(column)) ;

  return ;
}





gboolean zmapWindowItemIsCompound(FooCanvasItem *item)
{
  gboolean result = FALSE ;

  if (FOO_IS_CANVAS_GROUP(item))
    {
      ZMapWindowItemFeatureType item_feature_type ;
      
      /* Retrieve the feature item info from the canvas item. */
      item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
      zMapAssert(item_feature_type == ITEM_FEATURE_PARENT) ;

      result = TRUE ;
    }

  return result ;
}


/* Get "parent" item of feature, for simple features, this is just the item itself but
 * for compound features we need the parent group.
 *  */
FooCanvasItem *zmapWindowItemGetTrueItem(FooCanvasItem *item)
{
  FooCanvasItem *true_item = NULL ;
  ZMapWindowItemFeatureType item_feature_type ;

  /* Retrieve the feature item info from the canvas item. */
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							ITEM_FEATURE_TYPE)) ;

  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    true_item = item ;
  else if (item_feature_type == ITEM_FEATURE_CHILD || item_feature_type == ITEM_FEATURE_BOUNDING_BOX)
    true_item = item->parent ;

  return true_item ;
}


/* Get nth child of a compound feature (e.g. transcript, alignment etc), index starts
 * at zero for the first child. */
FooCanvasItem *zmapWindowItemGetNthChild(FooCanvasGroup *compound_item, int child_index)
{
  FooCanvasItem *nth_item = NULL ;

  if (FOO_IS_CANVAS_GROUP(compound_item))
    {
      ZMapWindowItemFeatureType item_feature_type ;
      GList *nth ;
      
      /* Retrieve the feature item info from the canvas item. */
      item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(compound_item), ITEM_FEATURE_TYPE)) ;
      zMapAssert(item_feature_type == ITEM_FEATURE_PARENT) ;

      if ((nth = g_list_nth(compound_item->item_list, child_index)))
	nth_item = (FooCanvasItem *)(nth->data) ;
    }

  return nth_item ;
}

/* Need to test whether this works for groups...it should do....
 * 
 * For simple features or the parent of a compound feature the raise is done on the item
 * directly, for compound objects we want to raise the parent so that the whole item
 * is still raised.
 *  */
void zmapWindowRaiseItem(FooCanvasItem *item)
{
  ZMapWindowItemFeatureType item_feature_type ;

  /* Retrieve the feature item info from the canvas item. */
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							ITEM_FEATURE_TYPE)) ;


  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    foo_canvas_item_raise_to_top(item) ;
  else if (item_feature_type == ITEM_FEATURE_CHILD || item_feature_type == ITEM_FEATURE_BOUNDING_BOX)
    {
      foo_canvas_item_raise_to_top(item->parent) ;
    }

  return ;
}




/* Returns the container parent of the given canvas item which may not be feature, it might be
   some decorative box, e.g. as in the alignment colinear bars. */
FooCanvasGroup *zmapWindowItemGetParentContainer(FooCanvasItem *feature_item)
{
  FooCanvasGroup *parent_container = NULL ;

  parent_container = (FooCanvasGroup *)zmapWindowContainerCanvasItemGetContainer(feature_item);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapWindowItemFeatureType item_feature_type ;
  FooCanvasItem *parent_item = NULL ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(feature_item),
							ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type != ITEM_FEATURE_INVALID) ;

  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    {
      parent_item = feature_item ;
    }
  else
    {
      parent_item = feature_item->parent ;
    }


  /* It's possible for us to be called when we have no parent, e.g. when this routine is
   * called as a result of a GtkDestroy on one of our parents. */
  if (parent_item->parent)
    {
      parent_container = zmapWindowContainerGetParent(parent_item->parent) ;
      zMapAssert(parent_container) ;
    }

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return parent_container ;
}

FooCanvasItem *zmapWindowItemGetDNAParentItem(ZMapWindow window, FooCanvasItem *item)
{
  ZMapFeature feature;
  ZMapFeatureBlock block = NULL;
  FooCanvasItem *dna_item = NULL;
  GQuark feature_set_unique = 0, dna_id = 0;
  char *feature_name = NULL;

  feature_set_unique = zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME);

  if((feature = zmapWindowItemGetFeature(item)))
    {
      if((block = (ZMapFeatureBlock)(zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_BLOCK))) && 
         (feature_name = zMapFeatureDNAFeatureName(block)))
        {
          dna_id = zMapFeatureCreateID(ZMAPSTYLE_MODE_RAW_SEQUENCE, 
                                       feature_name, 
                                       ZMAPSTRAND_FORWARD, /* ALWAYS FORWARD */
                                       block->block_to_sequence.q1,
                                       block->block_to_sequence.q2,
                                       0,0);
          g_free(feature_name);
        }
      
      if((dna_item = zmapWindowFToIFindItemFull(window->context_to_item,
						block->parent->unique_id,
						block->unique_id,
						feature_set_unique,
						ZMAPSTRAND_FORWARD, /* STILL ALWAYS FORWARD */
						ZMAPFRAME_NONE,/* NO STRAND */
						dna_id)))
	{
	  if(!(FOO_CANVAS_ITEM(dna_item)->object.flags & FOO_CANVAS_ITEM_VISIBLE))
	    dna_item = NULL;
	}
    }
  else
    {
      zMapAssertNotReached();
    }

  return dna_item;
}

FooCanvasItem *zmapWindowItemGetDNATextItem(ZMapWindow window, FooCanvasItem *item)
{
  FooCanvasItem *dna_item = NULL;

  if ((dna_item = zmapWindowItemGetDNAParentItem(window, item))
      && FOO_IS_CANVAS_GROUP(dna_item)
      && FOO_CANVAS_GROUP(dna_item)->item_list)
    {
      dna_item = FOO_CANVAS_ITEM(FOO_CANVAS_GROUP(dna_item)->item_list->data);
  
      if (!FOO_IS_CANVAS_ZMAP_TEXT(dna_item))
	dna_item = NULL;
      else if (!(FOO_CANVAS_ITEM(dna_item)->object.flags & FOO_CANVAS_ITEM_VISIBLE))
	dna_item = NULL;  
    }

  return dna_item;
}

FooCanvasItem *zmapWindowItemGetDNAItem(ZMapWindow window, FooCanvasItem *item)
{
  /* lazy! */
  return zmapWindowItemGetDNATextItem(window, item);
}

/* highlights the dna given any foocanvasitem (with a feature) and a start and end */
/* This _only_ highlights in the current window! */
void zmapWindowItemHighlightDNARegion(ZMapWindow window, 
				      FooCanvasItem *item, 
				      int region_start, 
				      int region_end)
{
  FooCanvasItem *dna_item = NULL;

  if((dna_item = zmapWindowItemGetDNAItem(window, item)))
    {
      ZMapWindowOverlay overlay_manager;
      FooCanvasGroup *container;
      
      container = (FooCanvasGroup *)zmapWindowContainerCanvasItemGetContainer(dna_item);

      /* Check the column is visible, otherwise failure is imminent. */
      if((FOO_CANVAS_ITEM(container)->object.flags & FOO_CANVAS_ITEM_VISIBLE) &&
	 (overlay_manager = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_OVERLAY_DATA)))
        {
          StartEndTextHighlightStruct data = {0};

          data.start = region_start;
          data.end   = region_end;
	  data.item  = dna_item;

          zmapWindowOverlayUnmaskAll(overlay_manager);
          if(window->highlights_set.item)
            zmapWindowOverlaySetGdkColorFromGdkColor(overlay_manager, &(window->colour_item_highlight));
          zmapWindowOverlaySetLimitItem(overlay_manager, NULL);
          zmapWindowOverlaySetSubject(overlay_manager, item);
          zmapWindowOverlayMaskFull(overlay_manager, simple_highlight_region, &data);
        }
      
    }

  return ;
}

FooCanvasItem *zmapWindowItemGetTranslationItemFromItemFrame(ZMapWindow window, FooCanvasItem *item, ZMapFrame frame)
{
  ZMapFeatureBlock block;
  ZMapFeature feature;
  FooCanvasItem *translation = NULL;

  if((feature = zmapWindowItemGetFeature(item)))
    {
      /* First go up to block... */
      block = (ZMapFeatureBlock)
        (zMapFeatureGetParentGroup((ZMapFeatureAny)(feature), 
                                   ZMAPFEATURE_STRUCT_BLOCK));
      zMapAssert(block);

      /* Get the frame for the item... and its translation feature (ITEM_FEATURE_PARENT!) */
      translation = translation_item_from_block_frame(window, ZMAP_FIXED_STYLE_3FT_NAME, TRUE, block, frame);
    }
  else
    {
      zMapAssertNotReached();
    }

  return translation;
}

void zmapWindowItemHighlightRegionTranslations(ZMapWindow window, FooCanvasItem *item, 
					       int region_start, int region_end)
{
  int frame;

  for(frame = ZMAPFRAME_0; frame < ZMAPFRAME_2 + 1; frame++)
    {
      zmapWindowItemHighlightTranslationRegion(window, item, frame, region_start, region_end);
    }

  return ;
}

/* highlights the translation given any foocanvasitem (with a
 * feature), frame and a start and end (protein seq coords) */
/* This _only_ highlights in the current window! */
void zmapWindowItemHighlightTranslationRegion(ZMapWindow window, FooCanvasItem *item, 
					      ZMapFrame required_frame,
					      int region_start, int region_end)
{
  FooCanvasItem *translation_item = NULL;

  if((translation_item = zmapWindowItemGetTranslationItemFromItemFrame(window, item, required_frame)))
    {
      ZMapWindowOverlay overlay_manager;
      FooCanvasGroup *container;
      
      container = (FooCanvasGroup *)zmapWindowContainerCanvasItemGetContainer(translation_item);
      
      if((overlay_manager = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_OVERLAY_DATA)))
        {
          StartEndTextHighlightStruct data = {0};

	  data.item = translation_item ;
          data.start   = region_start * 3 ;
          data.end     = region_end * 3 ;

          zmapWindowOverlayUnmaskAll(overlay_manager);
          if(window->highlights_set.item)
            zmapWindowOverlaySetGdkColorFromGdkColor(overlay_manager, &(window->colour_item_highlight));
          zmapWindowOverlaySetLimitItem(overlay_manager, NULL);
          zmapWindowOverlaySetSubject(overlay_manager, item);
          zmapWindowOverlayMaskFull(overlay_manager, simple_highlight_region, &data);
        }
      
    }

  return ;
}

ZMapFrame zmapWindowItemFeatureFrame(FooCanvasItem *item)
{
  ZMapWindowItemFeature item_subfeature_data ;
  ZMapFeature feature;
  ZMapFrame frame = ZMAPFRAME_NONE;

  if((feature = zmapWindowItemGetFeature(item)))
    {
      frame = zmapWindowFeatureFrame(feature);

      if((item_subfeature_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA)))
        {
          int diff = item_subfeature_data->start - feature->x1;
          int sub_frame = diff % 3;
          frame -= ZMAPFRAME_0;
          frame += sub_frame;
          frame  = (frame % 3) + ZMAPFRAME_0;
        }
    }

  return frame;
}

FooCanvasGroup *zmapWindowItemGetTranslationColumnFromBlock(ZMapWindow window, ZMapFeatureBlock block)
{
  ZMapFeatureSet feature_set;
  FooCanvasItem *translation;
  GQuark feature_set_id;

  feature_set_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME);
  /* and look up the translation feature set with ^^^ */
  feature_set  = zMapFeatureBlockGetSetByID(block, feature_set_id);

  translation  = zmapWindowFToIFindSetItem(window->context_to_item,
                                           feature_set,
                                           ZMAPSTRAND_FORWARD, /* STILL ALWAYS FORWARD */
                                           ZMAPFRAME_NONE);
  

  return FOO_CANVAS_GROUP(translation);
}


FooCanvasItem *zmapWindowItemGetShowTranslationColumn(ZMapWindow window, FooCanvasItem *item)
{
  FooCanvasItem *translation = NULL;
  ZMapFeature feature;
  ZMapFeatureBlock block;

  if ((feature = zmapWindowItemGetFeature(item)))
    {
      ZMapFeatureSet feature_set;
      ZMapFeatureTypeStyle style;

      /* First go up to block... */
      block = (ZMapFeatureBlock)(zMapFeatureGetParentGroup((ZMapFeatureAny)(feature), ZMAPFEATURE_STRUCT_BLOCK));
      zMapAssert(block);

      /* Get the frame for the item... and its translation feature (ITEM_FEATURE_PARENT!) */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if ((style = zMapFeatureContextFindStyle((ZMapFeatureContext)(block->parent->parent),
					       ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME))
	  && !(feature_set = zMapFeatureBlockGetSetByID(block,
							zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME))))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	if ((style = zMapFindStyle(window->read_only_styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME)))
	    && !(feature_set = zMapFeatureBlockGetSetByID(block,
							  zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME))))

	{
	  /* Feature set doesn't exist, so create. */
	  feature_set = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME, NULL);
	  //feature_set->style = style;
	  zMapFeatureBlockAddFeatureSet(block, feature_set);
	}
      
      if (feature_set)
	{
	  ZMapWindowContainerGroup parent_container;
	  ZMapWindowContainerStrand forward_container;
	  ZMapWindowContainerFeatures forward_features;
	  FooCanvasGroup *tmp_forward, *tmp_reverse;

#ifdef SIMPLIER
	  FooCanvasGroup *forward_group, *parent_group, *tmp_forward, *tmp_reverse ;
	  /* Get the FeatureSet Level Container */
	  parent_group = zmapWindowContainerCanvasItemGetContainer(item);
	  /* Get the Strand Level Container (Could be Forward OR Reverse) */
	  parent_group = zmapWindowContainerGetSuperGroup(parent_group);
	  /* Get the Block Level Container... */
	  parent_group = zmapWindowContainerGetSuperGroup(parent_group);
#endif /* SIMPLIER */

	  parent_container = zmapWindowContainerUtilsItemGetParentLevel(item, ZMAPCONTAINER_LEVEL_BLOCK);
	  
	  /* Get the Forward Group Parent Container... */
	  forward_container = zmapWindowContainerBlockGetContainerStrand((ZMapWindowContainerBlock)parent_container, ZMAPSTRAND_FORWARD);
	  /* zmapWindowCreateSetColumns needs the Features not the Parent. */
	  forward_features  = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)forward_container);
	  
	  /* make the column... */
	  if (zmapWindowCreateSetColumns(window,
					 forward_features,
					 NULL,
					 block,
					 feature_set,
					 window->read_only_styles,
					 ZMAPFRAME_NONE,
					 &tmp_forward, &tmp_reverse, NULL))
	    {
	      translation = FOO_CANVAS_ITEM(tmp_forward);
	    }
	}
      else
	zMapLogWarning("Failed to find Feature Set for '%s'", ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME);
    }
  else
    zMapAssertNotReached();

  return translation;
}

static FooCanvasItem *translation_item_from_block_frame(ZMapWindow window, char *column_name, 
							gboolean require_visible,
							ZMapFeatureBlock block, ZMapFrame frame)
{
  FooCanvasItem *translation = NULL;
  GQuark feature_set_id, feature_id;
  ZMapFeatureSet feature_set;
  ZMapStrand strand = ZMAPSTRAND_FORWARD;

  feature_set_id = zMapStyleCreateID(column_name);
  /* and look up the translation feature set with ^^^ */

  if((feature_set = zMapFeatureBlockGetSetByID(block, feature_set_id)))
    {
      char *feature_name;

      /* Get the name of the framed feature... */
      feature_name = zMapFeature3FrameTranslationFeatureName(feature_set, frame);
      /* ... and its quark id */
      feature_id   = g_quark_from_string(feature_name);

      if((translation  = zmapWindowFToIFindItemFull(window->context_to_item,
						    block->parent->unique_id,
						    block->unique_id,
						    feature_set_id,
						    strand, /* STILL ALWAYS FORWARD */
						    frame,
						    feature_id)))
	{
	  if(require_visible && !(FOO_CANVAS_ITEM(translation)->object.flags & FOO_CANVAS_ITEM_VISIBLE))
	    translation = NULL;
	  else
	    translation = FOO_CANVAS_GROUP(translation)->item_list->data;
	}

      g_free(feature_name);
    }

  return translation;
}

FooCanvasItem *zmapWindowItemGetTranslationItemFromItem(ZMapWindow window, FooCanvasItem *item)
{
  ZMapFeatureBlock block;
  ZMapFeature feature;
  FooCanvasItem *translation = NULL;

  if((feature = zmapWindowItemGetFeature(item)))
    {
      /* First go up to block... */
      block = (ZMapFeatureBlock)
        (zMapFeatureGetParentGroup((ZMapFeatureAny)(feature), 
                                   ZMAPFEATURE_STRUCT_BLOCK));
      zMapAssert(block);

      /* Get the frame for the item... and its translation feature (ITEM_FEATURE_PARENT!) */
      translation = translation_item_from_block_frame(window, ZMAP_FIXED_STYLE_3FT_NAME, TRUE,
						      block, zmapWindowItemFeatureFrame(item));
    }
  else
    {
      zMapAssertNotReached();
    }

  return translation;
}



/* Returns a features style. We need this function because we only attach the style
 * to the top item of the feature. It is a fatal error if a feature does not have a
 * style so this function will always return a valid style or it will abort.
 * 
 *  */
ZMapFeatureTypeStyle zmapWindowItemGetStyle(FooCanvasItem *feature_item)
{
  ZMapFeatureTypeStyle style = NULL ;
  FooCanvasItem *parent_item = NULL ;
  ZMapWindowItemFeatureType item_feature_type ;

  parent_item = feature_item;

  style = g_object_get_data(G_OBJECT(parent_item), ITEM_FEATURE_ITEM_STYLE);

  return style;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(feature_item),
							ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type != ITEM_FEATURE_INVALID) ;

  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    {
      parent_item = feature_item ;
    }
  else
    {
      parent_item = feature_item->parent ;
    }

  style = g_object_get_data(G_OBJECT(parent_item), ITEM_FEATURE_ITEM_STYLE) ;
  zMapAssert(style) ;

  return style ;
}







/* Finds the feature item in a window corresponding to the supplied feature item..which is
 * usually one from a different window....
 * 
 * This routine can return NULL if the user has two different sequences displayed and hence
 * there will be items in one window that are not present in another.
 * 
 *  */
FooCanvasItem *zMapWindowFindFeatureItemByItem(ZMapWindow window, FooCanvasItem *item)
{
  FooCanvasItem *matching_item = NULL ;
  ZMapFeature feature ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapWindowContainerFeatureSet container;
  ZMapWindowCanvasItem canvas_item;

  /* Retrieve the feature item info from the canvas item. */
  canvas_item = zMapWindowCanvasItemIntervalGetObject(item);
  feature = zMapWindowCanvasItemGetFeature(canvas_item) ;
  zMapAssert(feature);

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE));

  container = (ZMapWindowContainerFeatureSet)zmapWindowContainerCanvasItemGetContainer(item) ;

  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    {
      matching_item = zmapWindowFToIFindFeatureItem(window->context_to_item,
						    container->strand, container->frame,
						    feature) ;
    }
  else
    {
      ZMapWindowItemFeature item_subfeature_data ;

      item_subfeature_data = (ZMapWindowItemFeature)g_object_get_data(G_OBJECT(item),
								      ITEM_SUBFEATURE_DATA) ;

      matching_item = zmapWindowFToIFindItemChild(window->context_to_item,
						  container->strand, container->frame,
						  feature,
						  item_subfeature_data->start,
						  item_subfeature_data->end) ;
    }

  return matching_item ;
}



/* Finds the feature item child in a window corresponding to the supplied feature item..which is
 * usually one from a different window....
 * A feature item child is something like the feature item representing an exon within a transcript. */
FooCanvasItem *zMapWindowFindFeatureItemChildByItem(ZMapWindow window, FooCanvasItem *item,
						    int child_start, int child_end)
{
  FooCanvasItem *matching_item = NULL ;
  ZMapFeature feature ;
  ZMapWindowContainerFeatureSet container;

  zMapAssert(window && item && child_start > 0 && child_end > 0 && child_start <= child_end) ;


  /* Retrieve the feature item info from the canvas item. */
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;

  container = (ZMapWindowContainerFeatureSet)zmapWindowContainerCanvasItemGetContainer(item) ;

  /* Find the item that matches */
  matching_item = zmapWindowFToIFindFeatureItem(window->context_to_item,
						container->strand, container->frame, feature) ;

  return matching_item ;
}


/* Tests to see whether an item is visible on the screen. If "completely" is TRUE then the
 * item must be completely on the screen otherwise FALSE is returned. */
gboolean zmapWindowItemIsOnScreen(ZMapWindow window, FooCanvasItem *item, gboolean completely)
{
  gboolean is_on_screen = FALSE ;
  double screenx1_out, screeny1_out, screenx2_out, screeny2_out ;
  double itemx1_out, itemy1_out, itemx2_out, itemy2_out ;

  /* Work out which part of the canvas is visible currently. */
  getVisibleCanvas(window, &screenx1_out, &screeny1_out, &screenx2_out, &screeny2_out) ;

  /* Get the items world coords. */
  my_foo_canvas_item_get_world_bounds(item, &itemx1_out, &itemy1_out, &itemx2_out, &itemy2_out) ;

  /* Compare them. */
  if (completely)
    {
      if (itemx1_out >= screenx1_out && itemx2_out <= screenx2_out
	  && itemy1_out >= screeny1_out && itemy2_out <= screeny2_out)
	is_on_screen = TRUE ;
      else
	is_on_screen = FALSE ;
    }
  else
    {
      if (itemx2_out < screenx1_out || itemx1_out > screenx2_out
	  || itemy2_out < screeny1_out || itemy1_out > screeny2_out)
	is_on_screen = FALSE ;
      else
	is_on_screen = TRUE ;
    }

  return is_on_screen ;
}


/* Scrolls to an item if that item is not visible on the scren.
 * 
 * NOTE: scrolling is only done in the axis in which the item is completely
 * invisible, the other axis is left unscrolled so that the visible portion
 * of the feature remains unaltered.
 *  */
void zmapWindowScrollToItem(ZMapWindow window, FooCanvasItem *item)
{
  double screenx1_out, screeny1_out, screenx2_out, screeny2_out ;
  double itemx1_out, itemy1_out, itemx2_out, itemy2_out ;
  gboolean do_x = FALSE, do_y = FALSE ;
  double x_offset = 0.0, y_offset = 0.0;
  int curr_x, curr_y ;

  /* Work out which part of the canvas is visible currently. */
  getVisibleCanvas(window, &screenx1_out, &screeny1_out, &screenx2_out, &screeny2_out) ;

  /* Get the items world coords. */
  my_foo_canvas_item_get_world_bounds(item, &itemx1_out, &itemy1_out, &itemx2_out, &itemy2_out) ;

  /* Get the current scroll offsets in world coords. */
  foo_canvas_get_scroll_offsets(window->canvas, &curr_x, &curr_y) ;
  foo_canvas_c2w(window->canvas, curr_x, curr_y, &x_offset, &y_offset) ;

  /* Work out whether any scrolling is required. */
  if (itemx1_out > screenx2_out || itemx2_out < screenx1_out)
    {
      do_x = TRUE ;

      if (itemx1_out > screenx2_out)
	{
	  x_offset = itemx1_out ;
	}
      else if (itemx2_out < screenx1_out)
	{
	  x_offset = itemx1_out ;
	}
    }

  if (itemy1_out > screeny2_out || itemy2_out < screeny1_out)
    {
      do_y = TRUE ;

      if (itemy1_out > screeny2_out)
	{
	  y_offset = itemy1_out ;
	}
      else if (itemy2_out < screeny1_out)
	{
	  y_offset = itemy1_out ;
	}
    }

  /* If we need to scroll then do it. */
  if (do_x || do_y)
    {
      foo_canvas_w2c(window->canvas, x_offset, y_offset, &curr_x, &curr_y) ;

      foo_canvas_scroll_to(window->canvas, curr_x, curr_y) ;
    }

  return ;
}


/* if(!zmapWindowItemRegionIsVisible(window, item))
 *   zmapWindowItemCentreOnItem(window, item, changeRegionSize, boundarySize);
 */
gboolean zmapWindowItemRegionIsVisible(ZMapWindow window, FooCanvasItem *item)
{
  gboolean visible = FALSE;
  double wx1, wx2, wy1, wy2;
  double ix1, ix2, iy1, iy2;
  double dummy_x = 0.0;
  double feature_x1 = 0.0, feature_x2 = 0.0 ;
  ZMapFeature feature;

  foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);
  foo_canvas_item_i2w(item, &dummy_x, &iy1);
  foo_canvas_item_i2w(item, &dummy_x, &iy2);

  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;         /* this should never fail. */
  
  /* Get the features canvas coords (may be very different for align block features... */
  zMapFeature2MasterCoords(feature, &feature_x1, &feature_x2);

  /* Get scroll region (clamped to sequence coords) */
  zmapWindowGetScrollRegion(window, &wx1, &wy1, &wx2, &wy2);

  wx2 = feature_x2 + 1;
  if(feature_x1 >= wx1 && feature_x2 <= wx2  &&
     iy1 >= wy1 && iy2 <= wy2)
    {
      visible = TRUE;
    }
  
  return visible;
}


void zmapWindowItemCentreOnItem(ZMapWindow window, FooCanvasItem *item,
                                gboolean alterScrollRegionSize,
                                double boundaryAroundItem)
{

  zmapWindowItemCentreOnItemSubPart(window, item, alterScrollRegionSize, boundaryAroundItem, 0.0, 0.0) ;

  return ;
}


/* If sub_start == sub_end == 0.0 then they are ignored. */
void zmapWindowItemCentreOnItemSubPart(ZMapWindow window, FooCanvasItem *item,
				       gboolean alterScrollRegionSize,
				       double boundaryAroundItem,
				       double sub_start, double sub_end)
{
  double ix1, ix2, iy1, iy2;
  int    cx1, cx2, cy1, cy2;
  int final_canvasx, final_canvasy, 
    tmpx, tmpy, cheight, cwidth;
  gboolean debug = FALSE;

  zMapAssert(window && item) ;

  if (zmapWindowItemIsShown(item))
    {
      /* If the item is a group then we need to use its background to check in long items as the
       * group itself is not a long item. */
      if (FOO_IS_CANVAS_GROUP(item) && zmapWindowContainerUtilsIsValid(FOO_CANVAS_GROUP(item)))
	{
	  FooCanvasItem *long_item ;

	  long_item = (FooCanvasItem *)zmapWindowContainerGetBackground(ZMAP_CONTAINER_GROUP(item)) ;

	  /* Item may have been clipped by long items code so reinstate its true bounds. */
	  my_foo_canvas_item_get_long_bounds(window->long_items, long_item,
					     &ix1, &iy1, &ix2, &iy2) ;
	}
      else
	{
	  foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2) ;
	}

      if (sub_start != 0.0 || sub_end != 0.0)
	{
	  zMapAssert(sub_start <= sub_end
		     && (sub_start >= iy1 && sub_start <= iy2)
		     && (sub_end >= iy1 && sub_end <= iy2)) ;

	  iy2 = iy1 ;
	  iy1 = iy1 + sub_start ;
	  iy2 = iy2 + sub_end ;
	}


      /* Fix the numbers to make sense. */
      foo_canvas_item_i2w(item, &ix1, &iy1);
      foo_canvas_item_i2w(item, &ix2, &iy2);


      /* the item coords are now WORLD */


      if (boundaryAroundItem > 0.0)
	{
	  /* think about PPU for X & Y!! */
	  ix1 -= boundaryAroundItem;
	  iy1 -= boundaryAroundItem;
	  ix2 += boundaryAroundItem;
	  iy2 += boundaryAroundItem;
	}

      if(alterScrollRegionSize)
	{
	  /* this doeesn't work. don't know why. */
	  double sy1, sy2;
	  sy1 = iy1; sy2 = iy2;
	  zmapWindowClampSpan(window, &sy1, &sy2);
	  zMapWindowMove(window, sy1, sy2);
	}
      else
	{
	  if(!zmapWindowItemRegionIsVisible(window, item))
	    {
	      double sx1, sx2, sy1, sy2, tmps, tmpi, diff;
	      /* Get scroll region (clamped to sequence coords) */
	      zmapWindowGetScrollRegion(window, &sx1, &sy1, &sx2, &sy2);
	      /* we now have scroll region coords */
	      /* set tmp to centre of regions ... */
	      tmps = sy1 + ((sy2 - sy1) / 2);
	      tmpi = iy1 + ((iy2 - iy1) / 2);
	      /* find difference between centre points */
	      diff = tmps - tmpi;

	      /* alter scroll region to match that */
	      sy1 -= diff; sy2 -= diff;
	      /* clamp in case we've moved outside the sequence */
	      zmapWindowClampSpan(window, &sy1, &sy2);
	      /* Do the move ... */
	      zMapWindowMove(window, sy1, sy2);
	    }
	}



      /* THERE IS A PROBLEM WITH HORIZONTAL SCROLLING HERE, THE CODE DOES NOT SCROLL FAR
       * ENOUGH TO THE RIGHT...I'M NOT SURE WHY THIS IS AT THE MOMENT. */


      /* get canvas coords to work with */
      foo_canvas_w2c(item->canvas, ix1, iy1, &cx1, &cy1); 
      foo_canvas_w2c(item->canvas, ix2, iy2, &cx2, &cy2); 

      if(debug)
	{
	  printf("[zmapWindowItemCentreOnItem] ix1=%f, ix2=%f, iy1=%f, iy2=%f\n", ix1, ix2, iy1, iy2);
	  printf("[zmapWindowItemCentreOnItem] cx1=%d, cx2=%d, cy1=%d, cy2=%d\n", cx1, cx2, cy1, cy2);
	}

      /* This should possibly be a function */
      tmpx = cx2 - cx1; tmpy = cy2 - cy1;
      if(tmpx & 1)
	tmpx += 1;
      if(tmpy & 1)
	tmpy += 1;
      final_canvasx = cx1 + (tmpx / 2);
      final_canvasy = cy1 + (tmpy / 2);
  
      tmpx = GTK_WIDGET(window->canvas)->allocation.width;
      tmpy = GTK_WIDGET(window->canvas)->allocation.height;
      if(tmpx & 1)
	tmpx -= 1;
      if(tmpy & 1)
	tmpy -= 1;
      cwidth = tmpx / 2; cheight = tmpy / 2;
      final_canvasx -= cwidth; 
      final_canvasy -= cheight;

      if(debug)
	{
	  printf("[zmapWindowItemCentreOnItem] cwidth=%d  cheight=%d\n", cwidth, cheight);
	  printf("[zmapWindowItemCentreOnItem] scrolling to"
		 " canvas x=%d y=%d\n", 
		 final_canvasx, final_canvasy);
	}

      /* Scroll to the item... */
      foo_canvas_scroll_to(window->canvas, final_canvasx, final_canvasy);
    }

  return ;
}




/* Scroll to the specified item.
 * If necessary, recalculate the scroll region, then scroll to the item
 * and highlight it.
 */
gboolean zMapWindowScrollToItem(ZMapWindow window, FooCanvasItem *item)
{
  gboolean result = FALSE ;

  zMapAssert(window && item) ;

  if ((result = zmapWindowItemIsShown(item)))
    {
      zmapWindowItemCentreOnItem(window, item, FALSE, 100.0) ;

      result = TRUE ;
    }

  return result ;
}


void zmapWindowShowItem(FooCanvasItem *item)
{
  /* Looks like old code and we're changing the style stuff. Yes I'm being lazy... */
  g_return_if_fail(item == NULL);

#ifdef OLD_CODE__
  ZMapFeature feature ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapWindowItemFeature item_subfeature_data ;

  /* Retrieve the feature item info from the canvas item. */
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							ITEM_FEATURE_TYPE)) ;

  item_subfeature_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA) ;


  printf("\nItem:\n"
	 "Name: %s, type: %s,  style: %s,  x1: %d,  x2: %d,  "
	 "item_x1: %d,  item_x1: %d\n",
	 (char *)g_quark_to_string(feature->original_id),

	 zMapStyleMode2ExactStr(zMapStyleGetMode(feature->style)),

	 zMapStyleGetName(zMapFeatureGetStyle((ZMapFeatureAny)feature)),
	 feature->x1,
	 feature->x2,
	 item_subfeature_data->start, item_subfeature_data->end) ;

#endif /* OLD_CODE__ */

  return ;
}


/* Prints out an items coords in local coords, good for debugging.... */
void zmapWindowPrintLocalCoords(char *msg_prefix, FooCanvasItem *item)
{
  double x1, y1, x2, y2 ;

  /* Gets bounding box in parents coord system. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  printf("%s:\t%f,%f -> %f,%f\n",
	 (msg_prefix ? msg_prefix : ""),
	 x1, y1, x2, y2) ;


  return ;
}



/* Prints out an items coords in world coords, good for debugging.... */
void zmapWindowPrintItemCoords(FooCanvasItem *item)
{
  double x1, y1, x2, y2 ;

  /* Gets bounding box in parents coord system. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  printf("P %f, %f, %f, %f -> ", x1, y1, x2, y2) ;

  foo_canvas_item_i2w(item, &x1, &y1) ;
  foo_canvas_item_i2w(item, &x2, &y2) ;

  printf("W %f, %f, %f, %f\n", x1, y1, x2, y2) ;

  return ;
}


/* Converts given world coords to an items coord system and prints them. */
void zmapWindowPrintW2I(FooCanvasItem *item, char *text, double x1_in, double y1_in)
{
  double x1 = x1_in, y1 = y1_in;

  my_foo_canvas_item_w2i(item, &x1, &y1) ;

  if (!text)
    text = "Item" ;

  printf("%s -  world(%f, %f)  ->  item(%f, %f)\n", text, x1_in, y1_in, x1, y1) ;

  return ;
}

/* Converts coords in an items coord system into world coords and prints them. */
/* Prints out item coords position in world and its parents coords.... */
void zmapWindowPrintI2W(FooCanvasItem *item, char *text, double x1_in, double y1_in)
{
  double x1 = x1_in, y1 = y1_in;

  my_foo_canvas_item_i2w(item, &x1, &y1) ;

  if (!text)
    text = "Item" ;

  printf("%s -  item(%f, %f)  ->  world(%f, %f)\n", text, x1_in, y1_in, x1, y1) ;


  return ;
}


/* moves a feature to new coordinates */
void zMapWindowMoveItem(ZMapWindow window, ZMapFeature origFeature, 
			ZMapFeature modFeature, FooCanvasItem *item)
{
  double top, bottom, offset;

  if (FOO_IS_CANVAS_ITEM (item))
    {
      offset = ((ZMapFeatureBlock)(modFeature->parent->parent))->block_to_sequence.q1;
      top = modFeature->x1;
      bottom = modFeature->x2;
      zmapWindowSeq2CanOffset(&top, &bottom, offset);
      
      if (modFeature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
	{
	  zMapAssert(origFeature);
	  
	  foo_canvas_item_set(item->parent, "y", top, NULL);
	}
      else
	{
	  foo_canvas_item_set(item, "y1", top, "y2", bottom, NULL);
	}

      zMapWindowUpdateInfoPanel(window, modFeature, item, NULL, TRUE, TRUE) ;
    }
  return;
}


void zMapWindowMoveSubFeatures(ZMapWindow window, 
			       ZMapFeature originalFeature, 
			       ZMapFeature modifiedFeature,
			       GArray *origArray, GArray *modArray,
			       gboolean isExon)
{
  FooCanvasItem *item = NULL, *intron, *intron_box;
  FooCanvasPoints *points ;
  ZMapSpanStruct origSpan, modSpan;
  ZMapWindowItemFeatureType itemFeatureType;
  int i, offset;
  double top, bottom, left, right, middle;
  double x1, y1, x2, y2, transcriptOrigin, transcriptBottom;
  ZMapWindowItemFeature box_data, intron_data ;


  offset = ((ZMapFeatureBlock)(modifiedFeature->parent->parent))->block_to_sequence.q1;
  transcriptOrigin = modifiedFeature->x1;
  transcriptBottom = modifiedFeature->x2;
  zmapWindowSeq2CanOffset(&transcriptOrigin, &transcriptBottom, offset);
  
  for (i = 0; i < modArray->len; i++)
    {
      /* get the FooCanvasItem using original feature */
      origSpan = g_array_index(origArray, ZMapSpanStruct, i);
      

      /* needs changing....to accept frame/strand.... */

#warning "code needs changing for strand/frame"
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

      item = zmapWindowFToIFindItemChild(window->context_to_item,
					 originalFeature, origSpan.x1, origSpan.x2);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      



      /* coords are relative to start of transcript. */
      modSpan  = g_array_index(modArray, ZMapSpanStruct, i);
      top = (double)modSpan.x1 - transcriptOrigin;
      bottom = (double)modSpan.x2 - transcriptOrigin;

      if (isExon)
	{
	  foo_canvas_item_set(item, "y1", top, "y2", bottom, NULL);
	  
	  box_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA);
	  box_data->start = top + transcriptOrigin;
	  box_data->end = bottom + transcriptOrigin;
	}
      else
	{
	  itemFeatureType = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE));

	  if (itemFeatureType == ITEM_FEATURE_BOUNDING_BOX)
	    {
	      intron_box = item;
	      foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
	      box_data = g_object_get_data(G_OBJECT(intron_box), ITEM_SUBFEATURE_DATA);
	      intron = box_data->twin_item;
	      
	      box_data->start = top + transcriptOrigin;
	      box_data->end = bottom + transcriptOrigin;
	    }
	  else
	    {
	      intron = item;
	      intron_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA);
	      intron_box = intron_data->twin_item;
	      foo_canvas_item_get_bounds(intron_box, &x1, &y1, &x2, &y2);
	      
	      intron_data->start = top + transcriptOrigin;
	      intron_data->end = bottom + transcriptOrigin;
	    }

	  points = foo_canvas_points_new(ZMAP_WINDOW_INTRON_POINTS) ;

	  left = (x2 - x1)/2;
	  right = x2;
	  middle = top + (bottom - top + 1)/2;

	  points->coords[0] = left;
	  points->coords[1] = top;
	  points->coords[2] = right;
	  points->coords[3] = middle;
	  points->coords[4] = left;
	  points->coords[5] = bottom;

	  foo_canvas_item_set(intron_box, "y1", top, "y2", bottom, NULL);
	  foo_canvas_item_set(intron, "points", points, NULL);

	  foo_canvas_points_free(points);
	}
    }
  return;
}

/* Returns the sequence coords that correspond to the given _world_ foocanvas coords.
 * 
 * NOTE that although we only return y coords, we need the world x coords as input
 * in order to find the right foocanvas item from which to get the sequence coords. */
gboolean zmapWindowWorld2SeqCoords(ZMapWindow window,
				   double wx1, double wy1, double wx2, double wy2,
				   FooCanvasGroup **block_grp_out, int *y1_out, int *y2_out)
{
  gboolean result = FALSE ;
  FooCanvasItem *item ;

  /* Try to get the item at wx1, wy1... we have to start somewhere... */
  if ((item = foo_canvas_get_item_at(window->canvas, wx1, wy1)))
    {
      FooCanvasGroup *block_container ;
      ZMapFeatureBlock block ;
      
      /* Getting the block struct as well is a bit belt and braces...we could return it but
       * its redundant info. really. */
      if ((block_container = zmapWindowContainerUtilsItemGetParentLevel(item, ZMAPCONTAINER_LEVEL_BLOCK))
	  && (block = zmapWindowItemGetFeatureBlock(block_container)))
	{
	  double offset ;

	  offset = (double)(block->block_to_sequence.q1 - 1) ; /* - 1 for 1 based coord system. */

	  my_foo_canvas_world_bounds_to_item(FOO_CANVAS_ITEM(block_container), &wx1, &wy1, &wx2, &wy2) ;

	  if (block_grp_out)
	    *block_grp_out = block_container ;
	  if (y1_out)
	    *y1_out = floor(wy1 - offset + 0.5) ;
	  if (y2_out)
	    *y2_out = floor(wy2 - offset + 0.5) ;

	  result = TRUE ;
	}
      else
	zMapLogWarning("%s", "No Block Container");
    }
  else 
    {
      get_item_at_workaround_struct workaround_struct = {NULL};
      double scroll_x2;

      workaround_struct.wx1 = wx1;
      workaround_struct.wx2 = wx2;
      workaround_struct.wy1 = wy1;
      workaround_struct.wy2 = wy2;

      /* For some reason foo_canvas_get_item_at() fails to find items
       * a lot of the time even when it shouldn't and so we need a solution. */

      /* Incidentally some of the time, for some users, so does this. (RT # 55131) */
      /* The cause: if the mark is > 10% out of the scroll region the threshold (90%)
       * for intersection will not be met.  So I'm going to clamp to the scroll
       * region and lower the threshold to 55%...
       */
      zmapWindowGetScrollRegion(window, NULL, NULL, &scroll_x2, NULL);

      if(workaround_struct.wx2 > scroll_x2)
	workaround_struct.wx2 = scroll_x2;

      if(workaround_struct.wx2 == 0)
	workaround_struct.wx2 = workaround_struct.wx1 + 1;

      /* Here's another fix for this workaround code.  It fixes the
       * problem seen in RT ticket #75034.  The long items code is
       * causing the issue, as the block background, used to get the
       * block size, is resized by the long items code.  The
       * workaround uses the item size of the background in the
       * calculation of intersection and as this only slightly
       * intersects with the mark when zoomed in the intersection test
       * fails. Passing the window in allows for fetching of the long
       * item's (block container background) original size. */
      workaround_struct.window = window;

      zmapWindowContainerUtilsExecute(window->feature_root_group, ZMAPCONTAINER_LEVEL_BLOCK, 
				      fill_workaround_struct,     &workaround_struct);

      if((result = workaround_struct.result))
	{
	  if (block_grp_out)
	    *block_grp_out = workaround_struct.block;
	  if (y1_out)
	    *y1_out = workaround_struct.seq_x;
	  if (y2_out)
	    *y2_out = workaround_struct.seq_y;
	}
    }


  return result ;
}


gboolean zmapWindowItem2SeqCoords(FooCanvasItem *item, int *y1, int *y2)
{
  gboolean result = FALSE ;



  return result ;
}



static gboolean simple_highlight_region(FooCanvasPoints **points_out, 
                                        FooCanvasItem    *subject, 
                                        gpointer          user_data)
{
  StartEndTextHighlight high_data = (StartEndTextHighlight)user_data;
  FooCanvasPoints *points;
  double first[ITEMTEXT_CHAR_BOUND_COUNT], last[ITEMTEXT_CHAR_BOUND_COUNT];
  int index1, index2;
  gboolean first_found, last_found;
  gboolean redraw = FALSE;
  
  points = foo_canvas_points_new(8);

  index1 = high_data->start;
  index2 = high_data->end;

  /* SWAP MACRO? */
  if(index1 > index2)
    ZMAP_SWAP_TYPE(int, index1, index2);

  /* From the text indices, get the bounds of that char */
  first_found = zmapWindowItemTextIndex2Item(high_data->item, index1, &first[0]);
  last_found  = zmapWindowItemTextIndex2Item(high_data->item, index2, &last[0]);

  if(first_found && last_found)
    {
      double minx, maxx;
      foo_canvas_item_get_bounds(high_data->item, &minx, NULL, &maxx, NULL);
      zmapWindowItemTextOverlayFromCellBounds(points,
					      &first[0],
					      &last[0],
					      minx, maxx);
      zmapWindowItemTextOverlayText2Overlay(high_data->item, points);

      /* set the points */
      if(points_out)
	{
	  *points_out = points;
	  /* record such */
	  redraw = TRUE;
	}
    }

  return redraw;
}



/**
 * my_foo_canvas_item_get_world_bounds:
 * @item: A canvas item.
 * Last edited: Dec  6 13:10 2006 (edgrif)
 * @rootx1_out: X left coord of item (output value).
 * @rooty1_out: Y top coord of item (output value).
 * @rootx2_out: X right coord of item (output value).
 * @rootx2_out: Y bottom coord of item (output value).
 *
 * Returns the _world_ bounds of an item, no function exists in FooCanvas to do this.
 **/
void my_foo_canvas_item_get_world_bounds(FooCanvasItem *item,
					 double *rootx1_out, double *rooty1_out,
					 double *rootx2_out, double *rooty2_out)
{
  double rootx1, rootx2, rooty1, rooty2 ;

  g_return_if_fail (FOO_IS_CANVAS_ITEM (item)) ;
  g_return_if_fail (rootx1_out != NULL || rooty1_out != NULL || rootx2_out != NULL || rooty2_out != NULL ) ;

  foo_canvas_item_get_bounds(item, &rootx1, &rooty1, &rootx2, &rooty2) ;
  foo_canvas_item_i2w(item, &rootx1, &rooty1) ;
  foo_canvas_item_i2w(item, &rootx2, &rooty2) ;

  if (rootx1_out)
    *rootx1_out = rootx1 ;
  if (rooty1_out)
    *rooty1_out = rooty1 ;
  if (rootx2_out)
    *rootx2_out = rootx2 ;
  if (rooty2_out)
    *rooty2_out = rooty2 ;

  return ;
}




/**
 * my_foo_canvas_world_bounds_to_item:
 * @item: A canvas item.
 * @rootx1_out: X left coord of item (input/output value).
 * @rooty1_out: Y top coord of item (input/output value).
 * @rootx2_out: X right coord of item (input/output value).
 * @rootx2_out: Y bottom coord of item (input/output value).
 *
 * Returns the given _world_ bounds in the given item coord system, no function exists in FooCanvas to do this.
 **/
void my_foo_canvas_world_bounds_to_item(FooCanvasItem *item,
					double *rootx1_inout, double *rooty1_inout,
					double *rootx2_inout, double *rooty2_inout)
{
  double rootx1, rootx2, rooty1, rooty2 ;

  g_return_if_fail (FOO_IS_CANVAS_ITEM (item)) ;
  g_return_if_fail (rootx1_inout != NULL || rooty1_inout != NULL || rootx2_inout != NULL || rooty2_inout != NULL ) ;

  rootx1 = rootx2 = rooty1 = rooty2 = 0.0 ;

  if (rootx1_inout)
    rootx1 = *rootx1_inout ;
  if (rooty1_inout)
    rooty1 = *rooty1_inout ;
  if (rootx2_inout)
    rootx2 = *rootx2_inout ;
  if (rooty2_inout)
    rooty2 = *rooty2_inout ;

  foo_canvas_item_w2i(item, &rootx1, &rooty1) ;
  foo_canvas_item_w2i(item, &rootx2, &rooty2) ;

  if (rootx1_inout)
    *rootx1_inout = rootx1 ;
  if (rooty1_inout)
    *rooty1_inout = rooty1 ;
  if (rootx2_inout)
    *rootx2_inout = rootx2 ;
  if (rooty2_inout)
    *rooty2_inout = rooty2 ;

  return ;
}


/* This function returns the original bounds of an item ignoring any long item clipping that may
 * have been done. */
void my_foo_canvas_item_get_long_bounds(ZMapWindowLongItems long_items, FooCanvasItem *item,
					double *x1_out, double *y1_out,
					double *x2_out, double *y2_out)
{
  zMapAssert(long_items && item) ;

  if (zmapWindowItemIsShown(item))
    {
      double x1, y1, x2, y2 ;
      double long_start, long_end ;

      foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

      if (zmapWindowLongItemCoords(long_items, item, &long_start, &long_end))
	{
	  if (long_start < y1)
	    y1 = long_start ;

	  if (long_end > y2)
	    y2 = long_end ;
	}

      *x1_out = x1 ;
      *y1_out = y1 ;
      *x2_out = x2 ;
      *y2_out = y2 ;
    }

  return ;
}





/* I'M TRYING THESE TWO FUNCTIONS BECAUSE I DON'T LIKE THE BIT WHERE IT GOES TO THE ITEMS
 * PARENT IMMEDIATELY....WHAT IF THE ITEM IS ALREADY A GROUP ?????? THE ORIGINAL FUNCTION
 * CANNOT BE USED TO CONVERT A GROUPS POSITION CORRECTLY......S*/

/**
 * my_foo_canvas_item_w2i:
 * @item: A canvas item.
 * @x: X coordinate to convert (input/output value).
 * @y: Y coordinate to convert (input/output value).
 *
 * Converts a coordinate pair from world coordinates to item-relative
 * coordinates.
 **/
void my_foo_canvas_item_w2i (FooCanvasItem *item, double *x, double *y)
{
  g_return_if_fail (FOO_IS_CANVAS_ITEM (item));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* orginal code... */
  item = item->parent;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if (!FOO_IS_CANVAS_GROUP (item))
    item = item->parent;

  while (item) {
    if (FOO_IS_CANVAS_GROUP (item)) {
      *x -= FOO_CANVAS_GROUP (item)->xpos;
      *y -= FOO_CANVAS_GROUP (item)->ypos;
    }

    item = item->parent;
  }

  return ;
}


/**
 * my_foo_canvas_item_i2w:
 * @item: A canvas item.
 * @x: X coordinate to convert (input/output value).
 * @y: Y coordinate to convert (input/output value).
 *
 * Converts a coordinate pair from item-relative coordinates to world
 * coordinates.
 **/
void my_foo_canvas_item_i2w (FooCanvasItem *item, double *x, double *y)
{
  g_return_if_fail (FOO_IS_CANVAS_ITEM (item));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Original code... */
  item = item->parent;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if (!FOO_IS_CANVAS_GROUP (item))
    item = item->parent;

  while (item) {
    if (FOO_IS_CANVAS_GROUP (item)) {
      *x += FOO_CANVAS_GROUP (item)->xpos;
      *y += FOO_CANVAS_GROUP (item)->ypos;
    }

    item = item->parent;
  }

  return ;
}

/* A major problem with foo_canvas_item_move() is that it moves an item by an
 * amount rather than moving it somewhere in the group which is painful for operations
 * like unbumping a column. */
void my_foo_canvas_item_goto(FooCanvasItem *item, double *x, double *y)
{
  double x1, y1, x2, y2 ;
  double dx, dy ;

  if (x || y)
    {
      x1 = y1 = x2 = y2 = 0.0 ; 
      dx = dy = 0.0 ;

      foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

      if (x)
	dx = *x - x1 ;
      if (y)
	dy = *y - y1 ;

      foo_canvas_item_move(item, dx, dy) ;
    }

  return ;
}


gboolean zmapWindowItemIsVisible(FooCanvasItem *item)
{
  gboolean visible = FALSE;

  visible = zmapWindowItemIsShown(item);

#ifdef RDS_DONT
  /* we need to check out our parents :( */
  /* we would like not to do this */
  if(visible)
    {
      FooCanvasGroup *parent = NULL;
      parent = FOO_CANVAS_GROUP(item->parent);
      while(visible && parent)
        {
          visible = zmapWindowItemIsShown(FOO_CANVAS_ITEM(parent));
          /* how many parents we got? */
          parent  = FOO_CANVAS_GROUP(FOO_CANVAS_ITEM(parent)->parent); 
        }
    }
#endif

  return visible;
}

gboolean zmapWindowItemIsShown(FooCanvasItem *item)
{
  gboolean visible = FALSE;
  
  zMapAssert(FOO_IS_CANVAS_ITEM(item)) ;

  g_object_get(G_OBJECT(item), 
               "visible", &visible,
               NULL);

  return visible;
}

void zmapWindowItemGetVisibleCanvas(ZMapWindow window, 
                                    double *wx1, double *wy1,
                                    double *wx2, double *wy2)
{

  getVisibleCanvas(window, wx1, wy1, wx2, wy2);

  return ;
}



/* 
 *                  Internal routines.
 */


static void highlightCB(gpointer list_data, gpointer user_data)
{
  ZMapWindowFocusItemArea data = (ZMapWindowFocusItemArea)list_data;
  FooCanvasItem *item = (FooCanvasItem *)data->focus_item ;
  ZMapWindow window = (ZMapWindow)user_data ;

  if(!data->highlighted)
    {
      GdkColor *highlight = NULL;

      highlightItem(window, item, TRUE) ;

      if(window->highlights_set.item)
        highlight = &(window->colour_item_highlight);

      zmapWindowFocusMaskOverlay(window->focus, item, highlight);

      data->highlighted = TRUE;
    }

  return ;
}

static void unhighlightCB(gpointer list_data, gpointer user_data)
{
  ZMapWindowFocusItemArea data = (ZMapWindowFocusItemArea)list_data;
  FooCanvasItem *item = (FooCanvasItem *)data->focus_item ;
  ZMapWindow window = (ZMapWindow)user_data ;

  highlightItem(window, item, FALSE) ;

  zmapWindowFocusRemoveFocusItem(window->focus, item);

  return ;
}


/* Do the right thing with groups and items */
static void highlightItem(ZMapWindow window, FooCanvasItem *item, gboolean highlight)
{
  GdkColor green = {}, blue = {};
  gdk_color_parse("green", &green);
  gdk_color_parse("blue", &blue);

  if(highlight)
    {
      if(window->highlights_set.item)
	zMapWindowCanvasItemSetIntervalColours(ZMAP_CANVAS_ITEM(item), ZMAPSTYLE_COLOURTYPE_SELECTED, 
					       &(window->colour_item_highlight));
      else
	zMapWindowCanvasItemSetIntervalColours(ZMAP_CANVAS_ITEM(item), ZMAPSTYLE_COLOURTYPE_SELECTED, NULL);
    }
  else
    zMapWindowCanvasItemSetIntervalColours(ZMAP_CANVAS_ITEM(item), ZMAPSTYLE_COLOURTYPE_NORMAL, NULL);

  return ;
}



/* GCompareFunc() to compare two features by their coords so they are sorted into ascending order. */
static gint sortByPositionCB(gconstpointer a, gconstpointer b)
{
  gint result ;
  FooCanvasItem *item_a = (FooCanvasItem *)a ;
  FooCanvasItem *item_b = (FooCanvasItem *)b ;
  ZMapFeature feat_a ;
  ZMapFeature feat_b ;
      
  feat_a = zmapWindowItemGetFeature(item_a);
  zMapAssert(feat_a) ;
  
  feat_b = zmapWindowItemGetFeature(item_b);
  zMapAssert(feat_b) ;

  if (feat_a->x1 < feat_b->x1)
    result = -1 ;
  else if (feat_a->x1 > feat_b->x1)
    result = 1 ;
  else
    result = 0 ;

  return result ;
}



static void extract_feature_from_item(gpointer list_data, gpointer user_data)
{
  GList **list = (GList **)user_data;
  FooCanvasItem *item = (FooCanvasItem *)list_data;
  ZMapFeature feature;

  if((feature = zmapWindowItemGetFeature(item)))
    {
      *list = g_list_append(*list, feature);
    }
  else
    zMapAssertNotReached();

  return ;
}


/* Get the visible portion of the canvas. */
static void getVisibleCanvas(ZMapWindow window,
			     double *screenx1_out, double *screeny1_out,
			     double *screenx2_out, double *screeny2_out)
{
  GtkAdjustment *v_adjust, *h_adjust ;
  double x1, x2, y1, y2 ;

  /* Work out which part of the canvas is visible currently. */
  v_adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
  h_adjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

  x1 = h_adjust->value ;
  x2 = x1 + h_adjust->page_size ;

  y1 = v_adjust->value ;
  y2 = y1 + v_adjust->page_size ;

  foo_canvas_window_to_world(window->canvas, x1, y1, screenx1_out, screeny1_out) ;
  foo_canvas_window_to_world(window->canvas, x2, y2, screenx2_out, screeny2_out) ;

  return ;
}

/* workaround for a failing foo_canvas_item_at(). Actually only looks for blocks! */
static void fill_workaround_struct(ZMapWindowContainerGroup container, 
				   FooCanvasPoints       *points,
				   ZMapContainerLevelType level,
				   gpointer               user_data)
{
  get_item_at_workaround workaround = (get_item_at_workaround)user_data;

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_BLOCK:
      {
	FooCanvasItem *cont_backgrd;
	ZMapFeatureBlock block;

	if((cont_backgrd = (FooCanvasItem *)zmapWindowContainerGetBackground(container)))
	  {
	    double offset;
	    AreaStruct area_src = {workaround->wx1, workaround->wy1, workaround->wx2, workaround->wy2}, 
	      area_block = {};
	    foo_canvas_item_get_bounds(cont_backgrd,
				       &(area_block.x1), &(area_block.y1), 
				       &(area_block.x2), &(area_block.y2));

	    /* The original size of the block needs to be used, not the longitem resized size. */
	    if(workaround->window && workaround->window->long_items)
	      {
		double long_y1, long_y2;
		/* Get the original size of the block's background, see caller & RT #75034 */
		if(zmapWindowLongItemCoords(workaround->window->long_items, cont_backgrd, 
					    &long_y1, &long_y2))
		  {
		    area_block.y1 = long_y1;
		    area_block.y2 = long_y2;
		  }
	      }

	    if((workaround->wx1 >= area_block.x1 && workaround->wx2 <= area_block.x2 &&
		workaround->wy1 >= area_block.y1 && workaround->wy2 <= area_block.y2) || 
	       areas_intersect_gt_threshold(&area_src, &area_block, 0.55))
	      {
		/* We're inside */
		workaround->block = (FooCanvasGroup *)container;
		block = zmapWindowItemGetFeatureBlock(container);

		offset = (double)(block->block_to_sequence.q1 - 1) ; /* - 1 for 1 based coord system. */

		my_foo_canvas_world_bounds_to_item(FOO_CANVAS_ITEM(cont_backgrd), 
						   &(workaround->wx1), &(workaround->wy1), 
						   &(workaround->wx2), &(workaround->wy2)) ;

		workaround->seq_x  = floor(workaround->wy1 - offset + 0.5) ;
		workaround->seq_y  = floor(workaround->wy2 - offset + 0.5) ;
		workaround->result = TRUE;
	      }
	    else
	      zMapLogWarning("fill_workaround_struct: Area block (%f, %f), (%f, %f) "
			     "workaround (%f, %f), (%f, %f) Roy needs to look at this.",
			     area_block.x1, area_block.y1,
			     area_block.x2, area_block.y2,
			     workaround->wx1, workaround->wy1,
			     workaround->wx2, workaround->wy2);
	  }
	
      }
      break;
    default:
      break;
    }

  return ;
}

static gboolean areas_intersection(AreaStruct *area_1, AreaStruct *area_2, AreaStruct *intersect)
{
  double x1, x2, y1, y2;
  gboolean overlap = FALSE;

  x1 = MAX(area_1->x1, area_2->x1);
  y1 = MAX(area_1->y1, area_2->y1);
  x2 = MIN(area_1->x2, area_2->x2);
  y2 = MIN(area_1->y2, area_2->y2);

  if(y2 - y1 > 0 && x2 - x1 > 0)
    {
      intersect->x1 = x1;
      intersect->y1 = y1;
      intersect->x2 = x2;
      intersect->y2 = y2;
      overlap = TRUE;
    }
  else
    intersect->x1 = intersect->y1 = 
      intersect->x2 = intersect->y2 = 0.0;

  return overlap;
}

/* threshold = percentage / 100. i.e. has a range of 0.00000001 -> 1.00000000 */
/* For 100% overlap pass 1.0, For 50% overlap pass 0.5 */
static gboolean areas_intersect_gt_threshold(AreaStruct *area_1, AreaStruct *area_2, double threshold)
{
  AreaStruct inter = {};
  double a1, aI;
  gboolean above_threshold = FALSE;

  if(areas_intersection(area_1, area_2, &inter))
    {
      aI = (inter.x2 - inter.x1 + 1.0) * (inter.y2 - inter.y1 + 1.0);
      a1 = (area_1->x2 - area_1->x1 + 1.0) * (area_1->y2 - area_1->y1 + 1.0);
      
      if(threshold > 0.0 && threshold < 1.0)
	threshold = 1.0 - threshold;
      else
	threshold = 0.0;		/* 100% overlap only */
      
      if((aI <= (a1 * (1.0 + threshold))) && (aI >= (a1 * (1.0 - threshold))))
	above_threshold = TRUE;
      else
	zMapLogWarning("%s", "intersection below threshold");
    }
  else
    zMapLogWarning("%s", "no intersection");

  return above_threshold;
}

#ifdef INTERSECTION_CODE
/* Untested... */
static gboolean foo_canvas_items_get_intersect(FooCanvasItem *i1, FooCanvasItem *i2, FooCanvasPoints **points_out)
{
  gboolean intersect = FALSE;
  
  if(points_out)
    {
      AreaStruct a1 = {};
      AreaStruct a2 = {};
      AreaStruct i  = {};
      
      foo_canvas_item_get_bounds(i1, 
				 &(a1.x1), (&a1.y1),
				 &(a1.x2), (&a1.y2));
      foo_canvas_item_get_bounds(i2, 
				 &(a2.x1), (&a2.y1),
				 &(a2.x2), (&a2.y2));

      if((intersect = areas_intersection(&a1, &a2, &i)))
	{
	  FooCanvasPoints *points = foo_canvas_points_new(2);
	  points->coords[0] = i.x1;
	  points->coords[1] = i.y1;
	  points->coords[2] = i.x2;
	  points->coords[3] = i.y2;
	  *points_out = points;
	}
    }

  return intersect;
}

/* Untested... */
static gboolean foo_canvas_items_intersect(FooCanvasItem *i1, FooCanvasItem *i2, double threshold)
{
  AreaStruct a1 = {};
  AreaStruct a2 = {};
  gboolean intersect = FALSE;

  foo_canvas_item_get_bounds(i1, 
			     &(a1.x1), (&a1.y1),
			     &(a1.x2), (&a1.y2));
  foo_canvas_item_get_bounds(i2, 
			     &(a2.x1), (&a2.y1),
			     &(a2.x2), (&a2.y2));

  intersect = areas_intersect_gt_threshold(&a1, &a2, threshold);

  return intersect;
}
#endif /* INTERSECTION_CODE */
