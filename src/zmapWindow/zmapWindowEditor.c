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
 * Last edited: Jul  5 14:55 2005 (rnc)
 * Created: Mon Jun 6 13:00:00 (rnc)
 * CVS info:   $Id: zmapWindowEditor.c,v 1.3 2005-07-05 14:53:17 rnc Exp $
 *-------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libfoocanvas/libfoocanvas.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapUtils.h>


/* this struct used for stringifying arrays. */
typedef struct ArrayRowStruct
{ 
  char *x1;  
  char *x2;
  char *x3;
  char *x4;
} arrayRowStruct, *ArrayRow;


typedef enum {LABEL, TEXT, ENTRY, CHECK, RADIO, COMBO, SPAN, ALIGN, LAST} fieldType;
enum {COL1, COL2, COL3, COL4, N_COLS};  /* columns to display arrays */


typedef struct MainTableStruct
{
  fieldType  fieldtype;  /* ENTRY, RADIO, etc */
  char      *name;  /* probably redundant */
  char      *label;
  GtkWidget *widget;
  union
  {
    char         *entry;
    GtkListStore *listStore;
  } value;
  gboolean editable;
} mainTableStruct, *mainTable;



typedef struct EditorDataStruct
{
  GtkWidget *window;
  GtkWidget *vbox;        /* most attributes are just stacked in here */
  GtkWidget *hbox;        /* exon & intron arrays stacked side by side. */
  FooCanvasItem *item;
  GQuark original_id;
  mainTable table;
  ZMapFeature originalFeature;

} editorDataStruct, *editorData;



/* function prototypes ************************************/

static void parseFeature(mainTableStruct table[], ZMapFeature feature);
static void parseField(mainTable table, ZMapFeature feature);
static void array2List(mainTable table, GArray *array, ZMapFeatureType feature_type);

static void createWindow(editorData editor_data);
static void closeWindowCB (GtkWidget *widget, gpointer data);
static void undoChangesCB(GtkWidget *widget, gpointer data);
static void saveChangesCB(GtkWidget *widget, gpointer data);

static void addFields (editorData editor_data);
static void addArrayCB(gpointer data, editorData editor_data);
static void addRadioGroupCB(GtkWidget *vbox, gpointer data);
static void addCheckButtonCB(GtkWidget *vbox, gpointer data);
static void addEntryCB(GtkWidget *vbox, gpointer data);
static void addLabelCB(GtkWidget *vbox, gpointer data);
static void arrayEditedCB(GtkCellRendererText *renderer, 
			  char *path, char *new_text, gpointer user_data);

static void freeTable(mainTableStruct table[]);


/* function code *******************************************/

void zmapWindowEditor(ZMapWindow zmapWindow, FooCanvasItem *item)
{
  ZMapFeature feature;
  editorData editor_data;

  /* Keep the freeTable() function in sync with this */
  mainTableStruct init[] = {{ LABEL, "feature_type"  , "Type"           , NULL, NULL, FALSE },
			    { ENTRY, "feature_x1"    , "Start"          , NULL, NULL, TRUE  },
			    { ENTRY, "feature_x2"    , "End"            , NULL, NULL, TRUE  },
			    { ENTRY, "feature_strand", "Strand"         , NULL, NULL, FALSE },
			    { ENTRY, "feature_phase" , "Phase"          , NULL, NULL, FALSE },
			    { ENTRY, "feature_score" , "Score"          , NULL, NULL, TRUE  },
			    
			    { LABEL, "blank line"    , " "              , NULL, NULL, TRUE  },
			    
			    { ENTRY, "homol_type"    , "Type"           , NULL, NULL, TRUE  },
			    { ENTRY, "homol_y1"      , "Start"          , NULL, NULL, TRUE  },
			    { ENTRY, "homol_y2"      , "End"            , NULL, NULL, TRUE  },
			    { ENTRY, "homol_strand"  , "Strand"         , NULL, NULL, FALSE },
			    { ENTRY, "homol_phase"   , "Phase"          , NULL, NULL, FALSE },
			    { ENTRY, "homol_score"   , "Score"          , NULL, NULL, TRUE  },
			    { ALIGN, "homol_align"   , "Alignments"     , NULL, NULL, TRUE  },
			    
			    { ENTRY, "trans_Start"   , "CDS Start"      , NULL, NULL, TRUE  },
			    { ENTRY, "trans_End"     , "CDS End"        , NULL, NULL, TRUE  },
			    { ENTRY, "trans_Phase"   , "CDS Phase"      , NULL, NULL, FALSE },
			    { CHECK, "trans_SNF"     , "Start Not Found", NULL, NULL, TRUE  },
			    { CHECK, "trans_ENF"     , "End Not Found " , NULL, NULL, TRUE  },
			    { SPAN , "trans_exons"   , "Exons"          , NULL, NULL, TRUE  },
			    { SPAN , "trans_introns" , "Introns"        , NULL, NULL, TRUE  },
			    
			    { LAST , NULL, NULL, NULL, NULL, NULL }};

  feature = g_object_get_data(G_OBJECT(item), "feature");
  zMapAssert(feature);       /* something badly wrong if no feature. */

  editor_data = g_new(editorDataStruct, 1);
  editor_data->item = item;
  editor_data->original_id = feature->original_id;
  editor_data->hbox = NULL;
  editor_data->table = (mainTable)g_memdup(init, sizeof(init));

  parseFeature(editor_data->table, feature);
  createWindow(editor_data);
  addFields(editor_data);

  gtk_widget_show_all(editor_data->window);
  
  return;
}





/* So that the draw routines don't need to know more than the minimum about what they're
 * drawing, everything in the table, apart from the arrays, is held as a string. */
static void parseFeature(mainTableStruct table[], ZMapFeature feature)
{
  int i = 0;

  while (table[i].fieldtype != LAST)
    {
      switch (i)
	{
	case 0:
	  table[i].value.entry = zmapFeatureLookUpEnum(feature->type, TYPE_ENUM);
	  break;
	case 1:
	  table[i].value.entry = g_strdup_printf("%d", feature->x1);
	  break;
	case 2:
	  table[i].value.entry = g_strdup_printf("%d", feature->x2);
	  break;
	case 3:
	  switch (feature->strand)
	    {
	    case ZMAPSTRAND_FORWARD:
	      table[i].value.entry = "Forward";
	      break;
	    case ZMAPSTRAND_REVERSE:
	      table[i].value.entry = "Reverse";
	      break;
	    case ZMAPSTRAND_NONE:
	      table[i].value.entry = "Unspecified";
	      break;
	    }
	  break;
	case 4:
	  table[i].value.entry = g_strdup_printf("%d", feature->phase);
	  break;
	case 5:
	  table[i].value.entry = g_strdup_printf("%.2f", feature->score);
	  break;
	case 6:
	  table[i].value.entry = " ";  /* blank label to separate feature fields from the rest */
	  break;
	case 7:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	  switch (feature->feature.homol.type)
	    {
	    case ZMAPHOMOL_X_HOMOL:
	      table[i].value.entry = "X";
	      break;
	    case ZMAPHOMOL_N_HOMOL:
	      table[i].value.entry = "N";
	      break;
	    case ZMAPHOMOL_TX_HOMOL:
	      table[i].value.entry = "TX";
	      break;
	    }
	  break;
	case 8:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    table[i].value.entry = g_strdup_printf("%d", feature->feature.homol.y1);
	  break;
	case 9:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    table[i].value.entry = g_strdup_printf("%d", feature->feature.homol.y2);
	  break;
	case 10:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    {
	      switch (feature->feature.homol.target_strand)
		{
		case ZMAPSTRAND_FORWARD:
		  table[i].value.entry = "Forward";
		  break;
		case ZMAPSTRAND_REVERSE:
		  table[i].value.entry = "Reverse";
		  break;
		case ZMAPSTRAND_NONE:
		  table[i].value.entry = "Unspecified";
		  break;
		}
	    }
	  break;
	case 11:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    table[i].value.entry = g_strdup_printf("%d", feature->feature.homol.target_phase);
	  break;
	case 12:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    table[i].value.entry = g_strdup_printf("%.2f", feature->feature.homol.score);
	  break;
	case 13:
	  if (feature->type == ZMAPFEATURE_HOMOL
	      && feature->feature.homol.align->len > 0)
	    array2List(&table[i], feature->feature.homol.align, feature->type);
	  break;
	case 14:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    table[i].value.entry = g_strdup_printf("%d", feature->feature.transcript.cdsStart);
	  break;
	case 15:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    table[i].value.entry = g_strdup_printf("%d", feature->feature.transcript.cdsEnd);
	  break;
	case 16:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    table[i].value.entry = g_strdup_printf("%d", feature->feature.transcript.cds_phase);
	  break;
	case 17:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    table[i].value.entry = feature->feature.transcript.start_not_found ? "True" : "False";
	  break;
	case 18:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    table[i].value.entry = feature->feature.transcript.endNotFound ? "True" : "False";
	  break;
	case 19:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT
	      && feature->feature.transcript.exons->len > 0)
	    array2List(&table[i], feature->feature.transcript.exons, feature->type);
	  break;
	case 20:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT
	      && feature->feature.transcript.introns->len > 0)
	    array2List(&table[i], feature->feature.transcript.introns, feature->type);
	  break;
	}
      i++;
    } 
  return;
}



static void array2List(mainTable table, GArray *array, ZMapFeatureType feature_type)
{
  int i;
  ZMapSpan span;
  ZMapAlignBlock align;
  GtkTreeIter iter;

  table->value.listStore = gtk_list_store_new(N_COLS, 
					      G_TYPE_INT,
					      G_TYPE_INT,
					      G_TYPE_INT,
					      G_TYPE_INT);
  
  for (i = 0; i < array->len; i++)
    {
      gtk_list_store_append (table->value.listStore, &iter);  /* Acquire an iterator */
      if (feature_type == ZMAPFEATURE_TRANSCRIPT)
	{
	  span = &g_array_index(array, ZMapSpanStruct, i);
	  gtk_list_store_set (table->value.listStore, &iter,
			      COL1, span->x1,
			      COL2, span->x2,
			      COL3, NULL,
			      COL4, NULL,
			      -1);
	}
      else if (feature_type == ZMAPFEATURE_HOMOL)
	{
	  align = &g_array_index(array, ZMapAlignBlockStruct, i);
	  gtk_list_store_set (table->value.listStore, &iter,
			      COL1, align->q1,
			      COL2, align->q2,
			      COL3, align->t1,
			      COL4, align->t2,
			      -1);
	}
    }
  
  return;
}

static void createWindow(editorData editor_data)
{
  GtkWidget *buttonbox, *button;
  GtkWidget *frame, *hbox;

  editor_data->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (editor_data->window), "Feature editor");
  g_signal_connect (G_OBJECT (editor_data->window), "destroy",
		    G_CALLBACK (closeWindowCB), editor_data);

  gtk_container_set_border_width (GTK_CONTAINER (editor_data->window), 10);

  editor_data->vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(editor_data->window), editor_data->vbox);

  frame = gtk_frame_new(g_quark_to_string(editor_data->original_id));
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(editor_data->vbox), frame, FALSE, FALSE, 0);

  buttonbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), buttonbox);

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

  return;
}



static void addFields(editorData editor_data)
{
  mainTable table = editor_data->table;
  GtkWidget *hbox;
  int i = 0;

  while (table[i].fieldtype != LAST)
    {
      switch (table[i].fieldtype)
	{
	case LABEL:
	  addLabelCB(editor_data->vbox, &table[i]);
	  break;
	  
	case ENTRY:
	  if (table[i].value.entry != NULL)
	    addEntryCB(editor_data->vbox, &table[i]);
	  break;
	  
	case RADIO:
	  if (table[i].value.entry != NULL)
	    addRadioGroupCB(editor_data->vbox, &table[i]);
	  break;
	  
	case CHECK:
	  if (table[i].value.entry != NULL)
	    addCheckButtonCB(editor_data->vbox, &table[i]);
	  break;
	  
	case ALIGN:
	  if (table[i].value.listStore != NULL)
	    addArrayCB(&table[i], editor_data);
	  break;

	case SPAN:
	  if (table[i].value.listStore != NULL)
	    addArrayCB(&table[i], editor_data);
	  break;

	}
      i++;
    }

  return;
}



static void addLabelCB(GtkWidget *vbox, gpointer data)
{
  GtkWidget *hbox, *label;
  mainTable table = (mainTable)data;

  label = gtk_label_new(table->label);
  hbox  = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  label = gtk_label_new(table->value.entry);
  gtk_box_pack_end(GTK_BOX(hbox), label , FALSE, FALSE, 0);
  
  gtk_box_pack_start(GTK_BOX(vbox), hbox  , FALSE, FALSE, 0);

  return;
}


  

static void addEntryCB(GtkWidget *vbox, gpointer data)
{
  GtkWidget *hbox, *label;
  mainTable table = (mainTable)data;

  label = gtk_label_new(table->label);
  hbox  = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  table->widget  = gtk_entry_new();
  gtk_box_pack_end(GTK_BOX(hbox), table->widget , FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox  , FALSE, FALSE, 0);
  
  gtk_entry_set_text(GTK_ENTRY(table->widget), table->value.entry);

  if (table->editable == FALSE)
    gtk_widget_set_sensitive(table->widget, FALSE);

  return;
}



/* incomplete - never called */
static void addRadioGroupCB(GtkWidget *vbox, gpointer data)
{
  GtkWidget *hbox, *label;
  mainTable table = (mainTable)data;
  int i;

  label = gtk_label_new(table->label);
  hbox  = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  return;
}




static void addCheckButtonCB(GtkWidget *vbox, gpointer data)
{
  GtkWidget *hbox, *label;
  mainTable table = (mainTable)data;

  label = gtk_label_new(table->label);
  hbox  = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  table->widget = gtk_check_button_new();
  gtk_box_pack_start(GTK_BOX(hbox), table->widget, FALSE, FALSE, 0);

  if (strcmp(table->value.entry, "True") == 0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->widget), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->widget), FALSE);

  return;
}





static void addArrayCB(gpointer data, editorData editor_data)
{
  mainTable table = (mainTable)data;
  GtkWidget *treeView, *frame, *scrolledWindow;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  char *label1 = "Start", *label2 = "End", *frameLabel;
  int column_width = 50;


  gtk_window_resize(GTK_WINDOW(editor_data->window), 100, 600); 

  treeView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (table->value.listStore));
  g_object_unref (G_OBJECT (table->value.listStore));

  renderer = gtk_cell_renderer_text_new ();
  g_object_set(G_OBJECT(renderer), "editable", TRUE, "editable_set", TRUE, NULL);
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (arrayEditedCB), editor_data);

  column = gtk_tree_view_column_new_with_attributes (label1,
						     renderer,
						     "text", COL1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
  gtk_tree_view_column_set_min_width(column, column_width);

  column = gtk_tree_view_column_new_with_attributes (label2,
						     renderer,
						     "text", COL2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
  gtk_tree_view_column_set_min_width(column, column_width);

  if (table->fieldtype == ALIGN)
    {
      column = gtk_tree_view_column_new_with_attributes (label1,
							 renderer,
							 "text", COL3,
							 NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
      gtk_tree_view_column_set_min_width(column, column_width);
      
      column = gtk_tree_view_column_new_with_attributes (label2,
							 renderer,
							 "text", COL4,
							 NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
      gtk_tree_view_column_set_min_width(column, column_width);
    }
  
  gtk_object_destroy(GTK_OBJECT(renderer));
  gtk_object_destroy(GTK_OBJECT(column));

  if (editor_data->hbox == NULL)
    {
      editor_data->hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(editor_data->vbox), editor_data->hbox, TRUE, TRUE, 10);

      if (table->fieldtype == ALIGN)
	frameLabel = "Query              Target";
      else
	frameLabel = "Exons";
    }
  else
    frameLabel = "Introns";

  frame = gtk_frame_new(frameLabel);
  gtk_box_pack_start(GTK_BOX(editor_data->hbox), frame, TRUE, TRUE, 0);

  g_free(label1);
  g_free(label2);
  g_free(frameLabel);

  scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(frame), scrolledWindow);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(scrolledWindow), treeView);

  return;
}



static void arrayEditedCB(GtkCellRendererText *renderer, 
			  char *path, char *new_text, gpointer user_data)
{
  printf("arrayEditCB called\n");
  return;
}
 

static void undoChangesCB(GtkWidget *widget, gpointer data)
{
  return;
}



static void saveChangesCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorData)data;
  mainTable table = editor_data->table;
  ZMapFeature feature = g_object_get_data(G_OBJECT(editor_data->item), "feature");
  const char *value;
  int i;
  gboolean pressed;

  editor_data->originalFeature = zMapFeatureCopy(feature);

  for (i = 1; table[i].fieldtype != LAST; i++)
    {
      switch (table[i].fieldtype)
	{
	case ENTRY:
	  if (table[i].widget != NULL)
	    {
	      value = gtk_entry_get_text(GTK_ENTRY(table[i].widget));
	      
	      switch (i)
		{
		case 1:
		  feature->x1 = atoi(value);
		  break;
		  
		case 2:
		  feature->x2 = atoi(value);
		  break;
		  
		case 3: /* strand */ case 4: /* phase */ 
		case 10: /* homol strand */ case 11: /* homol phase */
		case 16: /* transcript phase */ 
		  break;
		  
		case 5:
		  feature->score = atof(value);
		  break;
		  
		case 7:
		  if (feature->type == ZMAPFEATURE_HOMOL)
		    if (value == "X")
		      feature->feature.homol.type = ZMAPHOMOL_X_HOMOL;
		    else if (value == "N")
		      feature->feature.homol.type = ZMAPHOMOL_N_HOMOL;
		    else if (value == "TX")
		      feature->feature.homol.type = ZMAPHOMOL_TX_HOMOL;
		  break;
		  
		case 8:
		  if (feature->type == ZMAPFEATURE_HOMOL)
		    feature->feature.homol.y1 = atoi(value);
		  break;
		  
		case 9:
		  if (feature->type == ZMAPFEATURE_HOMOL)
		    feature->feature.homol.y2 = atoi(value);
		  break;
		  
		case 12:
		  if (feature->type == ZMAPFEATURE_HOMOL)
		    feature->feature.homol.score = atof(value);
		  break;
		  
		case 14:
		  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
		    feature->feature.transcript.cdsStart = atoi(value);
		  break;
		  
		case 15:
		  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
		    feature->feature.transcript.cdsEnd = atoi(value);
		  break;
		}
	    }
	  break;

	case CHECK:
	  if (table[i].widget != NULL)
	    {
	      pressed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table[i].widget));
	      if (i == 17)
		feature->feature.transcript.start_not_found = pressed;
	      else if (i == 18)
		feature->feature.transcript.endNotFound = pressed;
	    }
	  break;
	}
    }
  return;
}



static void closeButtonCB(GtkWidget *widget, gpointer data)
{

  return;
}
  


static void closeWindowCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorDataStruct*)data;
    
  if (editor_data->table != NULL)                                                                            
    freeTable(editor_data->table);
  editor_data->table = NULL;  /* stop it trying to free the data twice */

  gtk_widget_destroy(GTK_WIDGET(editor_data->window));
  g_free(editor_data);
                                                                                
  return;
}



/* Keep this case statement in sync with the table structure,
 * so you free off the variables correctly once they're done with. */
static void freeTable(mainTableStruct table[])
{
  int i;

  for (i = 1; table[i].fieldtype != LAST; i++)
    {
      switch (i)
	{
	case 1: case 2: case 4: case 5: case 8: case 9:
	case 11: case 12: case 14: case 15: case 16:
	  if (table[i].value.entry != NULL)
	    g_free(table[i].value.entry);
	  break;
	}
    } 

  g_free(table);

  return;
}



/********************* end of file ********************************/
