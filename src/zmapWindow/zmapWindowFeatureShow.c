/*  File: zmapWindowFeatureShow.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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
 * Last edited: Feb 15 11:45 2011 (edgrif)
 * Created: Wed Jun  6 11:42:51 2007 (edgrif)
 * CVS info:   $Id: zmapWindowFeatureShow.c,v 1.28 2011-02-15 11:50:59 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

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



typedef struct ZMapWindowFeatureShowStruct_
{
  ZMapWindow zmapWindow ;

  gboolean reusable ;					    /* Can this window be reused for a new feature. */

  FooCanvasItem *item;					    /* The item the user called us on  */
  ZMapFeature    origFeature;				    /* Feature from item. */


  ZMapGuiNotebook feature_book ;

      /*
       * MH17: re-using code, this is a bit of a hack
       * as this code was written to drive a dialog not to extract data from XML
       * it's here as a prototype and only handle half of the task
       * which is evidence from transcript
       * we also want transcript from evidence
       * however this will allow demonstration of highlighting and also display in a new column
       *
       * the feature_details XML contains a list of evidence features
       * and the code in tbis file uses a series of tag handler callbacks to take the
       * data and construct a GTK notebook full of widgets
       * some of the XML fields are compound which is a bit of a shame, XML was created to not do that
       *
       * rather than duplicating code I've added an option to accumulate the data wanted
       * in parallel with creating the notebook. If not set then existing code will be unaffected
       * this list should be freed by the caller if it is not null. It only affects this file.
       *
       * as the tag handlers assume the are making a notebook, we have to do this and free it afterwards
       */

  gboolean get_evidence;
#define WANT_EVIDENCE   1     /* constructung the list */
#define GOT_EVIDENCE    2     /* in the right paragraph */
  int evidence_column;        /* in the composite free-text data */
  GList *evidence;

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

static void destroyCB(GtkWidget *widget, gpointer data);

static void preserveCB(gpointer data, guint cb_action, GtkWidget *widget);
static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);

static void cleanUp(ZMapGuiNotebookAny any_section, void *user_data) ;
static void getAllMatches(ZMapWindow window,
			  ZMapFeature feature, FooCanvasItem *item, ZMapGuiNotebookSubsection subsection) ;
static void addTagValue(gpointer data, gpointer user_data) ;
static ZMapGuiNotebook makeTranscriptExtras(ZMapFeature feature) ;


/*
 *                           Globals
 */


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



static gboolean alert_client_debug_G = FALSE ;


/* Start and end tag handlers for converting zmap notebook style xml into a zmap notebook. */
static  ZMapXMLObjTagFunctionsStruct starts[] =
  {
    {XML_TAG_ZMAP,       xml_zmap_start_cb     },
    {XML_TAG_RESPONSE,   xml_response_start_cb },
    {XML_TAG_NOTEBOOK,   xml_notebook_start_cb },
    {XML_TAG_CHAPTER,    xml_chapter_start_cb },
    {XML_TAG_PAGE,       xml_page_start_cb },
    {XML_TAG_PARAGRAPH,  xml_paragraph_start_cb },
    {XML_TAG_SUBSECTION, xml_subsection_start_cb },
    {XML_TAG_TAGVALUE,   xml_tagvalue_start_cb },
    {NULL, NULL}
  } ;

static ZMapXMLObjTagFunctionsStruct ends[] =
  {
    {XML_TAG_ZMAP,       xml_zmap_end_cb  },
    {XML_TAG_RESPONSE,   xml_response_end_cb },
    {XML_TAG_NOTEBOOK,   xml_notebook_end_cb },
    {XML_TAG_PAGE,       xml_page_end_cb },
    {XML_TAG_CHAPTER,    xml_chapter_end_cb },
    {XML_TAG_SUBSECTION, xml_subsection_end_cb },
    {XML_TAG_PARAGRAPH,  xml_paragraph_end_cb },
    {XML_TAG_TAGVALUE,   xml_tagvalue_end_cb },
    {XML_TAG_ERROR,      xml_error_end_cb },
    {NULL, NULL}
  } ;




/* MY GUT FEELING IS THAT ROB LEFT STUFF ALLOCATED AND DID NOT CLEAR UP SO I NEED TO CHECK UP ON
 * ALL THAT....
 * 
 * This sentiment is correct, particularly in some of the compound tagvalue creation
 * there are GLists and probably other data allocated which is not free'd.....
 * 
 *  */



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

  if ((feature = getFeature(item))) /* && zMapStyleIsTrueFeature(feature->style)) */
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

  feature = zmapWindowItemGetFeature(item);
  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;

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
 * In a perfect world this code would be rationalised so that the only way
 * we constructed our feature details window would be:
 * 
 * feature details ---> xml string ---> xml parse ---> zmapguinotebook
 * 
 * currently we do this with data returned by the external program
 * but internally we just do:
 * 
 * feature details ---> zmapguinotebook
 * 
 * meaning there has to be some duplicate code......
 * 
 *  */

static ZMapGuiNotebook createFeatureBook(ZMapWindowFeatureShow show, char *name,
					 ZMapFeature feature, FooCanvasItem *item)
{
  ZMapGuiNotebook feature_book = NULL ;
  ZMapGuiNotebookChapter dummy_chapter ;
  ZMapGuiNotebookPage page ;
  ZMapGuiNotebookSubsection subsection ;
  ZMapGuiNotebookParagraph paragraph ;
  ZMapGuiNotebookTagValue tag_value ;
  ZMapFeatureTypeStyle style;
  char *chapter_title, *page_title, *description ;
  char *tmp ;
  char *notes ;
  ZMapGuiNotebook extras_notebook = NULL ;


  feature_book = zMapGUINotebookCreateNotebook(name, FALSE, cleanUp, NULL) ;

  /* The feature fundamentals page. */
  switch(feature->type)
    {
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
    case ZMAPSTYLE_MODE_GRAPH:
    case ZMAPSTYLE_MODE_GLYPH:
    case ZMAPSTYLE_MODE_TEXT:
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

  style = feature->style; /* zMapFindStyle(show->zmapWindow->display_styles, feature->style_id); */

  tag_value = zMapGUINotebookCreateTagValue(paragraph, "Feature Group [style_id]",
					    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					    "string", g_strdup(zMapStyleGetName(style)), NULL) ;

  if ((description = zMapStyleGetDescription(style)))
    {
      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Style Description",
						ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,   /* SCROLLED_TEXT,*/
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
	tmp = g_strdup_printf("%d -> %d", feature->feature.transcript.cds_start, feature->feature.transcript.cds_end) ;
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


  /* If we have an external program driving us then ask it for any extra information.
   * This will come as xml which we decode via the callbacks listed in the following structs.
   * If that fails then we may have extra stuff to add anyway. */
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
      extras_notebook = show->xml_curr_notebook ;
    }
  else
    {
      /* Clear up any half finished stuff from failed remote call. */
      if (show->xml_curr_chapter)
	zMapGUINotebookDestroyAny((ZMapGuiNotebookAny)(show->xml_curr_notebook)) ;

      /* For transcripts create our own extras (e.g. exons), add code for new feature
       * types as necessary here. */
      switch(feature->type)
	{
	case ZMAPSTYLE_MODE_TRANSCRIPT:
	  {
	    extras_notebook = makeTranscriptExtras(feature) ;
	    break ;
	  }
	default:
	  {
	    break ;
	  }
	}
    }

  /* If there is any extra information then do the merge. */
  if (extras_notebook)
    zMapGUINotebookMergeNotebooks(feature_book, extras_notebook) ;


  return feature_book ;
}



/* get evidence feature names from otterlace, reusing code from createFeatureBook() above
 * the show code does some complet stuff with reusable lists of windows,
 * probably to stop these accumulating
 * but as we donlt diasplay a window we don't care
 */

GList *zmapWindowFeatureGetEvidence(ZMapWindow window,ZMapFeature feature)
{
    ZMapWindowFeatureShow show;
    GList *evidence;

    show = g_new0(ZMapWindowFeatureShowStruct, 1) ;
    show->reusable = windowIsReusable() ;
    show->zmapWindow = window ;

    show->xml_parsing_status = TRUE ;
    show->xml_curr_tag = ZMAPGUI_NOTEBOOK_INVALID ;
    show->xml_curr_chapter = NULL ;
    show->xml_curr_page = NULL ;
    show->item = NULL;  /* is not used anyway */

    show->get_evidence = WANT_EVIDENCE;
    show->evidence_column = -1;     /* invalid */

    zmapWindowUpdateXRemoteDataFull(show->zmapWindow,
                              (ZMapFeatureAny)feature,
                              "feature_details",
                              show->item,
                              starts, ends, show);
    evidence = show->evidence;

    if(show->xml_curr_notebook)
      zMapGUINotebookDestroyAny((ZMapGuiNotebookAny)(show->xml_curr_notebook)) ;

    g_free(show);

    return(evidence);
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




static void destroyCB(GtkWidget *widget, gpointer data)
{
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)data ;

  g_ptr_array_remove(feature_show->zmapWindow->feature_show_windows, (gpointer)feature_show->window);

  g_free(feature_show) ;

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
        if(show->get_evidence && !g_ascii_strcasecmp(paragraph_name,"Evidence"))
            show->get_evidence = GOT_EVIDENCE;
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
              int col_ind = 0;

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
                    if(show->get_evidence && !g_ascii_strcasecmp(new_col,"Accession.SV"))
                    {
                        show->evidence_column = col_ind;
                    }
                    col_ind++;
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

  if(show->get_evidence)
    {
      show->get_evidence = WANT_EVIDENCE;
      show->evidence_column = -1;
    }

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
        int col_ind = 0;

	  /* Make a list of the names of the columns. */

	  do
	    {
	      char *new_col ;

	      if ((new_col = strtok(target, " ")))
		{
		  if (GPOINTER_TO_INT(type->data) == g_quark_from_string("bool") ||
		      GPOINTER_TO_INT(type->data) == G_TYPE_BOOLEAN)
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
		  else if (GPOINTER_TO_INT(type->data) == g_quark_from_string("int") ||
			   GPOINTER_TO_INT(type->data) == G_TYPE_INT)
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
		  else if (GPOINTER_TO_INT(type->data) == g_quark_from_string("float") ||
			   GPOINTER_TO_INT(type->data) == G_TYPE_FLOAT)
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
		  else if (GPOINTER_TO_INT(type->data) == g_quark_from_string("string") ||
			   GPOINTER_TO_INT(type->data) == G_TYPE_STRING)
		    {
		      /* Hardly worth checking but better than nothing ? */
		      if (new_col && *new_col)
			{
			  column_data = g_list_append(column_data, g_strdup(new_col)) ;
                    if(show->get_evidence == GOT_EVIDENCE && col_ind == show->evidence_column)
                    {
#if MH17_FEATURES_HAVE_PREFIXES
/*
 * feature prefixes are not desired but got added back on due to some otterlace inconsistency thing
 */
                        char *p = g_strstr_len(new_col,-1,":");
                        if(p) /* strip data type off the front */
                              new_col = p + 1;
#endif
                        show->evidence = g_list_prepend(show->evidence, GUINT_TO_POINTER(g_quark_from_string(new_col)));

                    }
			  status = TRUE ;
			}
		      else
			{
			  zMapLogWarning("Invalid string: %s", (new_col ? "No string" : "NULL string pointer")) ;
			}
		    }

              col_ind++;
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

  if ((list = zmapWindowFToIFindSameNameItems(window,window->context_to_item,
					      zMapFeatureStrand2Str(set_strand), zMapFeatureFrame2Str(set_frame),
					      feature)))
    {
      ZMapGuiNotebookParagraph paragraph ;
      GList *headers = NULL, *types = NULL ;

      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Sequence"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Strand: Sequence/Match"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Sequence Start"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Sequence End"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Match Start"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Match End"))) ;
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

  /* Align to col. header: "Strand: Sequence/Match" */
  strand = g_strdup_printf("       %s / %s        ",
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



static ZMapGuiNotebook makeTranscriptExtras(ZMapFeature feature)
{
  ZMapGuiNotebook extras_notebook = NULL ;
  ZMapGuiNotebookChapter dummy_chapter ;
  ZMapGuiNotebookPage page ;
  ZMapGuiNotebookSubsection subsection ;
  ZMapGuiNotebookParagraph paragraph ;
  GList *headers = NULL, *types = NULL ;
  int i ;

  extras_notebook = zMapGUINotebookCreateNotebook(NULL, FALSE, cleanUp, NULL) ;

  dummy_chapter = zMapGUINotebookCreateChapter(extras_notebook, NULL, NULL) ;

  page = zMapGUINotebookCreatePage(dummy_chapter, "Exons") ;


  subsection = zMapGUINotebookCreateSubsection(page, NULL) ;


  headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Start"))) ;
  headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("End"))) ;

  types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("int"))) ;
  types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("int"))) ;

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
					     ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE, headers, types) ;

  for (i = 0 ; i < feature->feature.transcript.exons->len ; i++)
    {
      ZMapSpan exon_span ;
      GList *column_data = NULL ;
      ZMapGuiNotebookTagValue tag_value ;

      exon_span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);

      column_data = g_list_append(column_data, GINT_TO_POINTER(exon_span->x1)) ;
      column_data = g_list_append(column_data, GINT_TO_POINTER(exon_span->x2)) ;

      tag_value = zMapGUINotebookCreateTagValue(paragraph,
						NULL, ZMAPGUI_NOTEBOOK_TAGVALUE_COMPOUND,
						"compound", column_data,
						NULL) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Looks like our page code probably leaks as we don't clear stuff up.... */
      g_list_free(column_data) ;
      column_data = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }


  return extras_notebook ;
}





/********************* end of file ********************************/
