/*  File: zmapWindowDrawFeatures.c
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk and
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *
 * Description: Draws genomic features in the data display window.
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: Jul 21 12:12 2004 (edgrif)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.7 2004-07-21 11:15:30 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapDraw.h>

typedef struct _ParamStruct
{
  FooCanvas *thisCanvas;
  FooCanvasItem *columnGroup;
  double height;
  double length;
  double column_position;
  GData *types;
  ZMapFeatureTypeStyle thisType;
} ParamStruct;



static void zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data);
static void zmapWindowProcessFeature(GQuark key_id, gpointer data, gpointer user_data);


void zmapWindowDrawFeatures(FooCanvas *canvas, ZMapFeatureContext feature_context, GData *types)
{
  double offset = 20.0;
  float result;
  double x1, x2, y1, y2;
  GtkWidget *parent, *label, *vbox, *vscale, *frame;
  ParamStruct params;

  foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);  // for display panel

  params.thisCanvas = canvas;
  params.height = (y2 - y1)*2; // arbitrarily double the size for now
  params.length = feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1;
  params.column_position = 20.0;
  params.types = types;

  result = zmapDrawScale(canvas, offset, 
			 feature_context->sequence_to_parent.c1, 
			 feature_context->sequence_to_parent.c2);

  if (feature_context)
    g_datalist_foreach(&(feature_context->feature_sets), zmapWindowProcessFeatureSet, &params);


  return;
}



/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ParamStruct *params = (ParamStruct*)user_data;

  FooCanvasItem *col_group;  // each feature_set represents a column
  double column_spacing = 20.0;


  // NB for each column we create a new canvas group, with the initial y coordinate set to
  // 10.0 to just drop it a teeny bit from the top of the window.  All items live in the group.
  // Note that this y coord adjustment is directly linked to the the manipulation of 
  // params->height performed in zmapWindowDrawFeatures.  Change them together or not at all.
  params->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(params->types), feature_set->source) ;

  params->column_position += column_spacing;

  params->columnGroup = foo_canvas_item_new(foo_canvas_root(params->thisCanvas),
					    foo_canvas_group_get_type(),
					    "x", (double)params->column_position,
					    "y", (double)10.0,
					    NULL);

  g_datalist_foreach(&(feature_set->features), zmapWindowProcessFeature, user_data) ;

  return ;
}



/* ROB, what I have done may mess up your group structs...e.g. how does the group id get
 * into this routine ? Needs to be added to your params struct maybe ? I didn't try and sort
 * this out as I didn't want to mess stuff up */

static void zmapWindowProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature zMapFeature = (ZMapFeature)data ;
  ParamStruct *params = (ParamStruct*)user_data;

  ZMapSpan zMapSpan, prevSpan;
  ZMapAlignBlock *zMapAlign, *prevAlign;
  int i, j;
  FooCanvasItem *feature_group;
  int x1, x2, y0, y1, y2;
  float middle, line_width = 1.0;
  double magFactor = params->height/params->length;

  if (!params->thisType)       // this needs to be handled properly
    {
      params->thisType = g_new0(ZMapFeatureTypeStyleStruct, 1);

      params->thisType->width = 10.0;
      params->thisType->foreground = "dark blue";
      params->thisType->background = "white";
    }

  switch (zMapFeature->type)
    {
    case ZMAPFEATURE_BASIC: case ZMAPFEATURE_VARIATION:     /* type 0 is a basic, 5 is allele */
    case ZMAPFEATURE_BOUNDARY: case ZMAPFEATURE_SEQUENCE:   /* boundary is 6, sequence is 7 */
      feature_group = foo_canvas_item_new(FOO_CANVAS_GROUP(params->columnGroup),
					  foo_canvas_group_get_type(),
					  "x", (double)params->column_position,
					  "y", (double)zMapFeature->x1 * magFactor,
					  NULL);
	      
      if (zMapFeature->x2 > zMapFeature->x2)
	{
	  zmapDrawBox(feature_group, params->column_position,
		      (zMapFeature->x1 * magFactor), 
		      params->column_position + params->thisType->width, 
		      (zMapFeature->x2 * magFactor),
		      "black", params->thisType->foreground);
	}
      break;

    case ZMAPFEATURE_HOMOL:     /* type 1 is a homol */
      feature_group = foo_canvas_item_new(foo_canvas_root(params->thisCanvas),
				  foo_canvas_group_get_type(),
				  "x", (double)params->column_position,
				  "y", (double)zMapFeature->x1 * magFactor,
				  NULL);

      if (zMapFeature->feature.homol.y2 > zMapFeature->feature.homol.y1)
	{	      
	  zmapDrawBox(feature_group, params->column_position,
		      (zMapFeature->feature.homol.y1 * magFactor), 
		      params->column_position + params->thisType->width, 
		      (zMapFeature->feature.homol.y2 * magFactor),
		      "black", params->thisType->foreground);
	}
      break;

    case ZMAPFEATURE_TRANSCRIPT:     /* type 4 is a transcript */
      feature_group = foo_canvas_item_new(foo_canvas_root(params->thisCanvas),
				  foo_canvas_group_get_type(),
				  "x", (double)params->column_position,
				  "y", (double)zMapFeature->x1 * magFactor,
				  NULL);
	  
      for (j = 1; j < zMapFeature->feature.transcript.exons->len; j++)
	{
	  zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j);
	  prevSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j-1);
	  middle = ((prevSpan->x2 + zMapSpan->x1)/2) * magFactor;
	  y0 = prevSpan->x1 * magFactor; // y coord of preceding exon
	  y1 = prevSpan->x2 * magFactor;
	  y2 = zMapSpan->x1 * magFactor;

	  // skip this intron if its preceding exon is too small to draw.  We use
	  // y0 and y1 since they are screen coordinates independent of magnification.
	  // Note this is a kludge which will meet its come-uppance during zooming
	  // because zooming doesn't revisit this block of code, so tiny exons will
	  // appear when zoomed, but the intervening introns will be lost forever.

	  if (y2 > y1 && y0+2 <= y1)
	    {
	      zmapDrawLine(FOO_CANVAS_GROUP(feature_group), params->column_position + params->thisType->width/2, y1,
			   params->column_position + params->thisType->width, middle, 
			   params->thisType->foreground, line_width);
	      zmapDrawLine(FOO_CANVAS_GROUP(feature_group), params->column_position + params->thisType->width, middle, 
			   params->column_position + params->thisType->width/2, y2, 
			   params->thisType->foreground, line_width);
	    }
	}	  
      for (j = 0; j < zMapFeature->feature.transcript.exons->len; j++)
	{
	  zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j);
	      
	  if (zMapSpan->x2 > zMapSpan->x1)
	    {
	      zmapDrawBox(feature_group, params->column_position,
			  (zMapSpan->x1 * magFactor), 
			  params->column_position + params->thisType->width,
			  (zMapSpan->x2 * magFactor),
			  "black", params->thisType->foreground);
	    }
	}
      break;

    default:
      break;
    }
  
  return;
}



/****************** end of file ************************************/
