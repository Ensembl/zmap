/*  File: zmapWindow.h
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * and was written by
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *
 * Description: Takes the data provided by the server and displays it
 *              on the canvas, calling drawing functions in zmapDraw
 *              to do so.
 * HISTORY:
 * Last edited: Aug 17 09:59 2004 (rnc)
 * Created: Fri Aug 13 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.h,v 1.1 2004-08-18 15:05:01 rnc Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_DRAWFEATURES_H
#define ZMAP_WINDOW_DRAWFEATURES_H

#include <ZMap/zmapSys.h>		       /* For callback funcs... */
#include <ZMap/zmapFeature.h>


typedef struct _ParamStruct
{
  ZMapWindow window;
  FooCanvas *thisCanvas;
  FooCanvasItem *columnGroup;
  double height;
  double length;
  double column_position;
  GData *types;
  ZMapFeatureTypeStyle thisType;
  ZMapFeature feature;
} ParamStruct;


typedef void (*ZMapFeatureCallbackFunc)(ParamStruct *params);

/* Set of callback routines that allow the caller to be notified when events happen
 * to a feature. */
typedef struct _ZMapFeatureCallbacksStruct
{
  ZMapWindowFeatureCallbackFunc click ;
} ZMapFeatureCallbacksStruct, *ZMapFeatureCallbacks ;

#endif
/******************** end of file ****************************************/

