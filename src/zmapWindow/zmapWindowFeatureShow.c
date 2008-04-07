/*  File: zmapWindowFeatureShow.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Description: Implements textual display of feature details in a
 *              gtk notebook widget.
 *              
 *              Intention is to have contents of notebook dynamically
 *              configurable by the controller application via our xremote 
 *              interface.
 *
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Apr  7 14:28 2008 (rds)
 * Created: Wed Jun  6 11:42:51 2007 (edgrif)
 * CVS info:   $Id: zmapWindowFeatureShow.c,v 1.15 2008-04-07 13:29:39 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapXMLHandler.h>
#include <zmapWindow_P.h>


/* THERE IS QUITE A BIT OF DEBUGGING CODE IN HERE WHICH ATTEMPTS TO GET THIS WIDGET
 * TO BE A GOOD SIZE...HOWEVER IT SEEMS A LOST CAUSE AS GTK SEEMS ACTIVELY TO
 * PREVENT THIS...SIGH.....WHEN I HAVE THE URGE I'LL HAVE ANOTHER GO... */


#define LIST_COLUMN_WIDTH 50
#define SHOW_COL_DATA_KEY   "column_decoding_data"
#define SHOW_COL_NUMBER_KEY "column_number_data"
/* Be nice to have a struct to reduce gObject set/get data calls to 1 */


#define SET_TEXT "TRUE"
#define NOT_SET_TEXT "<NOT SET>"


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

typedef enum 
  {
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

enum
  {
    EDIT_COL_NAME,                /*!< feature name column  */
    EDIT_COL_TYPE,                /*!< feature type column  */
    EDIT_COL_START,               /*!< feature start  */
    EDIT_COL_END,                 /*!< feature end  */
    EDIT_COL_STRAND,              /*!< feature strand  */
    EDIT_COL_PHASE,               /*!< feature phase  */
    EDIT_COL_SCORE,               /*!< feature score  */
    EDIT_COL_NUMBER               /*!< number of columns  */
  };


/* Types/strings etc. for XML version of notebook pages etc....
 * 
 * Some of these strings need to be kept in step with the FeatureBook types above.
 */

/* the xml tags */
#define XML_TAG_ZMAP      "zmap"
#define XML_TAG_RESPONSE  "response"
#define XML_TAG_ERROR     "error"
#define XML_TAG_NOTEBOOK  "notebook"
#define XML_TAG_CHAPTER   "chapter"
#define XML_TAG_PAGE      "page"
#define XML_TAG_SUBSECTION "subsection"
#define XML_TAG_PARAGRAPH "paragraph"
#define XML_TAG_TAGVALUE  "tagvalue"

/* paragraph type strings */
#define XML_PARAGRAPH_SIMPLE          "simple"
#define XML_PARAGRAPH_TAGVALUE_TABLE  "tagvalue_table"
#define XML_PARAGRAPH_HOMOGENOUS      "homogenous"
#define XML_PARAGRAPH_COMPOUND_TABLE  "compound_table"

/* tagvalue type strings */
#define XML_TAGVALUE_CHECKBOX      "checkbox"
#define XML_TAGVALUE_SIMPLE        "simple"
#define XML_TAGVALUE_COMPOUND      "compound"
#define XML_TAGVALUE_SCROLLED_TEXT "scrolled_text"



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
} mainTableStruct, *mainTable;



typedef struct ZMapWindowFeatureShowStruct_
{
  ZMapWindow zmapWindow ;

  gboolean reusable ;					    /* Can this window be reused for a new feature. */

  FooCanvasItem *item;					    /* The item the user called us on  */
  ZMapFeature    origFeature;				    /* Feature from item. */


  ZMapGuiNotebook feature_book ;


  /* State while parsing an xml spec of a notebook. */
  gboolean xml_parsing_status ;
  ZMapGuiNotebookType xml_curr_tag ;

  ZMapGuiNotebook xml_curr_notebook ;
  ZMapGuiNotebookChapter xml_curr_chapter ;
  ZMapGuiNotebookPage xml_curr_page ;
  ZMapGuiNotebookSubsection xml_curr_subsection ;
  ZMapGuiNotebookParagraph xml_curr_paragraph ;
  ZMapGuiNotebookTagValue xml_curr_tagvalue ;

  char *xml_curr_tagvalue_name ;
  ZMapGuiNotebookTagValueDisplayType xml_curr_type ;


  /* State while parsing a notebook. */
  ZMapGuiNotebookParagraph curr_paragraph ;


  /* dialog widgets */
  GtkWidget *window ;
  GtkWidget *vbox ;
  GtkWidget *notebook ;

  GtkWidget *curr_notebook ;
  GtkWidget *curr_page_vbox ;
  GtkWidget *curr_paragraph_vbox ;
  GtkWidget *curr_paragraph_table ;
  guint curr_paragraph_rows, curr_paragraph_columns ;

} ZMapWindowFeatureShowStruct ;



/* Used from a size event handler to try and get scrolled window to be a reasonable size
 * compared to its children. */
typedef struct
{
  GtkWidget *scrolled_window ;
  GtkWidget *tree_view ;
  int init_width, init_height ;
  int curr_width, curr_height ;
} TreeViewSizeCBDataStruct, *TreeViewSizeCBData ;


static ZMapWindowFeatureShow featureShowCreate(ZMapWindow window, FooCanvasItem *item) ;
static void featureShowReset(ZMapWindowFeatureShow show, ZMapWindow window, char *title) ;
static ZMapGuiNotebook createFeatureBook(ZMapWindowFeatureShow show, char *name,
					 ZMapFeature feature, FooCanvasItem *item) ;

/* xml event callbacks */
static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement element, 
                                  ZMapXMLParser parser);
static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser);
static gboolean xml_notebook_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser);
static gboolean xml_chapter_start_cb(gpointer user_data, ZMapXMLElement element, 
				     ZMapXMLParser parser) ;
static gboolean xml_page_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser) ;
static gboolean xml_subsection_start_cb(gpointer user_data, ZMapXMLElement element, 
					ZMapXMLParser parser) ;
static gboolean xml_paragraph_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser) ;
static gboolean xml_tagvalue_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser) ;
static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, 
                                 ZMapXMLParser parser) ;
static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser) ;
static gboolean xml_response_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser) ;
static gboolean xml_notebook_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser) ;
static gboolean xml_chapter_end_cb(gpointer user_data, ZMapXMLElement element, 
				   ZMapXMLParser parser) ;
static gboolean xml_page_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser) ;
static gboolean xml_subsection_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser) ;
static gboolean xml_paragraph_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser) ;
static gboolean xml_tagvalue_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser) ;
static void printWarning(char *element, char *handler) ;


static ZMapWindowFeatureShow showFeature(ZMapWindowFeatureShow reuse_window, ZMapWindow window, FooCanvasItem *item) ;
static void createEditWindow(ZMapWindowFeatureShow feature_show, char *title) ;
static GtkWidget *makeMenuBar(ZMapWindowFeatureShow feature_show) ;

static ZMapWindowFeatureShow findReusableShow(GPtrArray *window_list) ;
static gboolean windowIsReusable(void) ;
static ZMapFeature getFeature(FooCanvasItem *item) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static GtkTreeModel *makeTreeModel(FooCanvasItem *item) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static GtkWidget *addFeatureSection(GtkWidget *parent, ZMapWindowFeatureShow show) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void array2List(mainTable table, GArray *array, ZMapStyleMode feature_type);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void destroyCB(GtkWidget *widget, gpointer data);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean selectionFunc(GtkTreeSelection *selection, 
                              GtkTreeModel     *model,
                              GtkTreePath      *path, 
                              gboolean          path_currently_selected,
                              gpointer          user_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static GtkCellRenderer *getColRenderer(ZMapWindowFeatureShow show);

static void sizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;
static void ScrsizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;
static void sizeRequestCB(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data) ;
static void ScrsizeRequestCB(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void preserveCB(gpointer data, guint cb_action, GtkWidget *widget);
static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);

static void cleanUp(ZMapGuiNotebookAny any_section, void *user_data) ;

static void getAllMatches(ZMapWindow window,
			  ZMapFeature feature, FooCanvasItem *item, ZMapGuiNotebookSubsection subsection) ;
static void addTagValue(gpointer data, gpointer user_data) ;


/* menu GLOBAL! */
static GtkItemFactoryEntry menu_items_G[] =
  {
    /* File */
    { "/_File",              NULL,         NULL,       0,          "<Branch>", NULL},
    { "/File/Preserve",      NULL,         preserveCB, 0,          NULL,       NULL},
    { "/File/Close",         "<control>W", requestDestroyCB, 0,    NULL,       NULL},

    /* Help */
    { "/_Help",                NULL, NULL,       0,  "<LastBranch>", NULL},
    { "/Help/Feature Display", NULL, helpMenuCB, 1,  NULL,           NULL},
    { "/Help/About ZMap",      NULL, helpMenuCB, 2,  NULL,           NULL}
  } ;



static gboolean alert_client_debug_G = TRUE ;



/* MY GUT FEELING IS THAT ROB LEFT STUFF ALLOCATED AND DID NOT CLEAR UP SO I NEED TO CHECK UP ON
 * ALL THAT.... */



/* Create a feature display window, this function _always_ creates a new window. */
ZMapWindowFeatureShow zmapWindowFeatureShowCreate(ZMapWindow zmapWindow, FooCanvasItem *item)
{
  ZMapWindowFeatureShow show = NULL ;

  show = showFeature(NULL, zmapWindow, item) ;

  return show ;
}


/* Displays a feature in a window, will reuse an existing window if it can, otherwise
 * it creates a new one. */
ZMapWindowFeatureShow zmapWindowFeatureShow(ZMapWindow window, FooCanvasItem *item)
{
  ZMapWindowFeatureShow show = NULL ;
  ZMapFeature feature ;

  if ((feature = getFeature(item)) && zMapStyleIsTrueFeature(feature->style))
    {
      /* Look for a reusable window. */
      show = findReusableShow(window->feature_show_windows) ;

      /* now show the window, if we found a reusable one that will be reused. */
      show = showFeature(show, window, item) ;

      zMapGUIRaiseToTop(show->window);
    }

  return show ;
}



/* 
 *                   Internal routines.
 */


static ZMapWindowFeatureShow showFeature(ZMapWindowFeatureShow reuse_window, ZMapWindow window, FooCanvasItem *item)
{
  ZMapWindowFeatureShow show = reuse_window ;
  ZMapFeature feature ;

  if ((feature = getFeature(item)))
    {
      char *title ;
      char *feature_name ;

      
      feature_name = (char *)g_quark_to_string(feature->original_id) ;
      title = zMapGUIMakeTitleString("Feature Show", feature_name) ;

      if (!show)
	{
	  show = featureShowCreate(window, item) ;

	  createEditWindow(show, title) ;
	}
      else
	{
	  gtk_widget_destroy(show->notebook) ;

	  /* other stuff needs clearing up here.... */
	  featureShowReset(show, window, title) ;
	}

      g_free(title) ;

      show->item = item ;
      show->origFeature = feature ;
      
      /* Make the notebook. */
      show->feature_book = createFeatureBook(show, feature_name, feature, item) ;
      

      /* Now display the pages..... */
      show->notebook = zMapGUINotebookCreateWidget(show->feature_book) ;

      gtk_box_pack_start(GTK_BOX(show->vbox), show->notebook, TRUE, TRUE, 0) ;

      gtk_widget_show_all(show->window) ;

      /* Now free the feature_book..not wanted after display ?? */
      zMapGUINotebookDestroyNotebook(show->feature_book) ;
    }

  return show ;
}


static ZMapFeature getFeature(FooCanvasItem *item)
{
  ZMapFeature feature = NULL ;
  ZMapStyleMode type ;

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(type == ITEM_FEATURE_SIMPLE || type == ITEM_FEATURE_PARENT || type == ITEM_FEATURE_CHILD) ;

  return feature ;
}


static ZMapWindowFeatureShow featureShowCreate(ZMapWindow window, FooCanvasItem *item)
{
  ZMapWindowFeatureShow show = NULL ;

  show = g_new0(ZMapWindowFeatureShowStruct, 1) ;
  show->reusable = windowIsReusable() ;
  show->zmapWindow = window ;

  return show ;
}


static void featureShowReset(ZMapWindowFeatureShow show, ZMapWindow window, char *title)
{
  show->reusable = windowIsReusable() ;
  show->zmapWindow = window ;

  show->curr_paragraph = NULL ;
  show->notebook = show->curr_notebook = show->curr_page_vbox = show->curr_paragraph_vbox = NULL ;
  show->curr_paragraph_rows = show->curr_paragraph_columns = 0 ;

  show->item = NULL ;
  show->origFeature = NULL ;

  gtk_window_set_title(GTK_WINDOW(show->window), title) ;

  return ;
}



/* Hard coded for now...will be settable from config file. */
static gboolean windowIsReusable(void)
{
  gboolean reusable = TRUE ;

  return reusable ;
}



/* Parse a feature into a text version in a notebook structure.
 * 
 * 
 *  */
static ZMapGuiNotebook createFeatureBook(ZMapWindowFeatureShow show, char *name,
					 ZMapFeature feature, FooCanvasItem *item)
{
  ZMapGuiNotebook feature_book = NULL ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapGuiNotebookCBStruct call_backs ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  ZMapGuiNotebookChapter dummy_chapter ;
  ZMapGuiNotebookPage page ;
  ZMapGuiNotebookSubsection subsection ;
  ZMapGuiNotebookParagraph paragraph ;
  ZMapGuiNotebookTagValue tag_value ;
  char *chapter_title, *page_title, *description ;
  char *tmp ;
  char *notes ;

  feature_book = zMapGUINotebookCreateNotebook(name, FALSE, cleanUp, NULL) ;


  /* The feature fundamentals page. */
  switch(feature->type)
    {
    case ZMAPSTYLE_MODE_BASIC:
      {
	chapter_title = "Feature" ;
	page_title = "Details" ;

	break ;
      }
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      {
	chapter_title = "Transcript" ;
	page_title = "Details" ;

	break ;
      }
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
	chapter_title = "Alignment" ;
	page_title = "Details" ;

	break ;
      }
    case ZMAPSTYLE_MODE_RAW_SEQUENCE:
      {
	chapter_title = "DNA Sequence" ;
	page_title = "Details" ;

	break ;
      }
    case ZMAPSTYLE_MODE_PEP_SEQUENCE:
      {
	chapter_title = "Peptide Sequence" ;
	page_title = "Details" ;

	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
	break ;
      }
    }


  dummy_chapter = zMapGUINotebookCreateChapter(feature_book, chapter_title, NULL) ;


  page = zMapGUINotebookCreatePage(dummy_chapter, page_title) ;


  subsection = zMapGUINotebookCreateSubsection(page, "Feature") ;


  /* General Feature Descriptions. */
  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
					     ZMAPGUI_NOTEBOOK_PARAGRAPH_SIMPLE, NULL, NULL) ;

  tag_value = zMapGUINotebookCreateTagValue(paragraph, "Feature Name",
					    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					    "string", g_strdup(g_quark_to_string(feature->original_id)),
					    NULL) ;

  tag_value = zMapGUINotebookCreateTagValue(paragraph, "Feature Group",
					    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					    "string", g_strdup(zMapStyleGetName(feature->style)), NULL) ;

  if ((description = zMapStyleGetDescription(feature->style)))
    {
      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Description",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SCROLLED_TEXT,
						"string", g_strdup(description), NULL) ;
    }


  if ((notes = zmapWindowFeatureDescription(feature)))
    {
      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Notes",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SCROLLED_TEXT,
						"string", g_strdup(notes), NULL) ;
    }


  /* Feature specific stuff. */
  if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
    {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      char *query_phase ;				    /* Not used...yet... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      char *query_length ;

      subsection = zMapGUINotebookCreateSubsection(page, "Align") ;


      paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
						 ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE, NULL, NULL) ;

      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Align Type",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
						"string", g_strdup(zMapFeatureHomol2Str(feature->feature.homol.type)),
						NULL) ;

      if (feature->feature.homol.length)
	query_length = g_strdup_printf("%d", feature->feature.homol.length) ;
      else
	query_length = g_strdup("<NOT SET>") ;
      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Query length",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
						"string", query_length,
						NULL) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I'm not sure we can set this meaningfully at the moment.... */
      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Query phase",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
						"string",
						g_strdup(zMapFeaturePhase2Str(feature->feature.homol.target_phase)),
						NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      
      /* Get a list of all the matches for this sequence.... */
      getAllMatches(show->zmapWindow, feature, item, subsection) ;
    }
  else if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      paragraph = zMapGUINotebookCreateParagraph(subsection, "Properties",
						 ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE, NULL, NULL) ;
      
      if (feature->feature.transcript.flags.cds)
	tmp = g_strdup_printf("%d %d", feature->feature.transcript.cds_start, feature->feature.transcript.cds_end) ;
      else
	tmp = g_strdup_printf("%s", NOT_SET_TEXT) ;
      
      tag_value = zMapGUINotebookCreateTagValue(paragraph, "CDS",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
						"string",
						tmp,
						NULL) ;

      if (feature->feature.transcript.flags.start_not_found)
	tmp = g_strdup_printf("%s", zMapFeaturePhase2Str(feature->feature.transcript.start_phase)) ;
      else
	tmp = g_strdup_printf("%s", NOT_SET_TEXT) ;

      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Start Not Found",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
						"string",
						tmp,
						NULL) ;

      if (feature->feature.transcript.flags.end_not_found)
	tmp = g_strdup_printf("%s", SET_TEXT) ;
      else
	tmp = g_strdup_printf("%s", NOT_SET_TEXT) ;

      tag_value = zMapGUINotebookCreateTagValue(paragraph, "End Not Found",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
						"string",
						tmp,
						NULL) ;
    }




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* This is dummied up to simulate information from otter....this stuff is only appropriate for
   * transcripts.... */
  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      subsection = zMapGUINotebookCreateSubsection(page, "Locus") ;

      paragraph = zMapGUINotebookCreateParagraph(subsection, NULL, ZMAPGUI_NOTEBOOK_PARAGRAPH_SIMPLE, NULL, NULL) ;
      
      if (feature->locus_id)
	tmp = g_strdup_printf("%s", g_strdup(g_quark_to_string(feature->locus_id))) ;
      else
	tmp = g_strdup_printf("%s", NOT_SET_TEXT) ;

      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Symbol",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
						"string",
						tmp,
						NULL) ;

      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Full Name",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
						"string",
						"A really fake name...",
						NULL) ;


      /* This is dummied up to simulate information from otter.... */
      subsection = zMapGUINotebookCreateSubsection(page, "Annotation") ;

      paragraph = zMapGUINotebookCreateParagraph(subsection, "Remark", ZMAPGUI_NOTEBOOK_PARAGRAPH_SIMPLE, NULL, NULL) ;

      tag_value = zMapGUINotebookCreateTagValue(paragraph, NULL,
						ZMAPGUI_NOTEBOOK_TAGVALUE_SCROLLED_TEXT,
						"string", g_strdup("Some old load of junk...."), NULL) ;

      paragraph = zMapGUINotebookCreateParagraph(subsection, NULL, ZMAPGUI_NOTEBOOK_PARAGRAPH_HOMOGENOUS, NULL, NULL) ;

      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Protein Match",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
						"string", g_strdup("Great Match !"), NULL) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* If we have an external program driving us then ask it for any extra information.
   * This will come as xml which we decode via the callbacks listed in the following structs. */
  {
    ZMapXMLObjTagFunctionsStruct starts[] =
      {
	{XML_TAG_ZMAP, xml_zmap_start_cb     },
	{XML_TAG_RESPONSE, xml_response_start_cb },
	{XML_TAG_NOTEBOOK, xml_notebook_start_cb },
	{XML_TAG_CHAPTER, xml_chapter_start_cb },
	{XML_TAG_PAGE, xml_page_start_cb },
	{XML_TAG_PARAGRAPH, xml_paragraph_start_cb },
	{XML_TAG_SUBSECTION, xml_subsection_start_cb },
	{XML_TAG_TAGVALUE, xml_tagvalue_start_cb },
	{NULL, NULL}
      } ;
    ZMapXMLObjTagFunctionsStruct ends[] =
      {
	{XML_TAG_ZMAP,  xml_zmap_end_cb  },
	{XML_TAG_RESPONSE, xml_response_end_cb },
	{XML_TAG_NOTEBOOK, xml_notebook_end_cb },
	{XML_TAG_PAGE, xml_page_end_cb },
	{XML_TAG_CHAPTER, xml_chapter_end_cb },
	{XML_TAG_SUBSECTION, xml_subsection_end_cb },
	{XML_TAG_PARAGRAPH, xml_paragraph_end_cb },
	{XML_TAG_TAGVALUE, xml_tagvalue_end_cb },
	{XML_TAG_ERROR, xml_error_end_cb },
	{NULL, NULL}
      } ;


    show->xml_parsing_status = TRUE ;
    show->xml_curr_tag = ZMAPGUI_NOTEBOOK_INVALID ;
    show->xml_curr_chapter = NULL ;
    show->xml_curr_page = NULL ;

    if (zmapWindowUpdateXRemoteDataFull(show->zmapWindow,
					(ZMapFeatureAny)feature,
					"feature_details",
					show->item,
					starts, ends, show))
      {
	/* If all went well and we got a valid xml format notebook back then
	 * merge into our existing notebook for the feature. */

	zMapGUINotebookMergeNotebooks(feature_book, show->xml_curr_notebook) ;
      }
    else
      {
	/* Clean up if something went wrong...  */
	if (show->xml_curr_chapter)
	  zMapGUINotebookDestroyAny((ZMapGuiNotebookAny)(show->xml_curr_notebook)) ;
      }
  }

  return feature_book ;
}





/* Hide this away to make the exposed function smaller... */
static void createEditWindow(ZMapWindowFeatureShow feature_show, char *title)
{
  GtkWidget *vbox ;

  /* Create the edit window. */
  feature_show->window = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  g_object_set_data(G_OBJECT(feature_show->window), "zmap_feature_show", feature_show) ;
  gtk_window_set_title(GTK_WINDOW(feature_show->window), title) ;
  gtk_container_set_border_width(GTK_CONTAINER (feature_show->window), 10);
  g_signal_connect(G_OBJECT (feature_show->window), "destroy",
		   G_CALLBACK (destroyCB), feature_show);
  gtk_window_set_default_size(GTK_WINDOW(feature_show->window), 500, -1) ; /* Stop window being too squashed. */

  /* Add ptrs so parent knows about us */
  g_ptr_array_add(feature_show->zmapWindow->feature_show_windows, (gpointer)(feature_show->window)) ;


  feature_show->vbox = vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_box_set_spacing(GTK_BOX(vbox), 5) ;
  gtk_container_add(GTK_CONTAINER(feature_show->window), vbox) ;

  gtk_box_pack_start(GTK_BOX(vbox), makeMenuBar(feature_show), FALSE, FALSE, 0);

  return ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GtkWidget *addFeatureSection(GtkWidget *parent, ZMapWindowFeatureShow show)
{
  GtkWidget *feature_widget = NULL ;
  GtkTreeModel *treeModel = NULL ;
  zmapWindowFeatureListCallbacksStruct windowCallbacks = {NULL} ;

  treeModel = makeTreeModel(show->item) ;

  windowCallbacks.selectionFuncCB = selectionFunc;

  feature_widget = zmapWindowFeatureListCreateView(ZMAPWINDOWLIST_FEATURE_TREE, treeModel, 
						   getColRenderer(show),
						   &windowCallbacks, 
						   show) ;

  return feature_widget ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static GtkTreeModel *makeTreeModel(FooCanvasItem *item)
{
  GtkTreeModel *tree_model = NULL ;
  GList *itemList         = NULL;

  tree_model = zmapWindowFeatureListCreateStore(ZMAPWINDOWLIST_FEATURE_TREE);

  itemList  = g_list_append(itemList, item);

  zmapWindowFeatureListPopulateStoreList(tree_model, ZMAPWINDOWLIST_FEATURE_TREE, itemList, NULL) ;

  return tree_model ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




static void destroyCB(GtkWidget *widget, gpointer data)
{
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)data ;
    
  g_ptr_array_remove(feature_show->zmapWindow->feature_show_windows, (gpointer)feature_show->window);

  g_free(feature_show) ;
                                                           
  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean selectionFunc(GtkTreeSelection *selection, 
                              GtkTreeModel     *model,
                              GtkTreePath      *path, 
                              gboolean          path_currently_selected,
                              gpointer          user_data)
{
  return TRUE;
}


static GtkCellRenderer *getColRenderer(ZMapWindowFeatureShow show)
{
  GtkCellRenderer *renderer = NULL;
  GdkColor background;

  gdk_color_parse("WhiteSmoke", &background);

  /* make renderer */
  renderer = gtk_cell_renderer_text_new ();

  g_object_set (G_OBJECT (renderer),
                "foreground", "red",
                "background-gdk", &background,
                "editable", FALSE,
                NULL);

  return renderer;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* widget is the treeview */
static void sizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data_unused)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (GTK_WIDGET_REALIZED(widget))
    {
      printf("TreeView: sizeAllocateCB: x: %d, y: %d, height: %d, width: %d\n", 
	     alloc->x, alloc->y, alloc->height, alloc->width); 

    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}

/* widget is the treeview */
static void ScrsizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data_unused)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (GTK_WIDGET_REALIZED(widget))
    {
      printf("ScrWin: sizeAllocateCB: x: %d, y: %d, height: %d, width: %d\n", 
	     alloc->x, alloc->y, alloc->height, alloc->width); 
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}



/* widget is the treeview */
static void sizeRequestCB(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data)
{
  enum {SCROLL_BAR_WIDTH = 20} ;			    /* Hack... */
  GtkWidget *scrwin ;
  TreeViewSizeCBData size_data = (TreeViewSizeCBData)user_data ;
  int width, height ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("TreeView: sizeRequestCB:  height: %d, width: %d\n", 
	 requisition->height, requisition->width); 
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (!size_data->init_width)
    {
      size_data->curr_width = size_data->init_width = requisition->width + SCROLL_BAR_WIDTH ;
      size_data->curr_height = size_data->init_height = requisition->height ;

      width = size_data->curr_width ;
      height = size_data->curr_height ;
    }
  else
    {
      width = height = -1 ;

      if (requisition->width != size_data->curr_width)
	{
	  if (requisition->width < size_data->init_width)
	    width = size_data->init_width ;
	  else
	    width = requisition->width ;
	}

      if (width != -1)
	size_data->curr_width = width ;

      if (requisition->height != size_data->curr_height)
	{
	  if (requisition->height < size_data->init_height)
	    height = size_data->init_height ;
	  else
	    {
	      /* If we haven't yet been realised then we clamp our height as stupid
	       * scrolled window wants to make us too big by the height of its horizontal
	       * scroll bar...sigh... */
	      if (!GTK_WIDGET_REALIZED(widget))
		height = size_data->init_height ;
	      else
		height = requisition->height ;
	    }

	  if (height > 800)
	    height = 800 ;
	}

      if (height != -1)
	size_data->curr_height = height ;
    }

  if (!(width == -1 && height == -1))
    {
      scrwin = gtk_widget_get_parent(widget) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      gtk_widget_set_size_request(widget, width, height) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      gtk_widget_set_size_request(scrwin, width, height) ;
    }

  return ;
}


/* widget is the treeview */
static void ScrsizeRequestCB(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data_unused)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (GTK_WIDGET_REALIZED(widget))
    {
      printf("ScrWin: sizeRequestCB:  height: %d, width: %d\n", 
	     requisition->height, requisition->width); 
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* make the menu from the global defined above !
 */

static GtkWidget *makeMenuBar(ZMapWindowFeatureShow feature_show)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkWidget *menubar ;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel_group = gtk_accel_group_new ();

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group );

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items_G, (gpointer)feature_show) ;

  gtk_window_add_accel_group(GTK_WINDOW(feature_show->window), accel_group) ;

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  return menubar ;
}



/* Stop this windows contents being overwritten with a new feature. */
static void preserveCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)data ;

  show->reusable = FALSE ;

  return ;
}


/* Request destroy of list window, ends up with gtk calling destroyCB(). */
static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)data;

  gtk_widget_destroy(GTK_WIDGET(feature_show->window)) ;

  return ;
}



static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)data ;
  GtkWidget *window ;

  window = feature_show->window ;

  switch(cb_action)
    {
    case 1:
      zMapShowMsg(ZMAP_MSG_WARNING, "No help yet.\n") ;
      break ;
    case 2:
      zMapGUIShowAbout() ;
      break ;
    default:
      zMapAssertNotReached() ;
      break;
    }

  return ;
}


static void cleanUp(ZMapGuiNotebookAny any_section, void *user_data)
{



  return ;
}



/* Find a show window in the list of show windows currently displayed that
 * is marked as reusable. */
static ZMapWindowFeatureShow findReusableShow(GPtrArray *window_list)
{
  ZMapWindowFeatureShow reusable_window = NULL ;
  int i ;

  if (window_list && window_list->len)
    {
      for (i = 0 ; i < window_list->len ; i++)
	{
	  GtkWidget *show_widg ;
	  ZMapWindowFeatureShow show ;

	  show_widg = (GtkWidget *)g_ptr_array_index(window_list, i) ;
	  show = g_object_get_data(G_OBJECT(show_widg), "zmap_feature_show") ;
	  if (show->reusable)
	    {
	      reusable_window = show ;
	      break ;
	    }
	}
    }

  return reusable_window ;
}



/* 
 * Following routines are callbacks registered to parse an xml spec for
 * a notebook page.
 * 
 */


static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement element, 
                                  ZMapXMLParser parser)
{
  printWarning("zmap", "start") ;

  return TRUE ;
}

static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser)
{
  printWarning("zmap", "end") ;

  return TRUE;
}




static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapXMLTagHandler message = wrapper->tag_handler;
  ZMapXMLAttribute handled_attribute = NULL ;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);

  printWarning("response", "start") ;

  if ((handled_attribute = zMapXMLElementGetAttributeByName(element, "handled"))
      && (message->handled == FALSE))
    {
      message->handled = zMapXMLAttributeValueToBool(handled_attribute);

      show->xml_curr_tag = ZMAPGUI_NOTEBOOK_INVALID ;
    }

  return TRUE;
}

static gboolean xml_response_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  printWarning("response", "end") ;

  return TRUE;
}




static gboolean xml_notebook_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow       show = (ZMapWindowFeatureShow)(wrapper->user_data) ;

  printWarning("notebook", "start") ;

  if (wrapper->tag_handler->handled)
    {
      if (show->xml_curr_tag != ZMAPGUI_NOTEBOOK_INVALID)
        {
          zMapXMLParserRaiseParsingError(parser, "Bad xml, response tag not set.");
        }
      else
        {
	  char *notebook_name = NULL ;
	  ZMapXMLAttribute attr = NULL ;

	  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_BOOK ;

          if ((attr = zMapXMLElementGetAttributeByName(element, "name")))
            {
              notebook_name = g_strdup(zMapXMLAttributeValueToStr(attr)) ;
	    }

          show->xml_curr_notebook = zMapGUINotebookCreateNotebook(notebook_name, FALSE, NULL, NULL) ;
        }
    }

  return TRUE;
}

static gboolean xml_notebook_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);

  printWarning("notebook", "end") ;

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_INVALID ;

  return TRUE;
}



static gboolean xml_chapter_start_cb(gpointer user_data, ZMapXMLElement element, 
				     ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data) ;

  printWarning("chapter", "start") ;

  if (wrapper->tag_handler->handled)
    {
      if (show->xml_curr_tag != ZMAPGUI_NOTEBOOK_BOOK)
        {
          zMapXMLParserRaiseParsingError(parser, "Bad xml, response tag not set.");
        }
      else
        {
	  ZMapXMLAttribute attr = NULL ;
	  char *chapter_name = NULL ;

          show->xml_curr_tag = ZMAPGUI_NOTEBOOK_CHAPTER ;

          if ((attr = zMapXMLElementGetAttributeByName(element, "name")))
            {
              chapter_name = g_strdup(zMapXMLAttributeValueToStr(attr)) ;
	    }
              
	  show->xml_curr_chapter = zMapGUINotebookCreateChapter(show->xml_curr_notebook, chapter_name, NULL) ;
        }
    }

  return TRUE;
}

static gboolean xml_chapter_end_cb(gpointer user_data, ZMapXMLElement element, 
				   ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);

  printWarning("chapter", "end") ;

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_BOOK ;

  return TRUE;
}




static gboolean xml_page_start_cb(gpointer user_data, ZMapXMLElement element, 
				  ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);
  ZMapXMLAttribute attr = NULL ;
  char *page_name = NULL ;

  printWarning("page", "start") ;

  if (wrapper->tag_handler->handled)
    {
      if (show->xml_curr_tag != ZMAPGUI_NOTEBOOK_CHAPTER)
        {
          zMapXMLParserRaiseParsingError(parser, "Bad xml, notebook tag not set.");
        }
      else
        {
          show->xml_curr_tag = ZMAPGUI_NOTEBOOK_PAGE ;
          
          if ((attr = zMapXMLElementGetAttributeByName(element, "name")))
            {
              page_name = g_strdup(zMapXMLAttributeValueToStr(attr)) ;
              
              show->xml_curr_page = zMapGUINotebookCreatePage(show->xml_curr_chapter, page_name) ;
            }
          else
            zMapXMLParserRaiseParsingError(parser, "name is a required attribute for page tag.");
        }
    }

  return TRUE;
}

static gboolean xml_page_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);

  printWarning("page", "end") ;

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_CHAPTER ;

  return TRUE;
}


static gboolean xml_subsection_start_cb(gpointer user_data, ZMapXMLElement element, 
					ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);
  ZMapXMLAttribute attr = NULL ;
  char *subsection_name = NULL ;

  printWarning("subsection", "start") ;

  if (wrapper->tag_handler->handled)
    {
      if (show->xml_curr_tag != ZMAPGUI_NOTEBOOK_PAGE)
        {
          zMapXMLParserRaiseParsingError(parser, "Bad xml, notebook tag not set.");
        }
      else
        {
          show->xml_curr_tag = ZMAPGUI_NOTEBOOK_SUBSECTION ;
          
          if ((attr = zMapXMLElementGetAttributeByName(element, "name")))
            {
              subsection_name = g_strdup(zMapXMLAttributeValueToStr(attr)) ;
            }

	  show->xml_curr_subsection = zMapGUINotebookCreateSubsection(show->xml_curr_page, subsection_name) ;
        }
    }

  return TRUE ;
}

static gboolean xml_subsection_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);

  printWarning("subsection", "end") ;

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_PAGE ;

  return TRUE;
}




static gboolean xml_paragraph_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);
  ZMapXMLAttribute attr = NULL ;

  printWarning("paragraph", "start") ;

  if(!(wrapper->tag_handler->handled))
    return TRUE;

  if (show->xml_curr_tag != ZMAPGUI_NOTEBOOK_SUBSECTION)
    {
      zMapXMLParserRaiseParsingError(parser, "Bad xml, page tag not set.");
    }
  else
    {
      gboolean status = TRUE ;
      char *paragraph_name = NULL ;
      char *paragraph_type = NULL ;
      ZMapGuiNotebookParagraphDisplayType type = ZMAPGUI_NOTEBOOK_PARAGRAPH_INVALID ;
      GList *headers = NULL, *types = NULL ;

      show->xml_curr_tag = ZMAPGUI_NOTEBOOK_PARAGRAPH ;

      if ((attr = zMapXMLElementGetAttributeByName(element, "name")))
	{
	  paragraph_name = g_strdup(zMapXMLAttributeValueToStr(attr)) ;
	}

      if ((attr = zMapXMLElementGetAttributeByName(element, "type")))
	{
	  paragraph_type = zMapXMLAttributeValueToStr(attr) ;

	  if (strcmp(paragraph_type, XML_PARAGRAPH_SIMPLE) == 0)
	    type = ZMAPGUI_NOTEBOOK_PARAGRAPH_SIMPLE ;
	  else if (strcmp(paragraph_type, XML_PARAGRAPH_TAGVALUE_TABLE) == 0)
	    type = ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE ;
	  else if (strcmp(paragraph_type, XML_PARAGRAPH_HOMOGENOUS) == 0)
	    type = ZMAPGUI_NOTEBOOK_PARAGRAPH_HOMOGENOUS ;
	  else if (strcmp(paragraph_type, XML_PARAGRAPH_COMPOUND_TABLE) == 0)
	    type = ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE ;
	  else
	    {
	      zMapXMLParserRaiseParsingError(parser, "Bad paragraph type attribute.") ;
	      status = FALSE ;
	    }
	}

      if (status)
	{
	  /* Get the column titles, should be a string of a form something like
	   * "'Start' 'End' 'whatever'". */

	  if (type != ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE)
	    {
	      headers = NULL ;
	      types = NULL ;
	    }
	  else
	    {
	      if (!(attr = zMapXMLElementGetAttributeByName(element, "columns")))
		{
		  zMapXMLParserRaiseParsingError(parser,
						 "Paragraph type \"compound_table\" requires \"columns\" attribute.") ;
		  status = FALSE ;
		}
	      else
		{
		  char *columns, *target ;
		  gboolean found = TRUE ;

		  target = columns = zMapXMLAttributeValueToStr(attr) ;

		  /* We need to strtok out the titles..... */
		  do
		    {
		      char *new_col ;

		      if ((new_col = strtok(target, "'")))
			{

			  /* HACK TO CHECK strtok'd THING IS NOT BLANK....remove when xml fixed... */
			  if (*new_col == ' ')
			    continue ;

			  headers = g_list_append(headers,
						  GINT_TO_POINTER(g_quark_from_string(new_col))) ;
			  target = NULL ;
			}
		      else
			found = FALSE ;

		    } while (found) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		  /* Would like to use this but it doesn't arrive until glib 2.14 */

		  GRegex *reg_ex ;
		  GMatchInfo *match_info;
		  GError *g_error ;

		  if ((reg_ex = g_regex_new("m/\'([^\']*)\'/",
					    G_REGEX_RAW,
					    G_REGEX_MATCH_NOTEMPTY,
					    &g_error)))
		    {
		      g_regex_match(reg_ex, columns, 0, &match_info) ;

		      while (g_match_info_matches(match_info))
			{
			  gchar *word = g_match_info_fetch(match_info, 0) ;

			  g_print("Found: %s\n", word) ;

			  g_free(word) ;

			  g_match_info_next(match_info, NULL) ;
			}

		      g_match_info_free(match_info) ;
		      g_regex_unref(reg_ex) ;
		    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
		}

	      if (status)
		{
		  /* Now get the column types which should be a list of the form something
		   * like "int int string". */

		  if (!(attr = zMapXMLElementGetAttributeByName(element, "column_types")))
		    {
		      zMapXMLParserRaiseParsingError(parser,
						     "Paragraph type \"compound_table\" requires \"column_types\" attribute.") ;
		      status = FALSE ;
		    }
		  else
		    {
		      gboolean found = TRUE ;
		      char *target ;
		      GList *column_data = NULL ;
		      
		      target = zMapXMLAttributeValueToStr(attr) ;

		      do
			{
			  char *new_col ;

			  if ((new_col = strtok(target, " ")))
			    {
			      column_data = g_list_append(column_data,
							  GINT_TO_POINTER(g_quark_from_string(new_col))) ;
			      target = NULL ;
			    }
			  else
			    found = FALSE ;

			} while (found) ;
	  
		      types = column_data ;

		    }

		}
	    }
	}

      if (status)
	{
	  show->xml_curr_paragraph = zMapGUINotebookCreateParagraph(show->xml_curr_subsection,
								    paragraph_name,
								    type, headers, types) ;
	}
    }

  return TRUE ;
}

static gboolean xml_paragraph_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);

  printWarning("paragraph", "end") ;

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_SUBSECTION ;

  return TRUE;
}



static gboolean xml_tagvalue_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);
  ZMapXMLAttribute attr = NULL ;

  printWarning("tagvalue", "start") ;

  if(!(wrapper->tag_handler->handled))
    return TRUE;

  if (show->xml_curr_tag != ZMAPGUI_NOTEBOOK_PARAGRAPH)
    {
      zMapXMLParserRaiseParsingError(parser, "Bad xml, paragraph tag not set.");
    }
  else
    {
      gboolean status = TRUE ;
      char *tagvalue_type = NULL ;
      gboolean has_name = TRUE ;

      show->xml_curr_tag = ZMAPGUI_NOTEBOOK_TAGVALUE ;

      if ((attr = zMapXMLElementGetAttributeByName(element, "name")))
	{
	  show->xml_curr_tagvalue_name = g_strdup(zMapXMLAttributeValueToStr(attr)) ;
	}
      else
	{
	  has_name = FALSE ;
	}

      if (status && (attr = zMapXMLElementGetAttributeByName(element, "type")))
	{
	  tagvalue_type = zMapXMLAttributeValueToStr(attr) ;

	  if ((strcmp(tagvalue_type, XML_TAGVALUE_COMPOUND) != 0) && !has_name)
	    {
	      zMapXMLParserRaiseParsingError(parser, "name is a required attribute for tagvalue.") ;
	      status = FALSE ;
	    }
	  else
	    {
	      if (strcmp(tagvalue_type, XML_TAGVALUE_CHECKBOX) == 0)
		show->xml_curr_type = ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX ;
	      else if (strcmp(tagvalue_type, XML_TAGVALUE_SIMPLE) == 0)
		show->xml_curr_type = ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE ;
	      else if (strcmp(tagvalue_type, XML_TAGVALUE_COMPOUND) == 0)
		show->xml_curr_type = ZMAPGUI_NOTEBOOK_TAGVALUE_COMPOUND ;
	      else if (strcmp(tagvalue_type, XML_TAGVALUE_SCROLLED_TEXT) == 0)
		show->xml_curr_type = ZMAPGUI_NOTEBOOK_TAGVALUE_SCROLLED_TEXT ;
	      else
		{
		  zMapXMLParserRaiseParsingError(parser, "Bad tagvalue type attribute.") ;
		  status = FALSE ;
		}
	    }
	}

    }

  return TRUE;
}

static gboolean xml_tagvalue_end_cb(gpointer user_data, ZMapXMLElement element, 
				    ZMapXMLParser parser)
{
  gboolean status = TRUE ;
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(wrapper->user_data);
  char *content ;

  printWarning("tagvalue", "end") ;

  if (!(wrapper->tag_handler->handled))
    return status ;

  if ((content = zMapXMLElementStealContent(element)))
    {
      GList *type ;
      if (show->xml_curr_type != ZMAPGUI_NOTEBOOK_TAGVALUE_COMPOUND)
	{
	  show->xml_curr_tagvalue = zMapGUINotebookCreateTagValue(show->xml_curr_paragraph,
								  show->xml_curr_tagvalue_name, show->xml_curr_type,
								  "string", content,
								  NULL) ;
	}
      else if((type = g_list_first(show->xml_curr_paragraph->compound_types)))
	{
	  gboolean found = TRUE ;
	  char *target = content ;
	  GList *column_data = NULL ;

	  /* Make a list of the names of the columns. */
 
	  do
	    {
	      char *new_col ;

	      if ((new_col = strtok(target, " ")))
		{
		  if (GPOINTER_TO_INT(type->data) == g_quark_from_string("bool"))
		    {
		      gboolean tmp = FALSE ;
		      
		      if ((status = zMapStr2Bool(new_col, &tmp)))
			{
			  column_data = g_list_append(column_data, GINT_TO_POINTER(tmp)) ;
			  status = TRUE ;
			}
		      else
			{
			  status = FALSE ;
			  found = FALSE ;
			  zMapLogWarning("Invalid boolean value: %s", new_col) ;
			}
		    }
		  else if (GPOINTER_TO_INT(type->data) == g_quark_from_string("int"))
		    {
		      int tmp = 0 ;

		      if ((status = zMapStr2Int(new_col, &tmp)))
			{
			  column_data = g_list_append(column_data, GINT_TO_POINTER(tmp)) ;
			  status = TRUE ;
			}
		      else
			{
			  zMapLogWarning("Invalid integer number: %s", new_col) ;
			}
		    }
		  else if (GPOINTER_TO_INT(type->data) == g_quark_from_string("float"))
		    {
		      float tmp = 0.0 ;

		      if ((status = zMapStr2Float(new_col, &tmp)))
			{
			  int int_tmp ;

			  /* Hack: we assume size(float) <= size(int) && size(float) == 4 and load the float into an int. */
			  memcpy(&int_tmp, &(tmp), 4) ;
			  column_data = g_list_append(column_data, GINT_TO_POINTER(int_tmp)) ;

			  status = TRUE ;
			}
		      else
			{
			  zMapLogWarning("Invalid float number: %s", new_col) ;
			}
		    }
		  else if (GPOINTER_TO_INT(type->data) == g_quark_from_string("string"))
		    {
		      /* Hardly worth checking but better than nothing ? */
		      if (new_col && *new_col)
			{
			  column_data = g_list_append(column_data, g_strdup(new_col)) ;
			  status = TRUE ;
			}
		      else
			{
			  zMapLogWarning("Invalid string: %s", (new_col ? "No string" : "NULL string pointer")) ;
			}
		    }


		  type = g_list_next(type) ;

		  target = NULL ;
		}
	      else
		found = FALSE ;

	    } while (found) ;

	  show->xml_curr_tagvalue = zMapGUINotebookCreateTagValue(show->xml_curr_paragraph,
								  show->xml_curr_tagvalue_name, show->xml_curr_type,
								  "compound", column_data,
								  NULL) ;
	}
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "tagvalue element is empty.");
    }

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_PARAGRAPH ;

  return TRUE;
}




/* Cleans up any dangling page stuff. */
static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, 
                                 ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapXMLTagHandler message = (ZMapXMLTagHandler)(wrapper->tag_handler);
  ZMapXMLElement mess_element = NULL;

  if ((mess_element = zMapXMLElementGetChildByName(element, "message"))
      && !message->error_message && mess_element->contents->str)
    {
      message->error_message = g_strdup(mess_element->contents->str);
    }
  
  return TRUE;
}


static void printWarning(char *element, char *handler)
{
  if (alert_client_debug_G)
    zMapLogWarning("In zmap %s %s Handler.", element, handler) ;

  return ;
}


static void getAllMatches(ZMapWindow window,
			  ZMapFeature feature, FooCanvasItem *item, ZMapGuiNotebookSubsection subsection)
{
  GList *list = NULL;
  ZMapStrand set_strand ;
  ZMapFrame set_frame ;
  gboolean result ;

  result = zmapWindowItemGetStrandFrame(item, &set_strand, &set_frame) ;
  zMapAssert(result) ;

  if ((list = zmapWindowFToIFindSameNameItems(window->context_to_item,
					      zMapFeatureStrand2Str(set_strand), zMapFeatureFrame2Str(set_frame),
					      feature)))
    {
      ZMapGuiNotebookParagraph paragraph ;
      GList *headers = NULL, *types = NULL ;

      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Clone/Sequence"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Sequence/Match Strand"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Start"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("End"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Query Start"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Query End"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Score"))) ;

      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("string"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("string"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("int"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("int"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("int"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("int"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("float"))) ;


      paragraph = zMapGUINotebookCreateParagraph(subsection, "Matches",
						 ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE, headers, types) ;

      g_list_foreach(list, addTagValue, paragraph) ;
    }

  return ;
}

/* GFunc() to create a tagvalue item for the supplied alignment feature. */
static void addTagValue(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapFeature feature ;
  ZMapGuiNotebookParagraph paragraph = (ZMapGuiNotebookParagraph)user_data ;
  GList *column_data = NULL ;
  ZMapGuiNotebookTagValue tagvalue ;
  int tmp = 0 ;
  char *clone_id, *strand ;


  feature = getFeature(item) ;

  if (feature->feature.homol.flags.has_clone_id)
    clone_id = g_strdup(g_quark_to_string(feature->feature.homol.clone_id)) ;
  else
    clone_id = g_strdup("<NOT SET>") ;
  column_data = g_list_append(column_data, clone_id) ;

  strand = g_strdup_printf(" %s / %s ",
			   zMapFeatureStrand2Str(feature->strand),
			   zMapFeatureStrand2Str(feature->feature.homol.strand)) ;
  column_data = g_list_append(column_data, strand) ;

  column_data = g_list_append(column_data, GINT_TO_POINTER(feature->x1)) ;
  column_data = g_list_append(column_data, GINT_TO_POINTER(feature->x2)) ;
  column_data = g_list_append(column_data, GINT_TO_POINTER(feature->feature.homol.y1)) ;
  column_data = g_list_append(column_data, GINT_TO_POINTER(feature->feature.homol.y2)) ;

  /* Hack: we assume size(float) <= size(int) && size(float) == 4 and load the float into an int. */
  memcpy(&tmp, &(feature->score), 4) ;
  column_data = g_list_append(column_data, GINT_TO_POINTER(tmp)) ;
	  
  tagvalue = zMapGUINotebookCreateTagValue(paragraph,
					   NULL, ZMAPGUI_NOTEBOOK_TAGVALUE_COMPOUND,
					   "compound", column_data,
					   NULL) ;

  return ;
}





/********************* end of file ********************************/
