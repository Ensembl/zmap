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
 * Last edited: Feb  6 10:44 2007 (rds)
 * Created: Thu Sep  8 10:37:24 2005 (edgrif)
 * CVS info:   $Id: zmapWindowItem.c,v 1.66 2007-02-06 10:44:51 rds Exp $
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>



/* Used to hold highlight information for the hightlight callback function. */
typedef struct
{
  ZMapWindow window ;
  ZMapWindowMark mark ;
  gboolean highlight ;
} HighlightStruct, *Highlight ;

#ifdef RDS_BREAKING_STUFF
/* text group selection stuff for the highlighting of dna, etc... */
typedef struct _ZMapWindowItemHighlighterStruct
{
  gboolean need_update, free_string;
  GList *originItemListMember;
  FooCanvasGroup *tooltip, *highlight;
  double buttonCurrentX;
  double buttonCurrentY;
  int originIdx, seqFirstIdx, seqLastIdx;

  double x1, y1;
  double x2, y2;

  double originalX, originalY;

  char *data;

  FooCanvasPoints static_points[32], *points;

  GString *tooltip_text;
} ZMapWindowItemHighlighterStruct;
#endif

static void highlightCB(gpointer data, gpointer user_data) ;
static void unhighlightCB(gpointer data, gpointer user_data) ;

static void highlightItem(ZMapWindow window, FooCanvasItem *item, gboolean highlight) ;
static void highlightFuncCB(gpointer data, gpointer user_data);
static void colourReverseVideo(GdkColor *colour_inout);
static void setItemColour(ZMapWindow window, FooCanvasItem *item, gboolean highlight) ;


static void destroyZMapWindowItemHighlighter(FooCanvasItem *item, gpointer data);
static void pointerIsOverItem(gpointer data, gpointer user_data);
static gboolean updateInfoGivenCoords(ZMapWindowItemHighlighter select, 
                                      double currentX,
                                      double currentY); /* These are WORLD coords */

static gint sortByPositionCB(gconstpointer a, gconstpointer b) ;
static void extract_feature_from_item(gpointer list_data, gpointer user_data);



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

#ifdef RDS_BREAKING_STUFF

ZMapWindowItemHighlighter zmapWindowItemTextHighlightCreateData(FooCanvasGroup *group)
{
  ZMapWindowItemHighlighter selection = NULL;
  FooCanvasItem *group_as_item = FOO_CANVAS_ITEM(group);

  if(!(selection = (ZMapWindowItemHighlighter)g_object_get_data(G_OBJECT(group), 
                                                                ITEM_HIGHLIGHT_DATA)))
    {
      selection  = (ZMapWindowItemHighlighter)g_new0(ZMapWindowItemHighlighterStruct, 1);
      selection->tooltip      = zMapDrawToolTipCreate(group_as_item->canvas);
      selection->tooltip_text = g_string_sized_new(40);
      foo_canvas_item_hide(FOO_CANVAS_ITEM( selection->tooltip ));

      selection->highlight = FOO_CANVAS_GROUP(foo_canvas_item_new(FOO_CANVAS_GROUP(group_as_item->parent),
                                                                  foo_canvas_group_get_type(),
                                                                  NULL));
      selection->seqFirstIdx = selection->seqLastIdx = -1;
      g_object_set_data(G_OBJECT(group), ITEM_HIGHLIGHT_DATA, (gpointer)selection);
      /* Clear up when we get destroyed. */
      g_signal_connect(G_OBJECT(group), "destroy",
                       G_CALLBACK(destroyZMapWindowItemHighlighter), NULL);

    }

  return selection;
}

void zmapWindowItemTextHighlightReset(ZMapWindowItemHighlighter select_control)
{
  zMapAssert(select_control);

  foo_canvas_item_hide(FOO_CANVAS_ITEM( select_control->tooltip   ));
  foo_canvas_item_hide(FOO_CANVAS_ITEM( select_control->highlight ));

  select_control->seqFirstIdx = select_control->seqLastIdx = -1;
  select_control->originItemListMember = NULL;

  return ;
}

ZMapWindowItemHighlighter zmapWindowItemTextHighlightRetrieve(FooCanvasGroup *group)
{
  ZMapWindowItemHighlighter select_control = NULL;

  select_control = (ZMapWindowItemHighlighter)(g_object_get_data(G_OBJECT(group), ITEM_HIGHLIGHT_DATA));

  return select_control;
}
gboolean zmapWindowItemTextHighlightGetIndices(ZMapWindowItemHighlighter select_control, 
                                               int *firstIdx, int *lastIdx)
{
  gboolean set = FALSE;
  if(select_control->seqFirstIdx != -1 &&
     select_control->seqLastIdx  != -1)
    {
      set = TRUE;
      if(firstIdx)
        *firstIdx = select_control->seqFirstIdx;
      if(lastIdx)
        *lastIdx  = select_control->seqLastIdx;
    }
  return set;
}

void zmapWindowItemTextHighlightFinish(ZMapWindowItemHighlighter select_control)
{
  GtkClipboard *clip = NULL;
  int firstIdx, lastIdx;
  char *full_text = NULL;

  foo_canvas_item_hide(FOO_CANVAS_ITEM(select_control->tooltip));

  if(zmapWindowItemTextHighlightGetIndices(select_control, &firstIdx, &lastIdx))
    {
      if((clip = gtk_clipboard_get(GDK_SELECTION_PRIMARY)))
        {
          if((full_text  = select_control->data) != NULL)
            {
              full_text += firstIdx;
              gtk_clipboard_set_text(clip, full_text,
                                     lastIdx - firstIdx);
            }
        }
    }
  select_control->originItemListMember = NULL;
#ifdef RDS_DONT_INCLUDE
  select_control->seqFirstIdx = select_control->seqLastIdx = -1;
#endif

  testAreaCode(select_control);


  return ;
}

gboolean zmapWindowItemTextHighlightValidForMotion(ZMapWindowItemHighlighter select_control)
{
  gboolean valid = FALSE;
  valid = ( select_control->originItemListMember ? TRUE : FALSE );
  return valid;
}

void zmapWindowItemTextHighlightDraw(ZMapWindowItemHighlighter select_control,
                                     FooCanvasItem *item_receiving_event)
{
#ifdef RDS_DONT_INCLUDE
  zMapDrawHighlightTextRegion(select_control->highlight,
                              select_control->x1,
                              select_control->y1,
                              select_control->x2,
                              select_control->y2,
                              item_receiving_event);
#endif
  return ;
}
gboolean zmapWindowItemTextHighlightBegin(ZMapWindowItemHighlighter select_control,
                                          FooCanvasGroup *group_under_control,
                                          FooCanvasItem *item_receiving_event)
{
  gboolean updated = FALSE;
  GList *child     = NULL;

  if(select_control->originItemListMember)
    zmapWindowItemTextHighlightFinish(select_control);

  child = g_list_first(group_under_control->item_list);
  while(child)
    {
      /* pointers equal?? !! */
      if(item_receiving_event != child->data)
        child = child->next;
      else
        {
          select_control->originItemListMember = child;
          updated = TRUE;
          child   = NULL;
        }
    }

  select_control->seqFirstIdx = select_control->seqLastIdx = -1;

  return updated;
}
void zmapWindowItemTextHighlightRegion(ZMapWindowItemHighlighter select_control,
                                       FooCanvasItem *feature_parent,
                                       int firstIdx, int lastIdx)
{
  ZMapDrawTextRowData textRowData = NULL;
  FooCanvasItem *feature_child = NULL;
  GList *feature_children = NULL;
  double x1, x2, y1, y2;
  gboolean start_found = FALSE, end_found = FALSE;

  feature_children = (FOO_CANVAS_GROUP(feature_parent))->item_list;
  zMapAssert(feature_children);

  feature_child    = FOO_CANVAS_ITEM(feature_children->data);
  zMapAssert(feature_child);

  zmapWindowItemTextHighlightReset(select_control);

  if((textRowData = zMapDrawGetTextItemData(feature_child)))
    {
      int chars_drawn = 0, chars_screen = 0, first_base = 1, 
        row_increment = 0, row = 0, tmp = 0;
      double char_width = 0.0;
      /* pick out information from textRowData that we need */

      first_base   += textRowData->seq_index_start;
      row_increment = textRowData->seq_index_end;
      char_width    = textRowData->char_width;
      chars_drawn   = textRowData->chars_drawn;
      chars_screen  = textRowData->chars_on_screen;

      x1 = y1 = x2 = y2 = 0.0;

      do{
        feature_child = FOO_CANVAS_ITEM(feature_children->data);

        /* In both case here we obtain coords for top left of item
         * then add the pixels (char_width * offset) to the x coord.
         * offset is calculated from the distance from first base.
         * This needs to be truncated if greater than number of chars
         * drawn on the row.
         */

        if(first_base < firstIdx && first_base + row_increment > firstIdx)
          {
            tmp = firstIdx - first_base;
            foo_canvas_item_get_bounds(feature_child, &x1, &y1, NULL, NULL);
            x1 += ((tmp < chars_drawn ? tmp : chars_drawn) * char_width);
            start_found = TRUE;
          }
        if(first_base < lastIdx && first_base + row_increment > lastIdx)
          {
            tmp = lastIdx - first_base + 1;
            foo_canvas_item_get_bounds(feature_child, &x2, &y2, NULL, NULL);
            /* Watch the off by one! */
            x2 += ((tmp < chars_screen ? tmp : chars_screen) * char_width);
            end_found = TRUE;
          }
        
      }while((++row) && (first_base += row_increment) && 
             (feature_children = g_list_next(feature_children)));
    }
  
  if(start_found && end_found)
    {
      select_control->seqFirstIdx = firstIdx;
      select_control->seqLastIdx  = lastIdx;
      select_control->x1 = x1;
      select_control->y1 = y1;
      select_control->x2 = x2;
      select_control->y2 = y2;
      zmapWindowItemTextHighlightDraw(select_control, feature_child);
      //zmapWindowItemTextHighlightFinish (select_control);
    }

  return ;
}
void zmapWindowItemTextHighlightUpdateCoords(ZMapWindowItemHighlighter select_control,
                                             double event_x_coord, 
                                             double event_y_coord)
{
  updateInfoGivenCoords(select_control, event_x_coord, event_y_coord);
  return ;
}

void zmapWindowItemTextHighlightSetFullText(ZMapWindowItemHighlighter select_control,
                                            char *text_string, gboolean copy_string)
{
  if((select_control->free_string = copy_string))
    select_control->data = g_strdup(text_string);
  else
    select_control->data = text_string;

  return ;
}

#endif /* RDS_BREAKING_STUFF */



/*
 *                     Feature Item highlighting.... 
 */


/* Highlight a feature or list of related features (e.g. all hits for same query sequence).
 * 
 *  */
void zMapWindowHighlightObject(ZMapWindow window, FooCanvasItem *item, gboolean replace_highlight_item)
{                                               
  zmapWindowHighlightObject(window, item, replace_highlight_item) ;

  return ;
}


void zmapWindowHighlightObject(ZMapWindow window, FooCanvasItem *item, gboolean replace_highlight_item)
{                                               
  ZMapFeature feature ;
  ZMapWindowItemFeatureType item_feature_type ;
  GList *set_items ;


  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							ITEM_FEATURE_TYPE)) ;


  /* If any other feature(s) is currently in focus, revert it to its std colours */
  if (replace_highlight_item)
    zMapWindowUnHighlightFocusItems(window) ;


  /* For some types of feature we want to highlight all the ones with the same name in that column. */
  switch (feature->type)
    {
    case ZMAPFEATURE_ALIGNMENT:
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

	break ;
      }
    default:
      {
	/* Try highlighting both the item and its column. */
        zmapWindowFocusAddItem(window->focus, item);
      }
      break ;
    }

  zMapWindowHighlightFocusItems(window);

  /* We need to be more sophisticated with our raising of items...otherwise tabbing through them
   * just looks ridiculous...you end up tabbing everywhere... */
  /* FOR NOW i'VE LEFT THIS AS IT IS BECAUSE OTHERWISE THE USER MAY SEE ITEMS THEY HAVE SELECTED
   * FROM ONE OF THE SEARCH WINDOWS. */
  if (replace_highlight_item)
    zmapWindowRaiseItem(item) ;


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

FooCanvasItem *zmapWindowItemGetTranslationItem(ZMapWindow window, FooCanvasItem *item)
{
  ZMapFeature feature;
  ZMapFeatureBlock block = NULL;
  FooCanvasItem *tr_item = NULL;
  GQuark feature_set_unique = 0, dna_id = 0;
  char *feature_name = NULL;
  ZMapFrame frame = ZMAPFRAME_NONE; 

  feature_set_unique = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME);

  if((feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA)))
    {
      /* Implement this bit! */
      /* frame = getFrameFromFeature(feature); */

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
      
      tr_item = zmapWindowFToIFindItemFull(window->context_to_item,
					   block->parent->unique_id,
					   block->unique_id,
					   feature_set_unique,
					   ZMAPSTRAND_FORWARD, /* STILL ALWAYS FORWARD */
					   frame,
					   dna_id) ;
    }
  else
    {
      zMapAssertNotReached();
    }

  return tr_item;
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
  int cx1, cy1, cx2, cy2;
  double feature_x1 = 0.0, feature_x2 = 0.0 ;
  double x1, y1, x2, y2;
  ZMapFeature feature;

  zMapAssert(window && item) ;

  if(!(result = zmapWindowItemIsShown(item)))
    return result;

  zmapWindowItemCentreOnItem(window, item, FALSE, 100.0);

  /* UMMMM, What's going on here ????? */
  return TRUE;
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

      zMapWindowUpdateInfoPanel(window, modFeature, item, NULL, TRUE) ;
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
      highlightItem(window, item, TRUE) ;
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


/* This is a g_datalist callback function. */
static void highlightFuncCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  Highlight highlight = (Highlight)user_data ;

  setItemColour(highlight->window, item, highlight->highlight) ;

  return ;
}










/* The general premise of this function it to "highlight" the item.
 * To do this, rather than mess around having to declare the highlight
 * colour in style files, we just reverse video the colour.  
 *
 * There are a couple of caveats:
 * - Sometimes this doesn't actually highlight the item (greys).  
 * - The background isn't always set.  The foocanvas will accept a NULL
 *   pointer for the fill_color_gdk and the item will be "transparent".
 *   g_object_get doesn't return the right stuff though.
 *
 * We only actually guard against the second one as this is a feature.
 * The outline colour will be used instead rather than the background.
 * This only works for polygons and rectangle/ellipse item types.
 *
 * It's a shame we have to look in the style to find this info, but I
 * can't think of a better solution at the moment.  There are members
 * in the rect, ellipse and polygon items to obtain this, but then we 
 * need to look at the type to decide and would probably only be 
 * quicker if this were possible.

  GType item_type = G_TYPE_FROM_CLASS(FOO_CANVAS_ITEM_GET_CLASS(FOO_CANVAS_ITEM(item)));

  switch(item_type)
    {
    case FOO_TYPE_CANVAS_TEXT:
      break;
    case FOO_TYPE_CANVAS_RE:
      break;
    case FOO_TYPE_CANVAS_POLYGON:
      break;
    case FOO_TYPE_CANVAS_LINE:
      break;
    case FOO_TYPE_CANVAS_GROUP:
      break;
    case FOO_TYPE_CANVAS_ITEM:
      break;
    default:
      break;
    }
 * It makes me want to extend foo canvas items, but I haven't got time.
 * 
 */
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
      char *highlight_target ;

      /* If we definitely have a background or can only possibly have a background we'll use that */
      if (style->colours.background_set || (FOO_IS_CANVAS_LINE(item) || FOO_IS_CANVAS_TEXT(item)) || FOO_IS_CANVAS_LINE_GLYPH(item))
	highlight_target = "fill_color_gdk" ;
      else if (style->colours.outline_set && (FOO_IS_CANVAS_RE(item) || FOO_IS_CANVAS_POLYGON(item)))
	highlight_target = "outline_color_gdk" ;
      else
	zMapAssertNotReached() ;

      if (!(window->use_rev_video))
	{
	  if (highlight)
	    {
	      highlight_colour = &(window->colour_item_highlight) ;
	    }
	  else
	    {
	      /* I'm just trying this for now...in the end we will need unified code for different
		 types to decide how to colour stuff... */
	      if (exon_data && exon_data->subpart == ZMAPFEATURE_SUBPART_EXON_CDS)
		{
		  highlight_colour = &(style->colours.foreground) ;
		}
	      else
		{
		  if (style->colours.background_set)
		    highlight_colour = &(style->colours.background) ;
		  else
		    highlight_colour = &(style->colours.outline) ;
		}
	    }
	}
      else
	{
	  g_object_get(G_OBJECT(item), 
		       highlight_target, &highlight_colour,
		       NULL);

	  colourReverseVideo(highlight_colour) ;
	}

      foo_canvas_item_set(FOO_CANVAS_ITEM(item),
			  highlight_target, highlight_colour,
			  NULL) ;
    }

  return ;
}



static void colourReverseVideo(GdkColor *colour_inout)
{
  zMapAssert(colour_inout) ;

  colour_inout->red   = (65535 - colour_inout->red) ;
  colour_inout->green = (65535 - colour_inout->green) ;
  colour_inout->blue  = (65535 - colour_inout->blue) ;

  return ;
}




static void pointerIsOverItem(gpointer data, gpointer user_data)
{
#ifdef RDS_BREAKING_STUFF
  FooCanvasItem *item = (FooCanvasItem *)data;
  ZMapWindowItemHighlighter select = (ZMapWindowItemHighlighter)user_data;
  ZMapDrawTextRowData trd = NULL;
  double currentY = 0.0;
  double x1, x2, y1, y2;        /* These will be the world coords */
  currentY = select->buttonCurrentY; /* As this one is */

  if(select->need_update && (trd = zMapDrawGetTextItemData(item)))
    {
      foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
      foo_canvas_item_i2w(item, &x1, &y1);
      foo_canvas_item_i2w(item, &x2, &y2);
      /* Select the correct item, based on the input currentY */
      if(currentY > y1 &&
         currentY < y2)
        {
          int currentIdx;
          double topLeftX, topLeftY, dummy = 0.0;
          gboolean farRight, farLeft;

          /* item is under a horizontal line drawn in line with the pointer... */
          /* clamp the pointer location within the item */
          topLeftX  = ((farRight = (select->buttonCurrentX > x2))) ? x2 :
            ((farLeft = (select->buttonCurrentX < x1))) ? x1 : select->buttonCurrentX;
          topLeftX -= x1;

          /* we're zero based so we need to be able to select the
           * first base. revisit if it's an issue */
          currentIdx  = (int)(floor(topLeftX / trd->char_width));
          topLeftX    = currentIdx * trd->char_width;

          currentIdx += trd->seq_index_start;

          topLeftY    = y1;

          /* back to item coords for the y coordinate */
          foo_canvas_item_w2i(item, &dummy, &topLeftY);

#ifdef RDS_DONT_INCLUDE
          printf("pointerIsOverItem (%x): topLeftX=%f, topLeftY=%f\n", item, topLeftX, topLeftY);
#endif
          /* initialise if neccessary (when both == -1) */
          if(select->seqLastIdx  == -1 && 
             select->seqFirstIdx == select->seqLastIdx)
            {
              select->seqFirstIdx = 
                select->seqLastIdx = 
                select->originIdx = currentIdx;
              select->x1 = select->x2 = select->originalX = topLeftX;
              select->y1 = select->y2 = select->originalY = topLeftY;
            }
          
          /* resize to current index */            /* clamping to the original index */
          select->seqFirstIdx  = MIN(currentIdx, MAX(select->seqFirstIdx, select->originIdx));
          select->seqLastIdx   = MAX(currentIdx, MIN(select->seqLastIdx,  select->originIdx));
          
          select->y1 = MIN(topLeftY, MAX(select->y1, select->originalY));
          select->y2 = MAX(topLeftY, MIN(select->y2, select->originalY));

          /* Quite a specific requirement to X constraints */
          if(topLeftY == select->originalY)
            {
              select->x1 = MIN(topLeftX, select->originalX);
              select->x2 = MAX(topLeftX, select->originalX);
            }
          else if(currentIdx < select->originIdx)
            {
              select->x1 = topLeftX;
              select->x2 = MIN(select->x2,  select->originalX);
            }
          else if(currentIdx > select->originIdx)
            {
              select->x2 = topLeftX;
              select->x1 = MAX(select->x1, select->originalX);
            }

#ifdef RDS_DONT_INCLUDE
          printf("pointerIsOverItem (%x): select members, x1=%f, y1=%f, x2=%f, y2=%f\n", 
                 item, select->x1, select->y1, select->x2, select->y2);
#endif
          {
            int tmpSeqA, tmpSeqB;
            tmpSeqA = select->seqFirstIdx + 1;
            tmpSeqB = (select->seqLastIdx >= tmpSeqA ? select->seqLastIdx : tmpSeqA);
            g_string_printf(select->tooltip_text, 
                            "%d - %d", 
                            tmpSeqA, tmpSeqB);
          }

          zMapDrawToolTipSetPosition(select->tooltip, 
                                     x2,
                                     topLeftY,
                                     select->tooltip_text->str);

          select->need_update = FALSE; /* Stop us doing any more of this heavy stuff */
        }
    }
  else if(select->need_update)
    zMapLogWarning("%s", "Error: No Text Row Data");
#endif
  return ;
}

static gboolean updateInfoGivenCoords(ZMapWindowItemHighlighter select, 
                                      double currentX,
                                      double currentY) /* These are WORLD coords */
{
#ifdef RDS_BREAKING_STUFF
  GList *listStart = NULL;
  FooCanvasItem *origin;
  ZMapGListDirection forward = ZMAP_GLIST_FORWARD;

  select->buttonCurrentX = currentX;
  select->buttonCurrentY = currentY;
  select->need_update    = TRUE;
  listStart              = select->originItemListMember;

  /* Now work out where we are with custom g_list_foreach */
  
  /* First we need to know which way to go */
  origin = (FooCanvasItem *)(listStart->data);
  if(origin)
    {
      double x1, x2, y1, y2, halfy;
      foo_canvas_item_get_bounds(origin, &x1, &y1, &x2, &y2);
      foo_canvas_item_i2w(origin, &x1, &y2);
      foo_canvas_item_i2w(origin, &x2, &y2);
      halfy = ((y2 - y1) / 2) + y1;
      if(select->buttonCurrentY < halfy)
        forward = ZMAP_GLIST_REVERSE;
      else if(select->buttonCurrentY > halfy)
        forward = ZMAP_GLIST_FORWARD;
      else
        forward = ZMAP_GLIST_FORWARD;
    }

  if(select->need_update)
    zMap_g_list_foreach_directional(listStart, pointerIsOverItem, 
                                    (gpointer)select, forward);
#endif
  return TRUE;
}
 
static void destroyZMapWindowItemHighlighter(FooCanvasItem *item, gpointer data)
{
#ifdef RDS_BREAKING_STUFF
  ZMapWindowItemHighlighter select_control = NULL;

  if((select_control = (ZMapWindowItemHighlighter)g_object_get_data(G_OBJECT(item), ITEM_HIGHLIGHT_DATA)))
    {
      select_control->need_update = 0;
      select_control->tooltip   = NULL;
      select_control->highlight = NULL;

      g_list_free(select_control->originItemListMember);
      
      g_free(select_control);
    }
#endif
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


