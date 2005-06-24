/*  File: zmapWindowEditor.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *      Roy Storey (Sanger Instutute, UK) rds@sanger.ac.uk
 *
 * Description: Allows the user to edit details of displayed objects
 *              
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Jun  8 15:32 2005 (edgrif)
 * Created: Mon Jun 6 13:00:00 (rnc)
 * CVS info:   $Id: zmapWindowEditor.c,v 1.2 2005-06-24 13:23:22 edgrif Exp $
 *-------------------------------------------------------------------
 */
#include <stdio.h>
#include <gtk/gtk.h>
#include <libfoocanvas/libfoocanvas.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapUtils.h>


typedef struct EditorDataStruct
{
  ZMapWindow zmapWindow;
  GtkWidget *window;
  FooCanvasItem *item;

  /* These are the entry widgets for the user to change the data */
  GtkWidget *feature_type;
  GtkWidget *feature_x1;
  GtkWidget *feature_x2;
  GtkWidget *feature_strand;
  GtkWidget *feature_phase; 
  GtkWidget *feature_score;

  GtkWidget *homol_type;
  GtkWidget *homol_y1;
  GtkWidget *homol_y2;
  GtkWidget *homol_strand;
  GtkWidget *homol_phase;
  GtkWidget *homol_score;

  GtkWidget *trans_cdsStart;
  GtkWidget *trans_cdsEnd;
  GtkWidget *trans_cdsPhase;
  GtkWidget *trans_startNotFound;
  GtkWidget *trans_endNotFound;

  int current_x;       /* used by applyChanges */

} editorDataStruct, *editorData;



static gboolean applyChangesCB(GtkWidget *widget, gpointer data);
static gboolean saveChangesCB (GtkWidget *widget, gpointer data);
static gboolean undoChangesCB (GtkWidget *widget, gpointer data);
static void     closeWindowCB (GtkWidget *widget, gpointer data);
static void     setupEntries  (GtkWidget *vbox, editorData editor_data, gboolean undo);
static void     setupField    (GtkWidget *vbox, char *label, 
			       GtkWidget **entry, char *value, gboolean undo);
 




void zmapWindowEditor(ZMapWindow zmapWindow, FooCanvasItem *item)
{
  GtkWidget *vbox, *buttonbox, *button;
  GtkWidget *frame, *hbox;
  ZMapFeature feature;
  editorData editor_data;

  editor_data = g_new(editorDataStruct, 1);
  editor_data->zmapWindow = zmapWindow;
  editor_data->item = item;

  feature = g_object_get_data(G_OBJECT(item), "feature");
  zMapAssert(feature);       /* something badly wrong if no feature. */

  editor_data->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (editor_data->window), "Feature editor");
  g_signal_connect (G_OBJECT (editor_data->window), "destroy",
		    G_CALLBACK (closeWindowCB), editor_data);

  gtk_container_set_border_width (GTK_CONTAINER (editor_data->window), 10);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(editor_data->window), vbox);

  frame = gtk_frame_new(g_quark_to_string(feature->original_id));
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

  buttonbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), buttonbox);

  button = gtk_button_new_with_label("Apply");
  g_signal_connect(GTK_OBJECT(button), "clicked",
		   GTK_SIGNAL_FUNC(applyChangesCB), editor_data);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0) ;

  button = gtk_button_new_with_label("Undo");
  g_signal_connect(GTK_OBJECT(button), "clicked",
		   GTK_SIGNAL_FUNC(undoChangesCB), editor_data);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0) ;

  button = gtk_button_new_with_label("Save");
  g_signal_connect(GTK_OBJECT(button), "clicked",
		   GTK_SIGNAL_FUNC(saveChangesCB), editor_data);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0) ;

  button = gtk_button_new_with_label("Close");
  g_signal_connect(GTK_OBJECT(button), "clicked",
		   GTK_SIGNAL_FUNC(closeWindowCB), editor_data);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0) ;

  setupEntries(vbox, editor_data, FALSE);

  gtk_widget_show_all(editor_data->window);
  
  return;
}





static void setupEntries(GtkWidget *vbox, editorData editor_data, gboolean undo)
{
  GtkWidget *hbox, *label;
  ZMapFeature feature;

  /* These fields hold the actual data */
  /* I think I would be better using a single GString here
   * instead of all this individual char* fields. */
  char *fType, *fX1, *fX2, *fStrand, *fPhase, *fScore;
  char *hType, *hY1, *hY2, *hStrand, *hPhase, *hScore;
  char *tStart, *tEnd, *tPhase, *tSNF, *tENF;


  feature = g_object_get_data(G_OBJECT(editor_data->item), "feature");
  editor_data->current_x = feature->x1;

  fType   = g_strdup(zmapFeatureLookUpEnum(feature->type, TYPE_ENUM));
  fX1     = g_strdup_printf("%d", feature->x1);
  fX2     = g_strdup_printf("%d", feature->x2);

  if (feature->strand == ZMAPSTRAND_FORWARD)
    fStrand = g_strdup("Forward");
  else if (feature->strand == ZMAPSTRAND_REVERSE)
    fStrand = g_strdup("Reverse");
  else
    fStrand = g_strdup("None");

  fPhase  = g_strdup_printf("%d"   , feature->phase);
  fScore  = g_strdup_printf("%6.2f", feature->score);

  /* Note, on Undo, vbox is NULL.  setupField() can cope with this. */
  setupField(vbox, "Feature type", &editor_data->feature_type  , fType  , undo); 
  setupField(vbox, "          x1", &editor_data->feature_x1    , fX1    , undo); 
  setupField(vbox, "          x2", &editor_data->feature_x2    , fX2    , undo); 
  setupField(vbox, "      Strand", &editor_data->feature_strand, fStrand, undo); 
  setupField(vbox, "       Phase", &editor_data->feature_phase , fPhase , undo); 
  setupField(vbox, "       Score", &editor_data->feature_score , fScore , undo); 

  if (undo == FALSE)
    {
      gtk_editable_set_editable(GTK_EDITABLE(editor_data->feature_type  ), FALSE);
      gtk_editable_set_editable(GTK_EDITABLE(editor_data->feature_x1    ), TRUE);
      gtk_editable_set_editable(GTK_EDITABLE(editor_data->feature_x2    ), TRUE);
      gtk_editable_set_editable(GTK_EDITABLE(editor_data->feature_strand), FALSE); /* for now */
      gtk_editable_set_editable(GTK_EDITABLE(editor_data->feature_phase ), TRUE);
      gtk_editable_set_editable(GTK_EDITABLE(editor_data->feature_score ), TRUE);

      label = gtk_label_new(" ");
      gtk_box_pack_start(GTK_BOX(vbox), label  , FALSE, FALSE, 0);
    }
 
  if (feature->type == ZMAPFEATURE_HOMOL)
    {
      hType   = g_strdup(zmapFeatureLookUpEnum(feature->feature.homol.type, TYPE_ENUM));
      hY1     = g_strdup_printf("%d", feature->feature.homol.y1);
      hY2     = g_strdup_printf("%d", feature->feature.homol.y2);

      if (feature->feature.homol.target_strand == ZMAPSTRAND_FORWARD)
	hStrand = g_strdup("Forward");
      else if (feature->feature.homol.target_strand == ZMAPSTRAND_REVERSE)
	hStrand = g_strdup("Reverse");
      else
	hStrand = g_strdup("None");

      hPhase  = g_strdup(zmapFeatureLookUpEnum(feature->feature.homol.target_phase , PHASE_ENUM));
      hScore  = g_strdup_printf("%6.2f", feature->feature.homol.score);
      
      setupField(vbox, "Homol type", &editor_data->homol_type  , hType   , undo); 
      setupField(vbox, "        y1", &editor_data->homol_y1    , hY1     , undo); 
      setupField(vbox, "        y2", &editor_data->homol_y2    , hY2     , undo); 
      setupField(vbox, "    Strand", &editor_data->homol_strand, hStrand , undo); 
      setupField(vbox, "     Phase", &editor_data->homol_phase , hPhase  , undo); 
      setupField(vbox, "     Score", &editor_data->homol_score , hScore  , undo); 

      if (undo == FALSE)
	{
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->homol_type  ), TRUE);
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->homol_y1    ), TRUE);
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->homol_y2    ), TRUE);
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->homol_strand), TRUE);
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->homol_phase ), TRUE);
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->homol_score ), TRUE);
	}
    }
  else
    {
      tStart = g_strdup_printf("%d", feature->feature.transcript.cdsStart);
      tEnd   = g_strdup_printf("%d", feature->feature.transcript.cdsEnd);
      tPhase = g_strdup_printf("%d", feature->feature.transcript.cds_phase);
      tSNF   = g_strdup(feature->feature.transcript.start_not_found ? "TRUE" : "FALSE");
      tENF   = g_strdup(feature->feature.transcript.endNotFound ? "TRUE" : "FALSE");

      setupField(vbox, "Transcript CDS Start", &editor_data->trans_cdsStart     , tStart, undo); 
      setupField(vbox, "           CDS End  ", &editor_data->trans_cdsEnd       , tEnd  , undo); 
      setupField(vbox, "               Phase", &editor_data->trans_cdsPhase     , tPhase, undo); 
      setupField(vbox, "     Start Not Found", &editor_data->trans_startNotFound, tSNF  , undo); 
      setupField(vbox, "       End Not Found", &editor_data->trans_endNotFound  , tENF  , undo); 

      if (undo == FALSE)
	{
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->trans_cdsStart     ), TRUE);
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->trans_cdsEnd       ), TRUE);
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->trans_cdsPhase     ), TRUE);
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->trans_startNotFound), TRUE);
	  gtk_editable_set_editable(GTK_EDITABLE(editor_data->trans_endNotFound  ), TRUE);
	}
    } 


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* TEMP. BECAUSE CODE CRASHES OTHERWISE.... */

  g_free(fType);
  g_free(fX1);
  g_free(fX2);
  g_free(fStrand);
  g_free(fPhase);
  g_free(fScore);
  g_free(hType);
  g_free(hY1); 
  g_free(hY2);
  g_free(hStrand);
  g_free(hPhase);
  g_free(hScore);
  g_free(tStart);
  g_free(tEnd);
  g_free(tPhase);
  g_free(tSNF);
  g_free(tENF);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return;
}
  

static void setupField(GtkWidget *vbox, char *labelString, 
		       GtkWidget **entry, char *value, gboolean undo)
{
  GtkWidget *hbox, *label;

  if (undo == FALSE)
    {
      label = gtk_label_new(labelString);
      hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(hbox), label  , FALSE, FALSE, 0);
      *entry  = gtk_entry_new();
      gtk_box_pack_end(GTK_BOX(hbox), *entry , FALSE, FALSE, 0);
      gtk_box_pack_start(GTK_BOX(vbox), hbox  , FALSE, FALSE, 0);
    }

  gtk_entry_set_text(GTK_ENTRY(*entry), value);

  return;
}



static gboolean applyChangesCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorDataStruct*)data;
  ZMapFeature feature;
  int x1, x2, xdiff;
  double xa, xb, ya, yb;
  GString *string;


  feature = g_object_get_data(G_OBJECT(editor_data->item), "feature");

  string = g_string_new(gtk_editable_get_chars(GTK_EDITABLE(editor_data->feature_x1), 0, -1));
  x1 = atoi(string->str);
  xdiff = x1 - editor_data->current_x;
  editor_data->current_x = x1;

  string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->feature_x2), 0, -1));
  x2 = atoi(string->str);

  /* this doesn't work yet.  foo_canvas_item_move() did work fine, but it just moves
   * the whole object, while the user might have changed just x2, for instance, and
   * to change the shape of the object we need to use foo_canvas_item_set I think. */
  if (feature->type == ZMAPFEATURE_HOMOL)
    {
      foo_canvas_item_move(editor_data->item, 0.0, (double)xdiff);
      foo_canvas_item_get_bounds(editor_data->item, &xa, &ya, &xb, &yb);
      foo_canvas_item_set(editor_data->item,
			  "y2", ya + (x2 - x1),
			  NULL) ;  
    }
  
  zMapWindowScrollToItem(editor_data->zmapWindow, editor_data->item) ;
  g_string_free(string, TRUE);

  return FALSE;
}



static gboolean saveChangesCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorDataStruct*)data;
  ZMapFeature feature;
  GString *string;

  feature = g_object_get_data(G_OBJECT(editor_data->item), "feature");

  applyChangesCB(NULL, (gpointer)editor_data);

  string = g_string_new(gtk_editable_get_chars(GTK_EDITABLE(editor_data->feature_x1), 0, -1));
  feature->x1 = atoi(string->str);

  string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->feature_x2), 0, -1));
  feature->x2 = atoi(string->str);
  
  string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->feature_phase), 0, -1));
  feature->phase = atoi(string->str);

  string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->feature_score), 0, -1));
  feature->score = atof(string->str);

  if (feature->type == ZMAPFEATURE_HOMOL)
    {
      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->homol_type), 0, -1));
      if (string->str == "HOMOL")
	feature->feature.homol.type = ZMAPFEATURE_HOMOL;
      else if (string->str == "TRANSCRIPT")
	feature->feature.homol.type = ZMAPFEATURE_TRANSCRIPT;

      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->homol_y1), 0, -1));
      feature->feature.homol.y1 = atoi(string->str);

      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->homol_y2), 0, -1));
      feature->feature.homol.y2 = atoi(string->str);

      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->homol_strand), 0, -1));
      if (string->str == "Forward")
	feature->feature.homol.target_strand = ZMAPSTRAND_FORWARD;
      else if (string->str == "Reverse")
	feature->feature.homol.target_strand = ZMAPSTRAND_REVERSE;
      else
	feature->feature.homol.target_strand = ZMAPSTRAND_NONE;

      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->homol_phase), 0, -1));
      feature->feature.homol.target_phase = atoi(string->str);

      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->homol_score), 0, -1));
      feature->feature.homol.score = atof(string->str);
    }
  else
    {
      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->trans_cdsStart), 0, -1));
      feature->feature.transcript.cdsStart = atoi(string->str);

      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->trans_cdsEnd), 0, -1));
      feature->feature.transcript.cdsEnd = atoi(string->str);

      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->trans_cdsPhase), 0, -1));
      feature->feature.transcript.cds_phase = atoi(string->str);

      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->trans_startNotFound), 0, -1));
      feature->feature.transcript.start_not_found = atoi(string->str);

      string = g_string_assign(string, gtk_editable_get_chars(GTK_EDITABLE(editor_data->trans_endNotFound), 0, -1));
      feature->feature.transcript.endNotFound = atoi(string->str);
    }

  g_string_free(string, TRUE);

  return FALSE;
}


/* This just reverts all values to those in the feature struct */
static gboolean undoChangesCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorDataStruct*)data;
  ZMapFeature feature;
  int current_x;

  current_x = editor_data->current_x;
  feature = g_object_get_data(G_OBJECT(editor_data->item), "feature");

  setupEntries(NULL, editor_data, TRUE);

  editor_data->current_x = current_x;
  applyChangesCB(NULL, (gpointer)editor_data);
  editor_data->current_x = feature->x1;

  return FALSE;
}



static void closeWindowCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorDataStruct*)data;

  gtk_widget_destroy(GTK_WIDGET(editor_data->window));
  g_free(editor_data);

  return;
}





/********************* end of file ********************************/
