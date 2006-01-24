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
 * Last edited: Jan 24 10:33 2006 (rds)
 * Created: Mon Jun 6 13:00:00 (rnc)
 * CVS info:   $Id: zmapWindowEditor.c,v 1.20 2006-01-24 10:38:15 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>

#define LIST_COLUMN_WIDTH 50
#define EDITOR_COL_DATA_KEY   "column_decoding_data"
#define EDITOR_COL_NUMBER_KEY "column_number_data"
/* Be nice to have a struct to reduce gObject set/get data calls to 1 */

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

typedef enum {
  NONE, 
  LABEL, 
  ENTRY, 
  INT, 
  FLOAT, 
  STRAND, 
  PHASE, 
  FTYPE, 
  HTYPE, 
  CHECK, 
  ALIGN, 
  EXON, 
  INTRON, 
  LAST
} fieldType;

enum {
  EDIT_COL_NAME,                /*!< feature name column  */
  EDIT_COL_TYPE,                /*!< feature type column  */
  EDIT_COL_START,               /*!< feature start  */
  EDIT_COL_END,                 /*!< feature end  */
  EDIT_COL_STRAND,              /*!< feature strand  */
  EDIT_COL_PHASE,               /*!< feature phase  */
  EDIT_COL_SCORE,               /*!< feature score  */
  EDIT_COL_NUMBER               /*!< number of columns  */
};

typedef enum {
  VIEW_ONLY   = 1 << 1, /*!< Only ever view the properties */
  EDIT_ONLY   = 1 << 2, /*!< Allow users to edit stuff */
  CREATE_ONLY = 1 << 3, /*!< Uncoded version where only creation of new features will be allowed */
  EDIT_CREATE = 1 << 4  /*!< Further uncoded where editing of created features will take place */
} editorMode;

typedef gboolean (*EditorCallbackFunc)(char *label, char *value, void *retValue) ;

/* An array of this struct is the main driver for the program */
typedef struct MainTableStruct
{
  fieldType  fieldtype;  /* controls the way the field is drawn */
  fieldType  datatype;   /* the type of data being held */
  char       label[20];
  int        pair;       /* for coords, gives start pos cf end in array.
			  * ie for validation, end pos has pair = -1. */
  void      *fieldPtr;   /* the field in the modified feature */
  GtkWidget *widget;     /* the widget on the screen */
  union
  {
    char         *entry;     /* the value being displayed */
    GtkListStore *listStore; /* the list of aligns, exons, introns, etc */
  } value;
  gboolean           editable;
  EditorCallbackFunc validateCB;

} mainTableStruct, *mainTable;

typedef struct _zmapWindowEditorDataStruct
{
  ZMapWindow     zmapWindow;

  GtkWidget     *window;        /*!< our window  */
  GtkWidget     *mainVbox;    /*!< maybe its the main vbox */
  GtkWidget     *vbox;        /* most attributes are just stacked in here */
  GtkWidget     *hbox;        /* exon & intron arrays stacked side by side. */

  GtkTreeStore  *store;
  GtkWidget *view;

  mainTable      table;

  FooCanvasItem *item;          /*!< The item the user called us on  */

  ZMapFeature    origFeature; /*!< Feature as supplied, this MUST not be changed */
  ZMapFeature    wcopyFeature;  /*!< Our working copy. */
  GList *appliedChanges;  /*!< A list Features with the changes the user made  */

  editorMode mode;        /*!< which mode the editor is in.  Not taken note of currently, but later...  */
  gboolean applyPressed;  /* I really do not want this */
  gboolean savePressed;   /* nor this, just silly.  Surely there's another way */
  gboolean editable;      /*!< Whether or not the treeView columns are editable  */
  gboolean Ontology_compliant; /*!< Whether or not we're interested in compliance with a user specified ontology  */

} zmapWindowEditorDataStruct;



/* function prototypes ************************************/
//static gboolean arrayClickedCB(GObject *gobject,
//			GParamSpec *arg1,
//		       gpointer user_data);

static void parseFeature(mainTableStruct table[], ZMapFeature origFeature, ZMapFeature feature);
//static void parseField(mainTable table, ZMapFeature feature);
static void array2List(mainTable table, GArray *array, ZMapFeatureType feature_type);

static void createEditWindow(ZMapWindowEditor editor_data, GtkTreeModel *treeModel);
static void closeWindowCB(GtkWidget *widget, gpointer data);
static void closeButtonCB(GtkWidget *widget, gpointer data);
static void undoChangesCB(GtkWidget *widget, gpointer data);
static gboolean applyChangesCB(GtkWidget *widget, gpointer data);
static void saveChangesCB(GtkWidget *widget, gpointer data);
static void saveArray    (GArray *original, GArray *modified);

static gboolean validateEntryCB (char *label, char *value, void *retValue);
static gboolean validateStrandCB(char *label, char *value, void *retValue);
static gboolean validatePhaseCB (char *label, char *value, void *retValue);
static gboolean validateFloatCB (char *label, char *value, void *retValue);
static gboolean validateArray   (ZMapWindowEditor editor_data);
static gboolean checkCoords     (int x1, int x2, ZMapSpan span);
static int      findNextElement (GArray *array, int start);

static void updateArray(mainTable table, GArray *array);

static void addFields (ZMapWindowEditor editor_data);
static void addArray(gpointer data, ZMapWindowEditor editor_data);
static void addCheckButton(GtkWidget *vbox, mainTable table);
static void addEntry(GtkWidget *vbox, mainTable table);
static void addLabel(GtkWidget *vbox, mainTable table);

static void buildCol(colInfoStruct colInfo, GtkWidget *treeView, mainTable table);
static gboolean arrayEditedCB(GtkCellRendererText *renderer, 
			  char *path, char *new_text, gpointer user_data);

/* My new methods*/
static char *myFeatureLookupEnum(int id, int enumType);
static gboolean selectionFunc(GtkTreeSelection *selection, 
                              GtkTreeModel     *model,
                              GtkTreePath      *path, 
                              gboolean          path_currently_selected,
                              gpointer          user_data);
static void cellEditedCB(GtkCellRendererText *renderer, 
                         char *path, char *new_text, 
                         gpointer user_data);
static GtkCellRenderer *getColRenderer(ZMapWindowEditor editor);

/* function code *******************************************/
/* This is the create function!
 * What does it do?
 * - Creates the window in which to draw everything.
 * - fills in the original feature. If it exists
 * - Sets up all the frames and boxes and keeps ref to them.
 */
/* For now we accept the item here, but when we start being a feature
 * creator as well, it'll be set before draw. */
ZMapWindowEditor zmapWindowEditorCreate(ZMapWindow zmapWindow, FooCanvasItem *item)
{
  ZMapWindowEditor editor = NULL;
  int type                = 0;
  editor = g_new0(zmapWindowEditorDataStruct, 1);

  editor->origFeature  = g_object_get_data(G_OBJECT(item), "item_feature_data");
  type = GPOINTER_TO_INT( g_object_get_data(G_OBJECT(item), "item_feature_type") );
  zMapAssert(editor->origFeature && ((type == ITEM_FEATURE_SIMPLE) || 
                                     (type == ITEM_FEATURE_PARENT) || 
                                     (type == ITEM_FEATURE_CHILD)
                                     )
             );
  /* Otherwise we end up with trash */

  editor->zmapWindow   = zmapWindow;
  editor->item         = item;
  editor->hbox         = NULL;
  editor->applyPressed = FALSE;
  editor->savePressed  = FALSE;
  editor->table        = g_new0(mainTableStruct, 16);
  editor->appliedChanges = NULL;
  editor->wcopyFeature   = zMapFeatureCopy(editor->origFeature);

  switch(editor->origFeature->type)
    {
    case ZMAPFEATURE_ALIGNMENT:
      editor->editable = FALSE;      
      break;
    case ZMAPFEATURE_TRANSCRIPT:
      editor->editable = TRUE;
      break;
    default:
      editor->editable = FALSE;      
      break;
    }

  parseFeature(editor->table, editor->origFeature, editor->wcopyFeature);

  {
    GtkTreeModel *treeModel = NULL;
    GList *itemList         = NULL;
    treeModel = zmapWindowFeatureListCreateStore(TRUE);
    itemList  = g_list_append(itemList, item);
    zmapWindowFeatureListPopulateStoreList(treeModel, itemList);

    createEditWindow(editor, treeModel);
  }

  gtk_widget_show_all(editor->window);

  return editor;
}


static char *myFeatureLookupEnum(int id, int enumType)
{
  /* These arrays must correspond 1:1 with the enums declared in zmapFeature.h */
  static char *types[]   = {"Basic", "Homol", "Exon", "Intron", "Transcript",
			    "Variation", "Boundary", "Sequence"} ;
  static char *strands[] = {".", "Forward", "Reverse" } ;
  static char *phases[]  = {"None", "0", "1", "2" } ;
  static char *homolTypes[] = {"ZMAPHOMOL_N_HOMOL", "ZMAPHOMOL_X_HOMOL", "ZMAPHOMOL_TX_HOMOL"} ;
  char *enum_str = NULL ;

  zMapAssert(enumType    == TYPE_ENUM 
             || enumType == STRAND_ENUM 
	     || enumType == PHASE_ENUM 
             || enumType == HOMOLTYPE_ENUM) ;

  switch (enumType)
    {
    case TYPE_ENUM:
      enum_str = types[id];
      break;
      
    case STRAND_ENUM:
      enum_str = strands[id];
      break;
      
    case PHASE_ENUM:
      enum_str = phases[id];
      break;

    case HOMOLTYPE_ENUM:
      enum_str = homolTypes[id];
      break;
    }
  
  return enum_str ;
}


/* I see what's going on here, but it doesn't feel dynamic enough to
 * cope with what I want. I don't need it to have super strength what
 * is that gonna be. But a little cleverer than it currently is would
 * be nice .
 */

/* So that the draw routines don't need to know more than the minimum
 * about what they're drawing, everything in the table, apart from the
 * arrays, is held as a string. 
 */
static void parseFeature(mainTableStruct table[], ZMapFeature origFeature, ZMapFeature feature)
{
  int i = 0;
  /*                          fieldtype          label                fieldPtr     value       validateCB
   *                                   datatype                    pair      widget     editable   */
  mainTableStruct ftypeInit   = { ENTRY , FTYPE , "Type"           , 0, NULL, NULL, {NULL}, FALSE, NULL },
                  fx1Init     = { ENTRY , INT   , "Start"          , 0, NULL, NULL, {NULL}, TRUE , validateEntryCB  },
                  fx2Init     = { ENTRY , INT   , "End"            ,-1, NULL, NULL, {NULL}, TRUE , validateEntryCB  },
	          fstrandInit = { ENTRY , STRAND, "Strand"         , 0, NULL, NULL, {NULL}, FALSE, NULL  },
	          fphaseInit  = { ENTRY , PHASE , "Phase"          , 0, NULL, NULL, {NULL}, TRUE , validatePhaseCB  },
	          fscoreInit  = { ENTRY , FLOAT , "Score"          , 0, NULL, NULL, {NULL}, TRUE , validateFloatCB  },
	      
	          blankInit   = { LABEL , LABEL , " "              , 0, NULL, NULL, {NULL}, TRUE , NULL  },
			    
		  htypeInit   = { ENTRY , HTYPE , "Homol Type"     , 0, NULL, NULL, {NULL}, FALSE, NULL  },
		  hy1Init     = { ENTRY , INT   , "Query Start"    , 0, NULL, NULL, {NULL}, FALSE, NULL  },
		  hy2Init     = { ENTRY , INT   , "Query End"      ,-1, NULL, NULL, {NULL}, FALSE, NULL  },
		  hstrandInit = { ENTRY , STRAND, "Query Strand"   , 0, NULL, NULL, {NULL}, FALSE, NULL  },
		  hphaseInit  = { ENTRY , PHASE , "Query Phase"    , 0, NULL, NULL, {NULL}, FALSE, NULL  },
		  hscoreInit  = { ENTRY , FLOAT , "Query Score"    , 0, NULL, NULL, {NULL}, FALSE, NULL  },
		  halignInit  = { ALIGN , ALIGN , "Alignments"     , 0, NULL, NULL, {NULL}, FALSE, NULL  },
			    
		  tx1Init     = { ENTRY , INT   , "CDS Start"      , 0, NULL, NULL, {NULL}, TRUE , validateEntryCB  },
		  tx2Init     = { ENTRY , INT   , "CDS End"        ,-1, NULL, NULL, {NULL}, TRUE , validateEntryCB  },
		  tphaseInit  = { ENTRY , PHASE , "CDS Phase"      , 0, NULL, NULL, {NULL}, TRUE , validatePhaseCB  },
		  tsnfInit    = { CHECK , CHECK , "Start Not Found", 0, NULL, NULL, {NULL}, TRUE , NULL  },
		  tenfInit    = { CHECK , CHECK , "End Not Found " , 0, NULL, NULL, {NULL}, TRUE , NULL  },
		  texonInit   = { EXON  , EXON  , "Exons"          , 0, NULL, NULL, {NULL}, TRUE , NULL  },
		  tintronInit = { INTRON, INTRON, "Introns"        , 0, NULL, NULL, {NULL}, TRUE , NULL  },
			    
		  lastInit    = { LAST  , NONE  ,  ""              , 0, NULL, NULL, {NULL}, FALSE , NULL };
		  
  table[i] = ftypeInit;
  table[i].fieldPtr = &feature->type;
  
  i++;
  table[i] = fx1Init;
  table[i].fieldPtr = &feature->x1;
  if (feature->type == ZMAPFEATURE_ALIGNMENT)
    table[i].editable = FALSE;
  
  i++;
  table[i] = fx2Init;
  table[i].fieldPtr = &feature->x2;
  if (feature->type == ZMAPFEATURE_ALIGNMENT)
    table[i].editable = FALSE;
  
  i++;
  table[i] = fstrandInit;
  table[i].fieldPtr = &feature->strand;
  
  i++;
  table[i] = fphaseInit;
  table[i].fieldPtr = &feature->phase;
  
  i++;
  table[i] = fscoreInit;
  table[i].fieldPtr = &feature->score;
  
  i++;
  table[i] = blankInit;
  table[i].value.entry = " ";
  
  if (feature->type == ZMAPFEATURE_ALIGNMENT)
    {
      i++;
      table[i] = htypeInit;
      table[i].fieldPtr = &feature->type;
      
      i++;
      table[i] = hy1Init;
      table[i].fieldPtr = &feature->feature.homol.y1;
      
      i++;
      table[i] = hy2Init;
      table[i].fieldPtr = &feature->feature.homol.y2;
      
      i++;
      table[i] = hstrandInit;
      table[i].fieldPtr = &feature->feature.homol.target_strand;
      
      i++;
      table[i] = hphaseInit;
      table[i].fieldPtr = &feature->feature.homol.target_phase;
      
      i++;
      table[i] = hscoreInit;
      table[i].fieldPtr = &feature->score ;
      
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
      table[i].fieldPtr = &feature->feature.transcript.cds_start;
      
      i++;
      table[i] = tx2Init;
      table[i].fieldPtr = &feature->feature.transcript.cds_end ;

      i++;
      table[i] = tphaseInit;
      table[i].fieldPtr = &feature->feature.transcript.start_phase ;
      

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

      /* Commented out for now as these are bitfields which do not have an address... */
      i++;
      table[i] = tsnfInit;
      table[i].fieldPtr = &feature->feature.transcript.start_not_found ;
      
      i++;
      table[i] = tenfInit;
      table[i].fieldPtr = &feature->feature.transcript.end_not_found ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      
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
#ifdef RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRr
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
      else if (feature_type == ZMAPFEATURE_ALIGNMENT)
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
#endif
  return;
}



/* Hide this away to make the exposed function smaller... */
static void createEditWindow(ZMapWindowEditor editor_data, GtkTreeModel *treeModel)
{
  GtkWidget *buttonHBox, *subFrame, *mainVBox;
  GtkWidget *treeView, *mainHBox;
#ifdef RDS_DONT_INCLUDE
  GtkTreeStore *treeStore;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  int colNo;                    /* the for loop iterator */
#endif
  char *title;
  zmapWindowFeatureListCallbacksStruct windowCallbacks = { NULL, NULL, NULL };

  /* Set the Title of the Window */
  title = g_strdup_printf("%s - Feature Editor",
			  g_quark_to_string(editor_data->origFeature->original_id));
  editor_data->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (editor_data->window), title);
  g_free(title);
  /* Finished setting title */

  /* Set a few properties ... */
  g_signal_connect (G_OBJECT (editor_data->window), "destroy",
		    G_CALLBACK (closeWindowCB), editor_data);
  gtk_container_set_border_width (GTK_CONTAINER (editor_data->window), 10);

  mainHBox = gtk_hbox_new(FALSE, 0);
  gtk_box_set_spacing(GTK_BOX(mainHBox), 5);
  gtk_container_add(GTK_CONTAINER(editor_data->window), mainHBox);

  /* Create a Vertical Box and add to Window */
  mainVBox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(mainHBox), mainVBox);

  /* This is where we'll put stuff */
  subFrame = gtk_frame_new(g_quark_to_string(editor_data->origFeature->original_id));
  gtk_container_add(GTK_CONTAINER(mainVBox), subFrame);
  //#define GRAPHICAL_DISPLAY
#ifdef GRAPHICAL_DISPLAY
  {
    GdkColor bgcolor;
    GtkWidget *scrolledWindow, *canvas;

    subFrame = gtk_frame_new("Editor graphical display...");
    gtk_container_add(GTK_CONTAINER(mainHBox), subFrame);

    scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(subFrame), GTK_WIDGET(scrolledWindow));

    canvas = foo_canvas_new();
    foo_canvas_set_pixels_per_unit_xy(FOO_CANVAS(canvas), 1.0, 1.0);
  
    gdk_color_parse("white", &bgcolor);
    gtk_widget_modify_bg(GTK_WIDGET(canvas), GTK_STATE_NORMAL, &(bgcolor));

    foo_canvas_item_new(FOO_CANVAS_GROUP(foo_canvas_root(FOO_CANVAS(canvas))),
                        foo_canvas_rect_get_type(),
                        "x1", 10.0, "x2", 20.0,
                        "y1", 10.0, "y2", 20.0,
                        NULL);

    gtk_container_add(GTK_CONTAINER(scrolledWindow), canvas);
  }
#endif

  windowCallbacks.selectionFuncCB = selectionFunc;
  editor_data->view = 
    treeView = zmapWindowFeatureListCreateView(treeModel, 
                                               getColRenderer(editor_data),
                                               &windowCallbacks, 
                                               editor_data);

  subFrame = gtk_frame_new("Details...");
  gtk_container_add(GTK_CONTAINER(mainVBox), subFrame);
  gtk_container_add(GTK_CONTAINER(subFrame), GTK_WIDGET(treeView));

  /* Sort out the buttons frame */
  subFrame   = gtk_frame_new("buttons");
  buttonHBox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(subFrame), buttonHBox);
  gtk_box_pack_start(GTK_BOX(mainVBox), subFrame, FALSE, FALSE, 0);

  if(1)
    {
      GtkWidget *button, *buttonBox;
      int spacing = 10;
      /* These should end up over the left hand side */
      buttonBox = gtk_hbutton_box_new();
      gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonBox), GTK_BUTTONBOX_START);
      gtk_box_set_spacing (GTK_BOX (buttonBox), spacing);
      gtk_container_set_border_width (GTK_CONTAINER (buttonBox), 5);
      gtk_button_box_set_child_size (GTK_BUTTON_BOX(buttonBox), 100, 1);

      button = gtk_button_new_with_label("Close");
      g_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(closeButtonCB), editor_data);
      gtk_container_add(GTK_CONTAINER(buttonBox), button) ;
      /*  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);  */

      button = gtk_button_new_with_label("Undo");
      g_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(undoChangesCB), editor_data);
      gtk_container_add(GTK_CONTAINER(buttonBox), button) ;

      /* Add them to the Hbox */
      gtk_box_pack_start(GTK_BOX(buttonHBox), buttonBox, TRUE, TRUE, 0) ;

      /* These on the right */
      buttonBox = gtk_hbutton_box_new();
      gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonBox), GTK_BUTTONBOX_END);
      gtk_box_set_spacing (GTK_BOX (buttonBox), spacing);
      gtk_container_set_border_width (GTK_CONTAINER (buttonBox), 5);
      gtk_button_box_set_child_size (GTK_BUTTON_BOX(buttonBox), 100, 1);

      button = gtk_button_new_with_label("Apply Changes");
      g_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(applyChangesCB), editor_data);
      gtk_container_add(GTK_CONTAINER(buttonBox), button) ;

      button = gtk_button_new_with_label("Save & Close");
      g_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(saveChangesCB), editor_data);
      gtk_container_add(GTK_CONTAINER(buttonBox), button) ;

      /* Add them to the Hbox */
      gtk_box_pack_start(GTK_BOX(buttonHBox), buttonBox, TRUE, TRUE, 0) ;      
    }


  return;
}



static void addFields(ZMapWindowEditor editor_data)
{
  mainTable table = editor_data->table;
  int i = 0;

  while (table[i].fieldtype != LAST)
    {
      switch (table[i].fieldtype)
	{
	case LABEL:
	  addLabel(editor_data->vbox, &table[i]);
	  break;
	  
	case ENTRY:
	  addEntry(editor_data->vbox, &table[i]);
	  break;
	  
	case CHECK:
	  addCheckButton(editor_data->vbox, &table[i]);
	  break;
	  
	case ALIGN:
	  if (table[i].value.listStore != NULL)
	    addArray(&table[i], editor_data);
	  break;

	case EXON: case INTRON:
	  if (table[i].value.listStore != NULL)
	    addArray(&table[i], editor_data);
	  break;
        default:
          break;
	}
      i++;
    }

  return;
}



static void addLabel(GtkWidget *vbox, mainTable table)
{
  GtkWidget *hbox, *label;

  label = gtk_label_new(table->label);
  hbox  = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  label = gtk_label_new(table->value.entry);
  gtk_box_pack_end(GTK_BOX(hbox), label , FALSE, FALSE, 0);
  
  gtk_box_pack_start(GTK_BOX(vbox), hbox  , FALSE, FALSE, 0);

  return;
}


  

static void addEntry(GtkWidget *vbox, mainTable table)
{
  GtkWidget *hbox, *label;

  label = gtk_label_new(table->label);
  hbox  = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

  table->widget  = gtk_entry_new();
  gtk_box_pack_end(GTK_BOX(hbox), table->widget , FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox  , FALSE, FALSE, 0);

  switch (table->datatype)
    {
    case FTYPE:
      gtk_entry_set_text(GTK_ENTRY(table->widget), 
			 zmapFeatureLookUpEnum(*((int *)table->fieldPtr), TYPE_ENUM));
      break;

    case INT: case PHASE:  
      gtk_entry_set_text(GTK_ENTRY(table->widget), g_strdup_printf("%d", *((int *)table->fieldPtr)));
      break;

    case FLOAT:
      gtk_entry_set_text(GTK_ENTRY(table->widget), g_strdup_printf("%.2f", *((float *)table->fieldPtr)));
      break;

    case STRAND:
      switch (*((int *)table->fieldPtr))
	{
	case ZMAPSTRAND_FORWARD:
	  gtk_entry_set_text(GTK_ENTRY(table->widget), "Forward");
	  break;

	case ZMAPSTRAND_REVERSE:
	  gtk_entry_set_text(GTK_ENTRY(table->widget), "Reverse");
	  break;

	case ZMAPSTRAND_NONE:
	  gtk_entry_set_text(GTK_ENTRY(table->widget), "None");
	  break;
	}
      break;

    case HTYPE:
      switch (*((int *)table->fieldPtr))
	{
	case ZMAPHOMOL_X_HOMOL:
	  gtk_entry_set_text(GTK_ENTRY(table->widget), "X");
	  break;

	case ZMAPHOMOL_N_HOMOL:
	  gtk_entry_set_text(GTK_ENTRY(table->widget), "N");
	  break;

	case ZMAPHOMOL_TX_HOMOL:
	  gtk_entry_set_text(GTK_ENTRY(table->widget), "TX");
	  break;
	}
      break;
    default:
      break;
    }

  if (table->editable == FALSE)
    gtk_widget_set_sensitive(table->widget, FALSE);

  return;
}




static void addCheckButton(GtkWidget *vbox, mainTable table)
{
  GtkWidget *hbox, *label;

  label = gtk_label_new(table->label);
  hbox  = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  table->widget = gtk_check_button_new();
  gtk_box_pack_start(GTK_BOX(hbox), table->widget, FALSE, FALSE, 0);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->widget), *((int *)table->fieldPtr));

  return;
}




static void addArray(gpointer data, ZMapWindowEditor editor_data)
{
#ifdef AAAAAAAAAAAAAAAAAAAAAAAAAAAAAa
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
  gtk_container_set_border_width(GTK_CONTAINER(scrolledWindow), 5);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(frame), scrolledWindow);
  gtk_container_add(GTK_CONTAINER(scrolledWindow), treeView);

#endif
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

  column = gtk_tree_view_column_new_with_attributes (colInfo.label,
						     renderer,
						     "text", colInfo.colNo,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
  gtk_tree_view_column_set_min_width(column, LIST_COLUMN_WIDTH);

  return;
}

static void cellEditedCB(GtkCellRendererText *renderer, 
                         char *path, char *new_text, 
                         gpointer user_data)
{
  ZMapWindowEditor editor   = (ZMapWindowEditor)user_data;
  int colNumber             = EDIT_COL_NUMBER;
  GtkTreePath *cursor_path  = NULL;
  GtkTreeViewColumn *column = NULL;
  GtkTreeModel *treeModel   = NULL;
  GtkTreeIter iter;
  GType colType;

  /* Get the model, we need this to set the value */
  treeModel = gtk_tree_view_get_model(GTK_TREE_VIEW(editor->view));
  /* Find where we are. bizarrely enough we don't actually know yet. */
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(editor->view), &cursor_path, &column);
  if(!cursor_path || !column)
    return ;                    /* We can't continue without these */
  gtk_tree_model_get_iter(GTK_TREE_MODEL(treeModel), &iter, cursor_path);
  gtk_tree_path_free(cursor_path); /* We need to free this, do it now */
  /* zmapWindowFeatureListGetColNumberFromTVC(column) would work too. */
  colNumber  = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), 
                                                 ZMAP_WINDOW_FEATURE_LIST_COL_NUMBER_KEY));
  colType    = gtk_tree_model_get_column_type(GTK_TREE_MODEL(treeModel), colNumber);

  printf("Cell Edited: column '%d', new text = '%s'\n", colNumber, new_text);
  /* We have to do some validation???? */
  switch(colType)
    {
    case G_TYPE_INT:
      gtk_tree_store_set(GTK_TREE_STORE(treeModel), &iter, 
                         colNumber, atoi(new_text), 
                         -1);
      break;
    case G_TYPE_STRING:
      gtk_tree_store_set(GTK_TREE_STORE(treeModel), &iter, 
                         colNumber, new_text, 
                         -1);
      break;
    case G_TYPE_FLOAT:
      {
        double value = 0.0;
        if((zMapStr2Double(new_text, &value)))
          gtk_tree_store_set(GTK_TREE_STORE(treeModel), &iter,
                             colNumber, value,
                             -1);
      }
      break;
    default:
      printf("Don't know how to edit this kind of column\n");
      break;
    }

  return ;
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

  colNo = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(renderer), "ColNo"));
  gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(listStore), &iter, path);
  gtk_list_store_set(listStore, &iter, colNo, atoi(new_text), -1);

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(listStore), &iter);
  /* What is this doing, we've already changed the data!?! */
  while (valid)
    {
#ifdef RDS_DONT_INCLUDE
      ZMapSpanStruct span;
      gtk_tree_model_get (GTK_TREE_MODEL(listStore), &iter, 
			  COL1, &span.x1,
			  COL2, &span.x2,
			  -1);
#endif /* RDS_DONT_INCLUDE */
      valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(listStore), &iter);
    }
 
  return FALSE;
}
 



/* copy the original feature over the modified one, then 
 * just redraw everything below the buttons. */
static void undoChangesCB(GtkWidget *widget, gpointer data)
{
  ZMapWindowEditor editor_data = (ZMapWindowEditor)data;

  if (editor_data->applyPressed == TRUE)
    {
      zmapFeatureDestroy(editor_data->wcopyFeature);
      //editor_data->wcopyFeature = zMapFeatureCopy(editor_data->appliedFeature);
      //      zmapFeatureDestroy(editor_data->appliedFeature);
      editor_data->applyPressed = FALSE;
    }

  if (editor_data->origFeature->type != ZMAPFEATURE_ALIGNMENT)
    {
      zMapWindowMoveItem(editor_data->zmapWindow, editor_data->wcopyFeature,
			 editor_data->origFeature, editor_data->item);

      if (editor_data->origFeature->type == ZMAPFEATURE_TRANSCRIPT)
	{
	  if (editor_data->wcopyFeature->feature.transcript.exons != NULL
	      && editor_data->wcopyFeature->feature.transcript.exons->len > (guint)0
	      && editor_data->origFeature->feature.transcript.exons != NULL
	      && editor_data->origFeature->feature.transcript.exons->len > (guint)0)
	    {
	      zMapWindowMoveSubFeatures(editor_data->zmapWindow, 
					editor_data->wcopyFeature,
					editor_data->origFeature,
					editor_data->wcopyFeature->feature.transcript.exons, 
					editor_data->origFeature->feature.transcript.exons,
					TRUE);
	    }
	  if (editor_data->wcopyFeature->feature.transcript.introns != NULL
	      && editor_data->wcopyFeature->feature.transcript.introns->len > (guint)0
	      && editor_data->origFeature->feature.transcript.introns != NULL
	      && editor_data->wcopyFeature->feature.transcript.introns->len > (guint)0)
	    {
	      zMapWindowMoveSubFeatures(editor_data->zmapWindow, 
					editor_data->wcopyFeature,
					editor_data->origFeature,
					editor_data->wcopyFeature->feature.transcript.introns, 
					editor_data->origFeature->feature.transcript.introns,
					FALSE);
	    } 
	}
    }
  editor_data->wcopyFeature = zMapFeatureCopy(editor_data->origFeature);

  parseFeature(editor_data->table, editor_data->origFeature, editor_data->wcopyFeature);

  gtk_widget_destroy(editor_data->vbox);
  editor_data->vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(editor_data->mainVbox), editor_data->vbox, TRUE, TRUE, 0);
  editor_data->hbox = NULL;  /* invalidated when its parent vbox was destroyed */

  addFields(editor_data);

  gtk_widget_show_all(editor_data->window);

  return;
}


/* Free the original feature, then copy the modified version and 
 * set the object data for the item to point to it. */
static void saveChangesCB(GtkWidget *widget, gpointer data)
{
  ZMapWindowEditor editor_data = (ZMapWindowEditor)data;
  gboolean valid = TRUE;

  if (editor_data->applyPressed == FALSE)
    valid = applyChangesCB(NULL, (gpointer)editor_data);

  if (valid == TRUE)
    {
      zmapFeatureDestroy(editor_data->origFeature);

      editor_data->origFeature = zMapFeatureCopy(editor_data->wcopyFeature);

      g_object_set_data(G_OBJECT(editor_data->item), "item_feature_data", editor_data->origFeature);

      editor_data->savePressed = TRUE;
      editor_data->applyPressed = FALSE;
    }

  return;
}



/* Save the contents of the modified array over the top
 * of the original. */
static void saveArray(GArray *original, GArray *modified)
{
  int i;
  ZMapSpanStruct span;

  if (original != NULL && modified != NULL)
    {
      /* nuke the old array and create a new one */
      g_array_free(original, TRUE);
      
      original = g_array_new(FALSE, TRUE, sizeof(ZMapSpanStruct));
      
      for (i = 0; i < modified->len; i++)
	{
	  span = g_array_index(modified, ZMapSpanStruct, i);
	  g_array_append_val(original, span);
	}
    }

  return;
}



/* saves any user changes in editor_data->wcopyFeature, but 
 * leaves the original copy untouched in case they want to Undo. */
static gboolean applyChangesCB(GtkWidget *widget, gpointer data)
{
  ZMapWindowEditor editor_data = (ZMapWindowEditor)data;
  mainTable table = editor_data->table;
  ZMapFeature feature;
  char *value;
  int i;
  gboolean valid = TRUE, checked = FALSE;
  void *retValue = NULL;
  int prevValue;

  retValue = g_new0(double, 1);
  zMapAssert(retValue);

  feature = editor_data->wcopyFeature;

  for (i = 1; table[i].fieldtype != LAST && valid == TRUE; i++)
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
		case INT:
		  if (table[i].pair != 0
		      && *((int *)retValue) < *((int *)table[i + table[i].pair].fieldPtr))
		    {
		      zMapMessage("Start coord (%d) exceeds end coord (%d)",
				  *((int *)table[i + table[i].pair].fieldPtr),  *((int *)retValue));
		      /* Set the start value back to what it was before */
		      *((int *)table[i + table[i].pair].fieldPtr) = prevValue;
		      valid = FALSE;
		    }
		  else
		    {
		      prevValue =  *((int *)table[i].fieldPtr);
		      *((int *)table[i].fieldPtr) = *((int *)retValue);
		    }
		  break;
		  
		case STRAND: case PHASE: 
		  *((int *)table[i].fieldPtr) = *((int *)retValue);
		  break;
		  
		case FLOAT:
		  /* retValue points to a double, because I used ZMapStr2Double
		   * to convert it. */
		  *((float *)table[i].fieldPtr) = *((double *)retValue);
		  break;
                default:
                  break;        /* 981 */
		}
	    }
	}

      if (valid == TRUE)
	{
	  switch (table[i].fieldtype)
	    {
	    case CHECK:
	      checked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table[i].widget));



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      /* Commented out for now as these are now bitfields.... */

	      if (checked == TRUE)
		{
		  table[i].value.entry = "True";

		  if (g_ascii_strcasecmp(table[i].label, "Start Not Found") == 0)
		    feature->feature.transcript.start_not_found = TRUE;
		  else
		    feature->feature.transcript.endNotFound = TRUE;
		}
	      else
		{
		  table[i].value.entry = "False";

		  if (g_ascii_strcasecmp(table[i].label, "Start Not Found") == 0)
		    feature->feature.transcript.start_not_found = FALSE;
		  else
		    feature->feature.transcript.endNotFound = FALSE;
		}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



	      break;
	      
	      /* homology alignments are not editable.  Note that this
	       * is just updating the modified feature arrays, not the
	       * original, which only gets updated by Save. */
	    case EXON: 
	      updateArray(&table[i], feature->feature.transcript.exons);
	      break;
	      
	    case INTRON: 
	      updateArray(&table[i], feature->feature.transcript.introns);
	      break;
            default:
              break;
	    }
	}
    }

  g_free(retValue);

  /* once both intron and exon arrays in the modified feature have been
   * updated with the data from the screen, validate them and if all is
   * well, move the items on the main display. */
  if (valid == TRUE)
    valid = validateArray(editor_data);

  if (valid == TRUE && editor_data->origFeature->type != ZMAPFEATURE_ALIGNMENT)
    {
      ZMapFeature origFeature;

      if (editor_data->applyPressed == TRUE)
	origFeature = origFeature; //editor_data->appliedFeature;
      else
	origFeature = editor_data->origFeature;

      zMapWindowMoveItem(editor_data->zmapWindow, origFeature,
			 editor_data->wcopyFeature, editor_data->item);

      if (editor_data->origFeature->type == ZMAPFEATURE_TRANSCRIPT)
	{
	  if (editor_data->wcopyFeature->feature.transcript.exons != NULL
	      && editor_data->wcopyFeature->feature.transcript.exons->len > (guint)0
	      && origFeature->feature.transcript.exons != NULL
	      && origFeature->feature.transcript.exons->len > (guint)0)
	    {
	      zMapWindowMoveSubFeatures(editor_data->zmapWindow, 
					origFeature,
					editor_data->wcopyFeature,
					origFeature->feature.transcript.exons, 
					editor_data->wcopyFeature->feature.transcript.exons,
				        TRUE);
	    }
	  if (editor_data->wcopyFeature->feature.transcript.introns != NULL
	      && editor_data->wcopyFeature->feature.transcript.introns->len > (guint)0
	      && origFeature->feature.transcript.introns != NULL
	      && origFeature->feature.transcript.introns->len > (guint)0)
	    {
	      zMapWindowMoveSubFeatures(editor_data->zmapWindow, 
					origFeature,
					editor_data->wcopyFeature,
					origFeature->feature.transcript.introns, 
					editor_data->wcopyFeature->feature.transcript.introns,
					FALSE);
	    } 
	}

      if (editor_data->applyPressed == TRUE)
	zmapFeatureDestroy(editor_data->wcopyFeature); /* was appliedFeature */

      //      editor_data->appliedFeature = zMapFeatureCopy(editor_data->wcopyFeature);
      editor_data->applyPressed = TRUE;
    }
    
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

  if (zMapFeatureStr2Strand(value, (ZMapStrand *)retValue) == FALSE)
    {
      zMapMessage("%s (%s) must be Forward, Reverse or None", label, value);
      status = FALSE;
    }

  return status;
}


static gboolean validatePhaseCB(char *label, char *value, void *retValue)
{
  gboolean status = TRUE;

  if ((status = zMapFeatureValidatePhase(value, (ZMapPhase *)retValue)) == FALSE)
    zMapMessage("%s (%s) must be 0, 1 or 2", label, value);

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
 * should just be a simple consecutive series of exons and introns.  */
static gboolean validateArray(ZMapWindowEditor editor_data)
{
  int i;
  gboolean valid = TRUE, finished = FALSE;
  ZMapFeature modifiedFeature = editor_data->wcopyFeature;
  ZMapSpanStruct span;

  if (modifiedFeature->type == ZMAPFEATURE_TRANSCRIPT)
    {
      /* get the first exon */
      span = g_array_index(modifiedFeature->feature.transcript.exons, ZMapSpanStruct, 0);
      
      valid = checkCoords(modifiedFeature->x1, modifiedFeature->x2, &span);

      while (valid == TRUE && finished == FALSE)
	{
	  /* look for an adjacent element amongst the introns */
	  if (modifiedFeature->feature.transcript.introns != NULL
	      && modifiedFeature->feature.transcript.introns->len > (guint)0)
	    {
	      i = findNextElement(modifiedFeature->feature.transcript.introns, span.x2);
	      
	      if (i < modifiedFeature->feature.transcript.introns->len)
		{
		  span = g_array_index(modifiedFeature->feature.transcript.introns, ZMapSpanStruct, i);
		  valid = checkCoords(modifiedFeature->x1, modifiedFeature->x2, &span);
		}
	    }

	  /* whether found or not, span holds the one we want to match, 
	   * so now look for an adjacent element amongst the exons. */
	  if (valid == TRUE)
	    {
	      if (span.x2 == modifiedFeature->x2)
		finished = TRUE;
	      else
		{
		  i = findNextElement(modifiedFeature->feature.transcript.exons, span.x2);
		  
		  if (i < modifiedFeature->feature.transcript.exons->len)
		    {
		      span = g_array_index(modifiedFeature->feature.transcript.exons, ZMapSpanStruct, i);
		      valid = checkCoords(modifiedFeature->x1, modifiedFeature->x2, &span);
		      
		      if (valid == TRUE)
			{
			  if (span.x2 == modifiedFeature->x2)
			    finished = TRUE;
			}
		    }
		  else
		    {
		      zMapMessage("Exons & introns don't make a contiguous series. Can't find element adjacent to %d.", 
				  span.x2);
		      valid = FALSE;
		    }
		}
	    }
	}
      /* if there's an error, we don't want the underlying arrays to hold that 
       * bad data, though it is still displayed on the screen.  */
      if (valid == FALSE)
	{
	  ZMapFeature origFeature = editor_data->origFeature;

	  if (origFeature->feature.transcript.exons != NULL
	      && origFeature->feature.transcript.exons->len > (guint)0)
	    {
	      g_array_free(modifiedFeature->feature.transcript.exons, TRUE);

	      modifiedFeature->feature.transcript.exons = 
		g_array_sized_new(FALSE, TRUE, 
				  sizeof(ZMapSpanStruct),
				  origFeature->feature.transcript.exons->len);
	      
	      for (i = 0; i < origFeature->feature.transcript.exons->len; i++)
		{
		  span = g_array_index(origFeature->feature.transcript.exons, ZMapSpanStruct, i);
		  modifiedFeature->feature.transcript.exons = 
		    g_array_append_val(modifiedFeature->feature.transcript.exons, span);
		}
	    }
	  
	  if (modifiedFeature->feature.transcript.introns != NULL
	      && modifiedFeature->feature.transcript.introns->len > (guint)0)
	    {
	      g_array_free(modifiedFeature->feature.transcript.introns, TRUE);

	      modifiedFeature->feature.transcript.introns = 
		g_array_sized_new(FALSE, TRUE, 
				  sizeof(ZMapSpanStruct),
				  origFeature->feature.transcript.introns->len);
	      
	      for (i = 0; i < origFeature->feature.transcript.introns->len; i++)
		{
		  span = g_array_index(origFeature->feature.transcript.introns, ZMapSpanStruct, i);
		  modifiedFeature->feature.transcript.introns = 
		    g_array_append_val(modifiedFeature->feature.transcript.introns, span);
		}
	    }
	}
    }
  return valid;
}



/* Checks that all the elements in the array have coordinates
 * within the range specified by x1 and x2. */
static gboolean checkCoords(int x1, int x2, ZMapSpan span)
{
  gboolean valid = TRUE;

  if (span->x1 < x1 || span->x1 > x2
      || span->x2 < x1 || span->x2 > x2)
    {
      valid = FALSE;
      zMapMessage("Coords (%d, %d) outside span of transcript (%d, %d)",
		  span->x1, span->x2, x1, x2);
    }

  return valid;
}


/* looks for an element with x1 = start + 1 */
static int findNextElement(GArray *array, int start)
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
  GtkTreeIter iter;
  gboolean valid = FALSE;
  
  if (array != NULL)
    {
      /* nuke the old array and create a new one */
      g_array_free(array, TRUE);

      array = g_array_new(FALSE, TRUE, sizeof(ZMapSpanStruct));


      /* load the new array with values from the GtkListStore */  
      if (table->value.listStore != NULL)
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(table->value.listStore), &iter);

      while (valid)
	{
	  /* GtkListStore enforces integers here */
#ifdef RDS_DONT_INCLUDE
          ZMapSpanStruct span;
	  gtk_tree_model_get (GTK_TREE_MODEL(table->value.listStore), &iter, 
			      COL1, &span.x1,
			      COL2, &span.x2,
			      -1);
	  g_array_append_val(array, span);
#endif /* RDS_DONT_INCLUDE */
	  valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(table->value.listStore), &iter);
	}
    }
  return;
}




/* We don't want the close button to call closeWindowCB directly
 * since that function is also called by the "destroy" signal we've
 * connected to the window itself and would end up being called
 * twice.  This way it's only called once. */
static void closeButtonCB(GtkWidget *widget, gpointer data)
{
  ZMapWindowEditor editor_data = (ZMapWindowEditor)data;

  if (editor_data->applyPressed == TRUE
      && editor_data->savePressed == FALSE)
    {
      /* this is tacky! */
      zMapMessage("You have unsaved changes! %s", "Press Undo before exiting.");
    }
  else
    g_signal_emit_by_name(editor_data->window, "destroy", NULL, NULL);

  return;
}
  


static void closeWindowCB(GtkWidget *widget, gpointer data)
{
  ZMapWindowEditor editor_data = (ZMapWindowEditor)data;
    
  if (editor_data->applyPressed == TRUE
      && editor_data->savePressed == FALSE)
    {
      /* this is tacky, too! */
      zMapMessage("You had unsaved changes! %s", "They have been lost!");
    }

  /* remove reference to this as the foocanvas still holds it and it
   * will clean it up reather than us. */
  editor_data->origFeature = NULL; 

  g_free(editor_data->table);
  g_free(editor_data->wcopyFeature);
  gtk_widget_destroy(GTK_WIDGET(editor_data->window));
  g_free(editor_data);
                                                           
  return;
}

static gboolean selectionFunc(GtkTreeSelection *selection, 
                              GtkTreeModel     *model,
                              GtkTreePath      *path, 
                              gboolean          path_currently_selected,
                              gpointer          user_data)
{
  
  return TRUE;
}
static GtkCellRenderer *getColRenderer(ZMapWindowEditor editor)
{
  GtkCellRenderer *renderer = NULL;
  GdkColor background;

  gdk_color_parse("WhiteSmoke", &background);

  /* make renderer */
  renderer = gtk_cell_renderer_text_new ();

  g_object_set (G_OBJECT (renderer),
                "foreground", "red",
                "background-gdk", &background,
                "editable", editor->editable,
                NULL);

  g_signal_connect (G_OBJECT (renderer), "edited",
                    G_CALLBACK(cellEditedCB), editor);

  return renderer;
}

/********************* end of file ********************************/
