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
 * Last edited: Sep 17 10:14 2004 (rnc)
 * Created: Fri Aug 13 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.h,v 1.5 2004-09-21 13:15:32 rnc Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_DRAWFEATURES_H
#define ZMAP_WINDOW_DRAWFEATURES_H

#include <ZMap/zmapSys.h>		       /* For callback funcs... */
#include <ZMap/zmapFeature.h>



// parameters passed between the various functions processing the features to be drawn on the canvas
typedef struct _ParamStruct
{
  ZMapWindow           window;
  FooCanvas           *thisCanvas;
  FooCanvasItem       *columnGroup;
  FooCanvasItem       *revColGroup;         // a group for reverse strand features
  double               height;
  double               length;
  double               column_position;
  double               revColPos;           // column position on reverse strand
  GData               *types;
  ZMapFeatureTypeStyle thisType;
  FooCanvasItem       *feature_group;       // the group this feature was drawn in
  ZMapFeature          feature;
  ZMapFeatureContext   feature_context;
  ZMapFeatureSet       feature_set;
  GQuark               context_key;
  GIOChannel          *channel;
  double               magFactor;
} ParamStruct;


// the function to be ultimately called when the user clicks on a canvas item.
typedef gboolean (*ZMapFeatureCallbackFunc)(ParamStruct *params, ZMapFeatureSet feature_set);


/* Set of callback routines that allow the caller to be notified when events happen
 * to a feature. */
typedef struct _ZMapFeatureCallbacksStruct
{
  ZMapWindowFeatureCallbackFunc click ;
  ZMapFeatureCallbackFunc       rightClick;
} ZMapFeatureCallbacksStruct, *ZMapFeatureCallbacks ;

#endif
/******************** end of file ****************************************/

