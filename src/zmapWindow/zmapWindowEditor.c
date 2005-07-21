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
 * Last edited: Jul 21 13:31 2005 (rnc)
 * Created: Mon Jun 6 13:00:00 (rnc)
 * CVS info:   $Id: zmapWindowEditor.c,v 1.10 2005-07-21 12:47:17 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>


#define LIST_COLUMN_WIDTH 50


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


typedef enum {LABEL, ENTRY, INT, FLOAT, STRAND, PHASE, HTYPE, CHECK, ALIGN, EXON, INTRON, LAST} fieldType;

enum {COL1, COL2, COL3, COL4, N_COLS};  /* columns to display arrays */


typedef gboolean (*EditorCallbackFunc)(char *label, char *value, void *retValue) ;


/* An array of this struct is the main driver for the program */
typedef struct MainTableStruct
{
  fieldType  fieldtype;  /* controls the way the field is drawn */
  fieldType  datatype;   /* the type of data being held */
  char       label[20];
  void      *fieldPtr;   /* the field in the modified feature */
  void      *OFfieldPtr; /* the field in the original feature */
  GtkWidget *widget;     /* the widget on the screen */
  union
  {
    char         *entry;     /* the value being displayed */
    GtkListStore *listStore; /* the list of aligns, exons, introns, etc */
  } value;
  gboolean editable;
  EditorCallbackFunc validateCB;

} mainTableStruct, *mainTable;



typedef struct EditorDataStruct
{
  ZMapWindow zmapWindow;
  GtkWidget *window;
  GtkWidget *mainVbox;    /* holds buttons and next vbox */
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
static gboolean arrayClickedCB(GObject *gobject,
				GParamSpec *arg1,
			       gpointer user_data);

static void parseFeature(mainTableStruct table[], ZMapFeature origFeature, ZMapFeature feature);
static void parseField(mainTable table, ZMapFeature feature);
static void array2List(mainTable table, GArray *array, ZMapFeatureType feature_type);

static void createWindow(editorData editor_data);
static void closeWindowCB(GtkWidget *widget, gpointer data);
static void closeButtonCB(GtkWidget *widget, gpointer data);
static void undoChangesCB(GtkWidget *widget, gpointer data);
static gboolean applyChangesCB(GtkWidget *widget, gpointer data);
static void saveChangesCB(GtkWidget *widget, gpointer data);
static void saveArray(GArray *original, GArray *modified);

static gboolean validateEntryCB (char *label, char *value, void *retValue);
static gboolean validateStrandCB(char *label, char *value, void *retValue);
static gboolean validatePhaseCB (char *label, char *value, void *retValue);
static gboolean validateFloatCB (char *label, char *value, void *retValue);
static gboolean validateArrayCB (editorData editor_data);
static gboolean checkAllCoords(int x1, int x2, GArray *array);
static int      getNextElement(GArray *array, int start);

static void updateArray(mainTable table, GArray *array);

static void addFields (editorData editor_data);
static void addArrayCB(gpointer data, editorData editor_data);
static void addCheckButtonCB(GtkWidget *vbox, gpointer data);
static void addEntryCB(GtkWidget *vbox, gpointer data);
static void addLabelCB(GtkWidget *vbox, gpointer data);

static void buildCol(colInfoStruct colInfo, GtkWidget *treeView, mainTable table);
static gboolean arrayEditedCB(GtkCellRendererText *renderer, 
			  char *path, char *new_text, gpointer user_data);

static void freeTable(mainTableStruct table[]);


/* function code *******************************************/

void zmapWindowEditor(ZMapWindow zmapWindow, FooCanvasItem *item)
{
  ZMapFeature feature;
  editorData editor_data;

  editor_data = g_new0(editorDataStruct, 1);
  editor_data->originalFeature = g_object_get_data(G_OBJECT(item), "item_feature_data");
  zMapAssert(editor_data->originalFeature);   
  /* need to rethink this if we're going to handle creating a new feature */

  editor_data->zmapWindow = zmapWindow;
  editor_data->item = item;
  editor_data->original_id = editor_data->originalFeature->original_id;
  editor_data->hbox = NULL;
  editor_data->table = g_malloc(sizeof(mainTableStruct) * 16);

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
  mainTableStruct ftypeInit   = { LABEL , LABEL , "Type"           , NULL, NULL, NULL, NULL, FALSE, NULL },
                  fx1Init     = { ENTRY , INT   , "Start"          , NULL, NULL, NULL, NULL, TRUE , validateEntryCB  },
                  fx2Init     = { ENTRY , INT   , "End"            , NULL, NULL, NULL, NULL, TRUE , validateEntryCB  },
	          fstrandInit = { ENTRY , STRAND, "Strand"         , NULL, NULL, NULL, NULL, FALSE, NULL  },
	          fphaseInit  = { ENTRY , PHASE , "Phase"          , NULL, NULL, NULL, NULL, TRUE , validatePhaseCB  },
	          fscoreInit  = { ENTRY , FLOAT , "Score"          , NULL, NULL, NULL, NULL, TRUE , validateFloatCB  },
	      
	          blankInit   = { LABEL , LABEL , " "        , NULL, NULL, NULL, NULL, TRUE , NULL  },
			    
		  htypeInit   = { ENTRY , HTYPE , "Homol Type"     , NULL, NULL, NULL, NULL, FALSE, NULL  },
		  hy1Init     = { ENTRY , INT   , "Query Start"    , NULL, NULL, NULL, NULL, FALSE, NULL  },
		  hy2Init     = { ENTRY , INT   , "Query End"      , NULL, NULL, NULL, NULL, FALSE, NULL  },
		  hstrandInit = { ENTRY , STRAND, "Query Strand"   , NULL, NULL, NULL, NULL, FALSE, NULL  },
		  hphaseInit  = { ENTRY , PHASE , "Query Phase"    , NULL, NULL, NULL, NULL, FALSE, NULL  },
		  hscoreInit  = { ENTRY , FLOAT , "Query Score"    , NULL, NULL, NULL, NULL, FALSE, NULL  },
		  halignInit  = { ALIGN , ALIGN , "Alignments"     , NULL, NULL, NULL, NULL, FALSE, NULL  },
			    
		  tx1Init     = { ENTRY , INT   , "CDS Start"      , NULL, NULL, NULL, NULL, TRUE , validateEntryCB  },
		  tx2Init     = { ENTRY , INT   , "CDS End"        , NULL, NULL, NULL, NULL, TRUE , validateEntryCB  },
		  tphaseInit  = { ENTRY , PHASE , "CDS Phase"      , NULL, NULL, NULL, NULL, TRUE , validatePhaseCB  },
		  tsnfInit    = { CHECK , CHECK , "Start Not Found", NULL, NULL, NULL, NULL, TRUE , NULL  },
		  tenfInit    = { CHECK , CHECK , "End Not Found " , NULL, NULL, NULL, NULL, TRUE , NULL  },
		  texonInit   = { EXON  , EXON  , "Exons"          , NULL, NULL, NULL, NULL, TRUE , NULL  },
		  tintronInit = { INTRON, INTRON, "Introns"        , NULL, NULL, NULL, NULL, TRUE , NULL  },
			    
		  lastInit    = { LAST  , NULL  ,  NULL            , NULL, NULL, NULL, NULL, NULL , NULL  };
		  
  table[i] = ftypeInit;
  table[i].value.entry = zmapFeatureLookUpEnum(feature->type, TYPE_ENUM);
  table[i].fieldPtr = &feature->type;
  table[i].OFfieldPtr = &origFeature->type;
  
  i++;
  table[i] = fx1Init;
  table[i].value.entry = g_strdup_printf("%d", feature->x1);
  table[i].fieldPtr = &feature->x1;
  table[i].OFfieldPtr = &origFeature->x1;
  if (feature->type == ZMAPFEATURE_HOMOL)
    table[i].editable = FALSE;
  
  i++;
  table[i] = fx2Init;
  table[i].value.entry = g_strdup_printf("%d", feature->x2);
  table[i].fieldPtr = &feature->x2;
  table[i].OFfieldPtr = &origFeature->x2;
  if (feature->type == ZMAPFEATURE_HOMOL)
    table[i].editable = FALSE;
  
  i++;
  table[i] = fstrandInit;
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
    }
  
  i++;
  table[i] = fphaseInit;
  table[i].value.entry = g_strdup_printf("%d", feature->phase);
  table[i].fieldPtr = &feature->phase;
  table[i].OFfieldPtr = &origFeature->phase;
  
  i++;
  table[i] = fscoreInit;
  table[i].value.entry = g_strdup_printf("%.2f", feature->score);
  table[i].fieldPtr = &feature->score;
  table[i].OFfieldPtr = &origFeature->score;
  
  i++;
  table[i] = blankInit;
  table[i].value.entry = " ";
  
  if (feature->type == ZMAPFEATURE_HOMOL)
    {
      i++;
      table[i] = htypeInit;
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
      
      i++;
      table[i] = hy1Init;
      table[i].value.entry = g_strdup_printf("%d", feature->feature.homol.y1);
      table[i].fieldPtr = &feature->feature.homol.y1;
      table[i].OFfieldPtr = &origFeature->feature.homol.y1;
      
      i++;
      table[i] = hy2Init;
      table[i].value.entry = g_strdup_printf("%d", feature->feature.homol.y2);
      table[i].fieldPtr = &feature->feature.homol.y2;
      table[i].OFfieldPtr = &origFeature->feature.homol.y2;
      
      i++;
      table[i] = hstrandInit;
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
      
      i++;
      table[i] = hphaseInit;
      table[i].value.entry = g_strdup_printf("%d", feature->feature.homol.target_phase);
      table[i].fieldPtr = &feature->feature.homol.target_phase;
      table[i].OFfieldPtr = &origFeature->feature.homol.target_phase;
      
      i++;
      table[i] = hscoreInit;
      table[i].value.entry = g_strdup_printf("%.2f", feature->feature.homol.score);
      table[i].fieldPtr = &feature->feature.homol.score;
      table[i].OFfieldPtr = &origFeature->feature.homol.score;
      
      i++;
      table[i] = halignInit;
      if (feature->feature.homol.align != NULL
	  && feature->feature.homol.align->len > (guint)0)
	array2List(&table[i], feature->feature.homol.align, feature->type);
    }
  else if (feature->type == ZMAPFEATURE_TRANSCRIPT)
    {
      i++;
      table[i] = tx1Init;
      table[i].value.entry = g_strdup_printf("%d", feature->feature.transcript.cdsStart);
      table[i].fieldPtr = &feature->feature.transcript.cdsStart;
      table[i].OFfieldPtr = &origFeature->feature.transcript.cdsStart;
      
      i++;
      table[i] = tx2Init;
      table[i].value.entry = g_strdup_printf("%d", feature->feature.transcript.cdsEnd);
      table[i].fieldPtr = &feature->feature.transcript.cdsEnd;
      table[i].OFfieldPtr = &origFeature->feature.transcript.cdsEnd;

      i++;
      table[i] = tphaseInit;
      table[i].value.entry = g_strdup_printf("%d", feature->feature.transcript.cds_phase);
      table[i].fieldPtr = &feature->feature.transcript.cds_phase;
      table[i].OFfieldPtr = &origFeature->feature.transcript.cds_phase;
      
      i++;
      table[i] = tsnfInit;
      table[i].value.entry = feature->feature.transcript.start_not_found ? "True" : "False";
      table[i].fieldPtr = &feature->feature.transcript.start_not_found;
      table[i].OFfieldPtr = &origFeature->feature.transcript.start_not_found;
      
      i++;
      table[i] = tenfInit;
      table[i].value.entry = feature->feature.transcript.endNotFound ? "True" : "False";
      table[i].fieldPtr = &feature->feature.transcript.endNotFound;
      table[i].OFfieldPtr = &origFeature->feature.transcript.endNotFound;
      
      i++;
      table[i] = texonInit;
      if (feature->feature.transcript.exons != NULL 
	  && feature->feature.transcript.exons->len > (guint)0)
	array2List(&table[i], feature->feature.transcript.exons, feature->type);
      
      i++;
      table[i] = tintronInit;
      if (feature->feature.transcript.introns != NULL 
	  && feature->feature.transcript.introns->len > (guint)0)
	array2List(&table[i], feature->feature.transcript.introns, feature->type);
    }

  i++;
  table[i] = lastInit;

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
			      COL1, align.t1,
			      COL2, align.t2,
			      COL3, align.q1,
			      COL4, align.q2,
			      -1);
	}
    }
  
  return;
}




static void createWindow(editorData editor_data)
{
  GtkWidget *buttonbox, *button;
  GtkWidget *frame, *hbox;
  char *title;

  title = g_strdup_printf("Feature Editor - %s", 
			  g_quark_to_string(editor_data->original_id));
  editor_data->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (editor_data->window), title);
  g_free(title);

  g_signal_connect (G_OBJECT (editor_data->window), "destroy",
		    G_CALLBACK (closeWindowCB), editor_data);

  gtk_container_set_border_width (GTK_CONTAINER (editor_data->window), 10);

  editor_data->mainVbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(editor_data->window), editor_data->mainVbox);

  frame = gtk_frame_new(NULL);
  gtk_container_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(editor_data->mainVbox), frame, FALSE, FALSE, 0);
  
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

  editor_data->vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(editor_data->mainVbox), editor_data->vbox, TRUE, TRUE, 0);

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
	  
	case ENTRY:
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
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

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
	frameLabel = "Target              Query";
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
  GdkColor background;

  gdk_color_parse("WhiteSmoke", &background);

  renderer = gtk_cell_renderer_text_new ();

  /* This is not ideal, but I've spent too long on it already.  For non-editable
   * things like homol alignments, I want the background pale grey so the user 
   * knows it's protected.  This works fine. But if I actively set the background
   * colour to White for editable objects, when you double click on a cell, the
   * contents of the cell next to it vanish. So instead of one command where I set
   * all the attributes as appropriate, I now have 2.  Maybe I'll find out what's
   * going on some time and can fix it properly. */
  if (table->editable == TRUE)
      g_object_set(G_OBJECT(renderer), 
		   "editable"    , table->editable, 
		   "editable_set", table->editable, 
		   NULL);
  else  
      g_object_set(G_OBJECT(renderer), 
		   "editable"      , table->editable, 
		   "editable_set"  , table->editable, 
		   "background-gdk", &background,
		   NULL);

  g_object_set_data(G_OBJECT(renderer), "ColNo", GUINT_TO_POINTER(colInfo.colNo));
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (arrayEditedCB), table->value.listStore);

  column = gtk_tree_view_column_new_with_attributes (colInfo.label,
						     renderer,
						     "text", colInfo.colNo,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
  gtk_tree_view_column_set_min_width(column, LIST_COLUMN_WIDTH);

  return;
}



/* If the user changes a cell in one of the lists of introns or
 * exons, we need to copy that change to the GtkListStore that drives
 * the GtkTreeView for that array. */
static gboolean arrayEditedCB(GtkCellRendererText *renderer, 
			  char *path, char *new_text, gpointer user_data)
{
  GtkListStore *listStore = (GtkListStore*)user_data;
  GtkTreeIter iter;
  guint colNo;
  gboolean valid = FALSE;
  ZMapSpanStruct span;

 
  colNo = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(renderer), "ColNo"));
  gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(listStore), &iter, path);
  gtk_list_store_set(listStore, &iter, colNo, atoi(new_text), -1);

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(listStore), &iter);
  
  while (valid)
    {
      gtk_tree_model_get (GTK_TREE_MODEL(listStore), &iter, 
			  COL1, &span.x1,
			  COL2, &span.x2,
			  -1);
      
      valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(listStore), &iter);
    }
 
  return FALSE;
}
 



/* copy the original feature over the modified one, then 
 * just redraw everything below the buttons. */
static void undoChangesCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorData)data;

  editor_data->modifiedFeature = zMapFeatureCopy(editor_data->originalFeature);

  parseFeature(editor_data->table, editor_data->originalFeature, editor_data->modifiedFeature);

  gtk_widget_destroy(editor_data->vbox);
  editor_data->vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(editor_data->mainVbox), editor_data->vbox, TRUE, TRUE, 0);
  editor_data->hbox = NULL;  /* invalidated when its parent vbox was destroyed */

  addFields(editor_data);
  zMapWindowMoveItem(editor_data->zmapWindow, editor_data->originalFeature,
		     editor_data->modifiedFeature, editor_data->item);

  gtk_widget_show_all(editor_data->window);

  return;
}



static void saveChangesCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorData)data;
  mainTable table = editor_data->table;
  int i;
  gboolean valid;

  valid = applyChangesCB(NULL, (gpointer)editor_data);

  if (valid == TRUE)
    {
      for (i = 1; table[i].fieldtype != LAST; i++)
	{
	  if (table[i].OFfieldPtr != NULL
	      && table[i].fieldPtr != NULL)
	    {
	      switch (table[i].datatype)
		{
		case INT: case STRAND: case PHASE:
		  *((int *)table[i].OFfieldPtr) = *((int *)table[i].fieldPtr);
		  break;
		  
		case FLOAT:
		  *((float *)table[i].OFfieldPtr) = *((float *)table[i].fieldPtr);
		  break;
		}
	    }
	  
	  switch (table[i].fieldtype)
	    {
	      /* homology alignments are not editable */
	    case EXON: 
	      saveArray(editor_data->originalFeature->feature.transcript.exons,
			editor_data->modifiedFeature->feature.transcript.exons);
	      break;
	      
	    case INTRON: 
	      saveArray(editor_data->originalFeature->feature.transcript.introns,
			editor_data->modifiedFeature->feature.transcript.introns);
	      break;
	    }
	}
    }

  return;
}



static void saveArray(GArray *original, GArray *modified)
{
  int i;
  ZMapSpanStruct span;

  if (original != NULL && original->len > (guint)0
      && modified != NULL && modified->len > (guint)0)
    {
      /* nuke the old array and create a new one */
      if (original != NULL
	  && original->len > (guint)0)
	{
	  g_array_free(original, TRUE);
	}
      
      original = g_array_new(FALSE, TRUE, sizeof(ZMapSpanStruct));
      
      for (i = 0; i < modified->len; i++)
	{
	  span = g_array_index(modified, ZMapSpanStruct, i);
	  g_array_append_val(original, span);
	}
    }

  return;
}



/* saves any user changes in editor_data->modifiedFeature, but 
 * leaves the original copy untouched in case they want to Undo. */
static gboolean applyChangesCB(GtkWidget *widget, gpointer data)
{
  editorData editor_data = (editorData)data;
  mainTable table = editor_data->table;
  ZMapFeature feature;
  char *value;
  int i, n;
  double d;
  gboolean valid = FALSE;
  void *retValue = NULL;

  retValue = g_malloc(sizeof(double));
  zMapAssert(retValue);

  feature = editor_data->modifiedFeature;

  for (i = 1; table[i].fieldtype != LAST; i++)
    {
      if (table[i].widget != NULL
	  && table[i].fieldPtr != NULL
	  && (table[i].validateCB != NULL))
	{
	  value = (char *)gtk_entry_get_text(GTK_ENTRY(table[i].widget));
	  
	  if ((valid = (table[i].validateCB)(table[i].label, value, retValue)) == TRUE)
	    {

	      switch (table[i].datatype)
		{
		case INT: case STRAND: case PHASE: 
		  *((int *)table[i].fieldPtr) = *(int *)retValue;
		  break;
		  
		case FLOAT:
		  /* retValue points to a double, because I used ZMapStr2Double
		   * to convert it. */
		  *((float *)table[i].fieldPtr) = *(double *)retValue;
		  break;
		}
	      g_free(retValue);
	    }
	}

      if (valid == TRUE)
	{
	  switch (table[i].fieldtype)
	    {
	      /* homology alignments are not editable */
	    case EXON: 
	      updateArray(&table[i], feature->feature.transcript.exons);
	      break;
	      
	    case INTRON: 
	      updateArray(&table[i], feature->feature.transcript.introns);
	      break;
	    }
	}
    }

  if (valid == TRUE && validateArrayCB(editor_data) == TRUE)
    zMapWindowMoveItem(editor_data->zmapWindow, editor_data->originalFeature,
		       editor_data->modifiedFeature, editor_data->item);
    
  return valid;
}




static gboolean validateEntryCB(char *label, char *value, void *retValue)
{
  gboolean status = TRUE;

  if (zMapStr2Int((char *)value, (int *)retValue) != TRUE)
    {
      zMapMessage("%s (%s) is not a valid entry", label, value);
      status = FALSE;
    }
  return status;
}


static gboolean validateStrandCB(char *label, char *value, void *retValue)
{
  gboolean status = TRUE;

  if (g_ascii_strcasecmp(value, "Forward") == 0)
    *((int *)retValue) = ZMAPSTRAND_FORWARD;
  else if (g_ascii_strcasecmp(value, "Reverse") == 0)
    *((int *)retValue) = ZMAPSTRAND_REVERSE;
  else if (g_ascii_strcasecmp(value, "Unspecified") == 0)
    *((int *)retValue) = ZMAPSTRAND_NONE;
  else 
    {
      zMapMessage("%s (%s) must be Forward, Reverse or Unspecified", label, value);
      status = FALSE;
    }

  return status;
}


static gboolean validatePhaseCB(char *label, char *value, void *retValue)
{
  gboolean status = TRUE;

  if (zMapStr2Int((char *)value, (int *)retValue) != TRUE 
      || *((int *)retValue) > 3)
    {
      zMapMessage("%s (%s) must be 0, 1 or 2", label, value);
      status = FALSE;
    }

  return status;
}


static gboolean validateFloatCB(char *label, char *value, void *retValue)
{
  gboolean status = TRUE;

  if (zMapStr2Double((char *)value, (double *)retValue) != TRUE)
    {
      zMapMessage("%s (%s) is not a valid number", label, value);
      status = FALSE;
    }

  return status;
}





/* Very simplistic validation;  assume that there
 * should just be a simple consecutive series of exons and introns.  We
 * take 2 looks at the arrays, first to check that the coords are within
 * feature->x1 and x2, then looking for the series. */
static gboolean validateArrayCB(editorData editor_data)
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

		  if (i < feature->feature.transcript.introns->len)
		    span = g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i);
		}

	      /* This allows for contiguous exons */ 
	      if (span.x2 == feature->x2)
		finished = TRUE;
	      else
		{
		  i = getNextElement(feature->feature.transcript.exons, span.x2);
	      
		  if (i < feature->feature.transcript.exons->len)
		    {
		      ZMapSpanStruct span2;
		      span2 = g_array_index(editor_data->modifiedFeature->feature.transcript.exons, ZMapSpanStruct, i);

		      span = g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);
		      if (span.x2 == feature->x2)
			finished = TRUE;
		    }
		  else
		    {
		      zMapMessage("Exons & introns don't make a consecutive series. Can't find element adjacent to %d.", 
				  span.x2);
		      valid = FALSE;
		    }
		}
	    }
	}
    }
  return valid;
}



/* Checks that all the elements in the array have coordinates
 * within the range specified by x1 and x2. */
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
	  zMapMessage("Coords (%d, %d) outside span of transcript (%d, %d)",
		      span.x1, span.x2, x1, x2);
	}
    }
  return status;
}


/* looks for an element with x1 = start + 1 */
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

  return i;
}



/* Update the appropriate array in the feature with data from the screen */
static void updateArray(mainTable table, GArray *array)
{
  ZMapAlignBlockStruct align;
  ZMapSpanStruct span;
  GtkTreeIter iter;
  gboolean valid = FALSE;
  
  /* nuke the old array and create a new one */
  if (array != NULL
      && array->len > (guint)0)
    {
      g_array_free(array, TRUE);
    }

  array = g_array_new(FALSE, TRUE, sizeof(ZMapSpanStruct));


  /* load the new array with values from the GtkListStore */  
  if (table->value.listStore != NULL)
    valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(table->value.listStore), &iter);

  while (valid)
    {
      /* GtkListStore enforces integers here */
      gtk_tree_model_get (GTK_TREE_MODEL(table->value.listStore), &iter, 
			  COL1, &span.x1,
			  COL2, &span.x2,
			  -1);
      g_array_append_val(array, span);
      
      valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(table->value.listStore), &iter);
    }

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
  editorData editor_data = (editorData)data;
    
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
	case ENTRY: case CHECK:
	  if (table[i].value.entry != NULL)
	    g_free(table[i].value.entry);
	  break;

	  /* alignments, introns and exons are held in
	   * GgkListStores which are attached to the
	   * window and destroyed automatically with it. */
	}
    } 
  g_free(table);

  return;
}



/********************* end of file ********************************/
