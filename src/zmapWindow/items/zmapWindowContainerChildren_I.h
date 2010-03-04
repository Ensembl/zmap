/*  File: zmapWindowContainerChildren_I.h
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 28 16:35 2009 (rds)
 * Created: Wed Dec  3 08:38:10 2008 (rds)
 * CVS info:   $Id: zmapWindowContainerChildren_I.h,v 1.2 2010-03-04 15:12:05 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_CONTAINER_CHILDREN_I_H
#define ZMAP_WINDOW_CONTAINER_CHILDREM_I_H

#include <zmapWindowContainerChildren.h>
#include <zmapWindowContainerUtils_P.h>


typedef enum
  {
    _CONTAINER_BACKGROUND_POSITION = 0,
    _CONTAINER_UNDERLAY_POSITION   = 1,
    _CONTAINER_FEATURES_POSITION   = 2,
    _CONTAINER_OVERLAY_POSITION    = 3,
  }ContainerPositions;

/* ==== Features ==== */

typedef struct _zmapWindowContainerFeaturesClassStruct
{
  FooCanvasGroupClass __parent__;

} zmapWindowContainerFeaturesClassStruct;

typedef struct _zmapWindowContainerFeaturesStruct
{
  FooCanvasGroup __parent__;

  /* Nothing as yet, but plenty of scope... */

} zmapWindowContainerFeaturesStruct;


/* ==== Underlay ==== */

typedef struct _zmapWindowContainerUnderlayClassStruct
{
  FooCanvasGroupClass __parent__;

} zmapWindowContainerUnderlayClassStruct;

typedef struct _zmapWindowContainerUnderlayStruct
{
  FooCanvasGroup __parent__;

  struct
  {
    unsigned int max_width  : 1;
    unsigned int max_height : 1;
  }flags;

} zmapWindowContainerUnderlayStruct;


/* ==== Overlay ==== */

typedef struct _zmapWindowContainerOverlayClassStruct
{
  FooCanvasGroupClass __parent__;

} zmapWindowContainerOverlayClassStruct;

typedef struct _zmapWindowContainerOverlayStruct
{
  FooCanvasGroup __parent__;

  struct
  {
    unsigned int max_width  : 1;
    unsigned int max_height : 1;
  }flags;

} zmapWindowContainerOverlayStruct;


/* ==== Background ==== */

typedef struct _zmapWindowContainerBackgroundClassStruct
{
  FooCanvasGroupClass __parent__;

} zmapWindowContainerBackgroundClassStruct;

typedef struct _zmapWindowContainerBackgroundStruct
{
  FooCanvasRect __parent__;

  GdkColor original_colour;

  unsigned int has_bg_colour : 1;

} zmapWindowContainerBackgroundStruct;




#endif /* ZMAP_WINDOW_CONTAINER_GROUP_I_H */
