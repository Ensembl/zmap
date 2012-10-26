/*  File: zmapWindowContainerBlock.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapSeqBitmap.h>
#include <zmapWindowContainerBlock_I.h>
#include <zmapWindowContainerFeatureSet_I.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindow_P.h>	/* ITEM_FEATURE_SET_DATA */

enum
  {
    ITEM_FEATURE_BLOCK_0,		/* zero == invalid prop value */
  };


#if BLOCK_MARK
/* There's some functions for managing ZMapWindowMark here... */
static gboolean maximise_mark_items_cb(ZMapWindowContainerGroup group_updated,
				       FooCanvasPoints         *group_bounds,
				       ZMapContainerLevelType   group_level,
				       gpointer                 user_data);
static void mark_items_create(ZMapWindowContainerBlock container_block,
			      GdkColor  *mark_colour,
			      GdkBitmap *mark_stipple);
static void mark_items_update_colour(ZMapWindowContainerBlock container_block,
				     GdkColor  *mark_colour,
				     GdkBitmap *mark_stipple);
static void mark_items_show(ZMapWindowContainerBlock container_block);
static void mark_items_hide(ZMapWindowContainerBlock container_block);

static gboolean mark_intersects_block(ZMapWindowMark mark, FooCanvasPoints *block_points,
				      double *mark_world_y1_out, double *mark_world_y2_out);
static gboolean mark_block_update_hook(ZMapWindowContainerGroup group_in_update,
				       FooCanvasPoints         *group_points,
				       ZMapContainerLevelType   group_level,
				       gpointer                 user_data);
static gboolean areas_intersection(FooCanvasPoints *area_1,
				   FooCanvasPoints *area_2,
				   FooCanvasPoints *intersect);
static gboolean areas_intersect_gt_threshold(FooCanvasPoints *area_1,
					     FooCanvasPoints *area_2,
					     double           threshold);
#endif

/* All the basic object functions */
static void zmap_window_container_block_class_init  (ZMapWindowContainerBlockClass block_data_class);
static void zmap_window_container_block_init        (ZMapWindowContainerBlock block_data);
static void zmap_window_container_block_set_property(GObject      *gobject,
						     guint         param_id,
						     const GValue *value,
						     GParamSpec   *pspec);
static void zmap_window_container_block_get_property(GObject    *gobject,
						     guint       param_id,
						     GValue     *value,
						     GParamSpec *pspec);
static void zmap_window_container_block_destroy     (GtkObject *gtkobject);
/* A ZMapWindowContainerGroup 'interface' function. */
#if BLOCK_MARK
static void zmap_window_container_block_post_create (ZMapWindowContainerGroup group);
#endif

static GObjectClass *parent_class_G = NULL;

/*!
 * \brief Get the GType for the ZMapWindowContainerBlock GObjects
 *
 * \return GType corresponding to the GObject as registered by glib.
 */

GType zmapWindowContainerBlockGetType(void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowContainerBlockClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_container_block_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowContainerBlock),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_container_block_init
      };

    group_type = g_type_register_static (ZMAP_TYPE_CONTAINER_GROUP,
					 ZMAP_WINDOW_CONTAINER_BLOCK_NAME,
					 &group_info,
					 0);
  }

  return group_type;
}

/*!
 * \brief Once a new ZMapWindowContainerBlock object has been created,
 *        various parameters need to be set.  This sets all the params.
 *
 * \param container     The container to set everything on.
 * \param feature       The container needs to know its Block.
 *
 * \return ZMapWindowContainerBlock that was edited.
 */

ZMapWindowContainerBlock zmapWindowContainerBlockAugment(ZMapWindowContainerBlock container_block,
							 ZMapFeatureBlock feature)
{
  zmapWindowContainerAttachFeatureAny((ZMapWindowContainerGroup)container_block,
				      (ZMapFeatureAny)feature);

  return container_block;
}

/*!
 * \brief Add a column that has been 'compressed' by the application.
 *
 * ZMap allows for the compressing/temporary hiding of columns and we need
 * to keep a list of these.
 *
 * \param container   The container.
 * \param column      The column that has been compressed.
 *
 * \return nothing
 */
void zmapWindowContainerBlockAddCompressedColumn(ZMapWindowContainerBlock block_data,
						 FooCanvasGroup *container)
{

  block_data->compressed_cols = g_list_append(block_data->compressed_cols, container) ;

  return ;
}

/* Test whether there are compressed cols. */
gboolean zmapWindowContainerBlockIsCompressedColumn(ZMapWindowContainerBlock block_data)
{
  gboolean result = FALSE ;

  if (block_data->compressed_cols)
    result = TRUE ;

  return result ;
}

/*!
 * \brief Remove columns that have been 'compressed' by the application.
 *
 * ZMap allows for the compressing/temporary hiding of columns and we need
 * to keep a list of these.  This gives access to the list and effectively
 * pops the list from the block.
 *
 * \param container   The container.
 *
 * \return the list of columns that the block holds as compressed.
 */
GList *zmapWindowContainerBlockRemoveCompressedColumns(ZMapWindowContainerBlock block_data)
{
  GList *list = NULL;

  list = block_data->compressed_cols;
  block_data->compressed_cols = NULL;

  return list;
}

/*!
 * \brief Add a column that has been 'bumped' by the application.
 *
 * ZMap allows for the bumping of columns and we need to keep a list of these.
 *
 * \param container   The container.
 * \param column      The column that has been bumped.
 *
 * \return nothing
 */
void zmapWindowContainerBlockAddBumpedColumn(ZMapWindowContainerBlock block_data, FooCanvasGroup *container)
{

  if(ZMAP_IS_CONTAINER_FEATURESET(container))
    {
      block_data->bumped_cols = g_list_append(block_data->bumped_cols, container) ;
    }

  return ;
}

/* Test whether there are bumped cols. */
gboolean zmapWindowContainerBlockIsBumpedColumn(ZMapWindowContainerBlock block_data)
{
  gboolean result = FALSE ;

  if (block_data->bumped_cols)
    result = TRUE ;

  return result ;
}

/*!
 * \brief Remove columns that have been 'bumped' by the application.
 *
 * ZMap allows for the bumping of columns and we need to keep a list of these.
 * This gives access to the list and effectively pops the list from the block.
 *
 * \param container   The container.
 *
 * \return the list of columns that the block holds as bumped.
 */
GList *zmapWindowContainerBlockRemoveBumpedColumns(ZMapWindowContainerBlock block_data)
{
  GList *list = NULL;

  list = block_data->bumped_cols;
  block_data->bumped_cols = NULL;

  return list;
}


#if BLOCK_MARK
/*!
 * \brief Code to actually mark the Block
 *
 * The zmapWindowMark code is only a controller for the mark.  The setting for
 * the items themselves happen here.
 *
 * \param container_block The container to mark
 * \param mark_colour     The colour to mark with
 * \param mark_stipple    The stipple to mark with
 * \param start           The 'start' of the mark
 * \param end             The 'end' of the mark
 *
 * \return nothing.
 * */
void zmapWindowContainerBlockMark(ZMapWindowContainerBlock container_block,
				  ZMapWindowMark           mark)
{
  ZMapWindowContainerGroup container;
#if USE_BACKGROUND
  ZMapWindowContainerBackground background;
#endif
  GdkColor  *mark_colour;
  GdkBitmap *mark_stipple;
  FooCanvasPoints bounds;
  double coords[4];

  container = (ZMapWindowContainerGroup)container_block;
#if USE_BACKGROUND
  if ((background = zmapWindowContainerGetBackground(container)))
#endif
    {
      mark_colour  = zmapWindowMarkGetColour(mark);
      mark_stipple = zmapWindowMarkGetStipple(mark);
      /* Get the gdk stuff ^^ and then update the colour and stipple. */
      mark_items_update_colour(container_block, mark_colour, mark_stipple);


#if USE_BACKGROUND
	/* We get the current bounds of the background.  It will have
       * been cropped to the scroll region. The UpdateHook takes care
       * of maximising it. */
      foo_canvas_item_get_bounds((FooCanvasItem *)background,
#else
	foo_canvas_get_scroll_region(((FooCanvasItem *) container)->canvas,
#endif
				 &coords[0], &coords[1],
				 &coords[2], &coords[3]);


      /* If the coords look valid, at least one is > zero, then we can just attempt to
       * update the mark items and show them here.
       */
      if (coords[0] > 0.0 || coords[1] > 0.0 || coords[2] > 0.0 || coords[3] > 0.0)
	{
	  /* A FooCanvasPoints on the stack, while we update the mark items */
	  bounds.coords     = &coords[0];
	  bounds.num_points = 2;
	  bounds.ref_count  = 1;
	  /* Call the update hook as if we were in an update cycle. */
	  mark_block_update_hook(container, &bounds, ZMAPCONTAINER_LEVEL_BLOCK, mark);

	  /*
	   * This seemed like a good idea, but I don't think it's necessary, so I've commented it.
	   * It does mean we have to Request Reposition though, which might be time consuming.
	   *
	   * maximise_mark_items_cb(container, &bounds, ZMAPCONTAINER_LEVEL_BLOCK, NULL);
	   */
	}
      else
	{
	  /* The coords don't look valid so we'll have to setup an update hook to update
	   * the mark items next time the update cycle comes around. */
	  zmapWindowContainerGroupAddUpdateHook(container, mark_block_update_hook, mark);
	}

      /* This is required to preserve and crop the mark while zooming. */
      zmapWindowContainerGroupAddUpdateHook(container, maximise_mark_items_cb,
					    &(container_block->mark));

      zmapWindowRequestReposition(container);
    }

  return ;
}

/*!
 * \brief Unmark the block
 *
 * Reverse of Mark
 *
 * \param container_block  The container to remove the mark from.
 *
 * \return nothing
 * */
void zmapWindowContainerBlockUnmark(ZMapWindowContainerBlock container_block)
{
  ZMapWindowContainerGroup container;

  container = (ZMapWindowContainerGroup)container_block;

  mark_items_hide(container_block);

  zmapWindowContainerGroupRemoveUpdateHook(container, maximise_mark_items_cb,
					   &(container_block->mark));

  container_block->mark.start = 0.0;
  container_block->mark.end   = 0.0;

  return ;
}
#endif

/* --- */


ZMapWindowContainerBlock zmapWindowContainerBlockDestroy(ZMapWindowContainerBlock container_block)
{
  gtk_object_destroy(GTK_OBJECT(container_block));

  container_block = NULL;

  return container_block;
}


/*
 *                           INTERNAL
 */

#if BLOCK_MARK

static gboolean maximise_mark_items_cb(ZMapWindowContainerGroup group_updated,
				       FooCanvasPoints         *group_bounds,
				       ZMapContainerLevelType   group_level,
				       gpointer                 user_data)
{
  ZMapWindowContainerBlock container_block = (ZMapWindowContainerBlock)group_updated;
  FooCanvasRE *mark_item_rectangle;
  gboolean status = FALSE ;

  zMapAssert(group_level == ZMAPCONTAINER_LEVEL_BLOCK);

  if (container_block->mark.start >= 0.0 && container_block->mark.end >= 0.0)
    {
      if(container_block->mark.top_item)
	{
	  mark_item_rectangle = (FooCanvasRE *)container_block->mark.top_item;

	  mark_item_rectangle->x1 = group_bounds->coords[0];
	  mark_item_rectangle->y1 = group_bounds->coords[1];
	  mark_item_rectangle->x2 = group_bounds->coords[2];

	  mark_item_rectangle->y2 = container_block->mark.start;
	}

      if(container_block->mark.bottom_item)
	{
	  mark_item_rectangle = (FooCanvasRE *)container_block->mark.bottom_item;

	  mark_item_rectangle->x1 = group_bounds->coords[0];
	  /*  */
	  mark_item_rectangle->y1 = container_block->mark.end;
	  mark_item_rectangle->x2 = group_bounds->coords[2];
	  mark_item_rectangle->y2 = group_bounds->coords[3];
	}

      mark_items_show(container_block);

      status = TRUE ;
    }

  return status ;
}


/* Create the two mark items in the overlay, without a size and we'll update all that later */
static void mark_items_create(ZMapWindowContainerBlock container_block,
			      GdkColor  *mark_colour,
			      GdkBitmap *mark_stipple)
{
  ZMapWindowContainerGroup container;
  ZMapWindowContainerOverlay overlay;
#if USE_BACKGROUND
  ZMapWindowContainerBackground background;
#endif

  container = (ZMapWindowContainerGroup)container_block;

  if((overlay = zmapWindowContainerGetOverlay(container))
#if USE_BACKGROUND
	&& (background = zmapWindowContainerGetBackground(container))
#endif
	)
    {
      FooCanvasGroup *parent;
#ifdef DEBUG_MARK_WITH_OUTLINE
      GdkColor outline;

      gdk_color_parse("red", &outline);
#endif /* DEBUG_MARK_WITH_OUTLINE */
      parent = (FooCanvasGroup *)overlay;

      container_block->mark.top_item =
	foo_canvas_item_new(parent,
			    FOO_TYPE_CANVAS_RECT,
#ifdef DEBUG_MARK_WITH_OUTLINE
			    "outline_color_gdk", &outline,
			    "width_pixels",   1,
#endif /* DEBUG_MARK_WITH_OUTLINE */
			    NULL);

      container_block->mark.bottom_item =
	foo_canvas_item_new(parent,
			    FOO_TYPE_CANVAS_RECT,
#ifdef DEBUG_MARK_WITH_OUTLINE
			    "outline_color_gdk", &outline,
			    "width_pixels",   1,
#endif /* DEBUG_MARK_WITH_OUTLINE */
			    NULL);

      mark_items_update_colour(container_block, mark_colour, mark_stipple);
    }

  return ;
}


static void mark_items_update_colour(ZMapWindowContainerBlock container_block,
				     GdkColor  *mark_colour,
				     GdkBitmap *mark_stipple)
{
  /* If they exist set the fill colour and stipple */
  if(container_block->mark.top_item)
    {
      foo_canvas_item_set(container_block->mark.top_item,
			  "fill_color_gdk", mark_colour,
			  "fill_stipple",   mark_stipple,
			  NULL);
    }

  if(container_block->mark.bottom_item)
    {
      foo_canvas_item_set(container_block->mark.bottom_item,
			  "fill_color_gdk", mark_colour,
			  "fill_stipple",   mark_stipple,
			  NULL);
    }

  return ;
}

static void mark_items_show(ZMapWindowContainerBlock container_block)
{
  /* If they exist show them */
  if(container_block->mark.top_item)
    foo_canvas_item_show(container_block->mark.top_item);

  if(container_block->mark.bottom_item)
    foo_canvas_item_show(container_block->mark.bottom_item);

  return ;
}

static void mark_items_hide(ZMapWindowContainerBlock container_block)
{
  /* If they exist hide them */
  if(container_block->mark.top_item)
    foo_canvas_item_hide(container_block->mark.top_item);

  if(container_block->mark.bottom_item)
    foo_canvas_item_hide(container_block->mark.bottom_item);

  return ;
}


static gboolean mark_intersects_block(ZMapWindowMark mark, FooCanvasPoints *block_points,
				      double *mark_world_y1_out, double *mark_world_y2_out)
{
  FooCanvasPoints mark_points;
  double mark_coords[4];
  gboolean result = FALSE;

  zmapWindowMarkGetWorldRange(mark,
			      &mark_coords[0], &mark_coords[1],
			      &mark_coords[2], &mark_coords[3]);
  mark_points.coords     = &mark_coords[0];
  mark_points.num_points = 2;
  mark_points.ref_count  = 1;

  if(mark_coords[0] == 0.0)
    mark_coords[0] = block_points->coords[0];
  if(mark_coords[2] == 0.0)
    mark_coords[2] = block_points->coords[2];

  if((result = areas_intersect_gt_threshold(block_points, &mark_points, 0.55)))
    {
      if(mark_world_y1_out)
	*mark_world_y1_out = mark_coords[1];
      if(mark_world_y2_out)
	*mark_world_y2_out = mark_coords[3];
    }

  return result;
}

static gboolean mark_block_update_hook(ZMapWindowContainerGroup group_in_update,
				       FooCanvasPoints         *group_points,
				       ZMapContainerLevelType   group_level,
				       gpointer                 user_data)
{
  gboolean status = FALSE;
#if USE_BACKGROUND
  ZMapWindowContainerBackground background;
#endif
  ZMapWindowContainerBlock container_block;
  ZMapWindowMark mark = (ZMapWindowMark)user_data;
  FooCanvasPoints block_points;
  double block_coords[4];
  double start, end, dummy_x;
  gboolean debug_update_hook = FALSE;

  zMapAssert(group_level == ZMAPCONTAINER_LEVEL_BLOCK);

  block_points.num_points = 2;
  block_points.ref_count  = 1;

#if USE_BACKGROUND
  if((background = zmapWindowContainerGetBackground(group_in_update)))
    {
      FooCanvasItem *bg_item = FOO_CANVAS_ITEM(background);

      if(debug_update_hook)
	{
	  /* I was concerned that the group_points and the
	   * item_get_bounds might not marry up, but it appears
	   * there's no need for concern. */
	  foo_canvas_item_get_bounds(bg_item,
				     &block_coords[0], &block_coords[1],
				     &block_coords[2], &block_coords[3]);

	  if(group_points->coords[0] != block_coords[0] ||
	     group_points->coords[0] != block_coords[0] ||
	     group_points->coords[0] != block_coords[0] ||
	     group_points->coords[0] != block_coords[0])
	    {
	      zMapLogWarning("block %p has item coords: [%f,%f] - [%f,%f]",
			     group_in_update,
			     block_coords[0], block_coords[1],
			     block_coords[2], block_coords[3]);
	      zMapLogWarning("block %p has grouppoints: [%f,%f] - [%f,%f]\n",
			     group_in_update,
			     group_points->coords[0], group_points->coords[1],
			     group_points->coords[2], group_points->coords[3]);
	    }

	  printf ("block %p has item coords: [%f,%f] - [%f,%f]\n",
		  group_in_update,
		  block_coords[0], block_coords[1],
		  block_coords[2], block_coords[3]);
	}
#else
    {
	FooCanvasItem *bg_item;

	/* NOTE
	* we want to not use the background: CanvasFeaturesets can work without this
	* backgroun is only used to drive foo canvas w21 etc and any FooCanvasItem/Group will do
	* (look at foo_canvas_item_i2w() to see why)
	* if the group does not have any then we don't need to do anything so no problem
	*/

      if(!((FooCanvasGroup *) group_in_update)->item_list)
		return FALSE;
	bg_item = (FooCanvasItem *) ((FooCanvasGroup *) group_in_update)->item_list->data;

#endif
      block_coords[0] = group_points->coords[0];
      block_coords[1] = group_points->coords[1];
      block_coords[2] = group_points->coords[2];
      block_coords[3] = group_points->coords[3];

      /* We need to compare world coords. coords are item coords */
      /* so we i2w them */
      foo_canvas_item_i2w(bg_item, &block_coords[0], &block_coords[1]);
      foo_canvas_item_i2w(bg_item, &block_coords[2], &block_coords[3]);

      if(debug_update_hook)
	{
	  printf ("block %p has wrld coords: [%f,%f] - [%f,%f]\n",
		  group_in_update,
		  block_coords[0], block_coords[1],
		  block_coords[2], block_coords[3]);
	}

      block_points.coords = &block_coords[0];


      /* check the intersection */
      start = end = 0.0;
      if (mark_intersects_block(mark, &block_points, &start, &end))
	{
	  double world_start, world_end;

	  /* Looks like we intersected */
	  container_block = (ZMapWindowContainerBlock)group_in_update;

	  dummy_x     = 0.0;
	  world_start = start;
	  world_end   = end;

	  foo_canvas_item_w2i(bg_item, &dummy_x, &start);
	  foo_canvas_item_w2i(bg_item, &dummy_x, &end);

	  container_block->mark.start = start;
	  container_block->mark.end   = end;

	  zmapWindowMarkSetBlockContainer(mark, ZMAP_CONTAINER_GROUP(container_block), start, end,
					  block_coords[0], world_start,
					  block_coords[2], world_end);
	}
    }

  return status;
}

/* checks whether two areas intersect.
 * Only good for a rectangle descibed by 2 points (e.g. x1,y1, x2,y2) */
static gboolean areas_intersection(FooCanvasPoints *area_1,
				   FooCanvasPoints *area_2,
				   FooCanvasPoints *intersect) /* intersect is filled */
{
  double x1, x2, y1, y2;
  gboolean overlap = FALSE;

  x1 = MAX(area_1->coords[0], area_2->coords[0]);
  y1 = MAX(area_1->coords[1], area_2->coords[1]);
  x2 = MIN(area_1->coords[2], area_2->coords[2]);
  y2 = MIN(area_1->coords[3], area_2->coords[3]);

  if(y2 - y1 > 0 && x2 - x1 > 0)
    {
      intersect->coords[0] = x1;
      intersect->coords[1] = y1;
      intersect->coords[2] = x2;
      intersect->coords[3] = y2;
      overlap = TRUE;
    }
  else
    intersect->coords[0] = intersect->coords[1] =
      intersect->coords[2] = intersect->coords[3] = 0.0;

  return overlap;
}


/* threshold = percentage / 100. i.e. has a range of 0.00000001 -> 1.00000000 */
/* For 100% overlap pass 1.0, For 50% overlap pass 0.5 */
static gboolean areas_intersect_gt_threshold(FooCanvasPoints *area_1,
					     FooCanvasPoints *area_2,
					     double           threshold)
{
  FooCanvasPoints inter;
  double inter_coords[4];
  double a1, aI;
  gboolean above_threshold = FALSE;

  inter.coords     = &inter_coords[0];
  inter.num_points = 2;
  inter.ref_count  = 1;

  if(areas_intersection(area_1, area_2, &inter))
    {
      aI = (inter.coords[2] - inter.coords[0] + 1.0) * (inter.coords[3] - inter.coords[1] + 1.0);
      a1 = (area_1->coords[2] - area_1->coords[0] + 1.0) * (area_1->coords[3] - area_1->coords[1] + 1.0);

      if(threshold > 0.0 && threshold < 1.0)
	threshold = 1.0 - threshold;
      else
	threshold = 0.0;		/* 100% overlap only */

      if(inter.coords[0] >= area_1->coords[0] &&
	 inter.coords[1] >= area_1->coords[1] &&
	 inter.coords[2] <= area_1->coords[2] &&
	 inter.coords[3] <= area_1->coords[3])
	{
	  above_threshold = TRUE; /* completely contained */
	}
      else if((aI <= (a1 * (1.0 + threshold))) && (aI >= (a1 * (1.0 - threshold))))
	above_threshold = TRUE;
      else
	zMapLogWarning("%s", "intersection below threshold");
    }
  else
    zMapLogWarning("%s", "no intersection");

  return above_threshold;
}

#endif

/*
 * OBJECT CODE
 */
static void zmap_window_container_block_class_init(ZMapWindowContainerBlockClass block_data_class)
{
  ZMapWindowContainerGroupClass group_class;
  GtkObjectClass *gtkobject_class;
  GObjectClass *gobject_class;

  gobject_class   = (GObjectClass *)block_data_class;
  gtkobject_class = (GtkObjectClass *)block_data_class;
  group_class     = (ZMapWindowContainerGroupClass)block_data_class;

  gobject_class->set_property = zmap_window_container_block_set_property;
  gobject_class->get_property = zmap_window_container_block_get_property;

  parent_class_G = g_type_class_peek_parent(block_data_class);

  group_class->obj_size = sizeof(zmapWindowContainerBlockStruct) ;
  group_class->obj_total = 0 ;


#ifdef RDS_DONT_INCLUDE
  /* width */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_BLOCK_WIDTH,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_WIDTH,
						      ZMAPSTYLE_PROPERTY_WIDTH,
						      "The minimum width the column should be displayed at.",
						      0.0, 32000.00, 16.0,
						      ZMAP_PARAM_STATIC_RO));

#endif

#if BLOCK_MARK
  group_class->post_create = zmap_window_container_block_post_create;
#endif

  gtkobject_class->destroy = zmap_window_container_block_destroy;

  return ;
}

static void zmap_window_container_block_init(ZMapWindowContainerBlock block_data)
{
  block_data->compressed_cols    = NULL;

  return ;
}

static void zmap_window_container_block_set_property(GObject      *gobject,
						      guint         param_id,
						      const GValue *value,
						      GParamSpec   *pspec)
{

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_container_block_get_property(GObject    *gobject,
						     guint       param_id,
						     GValue     *value,
						     GParamSpec *pspec)
{
  ZMapWindowContainerBlock block_data;

  block_data = ZMAP_CONTAINER_BLOCK(gobject);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_container_block_destroy(GtkObject *gtkobject)
{
  ZMapWindowContainerBlock block_data;
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(parent_class_G);

  block_data = ZMAP_CONTAINER_BLOCK(gtkobject);

  block_data->window = NULL;	/* not ours */

#if BLOCK_MARK
  /* I think that the overlay (FooCanvasGroup) cleanup will remove these */
  block_data->mark.top_item    = NULL;
  block_data->mark.bottom_item = NULL;
  /* just zero these for the paranoid... */
  block_data->mark.start = 0.0;
  block_data->mark.end   = 0.0;
#endif

  /* compressed and bumped columns are not ours. canvas owns them, just free the lists */
  if(block_data->compressed_cols)
    {
      g_list_free(block_data->compressed_cols) ;
      block_data->compressed_cols = NULL;
    }

  if(block_data->bumped_cols)
    {
      g_list_free(block_data->bumped_cols);
      block_data->bumped_cols = NULL;
    }


  if(gtkobject_class->destroy)
    (gtkobject_class->destroy)(gtkobject);

  return ;
}

#if BLOCK_MARK
static void zmap_window_container_block_post_create(ZMapWindowContainerGroup group)
{
  ZMapWindowContainerBlock block;

  block = ZMAP_CONTAINER_BLOCK(group);

  mark_items_create(block, NULL, NULL);

  return ;
}
#endif
