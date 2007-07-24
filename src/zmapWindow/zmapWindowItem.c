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
 * Last edited: Jul 24 12:43 2007 (rds)
 * Created: Thu Sep  8 10:37:24 2005 (edgrif)
 * CVS info:   $Id: zmapWindowItem.c,v 1.78 2007-07-24 11:44:22 rds Exp $
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>
#include <zmapWindowItemTextFillColumn.h>


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
  ZMapWindowItemTextContext context;
}StartEndTextHighlightStruct, *StartEndTextHighlight;

static gboolean simple_highlight_region(FooCanvasPoints **points_out, 
                                        FooCanvasItem    *subject, 
                                        gpointer          user_data);

static void highlightCB(gpointer data, gpointer user_data) ;
static void unhighlightCB(gpointer data, gpointer user_data) ;

static void highlightItem(ZMapWindow window, FooCanvasItem *item, gboolean highlight) ;
static void highlightFuncCB(gpointer data, gpointer user_data);
static void setItemColour(ZMapWindow window, FooCanvasItem *item, gboolean highlight) ;

static gint sortByPositionCB(gconstpointer a, gconstpointer b) ;
static void extract_feature_from_item(gpointer list_data, gpointer user_data);

static void getVisibleCanvas(ZMapWindow window,
			     double *screenx1_out, double *screeny1_out,
			     double *screenx2_out, double *screeny2_out) ;




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
  gboolean result = FALSE ;
  ZMapFeature feature ;
  FooCanvasGroup *set_group ;
  ZMapWindowItemFeatureSetData set_data ;


  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature && feature->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;

  set_group = zmapWindowContainerGetParentContainerFromItem(item) ;

  /* These should go in container some time.... */
  set_data = g_object_get_data(G_OBJECT(set_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  *set_strand = set_data->strand ;
  *set_frame = set_data->frame ;

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
              
              feature_current = g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_DATA);
              zMapAssert(feature_current);

              if(feature_in->type == ZMAPFEATURE_TRANSCRIPT && 
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
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),	ITEM_FEATURE_TYPE)) ;


  /* If any other feature(s) is currently in focus, revert it to its std colours */
  if (replace_highlight_item)
    zMapWindowUnHighlightFocusItems(window) ;


  /* For some types of feature we want to highlight all the ones with the same name in that column. */
  switch (feature->type)
    {
    case ZMAPFEATURE_ALIGNMENT:
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

	    zmapWindowFocusSetHotItem(window->focus, item) ;
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

      container = zmapWindowContainerGetParentContainerFromItem(dna_item);

      if((overlay_manager = g_object_get_data(G_OBJECT(container), "OVERLAY_MANAGER")))
        {
          zmapWindowOverlaySetLimitItem(overlay_manager, NULL);
          zmapWindowFocusAddOverlayManager(window->focus, overlay_manager);
        }
    }

  if((framed_3ft = zmapWindowItemGetTranslationItemFromItem(window, item)))
    {
      ZMapWindowOverlay overlay_manager;
      FooCanvasGroup *container;

      container = zmapWindowContainerGetParentContainerFromItem(framed_3ft);

      if((overlay_manager = g_object_get_data(G_OBJECT(container), "OVERLAY_MANAGER")))
        {
          zmapWindowOverlaySetLimitItem(overlay_manager, framed_3ft);
          zmapWindowFocusAddOverlayManager(window->focus, overlay_manager);
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
  zmapWindowContainerSetBackgroundColour(column, &(window->colour_column_highlight)) ;

  return ;
}


void zmapUnHighlightColumn(ZMapWindow window, FooCanvasGroup *column)
{
  zmapWindowContainerResetBackgroundColour(column) ;

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

  return parent_container ;
}

FooCanvasItem *zmapWindowItemGetDNAItem(ZMapWindow window, FooCanvasItem *item)
{
  ZMapFeature feature;
  ZMapFeatureBlock block = NULL;
  FooCanvasItem *dna_item = NULL;
  GQuark feature_set_unique = 0, dna_id = 0;
  char *feature_name = NULL;

  feature_set_unique = zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME);

  if((feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA)))
    {
      if((block = (ZMapFeatureBlock)(zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_BLOCK))) && 
         (feature_name = zMapFeatureMakeDNAFeatureName(block)))
        {
          dna_id = zMapFeatureCreateID(ZMAPFEATURE_RAW_SEQUENCE, 
                                       feature_name, 
                                       ZMAPSTRAND_FORWARD, /* ALWAYS FORWARD */
                                       block->block_to_sequence.q1,
                                       block->block_to_sequence.q2,
                                       0,0);
          g_free(feature_name);
        }
      
      dna_item = zmapWindowFToIFindItemFull(window->context_to_item,
					    block->parent->unique_id,
					    block->unique_id,
					    feature_set_unique,
					    ZMAPSTRAND_FORWARD, /* STILL ALWAYS FORWARD */
					    ZMAPFRAME_NONE,/* NO STRAND */
					    dna_id) ;
    }
  else
    {
      zMapAssertNotReached();
    }

  return dna_item;
}

/* highlights the dna given any foocanvasitem (with a feature) and a start and end */
/* This _only_ highlights in the current window! */
void zmapWindowItemHighlightDNARegion(ZMapWindow window, FooCanvasItem *item, int region_start, int region_end)
{
  FooCanvasItem *dna_item = NULL;

  if((dna_item = zmapWindowItemGetDNAItem(window, item)))
    {
      ZMapWindowItemTextContext context;
      ZMapWindowOverlay overlay_manager;
      FooCanvasGroup *container;
      
      container = zmapWindowContainerGetParentContainerFromItem(dna_item);
      
      if((overlay_manager = g_object_get_data(G_OBJECT(container), "OVERLAY_MANAGER")) &&
         (context = g_object_get_data(G_OBJECT(dna_item), ITEM_FEATURE_TEXT_DATA)))
        {
          StartEndTextHighlightStruct data = {0};

          data.start   = region_start;
          data.end     = region_end;
          data.context = context;

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

FooCanvasItem *zmapWindowItemGetTranslationItemFromItem(ZMapWindow window, FooCanvasItem *item)
{
  ZMapFeatureBlock block;
  ZMapFeatureSet feature_set;
  ZMapFeature feature;
  ZMapStrand strand = ZMAPSTRAND_FORWARD;
  ZMapFrame frame;
  FooCanvasItem *translation;
  char *feature_name;
  GQuark feature_set_id, feature_id;

  if((feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA)))
    {
      /* First go up to block... */
      block = (ZMapFeatureBlock)
        (zMapFeatureGetParentGroup((ZMapFeatureAny)(feature), 
                                   ZMAPFEATURE_STRUCT_BLOCK));
      zMapAssert(block);

      feature_set_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME);
      /* and look up the translation feature set with ^^^ */
      feature_set  = zMapFeatureBlockGetSetByID(block, feature_set_id);

      /* Get the strand and frame for the item...  */
      frame = zmapWindowFeatureFrame(feature);

      /* Get the name of the framed feature... */
      feature_name = zMapFeature3FrameTranslationFeatureName(feature_set, frame);
      /* ... and its quark id */
      feature_id   = g_quark_from_string(feature_name);

      frame        = ZMAPFRAME_NONE; /* reset this for the next call! */
      translation  = zmapWindowFToIFindItemFull(window->context_to_item,
                                                block->parent->unique_id,
                                                block->unique_id,
                                                feature_set_id,
                                                strand, /* STILL ALWAYS FORWARD */
                                                frame,
                                                feature_id);
      g_free(feature_name);
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
  FooCanvasGroup *set_group ;
  ZMapWindowItemFeatureSetData set_data ;


  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							ITEM_FEATURE_TYPE)) ;

  set_group = zmapWindowContainerGetParentContainerFromItem(item) ;

  /* These should go in container some time.... */
  set_data = g_object_get_data(G_OBJECT(set_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;


  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    {
      matching_item = zmapWindowFToIFindFeatureItem(window->context_to_item,
						    set_data->strand, set_data->frame,
						    feature) ;
    }
  else
    {
      ZMapWindowItemFeature item_subfeature_data ;

      item_subfeature_data = (ZMapWindowItemFeature)g_object_get_data(G_OBJECT(item),
								      ITEM_SUBFEATURE_DATA) ;

      matching_item = zmapWindowFToIFindItemChild(window->context_to_item,
						  set_data->strand, set_data->frame,
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
  FooCanvasGroup *set_group ;
  ZMapWindowItemFeatureSetData set_data ;

  zMapAssert(window && item && child_start > 0 && child_end > 0 && child_start <= child_end) ;


  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;

  set_group = zmapWindowContainerGetParentContainerFromItem(item) ;

  /* These should go in container some time.... */
  set_data = g_object_get_data(G_OBJECT(set_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  /* Find the item that matches */
  matching_item = zmapWindowFToIFindFeatureItem(window->context_to_item,
						set_data->strand, set_data->frame, feature) ;

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

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;         /* this should never fail. */
  
  /* Get the features canvas coords (may be very different for align block features... */
  zMapFeature2MasterCoords(feature, &feature_x1, &feature_x2);

  wx1 = wx2 = wy1 = wy2 = 0.0;
  zmapWindowScrollRegionTool(window, &wx1, &wy1, &wx2, &wy2);
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

  foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);

  if (sub_start != 0.0 && sub_end != 0.0)
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

  if(boundaryAroundItem > 0.0)
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
          sx1 = sx2 = sy1 = sy2 = 0.0;
          zmapWindowScrollRegionTool(window, &sx1, &sy1, &sx2, &sy2);
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
  ZMapFeature feature ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapWindowItemFeature item_subfeature_data ;

  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							ITEM_FEATURE_TYPE)) ;

  item_subfeature_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA) ;


  printf("\nItem:\n"
	 "Name: %s, type: %s,  style: %s,  x1: %d,  x2: %d,  "
	 "item_x1: %d,  item_x1: %d\n",
	 (char *)g_quark_to_string(feature->original_id),
	 zMapFeatureType2Str(feature->type),
	 zMapStyleGetName(zMapFeatureGetStyle((ZMapFeatureAny)feature)),
	 feature->x1,
	 feature->x2,
	 item_subfeature_data->start, item_subfeature_data->end) ;



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
      
      if (modFeature->type == ZMAPFEATURE_TRANSCRIPT)
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
      if ((block_container = zmapWindowContainerGetParentLevel(item, ZMAPCONTAINER_LEVEL_BLOCK))
	  && (block = g_object_get_data(G_OBJECT(block_container), ITEM_FEATURE_DATA)))
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
  ZMapWindowItemTextContext context;
  FooCanvasPoints *points;
  double first[ITEMTEXT_CHAR_BOUND_COUNT], last[ITEMTEXT_CHAR_BOUND_COUNT];
  int index1, index2, tmp;
  gboolean redraw = FALSE;

  points = foo_canvas_points_new(8);

  index1 = high_data->start;
  index2 = high_data->end;

  /* SWAP MACRO? */
  if(index1 > index2){ tmp = index1; index1 = index2; index2 = tmp; }

  context = high_data->context;
  /* From the text indices, get the bounds of that char */
  zmapWindowItemTextIndexGetBounds(context, index1, &first[0]);
  zmapWindowItemTextIndexGetBounds(context, index2, &last[0]);
  /* From the bounds, calculate the area of the overlay */
  zmapWindowItemTextCharBounds2OverlayPoints(context, &first[0],
                                             &last[0], points);
  /* set the points */
  if(points_out)
    {
      *points_out = points;
      /* record such */
      redraw = TRUE;
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
  if (FOO_IS_CANVAS_GROUP(item))
    {
      HighlightStruct highlight_data = {NULL} ;
      FooCanvasGroup *group = FOO_CANVAS_GROUP(item) ;

      highlight_data.window = window ;
      highlight_data.highlight = highlight ;
      
      g_list_foreach(group->item_list, highlightFuncCB, (void *)&highlight_data) ;
    }
  else
    {
      ZMapWindowItemFeatureType item_feature_type ;
      ZMapFeature feature = NULL;

      item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
      feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);

      if (item_feature_type == ITEM_FEATURE_BOUNDING_BOX || item_feature_type == ITEM_FEATURE_CHILD)
	{
	  ZMapWindowItemFeature item_data ;

	  item_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA) ;

	  if (item_data->twin_item)
	    setItemColour(window, item_data->twin_item, highlight) ;
	}

      setItemColour(window, item, highlight) ;

    }

  return ;
}


/* This is a g_list callback function. */
static void highlightFuncCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  Highlight highlight = (Highlight)user_data ;

  setItemColour(highlight->window, item, highlight->highlight) ;

  return ;
}

static void setItemColour(ZMapWindow window, FooCanvasItem *item, gboolean highlight)
{
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapFeature item_feature = NULL;
  ZMapFeatureTypeStyle style = NULL;
  ZMapWindowItemFeature exon_data ;


  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type != ITEM_FEATURE_INVALID) ;

  item_feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(item_feature) ;

  style = zmapWindowItemGetStyle(item) ;
  zMapAssert(style) ;

  exon_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA) ;

  
  /* Ok we need to be item type specific here now */
  /* Item type     has fill color   has outline color 
   * ------------------------------------------------*/
  /* Text items        yes              no
   * Rectangle/        yes              yes
   *   Ellipse items
   * Polygon items     yes              yes
   * Line items        yes              no
   * Widget and Pixel Buffer items have neither
   */
  

  if (!(FOO_IS_CANVAS_GROUP(item)) && item_feature_type != ITEM_FEATURE_BOUNDING_BOX)
    {
      GdkColor *highlight_colour = NULL ;
      GdkColor *fill_style = NULL, *draw_style = NULL, *border_style = NULL ;
      GdkColor *fill_colour = NULL,
	*border_colour = NULL ;
      ZMapStyleColourTarget target ;
      ZMapStyleColourType type ;
      gboolean status ;
      ZMapStyleMode mode ;

      /* Set a default highlight if there is one. */
      if (highlight && window->highlights_set.item)
	{
	  highlight_colour = &(window->colour_item_highlight) ;
	}

      /* Get the normal or selected colours from the style according to feature type. */
      if (highlight)
	type = ZMAPSTYLE_COLOURTYPE_SELECTED ;
      else
	type = ZMAPSTYLE_COLOURTYPE_NORMAL ;

      mode = zMapStyleGetMode(style) ;

      switch (mode)
	{
	case ZMAPSTYLE_MODE_TRANSCRIPT:
	  {
	    if (exon_data && exon_data->subpart == ZMAPFEATURE_SUBPART_EXON_CDS)
	      target = ZMAPSTYLE_COLOURTARGET_CDS ;
	    else
	      target = ZMAPSTYLE_COLOURTARGET_NORMAL ;	    

	    break ;
	  }

	case ZMAPSTYLE_MODE_GLYPH:
	  {
	    switch(zmapWindowFeatureFrame(item_feature))
	      {
	      case ZMAPFRAME_0:
		target = ZMAPSTYLE_COLOURTARGET_FRAME0 ;
		break ;
	      case ZMAPFRAME_1:
		target = ZMAPSTYLE_COLOURTARGET_FRAME1 ;
		break ;
	      case ZMAPFRAME_2:
		target = ZMAPSTYLE_COLOURTARGET_FRAME2 ;
		break ;
	      default:
		target = ZMAPSTYLE_COLOURTARGET_NORMAL ;
	      }
	    break ;
	  }

	case ZMAPSTYLE_MODE_ALIGNMENT:
	case ZMAPSTYLE_MODE_BASIC:
	case ZMAPSTYLE_MODE_TEXT:
	case ZMAPSTYLE_MODE_GRAPH:
	  {
	    target = ZMAPSTYLE_COLOURTARGET_NORMAL ;	    
	    break ;
	  }

	default:
	  {
	    zMapAssertNotReached() ;
	  }
	}

      status = zMapStyleGetColours(style, target, type, &fill_style, &draw_style, &border_style) ;


      /* Choose what to colour according the canvas item type we are colouring. */
      if (status)
	{
	  if (FOO_IS_CANVAS_LINE(item))
	    {
	      if (mode == ZMAPSTYLE_MODE_TRANSCRIPT)
		fill_colour = fill_style ;
	      else
		fill_colour = border_style ;
	    }
	  else if (FOO_IS_CANVAS_TEXT(item))
	    {
	      fill_colour = draw_style ;
	    }
	  else if (FOO_IS_CANVAS_LINE_GLYPH(item))
	    {
	      fill_colour = fill_style ;
	    }
	  else if (FOO_IS_CANVAS_RE(item) || FOO_IS_CANVAS_POLYGON(item))
	    {
	      fill_colour = fill_style ;
	      border_colour = border_style ;
	    }
	  else
	    {
	      zMapAssertNotReached() ;
	    }
	}


      /* Use default colour if set. */
      if (highlight && !fill_colour && highlight_colour)
	fill_colour = highlight_colour ;


      foo_canvas_item_set(FOO_CANVAS_ITEM(item),
			  "fill_color_gdk", fill_colour,
			  NULL) ;

      if (border_colour)
	foo_canvas_item_set(FOO_CANVAS_ITEM(item),
			    "outline_color_gdk", border_colour,
			    NULL) ;
    }


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
      
  feat_a = g_object_get_data(G_OBJECT(item_a), ITEM_FEATURE_DATA) ;
  zMapAssert(feat_a) ;
  
  feat_b = g_object_get_data(G_OBJECT(item_b), ITEM_FEATURE_DATA) ;
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

  if((feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA)))
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

