/*  File: zmapWindowDraw.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * Description: General drawing functions for zmap window, e.g.
 *              repositioning columns after one has been bumped
 *              or removed etc.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Jan  6 16:14 2006 (edgrif)
 * Created: Thu Sep  8 10:34:49 2005 (edgrif)
 * CVS info:   $Id: zmapWindowDraw.c,v 1.13 2006-01-06 16:15:07 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>



/* Used for holding state while bumping columns. */
typedef struct
{
  double offset ;
  double incr ;
  ZMapStyleOverlapMode bump_mode ;
  GHashTable *pos_hash ;
} BumpColStruct, *BumpCol ;


typedef struct
{
  double y1, y2 ;
  double offset ;
} BumpColRangeStruct, *BumpColRange ;


typedef struct
{
  BumpColRange curr ;
  BumpColRange new ;
} TempStruct, *Temp ;


typedef struct
{
  ZMapWindow window ;
  double zoom ;

} ZoomDataStruct, *ZoomData ;


typedef struct execOnChildrenStruct_
{
  ZMapContainerLevelType stop ;

  GFunc                  down_func_cb ;
  gpointer               down_func_data ;

  GFunc                  up_func_cb ;
  gpointer               up_func_data ;

} execOnChildrenStruct, *execOnChildren ;


static void bumpColCB(gpointer data, gpointer user_data) ;
static void valueDestroyCB(gpointer data) ;
static gboolean compareOverlapCB(gpointer key, gpointer value, gpointer user_data) ;

static void tempCompareOverlapCB(gpointer key, gpointer value, gpointer user_data) ;

static void itemDestroyCB(gpointer data, gpointer user_data);

static void repositionGroups(FooCanvasGroup *changed_group, double group_spacing) ;

static gint horizPosCompare(gconstpointer a, gconstpointer b) ;

static void addToList(gpointer data, gpointer user_data);


static void preZoomCB(gpointer data, gpointer user_data) ;
static void postZoomCB(gpointer data, gpointer user_data) ;
static void columnZoomChanged(FooCanvasGroup *container, double new_zoom, ZMapWindow window) ;




/* Sorts the children of a group by horizontal position, sorts all children that
 * are groups and leaves children that are items untouched, this is because item children
 * are background items that do not need to be ordered.
 */
void zmapWindowCanvasGroupChildSort(FooCanvasGroup *group_inout)
{

  zMapAssert(FOO_IS_CANVAS_GROUP(group_inout)) ;

  group_inout->item_list = g_list_sort(group_inout->item_list, horizPosCompare) ;

  return ;
}



/* Set the hidden status of a column group, currently this depends on the mag setting in the
 * style for the column but we could allow the user to switch this on and off as well. */
void zmapWindowColumnSetMagState(ZMapWindow window,
				 FooCanvasGroup *col_group, ZMapFeatureTypeStyle style)
{

  zMapAssert(window && FOO_IS_CANVAS_GROUP(col_group) && style) ;

  if (style->min_mag || style->max_mag)
    {
      double curr_zoom ;

      curr_zoom = zMapWindowGetZoomMagnification(window) ;

      if ((style->min_mag && curr_zoom < style->min_mag)
	  || (style->max_mag && curr_zoom > style->max_mag))
	foo_canvas_item_hide(FOO_CANVAS_ITEM(col_group)) ;
      else
	{
	  foo_canvas_item_show(FOO_CANVAS_ITEM(col_group)) ;

	  /* A little gotcha here....if you make something visible its background may not
	   * be big enough because if it was always hidden we will not have been able to
	   * get the groups size... */
	  zmapWindowContainerSetBackgroundSize(col_group, 0.0) ;
	}
    }

  return ;
}


/* NOTE: this function bumps an individual column but it DOES NOT move any other columns,
 * to do that you need to use zmapWindowColumnReposition(). The split is made because
 * we don't always want to reposition all the columns following a bump, e.g. when we
 * are creating a zmap window. */
void zmapWindowColumnBump(FooCanvasGroup *column_group, ZMapStyleOverlapMode bump_mode)
{
  BumpColStruct bump_data = {0.0} ;
  FooCanvasGroup *column_features ;
  ZMapFeatureTypeStyle style ;

  column_features = zmapWindowContainerGetFeatures(column_group) ;

  style = zmapWindowContainerGetStyle(column_group) ;

  bump_data.bump_mode = bump_mode ;
  bump_data.incr = style->width + BUMP_SPACING ;

  switch (bump_mode)
    {
    case ZMAPOVERLAP_COMPLETE:
      bump_data.incr = 0.0 ;
      break ;
    case ZMAPOVERLAP_SIMPLE:
      break ;
    case ZMAPOVERLAP_POSITION:
      bump_data.pos_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash */
						 NULL, valueDestroyCB) ;
      break ;
      case ZMAPOVERLAP_OVERLAP:
      bump_data.pos_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash */
						 NULL, valueDestroyCB) ;
      break ;
    case ZMAPOVERLAP_NAME:
      bump_data.pos_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash */
						 NULL, valueDestroyCB) ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised ZMapWindowItemFeatureType") ;
      break ;
    }

  /* bump all the features. */
  g_list_foreach(column_features->item_list, bumpColCB, (gpointer)&bump_data) ;

  if (bump_mode == ZMAPOVERLAP_POSITION || bump_mode == ZMAPOVERLAP_OVERLAP
      || bump_mode == ZMAPOVERLAP_NAME)
    g_hash_table_destroy(bump_data.pos_hash) ;

  /* Make the parent groups bounding box as large as the group.... */
  zmapWindowContainerSetBackgroundSize(column_group, 0.0) ;

  return ;
}





/* NOTE: THIS ROUTINE NEEDS TO BE MERGED WITH THE CODE IN THE CONTAINER PACKAGE AS THEY
 * DO THE SAME THING... */


/* We assume that the columns have been added sequentially in the order they are displayed in,
 * if this changes then this function will need to be rewritten to look at the position/size
 * of the columns to decide whether to reposition them or not. */
void zmapWindowColumnReposition(FooCanvasGroup *column)
{
  double x1, y1, x2, y2, bound_x1, bound_x2 ;
  FooCanvasGroup *column_set, *block, *align, *root ;


  column_set = zmapWindowContainerGetSuperGroup(column) ;

  block = zmapWindowContainerGetSuperGroup(column_set) ;	/* We don't use this as we don't
								   need to actually move the blocks. */
  align = zmapWindowContainerGetSuperGroup(block) ;

  root = zmapWindowContainerGetSuperGroup(align) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowContainerPrint(root) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Move all the columns in the current group that need to be moved. */
  repositionGroups(column, COLUMN_SPACING) ;

  /* Move all the column groups in the containing block that need to be moved. */
  repositionGroups(column_set, STRAND_SPACING) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Move all the block groups in the canvas that need to be moved. */
  repositionGroups(block, ALIGN_SPACING) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  {
    /* Actually blocks have to be looked at differently, we should be looking to see
     * the maximum block size and then changing the background to fit that.... */

    FooCanvasGroup *parent_container ;

    parent_container = zmapWindowContainerGetSuperGroup(block) ;
    zmapWindowContainerSetBackgroundSize(parent_container, 0.0) ;
  }

  /* Move all the alignment groups in the canvas that need to be moved. */
  repositionGroups(align, ALIGN_SPACING) ;

  /* Reset scroll region */
  foo_canvas_get_scroll_region(column->item.canvas, &x1, &y1, &x2, &y2) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(column->item.canvas)),
			     &bound_x1, NULL, &bound_x2, NULL) ;

  /* I think its a mistake to reset the x1 as we have the scale bar there, but when that 
   * goes we will need to move the x1 as well.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  foo_canvas_set_scroll_region(column->item.canvas, bound_x1, y1, bound_x2, y2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  foo_canvas_set_scroll_region(column->item.canvas, x1, y1, bound_x2, y2) ;


  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* Roys code below needs to be merged into my general feature handling.... */

/* GFunc to call on potentially all container groups.
 * This is still a little creased, get out the ironing board... 
 * Issues include:
 *  - Being unsure on whether parents should call this on children
 *  - small problem on timing, but that might need a check in setbackgroundsize.
 *  - Can parents and children (text esp.) be sorted by moving to sub features style organisation?
 */
static void zmapWindowContainerZoomChanged(gpointer data, 
                                           gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data;
  double          new_zoom  = 0.0;
  gpointer        cb_data   = NULL;
  ContainerType   type      = CONTAINER_INVALID ;
  zmapWindowContainerZoomChangedCallback callback = NULL;

  new_zoom = *((double *)user_data);

  callback = g_object_get_data(G_OBJECT(container), CONTAINER_REDRAW_CALLBACK);
  cb_data  = g_object_get_data(G_OBJECT(container), CONTAINER_REDRAW_DATA);

  type     = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container), 
                                               CONTAINER_TYPE_KEY));
  
  switch(type)
    {
      /* Is this convoluted?  Could we get round this, by just having
       * callbacks on features and text? I think this depends on where 
       * the style is going to sit.  If it sits on the parent it can
       * easily apply to both the feature, bckgrnd & text.  If it sits
       * on the feature then, maybe it's only a feature thing.  Will
       * we need an extra style for the text? Is this reasonable anyway? 
       * Where does the text get it's width from is it just guessed/
       * automatic?
       */
    case CONTAINER_ROOT:
    case CONTAINER_PARENT:    
      if(callback)
        (callback)(container, new_zoom, cb_data);
      /* Issue with whether we have a text child.... */
      /* Do we have a text child? Yes -> zmapWindowContainerZoomChangedCB(text_child, new_zoom, ...); */
      {
        FooCanvasGroup *text = NULL;
        if((text = zmapWindowContainerGetText(container)))
          zmapWindowContainerZoomChanged(text, &new_zoom);
      }
      /* We assume that the child will have changed it's size, so sort
       * out the background. No need for children to do this! */
      /* MUST watch out for the resize to bigger than 32K!!!!  THIS
       * function MUST be called BEFORE zmapWindowScrollRegionTool so
       * that the backgrounds get cropped by longitemcrop */
      /* However some columns need to know what the scroll region is
       * so zmapWindowScrollRegionTool MUST be called FIRST!
       * Arrrrgh. */
      break;
    case CONTAINER_TEXT:
    case CONTAINER_FEATURES:
    case CONTAINER_BACKGROUND:
      if(callback)
        (callback)(container, new_zoom, cb_data);
      break;
    default:
      zMapAssert(0 && "bad coding, unrecognised container type.") ;
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static void zmapWindowContainerRegionChanged(gpointer data, 
                                             gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data;
  gpointer        cb_data   = NULL;
  ContainerType   type      = CONTAINER_INVALID ;
  zmapWindowContainerZoomChangedCallback callback = NULL;

  callback = g_object_get_data(G_OBJECT(container), CONTAINER_REDRAW_CALLBACK);
  cb_data  = g_object_get_data(G_OBJECT(container), CONTAINER_REDRAW_DATA);

  type     = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container), 
                                               CONTAINER_TYPE_KEY));
  
  switch(type)
    {
    case CONTAINER_ROOT:
    case CONTAINER_PARENT:    
      {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        FooCanvasGroup *text = NULL;
        if((text = zmapWindowContainerGetText(container)))
          zmapWindowContainerRegionChanged(text, NULL);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      }
      break;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    case CONTAINER_TEXT:
      if(callback)
        (callback)(FOO_CANVAS_ITEM(container), 0.0, cb_data);
      break;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    case CONTAINER_FEATURES:
    case CONTAINER_BACKGROUND:
      if(callback)
        (callback)(FOO_CANVAS_ITEM(container), 0.0, cb_data);
      break;
    default:
      zMapAssert(0 && "bad coding, unrecognised container type.") ;
    }

  return ;
}


void zmapWindowContainerGetAllColumns(FooCanvasGroup *super_root, GList **list)
{
  ContainerType type = CONTAINER_INVALID ;
  
  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;

  zmapWindowContainerExecute(super_root,
			     ZMAPCONTAINER_LEVEL_STRAND,
			     addToList,
			     list,
			     NULL, NULL) ;
  
  return ;
}

void zmapWindowContainerMoveEvent(FooCanvasGroup *super_root, ZMapWindow window)
{
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type = CONTAINER_ROOT);

  zmapWindowContainerExecute(FOO_CANVAS_GROUP(super_root),
			     ZMAPCONTAINER_LEVEL_FEATURESET,
			     zmapWindowContainerRegionChanged, NULL,
			     NULL, NULL) ;
  return ;
}






/* Makes sure all the things that need to be redrawn for zooming get redrawn. */
void zmapWindowDrawZoom(ZMapWindow window)
{
  FooCanvasGroup *super_root ;
  ContainerType type = CONTAINER_INVALID ;
  ZoomDataStruct zoom_data = {NULL} ;

  super_root = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item, 0,0,0,0,0)) ;
  zMapAssert(super_root) ;


  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type = CONTAINER_ROOT) ;

  zoom_data.window = window ;
  zoom_data.zoom = zMapWindowGetZoomFactor(window) ;


  zmapWindowContainerExecute(FOO_CANVAS_GROUP(super_root),
			     ZMAPCONTAINER_LEVEL_FEATURESET,
			     preZoomCB,
			     &zoom_data,
			     postZoomCB,
			     &zoom_data) ;

  return ;
}


/* Some features have different widths according to their score, this helps annotators
 * gage whether something like an alignment is worth taking notice of.
 * 
 * Currently we only implement the algorithm given by acedb's Score_by_width but it maybe
 * that we will need to do others to support their display.
 *
 * I've done this because for James Havana DBs all methods that mention score have:
 *
 * Score_by_width	
 * Score_bounds	 70.000000 130.000000
 *
 * (the bounds are different for different data sources...)
 * 
 * The interface is slightly tricky here in that we use the style->width to calculate
 * width, not (x2 - x1), hence x2 is only used to return result.
 * 
 */
void zmapWindowGetPosFromScore(ZMapFeatureTypeStyle style, 
			       double score,
			       double *curr_x1_inout, double *curr_x2_out)
{
  double width ;
  double dx = 0.0 ;
  double numerator, denominator ;

  zMapAssert(style && curr_x1_inout && curr_x2_out) ;

  /* We only do stuff if there are both min and max scores to work with.... */
  if (style->max_score && style->min_score)
    {
      double start = *curr_x1_inout ;
      double half_width, mid_way ;

      half_width = style->width / 2 ;
      mid_way = start + half_width ;

      numerator = score - style->min_score ;
      denominator = style->max_score - style->min_score ;

      if (denominator == 0)				    /* catch div by zero */
	{
	  if (numerator < 0)
	    dx = 0.25 ;
	  else if (numerator > 0)
	    dx = 1 ;
	}
      else
	{
	  dx = 0.25 + (0.75 * (numerator / denominator)) ;
	}

      if (dx < 0.25)
	dx = 0.25 ;
      else if (dx > 1)
	dx = 1 ;
      
      *curr_x1_inout = mid_way - (half_width * dx) ;
      *curr_x2_out = mid_way + (half_width * dx) ;
    }

  return ;
}





/* 
 *                Internal routines.
 */




/*! g_list_foreach function to remove items */
static void itemDestroyCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;

  gtk_object_destroy(GTK_OBJECT(item));

  return ;
}



/* SEE COMMENTS ABOVE ABOUT MERGING..... */
/* The changed group is found in its parents list of items and then all items to the right
 * of the changed group are repositioned with respect to the changed group.
 * 
 * NOTE that we assume that the child items of the group_parent occur in the list
 * in their positional order. */
static void repositionGroups(FooCanvasGroup *changed_group, double group_spacing)
{
  double x1, y1, x2, y2 ;
  FooCanvasGroup *parent_container, *all_groups ;
  GList *curr_child ;
  double curr_bound ;

  parent_container = zmapWindowContainerGetSuperGroup(changed_group) ;

  all_groups = zmapWindowContainerGetFeatures(parent_container) ;

  /* Get the position of the changed group within its parent. */
  x1 = y1 = x2 = y2 = 0.0 ; 
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(changed_group), &x1, &y1, &x2, &y2) ;
  curr_bound = x2 + group_spacing ;


  /* Now move all the groups to the right of this one over so they are positioned properly,
   * N.B. this might mean moving them in either direction depending on whether the changed
   * group got bigger or smaller. */
  curr_child = g_list_find(all_groups->item_list, changed_group) ;
  while ((curr_child = g_list_next(curr_child)))	    /* n.b. start with next group to the right. */
    {
      FooCanvasGroup *curr_group = FOO_CANVAS_GROUP(curr_child->data) ;
      double dx = 0.0 ;

      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(curr_group), &x1, &y1, &x2, &y2) ;

      dx = curr_bound - x1 ;				    /* can be +ve or -ve */

      if (dx != 0.0)
	{
	  curr_bound = x2 + dx + group_spacing ;

	  foo_canvas_item_move(FOO_CANVAS_ITEM(curr_group), dx, 0.0) ;
							    /* N.B. we only shift in x, not in y. */
	}
    }


  /* Make the parent groups bounding box as large as the group.... */
  zmapWindowContainerSetBackgroundSize(parent_container, 0.0) ;


  return ;
}



static void bumpColCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  BumpCol bump_data   = (BumpCol)user_data ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapFeature feature ;
  double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0 ;
  gpointer y1_ptr = 0 ;
  gpointer key = NULL, value = NULL ;
  double offset = 0.0, dx = 0.0 ;
  BumpColRange range ;


  if(!(zmapWindowItemIsShown(item)))
    return ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "item_feature_type")) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;

  feature = g_object_get_data(G_OBJECT(item), "item_feature_data") ;
  zMapAssert(feature) ;


  /* x1, x2 always needed so might as well get y coords as well because foocanvas will have
   * calculated them anyway. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  switch (bump_data->bump_mode)
    {
    case ZMAPOVERLAP_POSITION:
      {
	y1_ptr = GINT_TO_POINTER((int)y1) ;

	if (g_hash_table_lookup_extended(bump_data->pos_hash, y1_ptr, &key, &value))
	  {
	    offset = *(double *)value + bump_data->incr ;
	    
	    *(double *)value = offset ;
	  }
	else
	  {
	    value = g_new(double, 1) ;
	    *(double *)value = offset ;
	    
	    g_hash_table_insert(bump_data->pos_hash, y1_ptr, value) ;
	  }
	
	break ;
      }
    case ZMAPOVERLAP_OVERLAP:
      {
	BumpColRangeStruct new_range ;
	BumpColRange curr_range ;

	/* TEMP CODE UNTIL 2.6 */
#warning "temp 2.2 code...agh"
	TempStruct callback_data = {NULL} ;
	

	new_range.y1 = y1 ;
	new_range.y2 = y2 ;
	new_range.offset = 0.0 ;


	callback_data.new = &new_range ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	/* THIS FUNCTION IS NOT AVAILABLE IN GLIB 2.2 leaving me to go quietly spare.... */

	/* if ((curr_range = g_hash_table_find(bump_data->pos_hash, compareOverlap, &new_range))) */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	g_hash_table_foreach(bump_data->pos_hash, tempCompareOverlapCB, &callback_data) ;
	if ((curr_range = callback_data.curr))

	  {
	    if (new_range.y1 < curr_range->y1)
	      curr_range->y1 = new_range.y1 ;

	    if (new_range.y2 > curr_range->y2)
	      curr_range->y2 = new_range.y2 ;

	    curr_range->offset += bump_data->incr ;
	  }
	else
	  {
	    curr_range = g_memdup(&new_range, sizeof(BumpColRangeStruct)) ;

	    g_hash_table_insert(bump_data->pos_hash, curr_range, curr_range) ;
	  }

	offset = curr_range->offset ;
	
	break ;
      }
    case ZMAPOVERLAP_NAME:
      {
	ZMapFeature feature ;

	feature = (ZMapFeature)(g_object_get_data(G_OBJECT(item), "item_feature_data")) ;

	if (g_hash_table_lookup_extended(bump_data->pos_hash,
					 GINT_TO_POINTER(feature->original_id), &key, &value))
	  {
	    offset = *(double *)value ;
	  }
	else
	  {
	    offset = bump_data->offset ;

	    value = g_new(double, 1) ;
	    *(double *)value = offset ;
	    
	    g_hash_table_insert(bump_data->pos_hash, GINT_TO_POINTER(feature->original_id), value) ;

	    bump_data->offset += bump_data->incr ;
	  }
	
	break ;
      }
    case ZMAPOVERLAP_COMPLETE:
    case ZMAPOVERLAP_SIMPLE:
      {
	offset = bump_data->offset ;
	bump_data->offset += bump_data->incr ;

	break ;
      }
    default:
      zMapAssert(0 && "Coding error, unrecognised bump type.") ;
      break ;
    }


  /* Some features are drawn with different widths to indicate things like score. In this case
   * their offset needs to be corrected to place them centrally. (We always do this which
   * seems inefficient but its a toss up whether it would be quicker to test (dx == 0). */
  dx = (feature->style->width - (x2 - x1)) / 2 ;
  offset += dx ;

  my_foo_canvas_item_goto(item, &(offset), NULL) ; 

  return ;
}


/* GDestroyNotify() function for freeing the data part of a hash association. */
static void valueDestroyCB(gpointer data)
{
  g_free(data) ;

  return ;
}


/* GHRFunc callback func, called from g_hash_table_find() to test whether current
 * element matches supplied overlap coords. */
static gboolean compareOverlapCB(gpointer key, gpointer value, gpointer user_data)
{
  gboolean result = TRUE ;
  BumpColRange curr_range = (BumpColRange)value, new_range = (BumpColRange)user_data ;

  /* Easier to test no overlap. */
  if (new_range->y1 > curr_range->y2 || new_range->y2 < curr_range->y1)
    result = FALSE ;

  return result ;
}


/* this is temporary code until we move completely to glib 2.6, it does the same as
 * compareOverlapCB but badly..., it can be deleted when we move to 2.6... */
static void tempCompareOverlapCB(gpointer key, gpointer value, gpointer user_data)
{
  BumpColRange curr_range = (BumpColRange)value ;
  Temp callback_data = (Temp)user_data ;

  if (!callback_data->curr)
    {
      if (callback_data->new->y1 > curr_range->y2 || callback_data->new->y2 < curr_range->y1)
	callback_data->curr = NULL ;
      else
	callback_data->curr = curr_range ;
    }

  return ;
}



/* MAY NEED TO INGNORE BACKGROUND BOXES....WHICH WILL BE ITEMS.... */

/* A GCompareFunc() called from g_list_sort(), compares groups on their horizontal position. */
static gint horizPosCompare(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;
  FooCanvasGroup *group_a = (FooCanvasGroup *)a, *group_b = (FooCanvasGroup *)b ;

  if (!FOO_IS_CANVAS_GROUP(group_a) || !FOO_IS_CANVAS_GROUP(group_b))
    {
      result = 0 ;
    }
  else
    {
      if (group_a->xpos < group_b->xpos)
	result = -1 ;
      else if (group_a->xpos > group_b->xpos)
	result = 1 ;
      else
	result = 0 ;
    }

  return result ;
}

static void printItem(FooCanvasItem *item, int indent, char *prefix)
{
  int i ;

  for (i = 0 ; i < indent ; i++)
    {
      printf("  ") ;
    }
  printf("%s", prefix) ;

  zmapWindowPrintItemCoords(item) ;

  return ;
}



static void addToList(gpointer data, gpointer user_data)
{
  FooCanvasGroup *grp = FOO_CANVAS_GROUP(data);
  GList **list = (GList **)user_data;
  *list = g_list_append(*list, grp);
  return ;
}


/* We need the window in here.... */
/* GFunc to call on potentially all container groups.
 */
static void preZoomCB(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  ZoomData zoom_data = (ZoomData)user_data ;
  ZMapContainerLevelType level ;

  level = zmapWindowContainerGetLevel(container) ;

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      columnZoomChanged(container, zoom_data->zoom, zoom_data->window) ;
      break ;
    default:
      break ;
    }

  return ;
}



static void postZoomCB(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  ZoomData zoom_data = (ZoomData)user_data ;
  ZMapContainerLevelType level ;

  level = zmapWindowContainerGetLevel(container) ;

  if (level != ZMAPCONTAINER_LEVEL_FEATURESET)
    zmapWindowContainerReposition(container) ;

  /* Correct for the stupid scale bar.... */
  if (level == ZMAPCONTAINER_LEVEL_ROOT)
    my_foo_canvas_item_goto(FOO_CANVAS_ITEM(container),
			    &(zoom_data->window->alignment_start), NULL) ; 


  return ;
}



static void columnZoomChanged(FooCanvasGroup *container, double new_zoom, ZMapWindow window)
{
  ZMapFeatureTypeStyle style = NULL;

  style = g_object_get_data(G_OBJECT(container), "item_feature_style") ;
  zMapAssert(style) ;

  zmapWindowColumnSetMagState(window, container, style) ;

  return ;
}

