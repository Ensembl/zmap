/*  File: zmapWindowContainerGroup_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Dec 15 13:56 2010 (edgrif)
 * Created: Wed Dec  3 08:38:10 2008 (rds)
 * CVS info:   $Id: zmapWindowContainerGroup_I.h,v 1.10 2010-12-20 12:20:00 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_CONTAINER_GROUP_I_H
#define ZMAP_WINDOW_CONTAINER_GROUP_I_H

#include <gdk/gdk.h>		/* GdkColor, GdkBitmap */
#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindowAllBase.h>
#include <zmapWindowContainerGroup.h>



#define CONTAINER_DATA     "container_data"
#define CONTAINER_TYPE_KEY "container_type"

typedef enum
  {
    CONTAINER_GROUP_INVALID,
    CONTAINER_GROUP_ROOT,
    CONTAINER_GROUP_PARENT,
    CONTAINER_GROUP_OVERLAYS,
    CONTAINER_GROUP_FEATURES,
    CONTAINER_GROUP_BACKGROUND,
    CONTAINER_GROUP_UNDERLAYS,
    CONTAINER_GROUP_COUNT
  } ZMapWindowContainerType;





/* This class is basically a foocanvas group, and might well be one... */
/* If ZMAP_USE_WINDOW_CONTAINER_GROUP is defined FooCanvasGroup will be used... */

typedef struct _zmapWindowContainerGroupClassStruct
{
  FooCanvasGroupClass __parent__;

  GdkBitmap *fill_stipple;

  unsigned int obj_size ;
  unsigned int obj_total ;


  /* Useful things that the interface provides... */

  /* We want to use foo_canvas_item_new and have default items created.  These
   * might not be the same for all our items... */
  void            (* post_create) (ZMapWindowContainerGroup window_canvas_item);

  void  (* reposition_group)(ZMapWindowContainerGroup container_group, 
			     double rect_x1, double rect_y1,
			     double rect_x2, double rect_y2,
			     double *x_repos, double *y_repos);

} zmapWindowContainerGroupClassStruct;

typedef struct _zmapWindowContainerGroupStruct
{
  FooCanvasGroup __parent__;

  ZMapFeatureAny feature_any;

  GQueue *user_hidden_children;

  GdkColor orig_background;

  ZMapContainerLevelType level;

#ifdef NEED_STATS
  ZMapWindowStats stats;
#endif

  GSList *pre_update_hooks;		/* list of ContainerUpdateHooks */
  GSList *post_update_hooks;		/* list of ContainerUpdateHooks */

  double child_spacing;
  double this_spacing;
  double height;

  double reposition_x;		/* used to position child contianers */
  double reposition_y;		/* currently unused */

  struct
  {
    unsigned int max_width  : 1;
    unsigned int max_height : 1;
    unsigned int column_redraw : 1;
    unsigned int need_reposition : 1;
    unsigned int debug_xml : 1;
    unsigned int debug_text : 1;
  } flags;

} zmapWindowContainerGroupStruct;


#endif /* ZMAP_WINDOW_CONTAINER_GROUP_I_H */
