/*  File: containers.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *-------------------------------------------------------------------
 */

#ifndef CONTAINERS_H
#define CONTAINERS_H

#include <glib.h>
#include <utils.h>

typedef struct _demoContainerStruct *demoContainer;

typedef gpointer (*demoContainerCallbackFunc)(gpointer parent_item_data, gpointer user_data);
typedef gpointer (*demoContainerCreateCallbackFunc)(gpointer parent_item_data, double leftmost, gpointer user_data);
typedef gpointer (*demoContainerBoundsCallbackFunc)(gpointer parent_item_data, double *x1, double *y1, double *x2, double *y2, gpointer user_data);

typedef struct _demoContainerCallbackSetStruct
{
  demoContainerCallbackFunc create_item;
  demoContainerCreateCallbackFunc create_group;
  demoContainerBoundsCallbackFunc get_bounds;
  demoContainerCallbackFunc hide;
  demoContainerCallbackFunc move;
  demoContainerCallbackFunc destroy;
}demoContainerCallbackSetStruct, *demoContainerCallbackSet;

demoContainer demoContainerCreate(demoContainer parent, demoContainerCallbackSet callbacks, gpointer user_data);
void demoContainerPositionChildren(demoContainer container);
void demoContainerHideChildren(demoContainer container);
void demoContainerExercise(demoContainer container);

#endif /* CONTAINERS_H */
