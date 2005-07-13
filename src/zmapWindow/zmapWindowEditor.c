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
 * Last edited: Jul 13 12:39 2005 (rnc)
 * Created: Mon Jun 6 13:00:00 (rnc)
 * CVS info:   $Id: zmapWindowEditor.c,v 1.7 2005-07-13 11:42:04 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>

/* this struct used to build the displays
 * of variable data, eg exons, introns, etc */
typedef struct ColInfoStruct
{
  char *label;
  int colNo;
} colInfoStruct;


/* this struct used for stringifying arrays. */
typedef struct ArrayRowStruct
{ 
  char *x1;  
  char *x2;
  char *x3;
  char *x4;
} arrayRowStruct, *ArrayRow;


typedef enum {LABEL, ENTRY, STRAND, PHASE, FLOAT, 
	      TYPE, CHECK, ALIGN, EXON, INTRON, LAST} fieldType;

typedef enum {FTYPE, FX1, FX2, FSTRAND, FPHASE, FSCORE, FLABEL,
	      HTYPE, HY1, HY2, HSTRAND, HPHASE, HSCORE, HALIGN,
	      TX1, TX2, TPHASE, TSNF, TENF, TEXON, TINTRON} fieldNo;

enum {COL1, COL2, COL3, COL4, N_COLS};  /* columns to display arrays */


/* An array of this struct is the main driver for the program */
typedef struct MainTableStruct
{
  fieldType  fieldtype;  /* controls how the field is maniuplated. */
  fieldNo    fieldno;    /* for initial loading of data */
  char      *label;
  void      *fieldPtr;   /* the field in the modified feature */
  void      *OFfieldPtr; /* the field in the original feature */
  GtkWidget *widget;     /* the widget on the screen */
  union
  {
    char         *entry;     /* the value being displayed */
    GtkListStore *listStore; /* the list of aligns, exons, introns, etc */
  } value;
  gboolean editable;
} mainTableStruct, *mainTable;



typedef struct EditorDataStruct
{
  ZMapWindow zmapWindow;
  GtkWidget *window;
  GtkWidget *vbox;        /* most attributes are just stacked in here */
  GtkWidget *hbox;        /* exon & intron arrays stacked side by side. */
  FooCanvasItem *item;
  GQuark original_id;
  mainTable table;
  ZMapFeature originalFeature;
  ZMapFeature modifiedFeature;
  GtkTreeModel *selectedModel;

} editorDataStruct, *editorData;



/* function prototypes ************************************/

static void parseFeature(mainTableStruct table[], ZMapFeature origFeature, ZMapFeature feature);
static void parseField(mainTable table, ZMapFeature feature);
static void array2List(mainTable table, GArray *array, ZMapFeatureType feature_type);

static void createWindow(editorData editor_data);
static void closeWindowCB(GtkWidget *widget, gpointer data);
static void closeButtonCB(GtkWidget *widget, gpointer data);
static void undoChangesCB(GtkWidget *widget, gpointer data);
static void applyChangesCB(GtkWidget *widget, gpointer data);

static gboolean validateEntry(const char *value);
static gboolean validateArrays(editorData editor_data);
static gboolean checkAllCoords(int x1, int x2, GArray *array);
static int      getNextElement(GArray *array, int start);

static void saveChangesCB(GtkWidget *widget, gpointer data);
static void updateArray(mainTable table, GArray *array);
static void refreshArray(mainTable table, GArray *array);

static void addFields (editorData editor_data);
static void addArrayCB(gpointer data, editorData editor_data);
static void addCheckButtonCB(GtkWidget *vbox, gpointer data);
static void addEntryCB(GtkWidget *vbox, gpointer data);
static void addLabelCB(GtkWidget *vbox, gpointer data);

static void buildCol(colInfoStruct colInfo, GtkWidget *treeView, mainTable table);
static void arrayEditedCB(GtkCellRendererText *renderer, 
			  char *path, char *new_text, gpointer user_data);
static void redrawFeature(editorData editor_data);

static void freeTable(mainTableStruct table[]);


/* function code *******************************************/

void zmapWindowEditor(ZMapWindow zmapWindow, FooCanvasItem *item)
{
  ZMapFeature feature;
  editorData editor_data;

  mainTableStruct init[] = {{ LABEL , FTYPE  , "Type"           , NULL, NULL, NULL, NULL, FALSE },
			    { ENTRY , FX1    , "Start"          , NULL, NULL, NULL, NULL, TRUE  },
			    { ENTRY , FX2    , "End"            , NULL, NULL, NULL, NULL, TRUE  },
			    { STRAND, FSTRAND, "Strand"         , NULL, NULL, NULL, NULL, FALSE },
			    { PHASE , FPHASE , "Phase"          , NULL, NULL, NULL, NULL, TRUE  },
			    { FLOAT , FSCORE , "Score"          , NULL, NULL, NULL, NULL, TRUE  },
			    
			    { LABEL , FLABEL , " "              , NULL, NULL, NULL, NULL, TRUE  },
			    
			    { TYPE  , HTYPE  , "Type"           , NULL, NULL, NULL, NULL, TRUE  },
			    { ENTRY , HY1    , "Start"          , NULL, NULL, NULL, NULL, TRUE  },
			    { ENTRY , HY2    , "End"            , NULL, NULL, NULL, NULL, TRUE  },
			    { STRAND, HSTRAND, "Strand"         , NULL, NULL, NULL, NULL, TRUE  },
			    { PHASE , HPHASE , "Phase"          , NULL, NULL, NULL, NULL, TRUE  },
			    { FLOAT , HSCORE , "Score"          , NULL, NULL, NULL, NULL, TRUE  },
			    { ALIGN , HALIGN , "Alignments"     , NULL, NULL, NULL, NULL, TRUE  },
			    
			    { ENTRY , TX1    , "CDS Start"      , NULL, NULL, NULL, NULL, TRUE  },
			    { ENTRY , TX2    , "CDS End"        , NULL, NULL, NULL, NULL, TRUE  },
			    { PHASE , TPHASE , "CDS Phase"      , NULL, NULL, NULL, NULL, TRUE  },
			    { CHECK , TSNF   , "Start Not Found", NULL, NULL, NULL, NULL, TRUE  },
			    { CHECK , TENF   , "End Not Found " , NULL, NULL, NULL, NULL, TRUE  },
			    { EXON  , TEXON  , "Exons"          , NULL, NULL, NULL, NULL, TRUE  },
			    { INTRON, TINTRON, "Introns"        , NULL, NULL, NULL, NULL, TRUE  },
			    
			    { LAST , NULL, NULL, NULL, NULL, NULL, NULL }};

  editor_data = g_new0(editorDataStruct, 1);
  editor_data->originalFeature = g_object_get_data(G_OBJECT(item), "feature");
  zMapAssert(editor_data->originalFeature);   
  /* need to rethink this if we're going to handle creating a new feature */

  editor_data->zmapWindow = zmapWindow;
  editor_data->item = item;
  editor_data->original_id = editor_data->originalFeature->original_id;
  editor_data->hbox = NULL;
  editor_data->table = (mainTable)g_memdup(init, sizeof(init));

  editor_data->modifiedFeature = zMapFeatureCopy(editor_data->originalFeature);

  parseFeature(editor_data->table, editor_data->originalFeature, editor_data->modifiedFeature);
  createWindow(editor_data);

  gtk_widget_show_all(editor_data->window);
  
  return;
}






/* So that the draw routines don't need to know more than the minimum about what they're
 * drawing, everything in the table, apart from the arrays, is held as a string. */
static void parseFeature(mainTableStruct table[], ZMapFeature origFeature, ZMapFeature feature)
{
  int i = 0;

  while (table[i].fieldtype != LAST)
    {
      switch (i)
	{
	case FTYPE:
	  table[i].value.entry = zmapFeatureLookUpEnum(feature->type, TYPE_ENUM);
	  table[i].fieldPtr = &feature->type;
	  table[i].OFfieldPtr = &origFeature->type;
	  break;

	case FX1:
	  table[i].value.entry = g_strdup_printf("%d", feature->x1);
	  table[i].fieldPtr = &feature->x1;
	  table[i].OFfieldPtr = &origFeature->x1;
	  break;

	case FX2:
	  table[i].value.entry = g_strdup_printf("%d", feature->x2);
	  table[i].fieldPtr = &feature->x2;
	  table[i].OFfieldPtr = &origFeature->x2;
	  break;

	case FSTRAND:
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

	case FPHASE:
	  table[i].value.entry = g_strdup_printf("%d", feature->phase);
	  table[i].fieldPtr = &feature->phase;
	  table[i].OFfieldPtr = &origFeature->phase;
	  break;

	case FSCORE:
	  table[i].value.entry = g_strdup_printf("%.2f", feature->score);
	  table[i].fieldPtr = &feature->score;
	  table[i].OFfieldPtr = &origFeature->score;
	  break;

	case FLABEL:
	  table[i].value.entry = " ";  /* blank label to separate feature fields from the rest */
	  break;

	case HTYPE:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    {
	      table[i].fieldPtr = &feature->type;
	      table[i].OFfieldPtr = &origFeature->type;
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
	    }
	  break;

	case HY1:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    {
	      table[i].value.entry = g_strdup_printf("%d", feature->feature.homol.y1);
	      table[i].fieldPtr = &feature->feature.homol.y1;
	      table[i].OFfieldPtr = &origFeature->feature.homol.y1;
	    }
	  break;

	case HY2:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    {
	      table[i].value.entry = g_strdup_printf("%d", feature->feature.homol.y2);
	      table[i].fieldPtr = &feature->feature.homol.y2;
	      table[i].OFfieldPtr = &origFeature->feature.homol.y2;
	    }
	  break;

	case HSTRAND:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    {
	      table[i].fieldPtr = &feature->feature.homol.target_strand;
	      table[i].OFfieldPtr = &origFeature->feature.homol.target_strand;
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

	case HPHASE:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    {
	      table[i].value.entry = g_strdup_printf("%d", feature->feature.homol.target_phase);
	      table[i].fieldPtr = &feature->feature.homol.target_phase;
	      table[i].OFfieldPtr = &origFeature->feature.homol.target_phase;
	    }
	  break;

	case HSCORE:
	  if (feature->type == ZMAPFEATURE_HOMOL)
	    {
	      table[i].value.entry = g_strdup_printf("%.2f", feature->feature.homol.score);
	      table[i].fieldPtr = &feature->feature.homol.score;
	      table[i].OFfieldPtr = &origFeature->feature.homol.score;
	    }
	  break;

	case HALIGN:
	  if (feature->type == ZMAPFEATURE_HOMOL
	      && feature->feature.homol.align != NULL
	      && feature->feature.homol.align->len > (guint)0)
	    array2List(&table[i], feature->feature.homol.align, feature->type);
	  break;

	case TX1:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    {
	      table[i].value.entry = g_strdup_printf("%d", feature->feature.transcript.cdsStart);
	      table[i].fieldPtr = &feature->feature.transcript.cdsStart;
	      table[i].OFfieldPtr = &origFeature->feature.transcript.cdsStart;
	    }
	  break;

	case TX2:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    {
	      table[i].value.entry = g_strdup_printf("%d", feature->feature.transcript.cdsEnd);
	      table[i].fieldPtr = &feature->feature.transcript.cdsEnd;
	      table[i].OFfieldPtr = &origFeature->feature.transcript.cdsEnd;
	    }
	  break;

	case TPHASE:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    {
	      table[i].value.entry = g_strdup_printf("%d", feature->feature.transcript.cds_phase);
	      table[i].fieldPtr = &feature->feature.transcript.cds_phase;
	      table[i].OFfieldPtr = &origFeature->feature.transcript.cds_phase;
	    }
	  break;

	case TSNF:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    {
	      table[i].value.entry = feature->feature.transcript.start_not_found ? "True" : "False";
	      table[i].fieldPtr = &feature->feature.transcript.start_not_found;
	      table[i].OFfieldPtr = &origFeature->feature.transcript.start_not_found;
	    }
	  break;

	case TENF:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    {
	      table[i].value.entry = feature->feature.transcript.endNotFound ? "True" : "False";
	      table[i].fieldPtr = &feature->feature.transcript.endNotFound;
	      table[i].OFfieldPtr = &origFeature->feature.transcript.endNotFound;
	    }
	  break;

	case TEXON:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT
	      && feature->feature.transcript.exons != NULL 
	      && feature->feature.transcript.exons->len > (guint)0)
	    array2List(&table[i], feature->feature.transcript.exons, feature->type);
	  break;

	case TINTRON:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT
	      && feature->feature.transcript.introns != NULL 
	      && feature->feature.transcript.introns->len > (guint)0)
	    array2List(&table[i], feature->feature.transcript.introns, feature->type);
	  break;
	}
      i++;
    } 
  return;
}



/* loads an array into a GtkListStore ready for addArrayCB to hook up to
 * a GtkTreeView in a scrolled window. */
static void array2List(mainTable table, GArray *array, ZMapFeatureType feature_type)
{
  int i;
  ZMapSpanStruct span;
  ZMapAlignBlockStruct align;
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
	  span = g_array_index(array, ZMapSpanStruct, i);
	  gtk_list_store_set (table->value.listStore, &iter,
			      COL1, span.x1,
			      COL2, span.x2,
			      COL3, NULL,
			      COL4, NULL,
			      -1);
	}
      else if (feature_type == ZMAPFEATURE_HOMOL)
	{
	  align = g_array_index(array, ZMapAlignBlockStruct, i);
	  gtk_list_store_set (table->value.listStore, &iter,
			      COL1, align.q1,
			      COL2, align.q2,
			      COL3, align.t1,
			      COL4, align.t2,
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
		   GTK_SIGNAL_FUNC(closeButtonCB), editor_data);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0) ;

  addFields(editor_data);

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
	  
	case ENTRY: case STRAND: case PHASE: case FLOAT: case TYPE:
	  if (table[i].value.entry != NULL)
	    addEntryCB(editor_data->vbox, &table[i]);
	  break;
	  
	case CHECK:
	  if (table[i].value.entry != NULL)
	    addCheckButtonCB(editor_data->vbox, &table[i]);
	  break;
	  
	case ALIGN:
	  if (table[i].value.listStore != NULL)
	    addArrayCB(&table[i], editor_data);
	  break;

	case EXON: case INTRON:
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

  if (g_ascii_strcasecmp(table->value.entry, "True") == 0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->widget), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->widget), FALSE);

  return;
}





static void addArrayCB(gpointer data, editorData editor_data)
{
  mainTable table = (mainTable)data;
  GtkWidget *treeView, *frame, *scrolledWindow;
  char *frameLabel;
  colInfoStruct colInfo[] = {{"Start", COL1},
			     {"End"  , COL2},
			     {"Start", COL3},
			     {"End"  , COL4}};
  int i, cols = 0;

  gtk_window_resize(GTK_WINDOW(editor_data->window), 100, 600); 

  treeView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (table->value.listStore));

  if (table->fieldtype == EXON || table->fieldtype == INTRON)
    cols = 2;
  else if (table->fieldtype == ALIGN)
    cols = 4;

  for (i = 0; i < cols; i++)
    {
      buildCol(colInfo[i], treeView, table);
    }

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

  scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(frame), scrolledWindow);
  gtk_container_add(GTK_CONTAINER(scrolledWindow), treeView);

  return;
}



/* Build a column in the GtkTreeView */
static void buildCol(colInfoStruct colInfo, GtkWidget *treeView, mainTable table)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  int column_width = 50;


  renderer = gtk_cell_renderer_text_new ();
  g_object_set(G_OBJECT(renderer), "editable", TRUE, "editable_set", TRUE, NULL);
  g_object_set_data(G_OBJECT(renderer), "ColNo", GUINT_TO_POINTER(colInfo.colNo));
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (arrayEditedCB), table->value.listStore);

  column = gtk_tree_view_column_new_with_attributes (colInfo.label,
						     renderer,
						     "text", colInfo.colNo,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
  gtk_tree_view_column_set_min_width(column, column_width);

  return;
}



/* If the user changes a cell in one of the lists of aligns, introns or
 * exons, we need to copy that change to the GtkListStore that drives
 * the GtkTreeView for that array. */
static void arrayEditedCB(GtkCellRendererText *renderer, 
			  char *path, char *new_text, gpointer user_data)
{
  GtkListStore *listStore = (GtkListStore*)user_data;
  GtkTreeIter iter;
  guint colNo;

  colNo = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(renderer), "ColNo"));
  gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(listStore), &iter, path);
  gtk_list_store_set(listStore, &iter, colNo, atoi(new_text), -1);

  return;
}
 

/* Restore the displayed data using the original feature */
static void undoChangesCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorData)data;
  mainTable table = editor_data->table;
  ZMapFeature feature = editor_data->originalFeature;
  int i;

  for  (i = 1; table[i].fieldtype != LAST; i++)
    {
      switch (table[i].fieldtype)
	{
	case ENTRY: case STRAND: case PHASE: case TYPE:
	  if (table[i].OFfieldPtr != NULL && table[i].fieldPtr != NULL)
	    {
	      *((int *)table[i].fieldPtr) = *((int *)table[i].OFfieldPtr);
	      gtk_entry_set_text(GTK_ENTRY(table[i].widget), table[i].value.entry);
	    }
	  break;

	case FLOAT:
	  if (table[i].OFfieldPtr != NULL && table[i].fieldPtr != NULL)
	    {
	      *((float *)table[i].fieldPtr) = *((float *)table[i].OFfieldPtr);
	      gtk_entry_set_text(GTK_ENTRY(table[i].widget), table[i].value.entry);
	    }
	  break;

	case ALIGN: 
	  if (feature->type == ZMAPFEATURE_HOMOL 
	      && feature->feature.homol.align != NULL
	      && feature->feature.homol.align->len > (guint)0)
	    refreshArray(&table[i], feature->feature.homol.align);
	  break;

	case EXON:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    {
	      if (feature->feature.transcript.exons != NULL 
		  && feature->feature.transcript.exons->len > (guint)0)
		refreshArray(&table[i], feature->feature.transcript.exons);
	    }
	  break;

	case INTRON:
	  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
	    {
	      if (feature->feature.transcript.introns != NULL 
		  && feature->feature.transcript.introns->len > (guint)0)
		refreshArray(&table[i], feature->feature.transcript.introns);
	    }
	  break;
	}
    }

  parseFeature(editor_data->table, editor_data->originalFeature, editor_data->modifiedFeature);
  redrawFeature(editor_data);

  return;
}


/* Actually this restores the GtkListStores that drive the
 * scrolled windows of variable length data, using data
 * from the original copy of the feature. */
static void refreshArray(mainTable table, GArray *array)
{
  int i;
  gboolean valid;
  GtkTreeIter iter;
  ZMapAlignBlockStruct align;
  ZMapSpanStruct span;

  if (table->value.listStore != NULL)
    {
      valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(table->value.listStore), &iter);

      for (i = 0; i < array->len && valid == TRUE; i++)
	{
	  if (table->fieldtype == ALIGN)  
	    {
	      align = g_array_index(array, ZMapAlignBlockStruct, i);
	      
	      gtk_list_store_set(table->value.listStore, &iter, 
				 COL1, align.q1, 
				 COL2, align.q2, 
				 COL3, align.t1, 
				 COL4, align.t2, 
				 -1);
	    }
	  else if (table->fieldtype == EXON
		   || table->fieldtype == INTRON)
	    {
	      span = g_array_index(array, ZMapSpanStruct, i);

	      gtk_list_store_set(table->value.listStore, &iter, 
				 COL1, span.x1,
				 COL2, span.x2,
				 -1);
	    }
	  valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(table->value.listStore), &iter);
	}
    }

  return;
}



/* I think I'll need some kind of externally-supplied callback here,
 * to send the amended feature back whence it came. */
static void saveChangesCB(GtkWidget *widget, gpointer data)
{
  printf("Save called\n");
  return;
}



/* saves any user changes in editor_data->modifiedFeature, but 
 * leaves the original copy untouched in case they want to Undo. */
static void applyChangesCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorData)data;
  mainTable table = editor_data->table;
  ZMapFeature feature;
  const char *value;
  int i, n;
  double d;
  gboolean pressed;

  feature = editor_data->modifiedFeature;

  for (i = 1; table[i].fieldtype != LAST; i++)
    {
      if (table[i].widget != NULL
	  && table[i].fieldPtr != NULL)
	{
	  if (table[i].fieldtype == ENTRY
	      || table[i].fieldtype == STRAND
	      || table[i].fieldtype == PHASE
	      || table[i].fieldtype == FLOAT
	      || table[i].fieldtype == TYPE)
	    value = gtk_entry_get_text(GTK_ENTRY(table[i].widget));
	  
	  switch (table[i].fieldtype)
	    {
	    case ENTRY:
	      if (g_ascii_strcasecmp(value, "0") == 0)
		*((int *)table[i].fieldPtr) = 0;
	      else if (zMapStr2Int((char *)value, &n) == TRUE)
		*((int *)table[i].fieldPtr) = n;
	      else
		zMapMessage("%s (%s) is not a valid entry", table[i].label, value);
	      break;
	      
	    case STRAND:
	      if (g_ascii_strcasecmp(value, "Forward") == 0)
		*((int *)table[i].fieldPtr) = ZMAPSTRAND_FORWARD;
	      else if (g_ascii_strcasecmp(value, "Reverse") == 0)
		*((int *)table[i].fieldPtr) = ZMAPSTRAND_REVERSE;
	      else if (g_ascii_strcasecmp(value, "Unspecified") == 0)
		*((int *)table[i].fieldPtr) = ZMAPSTRAND_NONE;
	      else 
		zMapMessage("Strand (%s) must be Forward, Reverse or Unspecified", value);
	      break;
		  
	    case PHASE:
	      if (g_ascii_strcasecmp(value, "0") == 0)
		*((int *)table[i].fieldPtr) = 0;
	      else if (zMapStr2Int((char *)value, &n) == TRUE 
		       && n < 4)
		*((int *)table[i].fieldPtr) = n;
	      else 
		zMapMessage("Phase (%s) must be 0, 1 or 2", value);
	      break;
		  
	    case FLOAT:                   /* scores */
	      if (g_ascii_strcasecmp(value, "0.00") == 0)
		*((int *)table[i].fieldPtr) = 0.00;
	      else if (zMapStr2Double((char *)value, &d) == TRUE)
		*((float *)table[i].fieldPtr) = (float)d;
	      else
		zMapMessage("Score (%s) is not a valid number", value);
	      break;
	      
	    case TYPE:                   /* homol type */
	      if (g_ascii_strcasecmp(value, "X") == 0)
		*((int *)table[i].fieldPtr) = ZMAPHOMOL_X_HOMOL;
	      else if (g_ascii_strcasecmp(value, "N") == 0)
		*((int *)table[i].fieldPtr) = ZMAPHOMOL_N_HOMOL;
	      else if (g_ascii_strcasecmp(value, "TX") == 0)
		*((int *)table[i].fieldPtr) = ZMAPHOMOL_TX_HOMOL;
	      else 
		zMapMessage("Homol Type (%s) must be X, N or TX", value);
	      break;
	      
	    case CHECK:                        /* transcript start/end not found */
	      pressed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table[i].widget));
	      *((int *)table[i].fieldPtr) = pressed;
	      break;
	    }
	}

      /* fieldPtr won't be set for the arrays */
      if (table[i].widget != NULL)
	{
	  switch (table[i].fieldtype)
	    {
	    case ALIGN:
	      if (feature->type == ZMAPFEATURE_HOMOL)
		updateArray(&table[i], feature->feature.homol.align);
	      break;
	      
	    case EXON: 
	      if (feature->type == ZMAPFEATURE_TRANSCRIPT)
		updateArray(&table[i], feature->feature.transcript.exons);
	      break;
	      
	    case INTRON: 
	      if (feature->type == ZMAPFEATURE_TRANSCRIPT)
		updateArray(&table[i], feature->feature.transcript.introns);
	      break;
	    }
	}
    }

  /*  if (validateArrays(editor_data) == TRUE)*/
    redrawFeature(editor_data);

  return;
}



/* Very simplistic validation; ignore duplicates and assume that there
 * should just be a simple consecutive series of exons and introns.  We
 * take 2 looks at the arrays, first to check that the coords are within
 * feature->x1 and x2, then looking for the series. */
static gboolean validateArrays(editorData editor_data)
{
  int i;
  gboolean valid = TRUE, finished = FALSE;
  ZMapFeature feature = editor_data->modifiedFeature;
  ZMapSpanStruct span;

  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
    {
      if (feature->feature.transcript.exons != NULL
	  && feature->feature.transcript.exons->len > (guint)0)
	valid = checkAllCoords(feature->x1, feature->x2, feature->feature.transcript.exons);

      if (valid == TRUE
	  && feature->feature.transcript.introns != NULL
	  && feature->feature.transcript.introns->len > (guint)0)
	valid = checkAllCoords(feature->x1, feature->x2, feature->feature.transcript.introns);

      if (valid == TRUE)
	{
	  /* get the first exon */
	  span = g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, 0);
	  if (span.x1 != feature->x1)
	    {
	      zMapMessage("First exon (%d) does not start at beginning of transcript (%d)",
			  span.x1, feature->x1);
	      valid = FALSE;
	    }
	  
	  while (valid == TRUE && finished == FALSE)
	    {
	      if (feature->feature.transcript.introns != NULL
		  && feature->feature.transcript.introns->len > (guint)0)
		{
		  i = getNextElement(feature->feature.transcript.introns, span.x2);

		  if (i > 0)
		    span = g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i);
		}

	      /* This allows for contiguous exons */ 
	      i = getNextElement(feature->feature.transcript.exons, span.x2);

	      if (i > 0)
		{
		  span = g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);
		  if (span.x2 == feature->x2)
		    finished = TRUE;
		}
	      else
		{
		  zMapMessage("Exons & introns don't make a consecutive series.  Can't find element adjacent to %d.", 
			      span.x2);
		  valid = FALSE;
		}
	    }

	}
    }
  return valid;
}



static gboolean checkAllCoords(int x1, int x2, GArray *array)
{
  int i;
  ZMapSpanStruct span;
  gboolean status = TRUE;

  for (i = 0; i < array->len; i++)
    {
      span = g_array_index(array, ZMapSpanStruct, i);
      if (span.x1 < x1 || span.x1 > x2
	  || span.x2 < x1 || span.x2 > x2)
	{
	  status = FALSE;
	  zMapMessage("Coords (%d %d) outside span of transcript (%d %d)",
		      span.x1, span.x2, x1, x2);
	}
    }
  return status;
}


static int getNextElement(GArray *array, int start)
{
  int i;
  ZMapSpanStruct span;

  for (i = 0; i < array->len; i++)
    {
      span = g_array_index(array, ZMapSpanStruct, i);
	if (span.x1 == start + 1)
	  break;
    }

  if (i >= array->len)
    i = 0;

  return i;
}



/* Update the appropriate array in the feature with data from the screen */
static void updateArray(mainTable table, GArray *array)
{
  ZMapAlignBlockStruct align;
  ZMapSpanStruct span;
  GtkTreeIter iter;
  gboolean valid = FALSE;
  
  if (table->fieldtype == ALIGN) 
    {
      if (array != NULL
	  && array->len > (guint)0)
	{
	  g_array_free(array, TRUE);
	}
      array = g_array_new(FALSE, TRUE, sizeof(ZMapAlignBlockStruct));
    }
  else if (table->fieldtype == EXON
	   || table->fieldtype == INTRON)
    {
      if (array != NULL
	  && array->len > (guint)0)
	{
	  g_array_free(array, TRUE);
	}
      array = g_array_new(FALSE, TRUE, sizeof(ZMapSpanStruct));
    }
  
  if (table->value.listStore != NULL)
    valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(table->value.listStore), &iter);

  while (valid)
    {
      if (table->fieldtype == ALIGN)  
	{
	  /* GtkListStore enforces integers here */
	  gtk_tree_model_get (GTK_TREE_MODEL(table->value.listStore), &iter, 
			      COL1, &align.q1,
			      COL2, &align.q2,
			      COL3, &align.t1,
			      COL4, &align.t2,
			      -1);
	  g_array_append_val(array, align);
	}
      else if (table->fieldtype == EXON
	       || table->fieldtype == INTRON)
	{
	  /* GtkListStore enforces integers here */
	  gtk_tree_model_get (GTK_TREE_MODEL(table->value.listStore), &iter, 
			      COL1, &span.x1,
			      COL2, &span.x2,
			      -1);
	  g_array_append_val(array, span);
	}
      
      valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(table->value.listStore), &iter);
    }

  return;
}



/* moves a feature if the coordinates have changed */
static void redrawFeature(editorData editor_data)
{
  ZMapFeature feature;
  ZMapWindow window;
  FooCanvasItem *item;
  ZMapWindowSelectStruct select = {NULL} ;
  double top, bottom;

  feature = editor_data->modifiedFeature;
  window = editor_data->zmapWindow;
  item = editor_data->item;

  zMapFeature2MasterCoords(feature, &top, &bottom);

  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
    foo_canvas_item_set(item->parent, "y", top, NULL);
  else
    foo_canvas_item_set(item, "y1", top, "y2", bottom, NULL);
  

  /* redo the info panel with new coords if any */
  select.text = g_strdup_printf("%s   %s   %d   %d   %s   %s", 
				(char *)g_quark_to_string(feature->original_id),
				zmapFeatureLookUpEnum(feature->strand, STRAND_ENUM),
				feature->x1,
				feature->x2,
				zmapFeatureLookUpEnum(feature->type, TYPE_ENUM),
				zMapStyleGetName(zMapFeatureGetStyle(feature))) ;
  select.item = item ;
  
  (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;
	    
  g_free(select.text) ;

  return;
}




/* We don't want the close button to call closeWindowCB directly
 * since that function is also called by the "destroy" signal we've
 * connected to the window itself and would end up being called
 * twice.  This way it's only called once. */
static void closeButtonCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorData)data;

  g_signal_emit_by_name(editor_data->window, "destroy", NULL, NULL);

  return;
}
  


static void closeWindowCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorDataStruct*)data;
    
  freeTable(editor_data->table);
  g_free(editor_data->modifiedFeature);
  gtk_widget_destroy(GTK_WIDGET(editor_data->window));
  g_free(editor_data);
                                                                                
  return;
}



static void freeTable(mainTableStruct table[])
{
  int i;

  for (i = 1; table[i].fieldtype != LAST; i++)
    {
      switch (table->fieldtype)
	{
	case ENTRY: case PHASE: case FLOAT: case TYPE:
	  if (table[i].value.entry != NULL)
	    g_free(table[i].value.entry);
	  break;
	}
    } 
  g_free(table);

  return;
}



/********************* end of file ********************************/
