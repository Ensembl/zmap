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
 * Last edited: Jun 15 14:08 2007 (edgrif)
 * Created: Wed Jun  6 11:42:51 2007 (edgrif)
 * CVS info:   $Id: zmapWindowFeatureShow.c,v 1.1 2007-06-15 13:09:17 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>


/* THERE IS QUITE A BIT OF DEBUGGING CODE IN HERE WHICH ATTEMPTS TO GET THIS WIDGET
 * TO BE A GOOD SIZE...HOWEVER IT SEEMS A LOST CAUSE AS GTK SEEMS ACTIVELY TO
 * PREVENT THIS...SIGH.....WHEN I HAVE THE URGE I'LL HAVE ANOTHER GO... */


#define LIST_COLUMN_WIDTH 50
#define SHOW_COL_DATA_KEY   "column_decoding_data"
#define SHOW_COL_NUMBER_KEY "column_number_data"
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



/* The general anynote struct, all the following structs must have the same
 * starting fields in the same order. */

/* Types of FeatureBook struct */
typedef enum
  {
    FEATUREBOOK_INVALID,
    FEATUREBOOK_BOOK,
    FEATUREBOOK_PAGE,
    FEATUREBOOK_PARAGRAPH,
    FEATUREBOOK_TAGVALUE
  } FeatureBookType ;


typedef struct
{
  FeatureBookType type ;
  char *name ;
  GList *children ;
} FeatureBookAnyStruct, *FeatureBookAny ;


/* Types of tag value pair. */
typedef enum
  {
    TAGVALUE_INVALID,
    TAGVALUE_FEATURE,					    /* treeview of feature fundamentals */
    TAGVALUE_SIMPLE,					    /* Simple tag value pair */
    TAGVALUE_SCROLLED_TEXT				    /* frame with scrolled text area. */
  } TagValueDisplayType ;


/* Types of paragraph. */
typedef enum
  {
    PARAGRAPH_INVALID,
    PARAGRAPH_SIMPLE,					    /* Simple vertical list of tag values. */
    PARAGRAPH_TAGVALUE_TABLE,				    /* Aligned table of tag value pairs. */
    PARAGRAPH_HOMOGENOUS				    /* All tag value pairs have same tag
							       which is only displayed once. */
  } ParagraphDisplayType ;


/* Each piece of data is a tag/values tuple */
typedef struct
{
  FeatureBookType type ;
  char *tag ;
  GList *children_unused ;
  TagValueDisplayType display_type ;
  char *text ;
} TagValueStruct, *TagValue ;


/* Sets of tagvalues come in paragraphs which may be named or not... */
typedef struct
{
  FeatureBookType type ;
  char *name ;
  GList *tag_values ;
  ParagraphDisplayType display_type ;
} ParagraphStruct, *Paragraph ;


/* Paragraphs come in pages which must be named. */
typedef struct
{
  FeatureBookType type ;
  char *name ;
  GList *paragraphs ;
} PageStruct, *Page ;


typedef struct
{
  FeatureBookType type ;
  char *name ;
  GList *pages ;
} FeatureBookStruct, *FeatureBook ;



/* Types/strings etc. for XML version of notebook pages etc....
 * 
 * Some of these strings need to be kept in step with the FeatureBook types above.
 */

/* the xml tags */
#define XML_TAG_ZMAP      "zmap"
#define XML_TAG_RESPONSE  "response"
#define XML_TAG_ERROR     "error"
#define XML_TAG_NOTEBOOK  "notebook"
#define XML_TAG_PAGE      "page"
#define XML_TAG_PARAGRAPH "paragraph"
#define XML_TAG_TAGVALUE  "tagvalue"

/* paragraph type strings */
#define XML_PARAGRAPH_SIMPLE          "simple"
#define XML_PARAGRAPH_TAGVALUE_TABLE  "tagvalue_table"
#define XML_PARAGRAPH_HOMOGENOUS      "homogenous"

/* tagvalue type strings, note that TAGVALUE_FEATURE cannot be specified via the xml interface. */
#define XML_TAGVALUE_SIMPLE        "simple"
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

  GtkWidget *window ;
  GtkWidget *vbox ;
  GtkWidget *notebook ;

  mainTable      table;
  FeatureBook feature_book ;

  /* State while parsing an xml spec of a notebook. */
  gboolean xml_parsing_status ;
  FeatureBookType xml_curr_tag ;
  Page xml_curr_page ;
  Paragraph xml_curr_paragraph ;
  TagValue xml_curr_tagvalue ;


  /* State while parsing a notebook. */
  Paragraph curr_paragraph ;
  GtkWidget *curr_notebook ;
  GtkWidget *curr_page_vbox ;
  GtkWidget *curr_paragraph_vbox ;
  GtkWidget *curr_paragraph_table ;
  guint curr_paragraph_rows, curr_paragraph_columns ;

  FooCanvasItem *item;          /*!< The item the user called us on  */
  ZMapFeature    origFeature; /*!< Feature as supplied, this MUST not be changed */

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

static FeatureBook createFeatureBook(ZMapWindowFeatureShow show, char *name, ZMapFeature feature) ;
static FeatureBookAny createFeatureBookAny(FeatureBookType type, char *name) ;
static FeatureBook destroyFeatureBook(FeatureBook feature_book) ;
static void freeBookResources(gpointer data, gpointer user_data) ;

/* xml event callbacks */
static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement element, 
                                  ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_notebook_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_page_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_paragraph_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_tagvalue_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, 
                                 ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_response_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_notebook_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_page_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_paragraph_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data);
static gboolean xml_tagvalue_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data);
static void printWarning(char *element, char *handler) ;


static ZMapWindowFeatureShow showFeature(ZMapWindowFeatureShow reuse_window, ZMapWindow window, FooCanvasItem *item) ;
static void createEditWindow(ZMapWindowFeatureShow feature_show, char *title) ;
static GtkWidget *makeMenuBar(ZMapWindowFeatureShow feature_show) ;

static ZMapWindowFeatureShow findReusableShow(GPtrArray *window_list) ;
static gboolean windowIsReusable(void) ;
static ZMapFeature getFeature(FooCanvasItem *item) ;

static GtkWidget *makeNotebook(ZMapWindowFeatureShow feature_show) ;
static void makePageCB(gpointer data, gpointer user_data) ;
static void makeParagraphCB(gpointer data, gpointer user_data) ;
static void makeTagValueCB(gpointer data, gpointer user_data) ;

static GtkTreeModel *makeTreeModel(FooCanvasItem *item) ;



static GtkWidget *addFeatureSection(GtkWidget *parent, ZMapWindowFeatureShow show) ;



static void parseFeature(ZMapWindowFeatureShow show) ;
static void array2List(mainTable table, GArray *array, ZMapFeatureType feature_type);


static void destroyCB(GtkWidget *widget, gpointer data);

static gboolean selectionFunc(GtkTreeSelection *selection, 
                              GtkTreeModel     *model,
                              GtkTreePath      *path, 
                              gboolean          path_currently_selected,
                              gpointer          user_data);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void cellEditedCB(GtkCellRendererText *renderer, 
                         char *path, char *new_text, 
                         gpointer user_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static GtkCellRenderer *getColRenderer(ZMapWindowFeatureShow show);


static void sizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;
static void ScrsizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;
static void sizeRequestCB(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data) ;
static void ScrsizeRequestCB(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data) ;
static void preserveCB(gpointer data, guint cb_action, GtkWidget *widget);
static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);




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

  /* Look for a reusable window. */
  show = findReusableShow(window->feature_show_windows) ;

  /* now show the window, if we found a reusable one that will be reused. */
  show = showFeature(show, window, item) ;

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
      
      parseFeature(show) ;
      
      show->feature_book = createFeatureBook(show, feature_name, feature) ;
      

      /* Now display the pages..... */
      show->notebook = makeNotebook(show) ;
      gtk_box_pack_start(GTK_BOX(show->vbox), show->notebook, TRUE, TRUE, 0) ;

      gtk_widget_show_all(show->window) ;

      /* Now free the feature_book..not wanted after display ?? */
      show->feature_book = destroyFeatureBook(show->feature_book) ;
    }

  return show ;
}


static ZMapFeature getFeature(FooCanvasItem *item)
{
  ZMapFeature feature = NULL ;
  ZMapFeatureType type ;

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
  show->table = g_new0(mainTableStruct, 16);

  return show ;
}


static void featureShowReset(ZMapWindowFeatureShow show, ZMapWindow window, char *title)
{
  show->reusable = windowIsReusable() ;
  show->zmapWindow = window ;

  /* This will all probably go... */
  g_free(show->table) ;
  show->table = g_new0(mainTableStruct, 16);

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
static FeatureBook createFeatureBook(ZMapWindowFeatureShow show, char *name, ZMapFeature feature)
{
  FeatureBook feature_book = NULL ;
  Page page ;
  Paragraph paragraph ;
  TagValue tag_value ;
  char *page_title ;
  char *description ;

  feature_book = (FeatureBook)createFeatureBookAny(FEATUREBOOK_BOOK, name) ;

  /* The feature fundamentals page. */
  page = (Page)createFeatureBookAny(FEATUREBOOK_PAGE, "Basics") ;
  feature_book->pages = g_list_append(feature_book->pages, page) ;
  
  paragraph = (Paragraph)createFeatureBookAny(FEATUREBOOK_PARAGRAPH, NULL) ;
  page->paragraphs = g_list_append(page->paragraphs, paragraph) ;

  tag_value = (TagValue)createFeatureBookAny(FEATUREBOOK_TAGVALUE, NULL) ;
  tag_value->display_type = TAGVALUE_FEATURE ;
  paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;


  switch(feature->type)
    {
    case ZMAPFEATURE_BASIC:
      {
	page_title = "Details" ;

	break ;
      }
    case ZMAPFEATURE_TRANSCRIPT:
      {
	page_title = "Transcript Details" ;

	break ;
      }
    case ZMAPFEATURE_ALIGNMENT:
      {
	page_title = "Alignment Details" ;

	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
	break ;
      }
    }
  page = (Page)createFeatureBookAny(FEATUREBOOK_PAGE, page_title) ;
  feature_book->pages = g_list_append(feature_book->pages, page) ;


  if ((description = zMapStyleGetDescription(feature->style)))
    {
      paragraph = (Paragraph)createFeatureBookAny(FEATUREBOOK_PARAGRAPH, NULL) ;
      paragraph->display_type = PARAGRAPH_TAGVALUE_TABLE ;
      page->paragraphs = g_list_append(page->paragraphs, paragraph) ;

      tag_value = (TagValue)createFeatureBookAny(FEATUREBOOK_TAGVALUE, "Description") ;
      tag_value->display_type = TAGVALUE_SCROLLED_TEXT ;
      tag_value->text = g_strdup(description) ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;
    }


  /* Secondary features. */
  if (feature->type == ZMAPFEATURE_ALIGNMENT)
    {
      paragraph = (Paragraph)createFeatureBookAny(FEATUREBOOK_PARAGRAPH, NULL) ;
      paragraph->display_type = PARAGRAPH_TAGVALUE_TABLE ;
      page->paragraphs = g_list_append(page->paragraphs, paragraph) ;

      tag_value = (TagValue)createFeatureBookAny(FEATUREBOOK_TAGVALUE, "Align Type") ;
      tag_value->display_type = TAGVALUE_SIMPLE ;
      tag_value->text = g_strdup(zMapFeatureHomol2Str(feature->feature.homol.type)) ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;

      tag_value = (TagValue)createFeatureBookAny(FEATUREBOOK_TAGVALUE, "Query length") ;
      tag_value->display_type = TAGVALUE_SIMPLE ;
      tag_value->text = g_strdup_printf("%d", feature->feature.homol.length) ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;

      tag_value = (TagValue)createFeatureBookAny(FEATUREBOOK_TAGVALUE, "Query strand") ;
      tag_value->display_type = TAGVALUE_SIMPLE ;
      tag_value->text = g_strdup(zMapFeatureStrand2Str(feature->feature.homol.target_strand)) ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;

      tag_value = (TagValue)createFeatureBookAny(FEATUREBOOK_TAGVALUE, "Query phase") ;
      tag_value->display_type = TAGVALUE_SIMPLE ;
      tag_value->text = g_strdup(zMapFeaturePhase2Str(feature->feature.homol.target_phase)) ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;
    }
  else if (feature->type == ZMAPFEATURE_TRANSCRIPT)
    {
      paragraph = (Paragraph)createFeatureBookAny(FEATUREBOOK_PARAGRAPH, NULL) ;
      paragraph->display_type = PARAGRAPH_TAGVALUE_TABLE ;
      page->paragraphs = g_list_append(page->paragraphs, paragraph) ;

      if (feature->feature.transcript.flags.start_not_found)
	{
	  tag_value = (TagValue)createFeatureBookAny(FEATUREBOOK_TAGVALUE, "Start Not Found") ;
	  tag_value->display_type = TAGVALUE_SIMPLE ;
	  tag_value->text = g_strdup(zMapFeaturePhase2Str(feature->feature.transcript.start_phase)) ;
	  paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;
	}

      if (feature->feature.transcript.flags.end_not_found)
	{
	  tag_value = (TagValue)createFeatureBookAny(FEATUREBOOK_TAGVALUE, "End Not Found") ;
	  tag_value->display_type = TAGVALUE_SIMPLE ;
	  tag_value->text = g_strdup("TRUE") ;
	  paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;
	}
    }


  /* If we have an external program driving us then ask it for any extra information.
   * This will come as xml.... */
  {
    gboolean externally_handled = FALSE ;
    ZMapXMLObjTagFunctionsStruct starts[] = {
      {XML_TAG_ZMAP, xml_zmap_start_cb     },
      {XML_TAG_RESPONSE, xml_response_start_cb },
      {XML_TAG_NOTEBOOK, xml_notebook_start_cb },
      {XML_TAG_PAGE, xml_page_start_cb },
      {XML_TAG_PARAGRAPH, xml_paragraph_start_cb },
      {XML_TAG_TAGVALUE, xml_tagvalue_start_cb },
      { NULL, NULL}
    };
    ZMapXMLObjTagFunctionsStruct ends[] = {
      {XML_TAG_ZMAP,  xml_zmap_end_cb  },
      {XML_TAG_RESPONSE, xml_response_end_cb },
      {XML_TAG_NOTEBOOK, xml_notebook_end_cb },
      {XML_TAG_PAGE, xml_page_end_cb },
      {XML_TAG_PARAGRAPH, xml_paragraph_end_cb },
      {XML_TAG_TAGVALUE, xml_tagvalue_end_cb },
      {XML_TAG_ERROR, xml_error_end_cb },
      { NULL, NULL}
      };

    show->xml_parsing_status = TRUE ;
    show->xml_curr_tag = FEATUREBOOK_INVALID ;
    show->xml_curr_page = NULL ;

    if (zmapWindowUpdateXRemoteDataFull(show->zmapWindow,
					(ZMapFeatureAny)feature,
					"feature_details",
					show->item,
					starts, ends, show))
      {
	feature_book->pages = g_list_append(feature_book->pages, show->xml_curr_page) ;
      }
    else
      {
	/* Clean up if something went wrong...  */
	if (show->xml_curr_page)
	  freeBookResources(show->xml_curr_page, NULL) ;
      }
  }

  return feature_book ;
}


static FeatureBookAny createFeatureBookAny(FeatureBookType type, char *name)
{
  FeatureBookAny book_any ;
  int size ;

  switch(type)
    {
    case FEATUREBOOK_BOOK:
      size = sizeof(FeatureBookStruct) ;
      break ;
    case FEATUREBOOK_PAGE:
      size = sizeof(PageStruct) ;
      break ;
    case FEATUREBOOK_PARAGRAPH:
      size = sizeof(ParagraphStruct) ;
      break ;
    case FEATUREBOOK_TAGVALUE:
      size = sizeof(TagValueStruct) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  book_any = g_malloc0(size) ;
  book_any->type = type ;
  if (name)
    book_any->name = g_strdup(name) ;

  return book_any ;
}


static FeatureBook destroyFeatureBook(FeatureBook feature_book)
{
  freeBookResources(feature_book, NULL) ;

  return NULL ;
}

/* A GFunc() to free up the book resources... */
static void freeBookResources(gpointer data, gpointer user_data)
{
  FeatureBookAny book_any = (FeatureBookAny)data ;

  if (book_any->children)
    {
      g_list_foreach(book_any->children, freeBookResources, NULL) ;
      g_list_free(book_any->children) ;
    }

  if (book_any->name)
    g_free(book_any->name) ;

  book_any->type = FEATUREBOOK_INVALID ;

  g_free(book_any) ;

  return ;
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

  /* Add ptrs so parent knows about us */
  g_ptr_array_add(feature_show->zmapWindow->feature_show_windows, (gpointer)(feature_show->window)) ;


  feature_show->vbox = vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_box_set_spacing(GTK_BOX(vbox), 5) ;
  gtk_container_add(GTK_CONTAINER(feature_show->window), vbox) ;

  gtk_box_pack_start(GTK_BOX(vbox), makeMenuBar(feature_show), FALSE, FALSE, 0);

  return ;
}



static GtkWidget *makeNotebook(ZMapWindowFeatureShow feature_show)
{
  GtkWidget *notebook = NULL ;
  FeatureBook feature_book = feature_show->feature_book ;

  notebook = feature_show->curr_notebook = gtk_notebook_new() ;

  g_list_foreach(feature_book->pages, makePageCB, feature_show) ;

  return notebook ;
}



/* A GFunc() to build notebook pages. */
static void makePageCB(gpointer data, gpointer user_data)
{
  Page page = (Page)data ;
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)user_data ;
  GtkWidget *notebook_label ;
  int notebook_index ;

  /* Put code here for a page... */
  notebook_label = gtk_label_new(page->name) ;

  feature_show->curr_page_vbox = gtk_vbox_new(FALSE, 0) ;

  g_list_foreach(page->paragraphs, makeParagraphCB, feature_show) ;

  notebook_index = gtk_notebook_append_page(GTK_NOTEBOOK(feature_show->curr_notebook),
					    feature_show->curr_page_vbox, notebook_label) ;

  return ;
}


/* A GFunc() to build notebook pages. */
static void makeParagraphCB(gpointer data, gpointer user_data)
{
  Paragraph paragraph = (Paragraph)data ;
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)user_data ;
  GtkWidget *paragraph_frame ;

  feature_show->curr_paragraph = paragraph ;

  paragraph_frame = gtk_frame_new(paragraph->name) ;
  gtk_container_set_border_width(GTK_CONTAINER(paragraph_frame), 5) ;
  gtk_container_add(GTK_CONTAINER(feature_show->curr_page_vbox), paragraph_frame) ;

  feature_show->curr_paragraph_vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(paragraph_frame), feature_show->curr_paragraph_vbox) ;

  if (paragraph->display_type == PARAGRAPH_TAGVALUE_TABLE || paragraph->display_type == PARAGRAPH_HOMOGENOUS)
    {
      feature_show->curr_paragraph_rows = 0 ;
      feature_show->curr_paragraph_columns = 2 ;
      feature_show->curr_paragraph_table = gtk_table_new(feature_show->curr_paragraph_rows,
                                                         feature_show->curr_paragraph_columns,
                                                         FALSE) ;
      gtk_container_add(GTK_CONTAINER(feature_show->curr_paragraph_vbox), feature_show->curr_paragraph_table) ;
    }

  g_list_foreach(paragraph->tag_values, makeTagValueCB, feature_show) ;

  return ;
}


/* A GFunc() to build notebook pages. */
static void makeTagValueCB(gpointer data, gpointer user_data)
{
  TagValue tag_value = (TagValue)data ;
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)user_data ;
  GtkWidget *container = NULL ;

  switch(tag_value->display_type)
    {
    case TAGVALUE_SIMPLE:
      {
	GtkWidget *tag, *value ;

	tag = gtk_label_new(tag_value->tag) ;
	gtk_label_set_justify(GTK_LABEL(tag), GTK_JUSTIFY_RIGHT) ;

	value = gtk_entry_new() ;
	gtk_entry_set_text(GTK_ENTRY(value), tag_value->text) ;
	gtk_entry_set_editable(GTK_ENTRY(value), FALSE) ;

	if (feature_show->curr_paragraph->display_type == PARAGRAPH_SIMPLE)
	  {
	    GtkWidget *hbox ;

	    container = hbox = gtk_hbox_new(FALSE, 0) ;
	    gtk_container_add(GTK_CONTAINER(feature_show->curr_paragraph_vbox), container) ;

	    gtk_container_add(GTK_CONTAINER(hbox), tag) ;

	    gtk_container_add(GTK_CONTAINER(hbox), value) ;
	  }
	else
	  {
	    feature_show->curr_paragraph_rows++ ;
	    gtk_table_resize(GTK_TABLE(feature_show->curr_paragraph_table),
			     feature_show->curr_paragraph_rows,
			     feature_show->curr_paragraph_columns) ;

	    if (feature_show->curr_paragraph->display_type == PARAGRAPH_TAGVALUE_TABLE
		|| (feature_show->curr_paragraph->display_type == PARAGRAPH_HOMOGENOUS
		    && feature_show->curr_paragraph_rows ==1))
	    gtk_table_attach_defaults(GTK_TABLE(feature_show->curr_paragraph_table),
				      tag,
				      0,
				      1,
				      feature_show->curr_paragraph_rows - 1,
				      feature_show->curr_paragraph_rows) ;

	    gtk_table_attach_defaults(GTK_TABLE(feature_show->curr_paragraph_table),
				      value,
				      1,
				      2,
				      feature_show->curr_paragraph_rows - 1,
				      feature_show->curr_paragraph_rows) ;
	  }

	break ;
      }

    case TAGVALUE_FEATURE:
      {
	GtkWidget *scrolled_window, *treeView ;
	TreeViewSizeCBData size_data ;

	size_data = g_new0(TreeViewSizeCBDataStruct, 1) ;

	container = size_data->scrolled_window = scrolled_window = gtk_scrolled_window_new(NULL, NULL) ;
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC) ;
	gtk_container_add(GTK_CONTAINER(feature_show->curr_paragraph_vbox), container) ;

	g_signal_connect(GTK_OBJECT(scrolled_window), "size-allocate",
			 GTK_SIGNAL_FUNC(ScrsizeAllocateCB), size_data) ;
	g_signal_connect(GTK_OBJECT(scrolled_window), "size-request",
			 GTK_SIGNAL_FUNC(ScrsizeRequestCB), size_data) ;

	size_data->tree_view = treeView = addFeatureSection(scrolled_window, feature_show) ;

	gtk_container_add(GTK_CONTAINER(scrolled_window), treeView) ;
  
	g_signal_connect(GTK_OBJECT(treeView), "size-allocate",
			 GTK_SIGNAL_FUNC(sizeAllocateCB), size_data) ;
	g_signal_connect(GTK_OBJECT(treeView), "size-request",
			 GTK_SIGNAL_FUNC(sizeRequestCB), size_data) ;

	break ;
      }

    case TAGVALUE_SCROLLED_TEXT:
      {
	GtkWidget *frame, *scrolled_window, *view ;
	GtkTextBuffer *buffer ;

	frame = gtk_frame_new(tag_value->tag) ;
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5) ;
	gtk_container_add(GTK_CONTAINER(feature_show->curr_paragraph_vbox), frame) ;

	scrolled_window = gtk_scrolled_window_new(NULL, NULL) ;
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC) ;
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window) ;

	view = gtk_text_view_new() ;
	gtk_container_add(GTK_CONTAINER(scrolled_window), view) ;
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE) ;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)) ;
	gtk_text_buffer_set_text(buffer, tag_value->text, -1) ;

	break ;
      }

    default:
      {
	zMapAssertNotReached() ;
	break ;
      }
    }



  return ;
}




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




static GtkTreeModel *makeTreeModel(FooCanvasItem *item)
{
  GtkTreeModel *tree_model = NULL ;
  GList *itemList         = NULL;

  tree_model = zmapWindowFeatureListCreateStore(ZMAPWINDOWLIST_FEATURE_TREE);

  itemList  = g_list_append(itemList, item);

  zmapWindowFeatureListPopulateStoreList(tree_model, ZMAPWINDOWLIST_FEATURE_TREE, itemList, NULL) ;

  return tree_model ;
}





/* THIS FUNCTION IS NO GOOD, ROB JUST DIDN'T UNDERSTAND WHAT WAS REQUIRED, IF WE EVER DO
 * EDITING IT WILL NEED TO BE COMPLETELY REWRITTEN.... */

/* I see what's going on here, but it doesn't feel dynamic enough to
 * cope with what I want. I don't need it to have super strength what
 * is that gonna be. But a little cleverer than it currently is would
 * be nice .
 */

/* So that the draw routines don't need to know more than the minimum
 * about what they're drawing, everything in the table, apart from the
 * arrays, is held as a string. 
 */
static void parseFeature(ZMapWindowFeatureShow show)
{
  int i = 0;
  /*                            fieldtype         label               fieldPtr      value         validateCB
   *                                     datatype                  pair      widget       editable   */
  mainTableStruct 
    ftypeInit   = { ENTRY , FTYPE , "Type"           , 0, NULL, NULL, {NULL} },
    fx1Init     = { ENTRY , INT   , "Start"          , 0, NULL, NULL, {NULL}  },
      fx2Init     = { ENTRY , INT   , "End"            ,-1, NULL, NULL, {NULL}  },
	fstrandInit = { ENTRY , STRAND, "Strand"         , 0, NULL, NULL, {NULL}  },
	  fphaseInit  = { ENTRY , PHASE , "Phase"          , 0, NULL, NULL, {NULL}  },
	    fscoreInit  = { ENTRY , FLOAT , "Score"          , 0, NULL, NULL, {NULL}  },
	      blankInit   = { LABEL , LABEL , " "              , 0, NULL, NULL, {NULL}  },
		htypeInit   = { ENTRY , HTYPE , "Homol Type"     , 0, NULL, NULL, {NULL}  },
		  hy1Init     = { ENTRY , INT   , "Query Start"    , 0, NULL, NULL, {NULL}  },
		    hy2Init     = { ENTRY , INT   , "Query End"      ,-1, NULL, NULL, {NULL}  },
		      hstrandInit = { ENTRY , STRAND, "Query Strand"   , 0, NULL, NULL, {NULL}  },
			hphaseInit  = { ENTRY , PHASE , "Query Phase"    , 0, NULL, NULL, {NULL}  },
			  hscoreInit  = { ENTRY , FLOAT , "Query Score"    , 0, NULL, NULL, {NULL}  },
			    halignInit  = { ALIGN , ALIGN , "Alignments"     , 0, NULL, NULL, {NULL}  },
			    
			      tx1Init     = { ENTRY , INT   , "CDS Start"      , 0, NULL, NULL, {NULL}  },
				tx2Init     = { ENTRY , INT   , "CDS End"        ,-1, NULL, NULL, {NULL}  },
				  tphaseInit  = { ENTRY , PHASE , "CDS Phase"      , 0, NULL, NULL, {NULL}  },
				    /*		  tsnfInit    = { CHECK , CHECK , "Start Not Found", 0, NULL, NULL, {NULL}, TRUE , NULL  },
				      tenfInit    = { CHECK , CHECK , "End Not Found " , 0, NULL, NULL, {NULL}, TRUE , NULL  }, */
				    texonInit   = { EXON  , EXON  , "Exons"          , 0, NULL, NULL, {NULL}  },
				      tintronInit = { INTRON, INTRON, "Introns"        , 0, NULL, NULL, {NULL}  },
			    
					lastInit    = { LAST  , NONE  ,  ""              , 0, NULL, NULL, {NULL} }
					;
				mainTable table = show->table ;
					ZMapFeature feature ;
  					
					feature = show->origFeature ;

  table[i] = ftypeInit;
  table[i].fieldPtr = &feature->type;
  
  i++;
  table[i] = fx1Init;
  table[i].fieldPtr = &feature->x1;
  
  i++;
  table[i] = fx2Init;
  table[i].fieldPtr = &feature->x2;
  
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



static void destroyCB(GtkWidget *widget, gpointer data)
{
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)data ;
    
  g_ptr_array_remove(feature_show->zmapWindow->feature_show_windows, (gpointer)feature_show->window);

  g_free(feature_show->table) ;

  g_free(feature_show) ;
                                                           
  return ;
}


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



/* widget is the treeview */
static void sizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data_unused)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (GTK_WIDGET_REALIZED(widget)
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
  if (GTK_WIDGET_REALIZED(widget)
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
  if (GTK_WIDGET_REALIZED(widget)
    {
      printf("ScrWin: sizeRequestCB:  height: %d, width: %d\n", 
	     requisition->height, requisition->width); 
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


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


/* Find an show window in the list of show windows currently displayed that
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
                                  ZMapXMLParser parser, gpointer handler_data)
{
  printWarning("zmap", "start") ;

  return TRUE ;
}

static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data)
{
  ZMapXMLTagHandler message = (ZMapXMLTagHandler)user_data ;
  ZMapXMLAttribute handled_attribute = NULL ;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)handler_data ;

  printWarning("response", "start") ;

  if ((handled_attribute = zMapXMLElementGetAttributeByName(element, "handled"))
      && (message->handled == FALSE))
    {
      message->handled = zMapXMLAttributeValueToBool(handled_attribute);

      show->xml_curr_tag = FEATUREBOOK_INVALID ;
    }

  return TRUE;
}

static gboolean xml_notebook_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data)
{
  ZMapXMLTagHandler message = (ZMapXMLTagHandler)user_data ;
  ZMapXMLAttribute attr = NULL ;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)handler_data ;

  printWarning("notebook", "start") ;

  if (show->xml_curr_tag != FEATUREBOOK_INVALID)
    {
      zMapXMLParserRaiseParsingError(parser, "Bad xml, response tag not set.");
    }
  else
    {
      show->xml_curr_tag = FEATUREBOOK_BOOK ;
    }

  return TRUE;
}

static gboolean xml_page_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data)
{
  ZMapXMLTagHandler message = (ZMapXMLTagHandler)user_data ;
  ZMapXMLAttribute attr = NULL ;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)handler_data ;
  char *page_name = NULL ;

  printWarning("page", "start") ;

  if (show->xml_curr_tag != FEATUREBOOK_BOOK)
    {
      zMapXMLParserRaiseParsingError(parser, "Bad xml, notebook tag not set.");
    }
  else
    {
      show->xml_curr_tag = FEATUREBOOK_PAGE ;

      if ((attr = zMapXMLElementGetAttributeByName(element, "name")))
	{
	  page_name = g_strdup(zMapXMLAttributeValueToStr(attr)) ;
	  
	  show->xml_curr_page = (Page)createFeatureBookAny(FEATUREBOOK_PAGE, page_name) ;
	}
      else
	zMapXMLParserRaiseParsingError(parser, "name is a required attribute for page tag.");
    }

  return TRUE;
}


static gboolean xml_paragraph_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data)
{
  ZMapXMLTagHandler message = (ZMapXMLTagHandler)user_data ;
  ZMapXMLAttribute attr = NULL ;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)handler_data ;

  printWarning("paragraph", "start") ;

  if (show->xml_curr_tag != FEATUREBOOK_PAGE)
    {
      zMapXMLParserRaiseParsingError(parser, "Bad xml, page tag not set.");
    }
  else
    {
      gboolean status = TRUE ;
      char *paragraph_name = NULL ;
      char *paragraph_type = NULL ;
      ParagraphDisplayType type = PARAGRAPH_INVALID ;

      show->xml_curr_tag = FEATUREBOOK_PARAGRAPH ;

      if ((attr = zMapXMLElementGetAttributeByName(element, "name")))
	{
	  paragraph_name = g_strdup(zMapXMLAttributeValueToStr(attr)) ;
	}

      if ((attr = zMapXMLElementGetAttributeByName(element, "type")))
	{
	  paragraph_type = zMapXMLAttributeValueToStr(attr) ;

	  if (strcmp(paragraph_type, XML_PARAGRAPH_SIMPLE) == 0)
	    type = PARAGRAPH_SIMPLE ;
	  else if (strcmp(paragraph_type, XML_PARAGRAPH_TAGVALUE_TABLE) == 0)
	    type = PARAGRAPH_TAGVALUE_TABLE ;
	  else if (strcmp(paragraph_type, XML_PARAGRAPH_HOMOGENOUS) == 0)
	    type = PARAGRAPH_HOMOGENOUS ;
	  else
	    {
	      zMapXMLParserRaiseParsingError(parser, "Bad paragraph type attribute.") ;
	      status = FALSE ;
	    }
	}

      if (status)
	{
	  show->xml_curr_paragraph = (Paragraph)createFeatureBookAny(FEATUREBOOK_PARAGRAPH, paragraph_name) ;
	  show->xml_curr_page->paragraphs = g_list_append(show->xml_curr_page->paragraphs, show->xml_curr_paragraph) ;

	  if (paragraph_type)
	    show->xml_curr_paragraph->display_type = type ;
	}
    }

  return TRUE;
}

static gboolean xml_tagvalue_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data)
{
  ZMapXMLTagHandler message = (ZMapXMLTagHandler)user_data ;
  ZMapXMLAttribute attr = NULL ;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)handler_data ;


  printWarning("tagvalue", "start") ;

  if (show->xml_curr_tag != FEATUREBOOK_PARAGRAPH)
    {
      zMapXMLParserRaiseParsingError(parser, "Bad xml, paragraph tag not set.");
    }
  else
    {
      gboolean status = TRUE ;
      char *tagvalue_name = NULL ;
      char *tagvalue_type = NULL ;
      TagValueDisplayType type = TAGVALUE_INVALID ;

      show->xml_curr_tag = FEATUREBOOK_TAGVALUE ;

      if ((attr = zMapXMLElementGetAttributeByName(element, "name")))
	{
	  tagvalue_name = g_strdup(zMapXMLAttributeValueToStr(attr)) ;
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, "name is a required attribute for tagvalue.") ;
	  status = FALSE ;
	}

      if (status && (attr = zMapXMLElementGetAttributeByName(element, "type")))
	{
	  tagvalue_type = zMapXMLAttributeValueToStr(attr) ;

	  if (strcmp(tagvalue_type, XML_TAGVALUE_SIMPLE) == 0)
	    type = TAGVALUE_SIMPLE ;
	  else if (strcmp(tagvalue_type, XML_TAGVALUE_SCROLLED_TEXT) == 0)
	    type = TAGVALUE_SCROLLED_TEXT ;
	  else
	    {
	      zMapXMLParserRaiseParsingError(parser, "Bad tagvalue type attribute.") ;
	      status = FALSE ;
	    }
	}

      if (status)
	{
	  show->xml_curr_tagvalue = (TagValue)createFeatureBookAny(FEATUREBOOK_TAGVALUE, tagvalue_name) ;
	  show->xml_curr_paragraph->tag_values = g_list_append(show->xml_curr_paragraph->tag_values,
							       show->xml_curr_tagvalue) ;

	  if (tagvalue_type)
	    show->xml_curr_tagvalue->display_type = type ;
	}
    }

  return TRUE;
}

static gboolean xml_tagvalue_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)handler_data ;
  char *content ;

  printWarning("tagvalue", "end") ;

  if ((content = zMapXMLElementStealContent(element)))
    {
      show->xml_curr_tagvalue->text = content ;
    }
  else
    zMapXMLParserRaiseParsingError(parser, "tagvalue element is empty.");

  show->xml_curr_tag = FEATUREBOOK_PARAGRAPH ;

  return TRUE;
}

static gboolean xml_paragraph_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)handler_data ;

  printWarning("paragraph", "end") ;

  show->xml_curr_tag = FEATUREBOOK_PAGE ;

  return TRUE;
}

static gboolean xml_page_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)handler_data ;

  printWarning("page", "end") ;

  show->xml_curr_tag = FEATUREBOOK_BOOK ;

  return TRUE;
}

static gboolean xml_notebook_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)handler_data ;

  printWarning("notebook", "end") ;

  show->xml_curr_tag = FEATUREBOOK_INVALID ;

  return TRUE;
}

static gboolean xml_response_end_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser, gpointer handler_data)
{
  printWarning("response", "end") ;

  return TRUE;
}


static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser, gpointer handler_data)
{
  printWarning("zmap", "end") ;

  return TRUE;
}


/* Cleans up any dangling page stuff. */
static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, 
                                 ZMapXMLParser parser, gpointer handler_data)
{
  ZMapXMLTagHandler message = (ZMapXMLTagHandler)user_data;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)handler_data ;
  ZMapXMLElement mess_element = NULL;

  if((mess_element = zMapXMLElementGetChildByName(element, "message"))
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



/********************* end of file ********************************/
