/*  File: zmapWindowGraphItem_I.h
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
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
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_GRAPH_ITEM_I_H
#define ZMAP_WINDOW_GARPH_ITEM_I_H

#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <zmapWindowCanvasItem_I.h>
#include <zmapWindowGraphItem.h>
#include <include/zmapStyle.h>


/*
 * this module implements normal graphs and density plot canvas items
 * as both are style mode=graph
 * and density is an option in graph mode
 * old style simple graph features are very simple and included in the same file
 */

typedef struct _zmapWindowDenistyItemClassStruct
{
  zmapWindowCanvasItemClass __parent__;
}

typedef struct _zmapWindowGraphItemStruct
{
  zmapWindowCanvasItem __parent__;

#warning rename this ansd make into skip list comapatable wrapper
  GList *listoffeatures;

} zmapWindowGraphItemStruct;






typedef struct _zmapWindowGraphItemClassStruct
{
  zmapWindowCanvasItemClass __parent__;

  GHashTable density_items;         /* singleton canvas items per column, indexed by unique id */

} zmapWindowGraphItemClassStruct;


/*
 * this is a container for graph data masquerading as a ZMapWindowCanvasItem
 * and the prior canvas handling of item is subverted to move most of the compute into ZMap from the foo canvas
 */

typedef struct _zmapWindowGraphItemStruct
{
  zmapWindowCanvasItem __parent__;

  ZMapStyleGraphMode mode;          /* type of graph */
  gboolean density;                 /* if true we re-bin the data according to zoom */

} zmapWindowGraphItemStruct;



#endif /* ZMAP_WINDOW_GRAPH_ITEM_I_H */
