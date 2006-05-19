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
 * Last edited: May 19 18:20 2006 (edgrif)
 * Created: Thu Sep  8 10:34:49 2005 (edgrif)
 * CVS info:   $Id: zmapWindowDraw.c,v 1.23 2006-05-19 17:24:01 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>



/* Used for holding state while bumping columns. */
typedef struct
{
  GHashTable *pos_hash ;
  GList *pos_list ;

  double offset ;
  double incr ;
  ZMapStyleOverlapMode bump_mode ;
} BumpColStruct, *BumpCol ;


typedef struct
{
  double y1, y2 ;
  double offset ;
  double incr ;
} BumpColRangeStruct, *BumpColRange ;



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

static void compareListOverlapCB(gpointer data, gpointer user_data) ;
static void destroyListOverlapCB(gpointer data, gpointer user_data) ;

static void repositionGroups(FooCanvasGroup *changed_group, double group_spacing) ;

static gint horizPosCompare(gconstpointer a, gconstpointer b) ;

static void addToList(gpointer data, gpointer user_data);


static void preZoomCB(gpointer data, gpointer user_data) ;
static void positionCB(gpointer data, gpointer user_data) ;
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
	{
	  zmapWindowColumnHide(col_group) ;
	}
      else
	{
	  zmapWindowColumnShow(col_group) ;
	}
    }

  return ;
}


/* Probably these should make better use of the containers... */
void zmapWindowColumnHide(FooCanvasGroup *column_group)
{
  zMapAssert(column_group && FOO_IS_CANVAS_GROUP(column_group)) ;

  foo_canvas_item_hide(FOO_CANVAS_ITEM(column_group)) ;

  return ;
}

void zmapWindowColumnShow(FooCanvasGroup *column_group)
{
  zMapAssert(column_group && FOO_IS_CANVAS_GROUP(column_group)) ;

  foo_canvas_item_show(FOO_CANVAS_ITEM(column_group)) ;

  /* A little gotcha here....if you make something visible its background may not
   * be big enough because if it was always hidden we will not have been able to
   * get the groups size... */
  zmapWindowContainerSetBackgroundSize(column_group, 0.0) ;

  return ;
}






/* NOTE: this function bumps an individual column but it DOES NOT move any other columns,
 * to do that you need to use zmapWindowColumnReposition(). The split is made because
 * we don't always want to reposition all the columns following a bump, e.g. when we
 * are creating a zmap window. */
void zmapWindowColumnBump(FooCanvasGroup *column_group,
			  ZMapStyleOverlapMode bump_mode)
{
  BumpColStruct bump_data = {NULL} ;
  FooCanvasGroup *column_features ;
  ZMapFeatureTypeStyle style ;
  double spacing ;

  /* Should check that it is a column group.... */

  column_features = zmapWindowContainerGetFeatures(column_group) ;

  style = zmapWindowContainerGetStyle(column_group) ;

  spacing = zmapWindowContainerGetSpacing(column_group) ;

  bump_data.bump_mode = bump_mode ;
  bump_data.incr = style->width + spacing ;

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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      bump_data.pos_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash */
						 NULL, valueDestroyCB) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      /* Try doing a list.... */


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

  if (bump_mode == ZMAPOVERLAP_POSITION || bump_mode == ZMAPOVERLAP_NAME)
    g_hash_table_destroy(bump_data.pos_hash) ;
  else if (bump_mode == ZMAPOVERLAP_OVERLAP)
    {
      g_list_foreach(bump_data.pos_list, destroyListOverlapCB, NULL) ;
    }



  /* Make the parent groups bounding box as large as the group.... */
  zmapWindowContainerSetBackgroundSize(column_group, 0.0) ;

  return ;
}





/* NOTE: THIS ROUTINE NEEDS TO BE MERGED WITH THE CODE IN THE CONTAINER PACKAGE AS THEY
 * DO THE SAME THING... */


/* WARNING, THIS CODE IS BUGGED, COLUMNS TO THE LEFT OF THE SUPPLIED GROUP DISAPPEAR ! */

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


#ifdef RDS_DONT_INCLUDE
static void zmapWindowContainerRegionChanged(gpointer data, 
                                             gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data;
  ContainerType   type      = CONTAINER_INVALID ;
  gpointer        cb_data   = NULL;
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
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if(callback)
        (callback)(FOO_CANVAS_ITEM(container), 0.0, cb_data);
#endif
      break;
    default:
      zMapAssert(0 && "bad coding, unrecognised container type.") ;
    }

  return ;
}
#endif

void zmapWindowContainerGetAllColumns(FooCanvasGroup *super_root, GList **list)
{
  ContainerType type = CONTAINER_INVALID ;
  
  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;

  zmapWindowContainerExecute(super_root,
			     ZMAPCONTAINER_LEVEL_STRAND,
			     addToList,
			     list,
			     NULL, NULL, FALSE) ;
  
  return ;
}

void zmapWindowContainerMoveEvent(FooCanvasGroup *super_root, ZMapWindow window)
{
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type = CONTAINER_ROOT);
  /* pre callback was set to zmapWindowContainerRegionChanged */
  zmapWindowContainerExecute(FOO_CANVAS_GROUP(super_root),
			     ZMAPCONTAINER_LEVEL_FEATURESET,
			     NULL, NULL,
			     NULL, NULL, TRUE) ;
  return ;
}






/* Makes sure all the things that need to be redrawn when the canvas needs redrawing. */
void zmapWindowNewReposition(ZMapWindow window)
{
  FooCanvasGroup *super_root ;
  ContainerType type = CONTAINER_INVALID ;

  super_root = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item, 0,0,0,0,0)) ;
  zMapAssert(super_root) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type = CONTAINER_ROOT) ;

  zmapWindowContainerExecute(FOO_CANVAS_GROUP(super_root),
			     ZMAPCONTAINER_LEVEL_FEATURESET,
			     NULL,
			     NULL,
			     positionCB,
			     window, TRUE) ;

  /* Must reset width as things like bumping can alter it. We don't need to do the
   * height as that is set via zoom function. */
  zmapWindowResetWidth(window) ;


  return ;
}


/* Reset scrolled region width so that user can scroll across whole of canvas. */
void zmapWindowResetWidth(ZMapWindow window)
{
  FooCanvasGroup *root ;
  double x1, x2, y1, y2 ;
  double root_x1, root_x2, root_y1, root_y2 ;
  double scr_reg_width, root_width ;


  foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);

  scr_reg_width = x2 - x1 + 1.0 ;

  root = foo_canvas_root(window->canvas) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(root), &root_x1, &root_y1, &root_x2, &root_y2) ;

  root_width = root_x2 - root_x1 + 1 ;

  if (root_width != scr_reg_width)
    {
      double excess ;

      excess = root_width - scr_reg_width ;

      x2 = x2 + excess ;

      foo_canvas_set_scroll_region(window->canvas, x1, y1, x2, y2) ;
    }


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
			     positionCB,
			     window, TRUE) ;

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


/* Is called once for each item in a column and sets the horizontal position of that
 * item under various "bumping" modes. */
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


  if(!(zmapWindowItemIsShown(item)))
    return ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;


  /* x1, x2 always needed so might as well get y coords as well because foocanvas will have
   * calculated them anyway. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  switch (bump_data->bump_mode)
    {
    case ZMAPOVERLAP_POSITION:
      {
	/* Bump features over if they have the same start coord. */

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
	/* Bump features over if they overlap at all. */
	BumpColRange new_range ;

	new_range = g_new0(BumpColRangeStruct, 1) ;
	new_range->y1 = y1 ;
	new_range->y2 = y2 ;
	new_range->offset = 0.0 ;
	new_range->incr = bump_data->incr ;

	g_list_foreach(bump_data->pos_list, compareListOverlapCB, new_range) ;

	bump_data->pos_list = g_list_append(bump_data->pos_list, new_range) ;

	offset = new_range->offset ;
	
	break ;
      }
    case ZMAPOVERLAP_NAME:
      {
	/* Bump features that have the same name, i.e. are the same feature, so that each
	 * vertical subcolumn is composed of just one feature in different positions. */
	ZMapFeature feature ;

	feature = (ZMapFeature)(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA)) ;

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


/* GFunc callback func, called from g_list_foreach_find() to test whether current
 * element matches supplied overlap coords. */
static void compareListOverlapCB(gpointer data, gpointer user_data)
{
  BumpColRange curr_range = (BumpColRange)data ;
  BumpColRange new_range = (BumpColRange)user_data ;

  /* Easier to test no overlap and negate. */
  if (!(new_range->y1 > curr_range->y2 || new_range->y2 < curr_range->y1))
    {
      new_range->offset = curr_range->offset + curr_range->incr ;
    }

  return ;
}


/* GFunc callback func, called from g_list_foreach_find() to free list resources. */
static void destroyListOverlapCB(gpointer data, gpointer user_data)
{
  g_free(data) ;

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




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




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


/* Called by zoom and also reposition functions, since all these need repositioning. */
static void positionCB(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapWindow window = (ZMapWindow)user_data ;		    /* not needed at the moment... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  ZMapContainerLevelType level ;

  level = zmapWindowContainerGetLevel(container) ;

  if (level != ZMAPCONTAINER_LEVEL_FEATURESET)
    zmapWindowContainerReposition(container) ;
  
  return ;
}



static void columnZoomChanged(FooCanvasGroup *container, double new_zoom, ZMapWindow window)
{
  ZMapFeatureTypeStyle style = NULL;

  style = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_STYLE) ;
  zMapAssert(style) ;

  zmapWindowColumnSetMagState(window, container, style) ;

  return ;
}

