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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: Draws genomic features in the data display window.
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: Jul 15 16:02 2004 (rnc)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.3 2004-07-15 16:34:51 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapDraw.h>

typedef struct _ParamStruct {
  FooCanvas *thisCanvas;
  int height;
  int length;
} ParamStruct;



static void zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data);


void zmapWindowDrawFeatures(FooCanvas *canvas, ZMapFeatureContext feature_context)
{
  float offset = 20.0;
  float result;
  double x1, x2, y1, y2;
  GtkWidget *parent, *label, *vbox, *vscale, *frame;
  GtkRequisition req;
  ParamStruct params;
  int ticks;    // number of tickmarks on scale
  int scaleUnit = 100; // spacing of tickmarks on scalebar
  int seqUnit;  // spacing, in bases of tickmarks
  int type;     // nomial, ie whether in K, M, etc.
  int unitType;
  int seqPos;
  int scalePos;
  int height, width;
  char cp[20], unitName[] = { 0, 'k', 'M', 'G', 'T', 'P' }, buf[2] ;
  GtkWidget *box;

  //  parent = gtk_widget_get_parent(GTK_WIDGET(canvas));
  //  gtk_widget_size_request(parent, &req);
  foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);  // for display panel
  //  gtk_widget_size_request(navHBox, &req);                    // for navigator

  params.thisCanvas = canvas;
  params.height = y2 - y1;
  params.length = feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1;

  // Draw the navigator vscale
  // how many tickmarks do we want?
  //  ticks = req.height/scaleUnit;
  //what is their spacing in bases
  //  seqUnit = (feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1)/ticks;
  // are we working in K, M, or what?
  //  for (type = 1, unitType = 0 ; 
  //       seqUnit > 0 && 1000 * type < seqUnit && unitType < 5; 
  //       unitType++, type *= 1000) ;
  // round seqUnit sensibly
  //  seqUnit = ((seqUnit + type/2)/type) * type;

  // a vbox to hold all the scale numbers packed in
  //  vbox = gtk_vbox_new(TRUE, 0);
  //  gtk_box_pack_start(GTK_BOX(navHBox), vbox, FALSE, FALSE, 0);

  // mark up the scale
  /*  for (scalePos = 0, seqPos = feature_context->sequence_to_parent.c1; 
       scalePos < req.height; 
       scalePos += scaleUnit, seqPos += seqUnit)
    {
      buf[0] = unitName[unitType] ; buf[1] = 0 ;
      sprintf (cp, "%d%s", seqPos/type, buf) ;
      if (width < strlen (cp))
        width = strlen (cp) ;
      box = gtk_hbox_new(TRUE, 0);
      label = gtk_label_new(cp);
      gtk_container_add(GTK_CONTAINER(box), label);
      gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);
    }		     
  */
  // just trying out the vscale in case it's useful
  //  vscale = gtk_vscale_new_with_range(0, req.height, scaleUnit);
  //  gtk_box_pack_start(GTK_BOX(navHBox), vscale, FALSE, FALSE, 0);

  result = zmapDrawScale(canvas   , offset, 
			 feature_context->sequence_to_parent.c1, 
			 feature_context->sequence_to_parent.c2);

  if (feature_context)
      g_datalist_foreach(&(feature_context->features), zmapWindowProcessFeatureSet, &params);

  //  gtk_widget_show_all(navHBox);
  return;
}


static void zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet zMapFeatureSet = (ZMapFeatureSet)data;
  ZMapFeature zMapFeature;
  ZMapSpan zMapSpan, prevSpan;
  ZMapAlignBlock *zMapAlign, *prevAlign;
  ParamStruct *params = (ParamStruct*)user_data;
  int i, j;
  double offset = 40.0, column_spacing = 40.0, column_position;
  double box_width = 10.0;
  FooCanvasItem *group;
  int x1, x2, y0, y1, y2;
  float middle, line_width = 1.0;


  // NB for each column we create a new canvas group, with the initial y coordinate set to
  // 5.0 to just drop it a teeny bit from the top of the window.  All items live in the group.
  for (i = 0; i < zMapFeatureSet->features->len; i++)
    {
      column_position = offset + column_spacing;

      zMapFeature = &g_array_index(zMapFeatureSet->features, ZMapFeatureStruct, i);

      switch (zMapFeature->type)
	{
	case ZMAPFEATURE_HOMOL:     /* type 1 is a homol */
	  group = foo_canvas_item_new(foo_canvas_root(params->thisCanvas),
				      foo_canvas_group_get_type(),
				      "x",(double)column_position,
				      "y",(double)5.0,
				      NULL);
	      
	  zmapDrawBox(group, column_position, (zMapFeature->feature.homol.y1 * params->height / params->length), 
		      column_position + box_width, (zMapFeature->feature.homol.y2 * params->height / params->length),
		      "black", "green");
	  break;

	case ZMAPFEATURE_TRANSCRIPT:     /* type 4 is a transcript */
	  column_position = offset;
	  group = foo_canvas_item_new(foo_canvas_root(params->thisCanvas),
				      foo_canvas_group_get_type(),
				      "x",(double)column_position,
				      "y",(double)5.0,
				      NULL);
	  
	  for (j = 1; j < zMapFeature->feature.transcript.exons->len; j++)
	    {
	      zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j);
	      prevSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j-1);
	      middle = ((prevSpan->x2 + zMapSpan->x1)/2) * params->height/params->length;
	      y0 = prevSpan->x1 * params->height/params->length; // y coord of preceding exon
	      y1 = prevSpan->x2 * params->height/params->length;
	      y2 = zMapSpan->x1 * params->height/params->length;

	      // skip this intron if its preceding exon is too small to draw.  We use
	      // y0 and y1 since they are screen coordinates independent of magnification.
	      // Note this is a kludge which will meet its come-uppance during zooming
	      // because zooming doesn't revisit this block of code, so tiny exons will
	      // appear when zoomed, but the intervening introns will be lost forever.

	      if (y2 > y1 && y0 +2 <= y1)
		{
		  zmapDrawLine(FOO_CANVAS_GROUP(group), column_position + box_width/2, y1,
			       column_position + box_width, middle, "blue", line_width);
		  zmapDrawLine(FOO_CANVAS_GROUP(group), column_position + box_width, middle, 
			       column_position + box_width/2, y2, "blue", line_width);
		}
	    }	  
	  for (j = 0; j < zMapFeature->feature.transcript.exons->len; j++)
	    {
	      zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j);
	      
	      zmapDrawBox(group, column_position, (zMapSpan->x1 * params->height / params->length), 
			  column_position + box_width, (zMapSpan->x2 * params->height / params->length),
			  "black", "blue");
	    }
	  break;

	default:
	  break;
	}
    }
  
  return;
}
/****************** end of file ************************************/
