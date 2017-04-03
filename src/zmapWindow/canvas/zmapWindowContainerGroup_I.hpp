/*  File: zmapWindowContainerGroup_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_CONTAINER_GROUP_I_H
#define ZMAP_WINDOW_CONTAINER_GROUP_I_H

#include <gdk/gdk.h>		/* GdkColor, GdkBitmap */
#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <ZMap/zmapFeature.hpp>
#include <zmapWindowAllBase.hpp>
#include <zmapWindowContainerGroup.hpp>



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

} zmapWindowContainerGroupClassStruct;

typedef struct _zmapWindowContainerGroupStruct
{
  FooCanvasGroup __parent__;

  ZMapFeatureAny feature_any;

  GQueue *user_hidden_children;

  GdkColor *background_fill;
  GdkColor *background_border;

  ZMapContainerLevelType level;

#ifdef NEED_STATS
  ZMapWindowStats stats;
#endif

  double child_spacing;
  double this_spacing;

//  double reposition_x;		/* used to position child contianers */
//  double reposition_y;		/* currently unused */

  struct
  {
    unsigned int max_width  : 1;
    unsigned int max_height : 1;
    unsigned int need_reposition : 1;
    unsigned int visible: 1;
    unsigned int filtered: 1;          /* gets set to true if the column has filtered-out features and only if the 'highlight_filtered_columns' setting is true */
  } flags;

} zmapWindowContainerGroupStruct;


#endif /* ZMAP_WINDOW_CONTAINER_GROUP_I_H */
