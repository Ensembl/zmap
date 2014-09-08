/*  File: zmapWindowFeatureShow.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements textual display of feature details in a
 *              gtk notebook widget.
 *
 *              Contents of notebook are dynamically configurable
 *              by the controller application via our xremote
 *              interface.
 *
 * Exported functions: See ZMap/zmapWindow.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapXMLHandler.h>
#include <zmapWindow_P.h>



#define TOP_BORDER_WIDTH 5


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


typedef struct AddParaStructName
{
  ZMapWindow window ;
  ZMapGuiNotebookParagraph paragraph ;
} AddParaStruct, *AddPara ;


/* Malcolm's stuff more properly defined....... */
typedef enum
  {
    INVALID_EVIDENCE,
    WANT_EVIDENCE,                                            /* constructing the list */
    GOT_EVIDENCE                                            /* in the right paragraph */
  } EvidenceType ;


typedef struct ZMapWindowFeatureShowStruct_
{
  ZMapWindow zmapWindow ;

  gboolean reusable ;                                            /* Can this window be reused for
                                                                    a new feature. */
  gboolean editable ;                                            /* Is the window data editable. */

  FooCanvasItem *item;                                            /* The item the user called us on  */
  ZMapFeature feature;                                            /* Feature from item. */


  ZMapGuiNotebook feature_book ;

  /* EG: This makes me quite cross...polluting a stand alone file with other stuff....
   * now someone else will have to spend time cleaning it up..... */
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

  EvidenceType get_evidence ;
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
  GtkWidget *scrolled_window ;
  GtkWidget *vbox ;
  GtkWidget *vbox_notebook ;
  GtkWidget *notebook ;

  gulong notebook_mapevent_handler ;
  gulong notebook_switch_page_handler ;

  GtkWidget *curr_notebook ;
  GtkWidget *curr_page_vbox ;
  GtkWidget *curr_paragraph_vbox ;
  GtkWidget *curr_paragraph_table ;
  guint curr_paragraph_rows, curr_paragraph_columns ;

  gboolean save_failed ; /* gets set to true if saving a chapter failed */

} ZMapWindowFeatureShowStruct, *ZMapWindowFeatureShow ;



typedef struct GetEvidenceDataStructType
{
  ZMapWindowFeatureShow show ;
  ZMapWindowGetEvidenceCB evidence_cb ;
  gpointer evidence_cb_data ;
  ZMapXMLObjTagFunctions starts ;
  ZMapXMLObjTagFunctions ends ;
} GetEvidenceDataStruct, *GetEvidenceData ;


/* Struct to hold the feature details found by readChapter */
typedef struct ChapterFeatureStructType {
  char *feature_name;
  char *feature_set;
  char *CDS;
  char *start_not_found;
  char *end_not_found;
  int start;
  int end;
  GList *exons; /* each data in this list is a GList with two integer items (the start and end coords) */
} ChapterFeatureStruct, *ChapterFeature;




static void remoteShowFeature(ZMapWindowFeatureShow show) ;
static void localShowFeature(ZMapWindowFeatureShow show) ;
static void showFeature(ZMapWindowFeatureShow show, ZMapGuiNotebook extras_notebook) ;
static void replyShowFeature(ZMapWindow window, gpointer user_data,
                             char *command, RemoteCommandRCType command_rc,
                             char *reason, char *reply) ;

static ZMapWindowFeatureShow featureShowCreate(ZMapWindow window, FooCanvasItem *item, const gboolean editable) ;
static void featureShowReset(ZMapWindowFeatureShow show, ZMapWindow window, char *title) ;
static ZMapGuiNotebook createFeatureBook(ZMapWindowFeatureShow show, char *name,
                                         ZMapFeature feature, FooCanvasItem *item,
                                         ZMapGuiNotebook extras_notebook) ;

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


static void createEditWindow(ZMapWindowFeatureShow feature_show, char *title) ;
static GtkWidget *makeMenuBar(ZMapWindowFeatureShow feature_show) ;

static ZMapWindowFeatureShow findReusableShow(GPtrArray *window_list, const gboolean editable) ;
static gboolean windowIsReusable(void) ;
static ZMapFeature getFeature(FooCanvasItem *item) ;

static gboolean mapeventCB(GtkWidget *widget, GdkEvent  *event, gpointer   user_data)  ;
static void switchPageHandler(GtkNotebook *notebook, gpointer new_page, guint new_page_index, gpointer user_data) ;
static void destroyCB(GtkWidget *widget, gpointer data);

static void applyCB(ZMapGuiNotebookAny any_section, void *user_data_unused);
static void saveCB(ZMapGuiNotebookAny any_section, void *user_data_unused);
static void cancelCB(ZMapGuiNotebookAny any_section, void *user_data_unused);

static void preserveCB(gpointer data, guint cb_action, GtkWidget *widget);
static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);

static void cleanUp(ZMapGuiNotebookAny any_section, void *user_data) ;
static void getAllMatches(ZMapWindow window,
                          ZMapFeature feature, FooCanvasItem *item, ZMapGuiNotebookSubsection subsection) ;
static void addTagValue(gpointer data, gpointer user_data) ;
static ZMapGuiNotebook makeTranscriptExtras(ZMapWindow window, ZMapFeature feature, const gboolean editable) ;

static void callXRemote(ZMapWindow window, ZMapFeatureAny feature_any,
                        char *action, FooCanvasItem *real_item,
                        ZMapRemoteAppProcessReplyFunc handler_func, gpointer handler_data) ;
static void localProcessReplyFunc(gboolean reply_ok, char *reply_error,
				  char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
				  gpointer reply_handler_func_data) ;

static void getEvidenceReplyFunc(gboolean reply_ok, char *reply_error,
                                 char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
                                 gpointer reply_handler_func_data) ;

static void remoteReplyErrHandler(ZMapRemoteControlRCType error_type, char *err_msg, void *user_data) ;

static void revcompTransChildCoordsCB(gpointer data, gpointer user_data) ;

static void saveChapter(ZMapGuiNotebookChapter chapter, ChapterFeature chapter_feature, ZMapWindowFeatureShow show) ;
static void saveChapterCB(gpointer data, gpointer user_data);
static ChapterFeature readChapter(ZMapGuiNotebookChapter chapter);


/*
 *                           Globals
 */


/* menu GLOBAL! */
static GtkItemFactoryEntry menu_items_G[] =
  {
    /* File */
    { "/_File",              NULL,         NULL,       0,          "<Branch>", NULL},
    { "/File/_Preserve",      NULL,         preserveCB, 0,          NULL,       NULL},
    { "/File/_Close",         "<control>W", requestDestroyCB, 0,    NULL,       NULL},

    /* Help */
    { "/_Help",                NULL, NULL,       0,  "<LastBranch>", NULL},
    { "/Help/_Feature Display", NULL, helpMenuCB, 1,  NULL,           NULL},
    { "/Help/_About ZMap",      NULL, helpMenuCB, 2,  NULL,           NULL}
  } ;



static gboolean alert_client_debug_G = FALSE ;


/* Start and end tag handlers for converting zmap notebook style xml into a zmap notebook. */
static  ZMapXMLObjTagFunctionsStruct starts_G[] =
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

static ZMapXMLObjTagFunctionsStruct ends_G[] =
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


/* Displays a feature in a window, will reuse an existing window if it can, otherwise
 * it creates a new one. */
void zmapWindowFeatureShow(ZMapWindow window, FooCanvasItem *item, const gboolean editable)
{
  ZMapWindowFeatureShow show = NULL ;
  ZMapFeature feature ;

  if ((feature = getFeature(item)))
    {
      char *title ;
      char *feature_name ;

      /* Look for a reusable window, if we find one then that gets used. */
      show = findReusableShow(window->feature_show_windows, editable) ;

      feature_name = (char *)g_quark_to_string(feature->original_id) ;

      title = g_strdup(feature_name) ;

      if (!show)
        {
          show = featureShowCreate(window, item, editable) ;

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
      show->feature = feature ;

      /* Now show the window, if there is an xremote client we may retrieve
       * extra data from it for display to the user. */
      if (window->xremote_client)
        {
          remoteShowFeature(show) ;
        }
      else
        {
          localShowFeature(show) ;
        }
    }

  return ;
}



/*
 *                    Package routines
 */


/* mh17: get evidence feature names from otterlace, reusing code from createFeatureBook() above
 * the show code does some complex stuff with reusable lists of windows,
 * probably to stop these accumulating but as we don't display a window we don't care
 *
 * edgrif: I've restructured this to use the new xremote.
 *
 */

void zmapWindowFeatureGetEvidence(ZMapWindow window, ZMapFeature feature,
                                  ZMapWindowGetEvidenceCB evidence_cb, gpointer evidence_cb_data)
{
  GetEvidenceData evidence_data ;
  ZMapWindowFeatureShow show ;

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
  show->evidence = NULL;

  evidence_data = g_new0(GetEvidenceDataStruct, 1) ;
  evidence_data->show = show ;
  evidence_data->evidence_cb = evidence_cb ;
  evidence_data->evidence_cb_data = evidence_cb_data ;
  evidence_data->starts = starts_G ;
  evidence_data->ends = ends_G ;

  callXRemote(show->zmapWindow,
              (ZMapFeatureAny)feature,
              ZACP_DETAILS_FEATURE,
              show->item,
              getEvidenceReplyFunc, evidence_data) ;

  return ;
}





/*
 *                   Internal routines.
 */

static void remoteShowFeature(ZMapWindowFeatureShow show)
{
  callXRemote(show->zmapWindow,
              (ZMapFeatureAny)(show->feature),
              ZACP_DETAILS_FEATURE,
              show->item,
              localProcessReplyFunc, show) ;

  return ;
}



static void replyShowFeature(ZMapWindow window, gpointer user_data,
                             char *command, RemoteCommandRCType command_rc,
                             char *reason, char *reply)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)user_data ;
  ZMapXMLParser parser ;

  /* We should not need to check that it's the right command, that should be validated at
   * the app level. */

  if (command_rc != REMOTE_COMMAND_RC_OK)
    {
      if (command_rc == REMOTE_COMMAND_RC_FAILED)
        {
          /* command may legitimately fail as peer may not have any extra feature details in which
           * case we do the best we can. */
          localShowFeature(show) ;
        }
      else
        {
          /* More serious error so log and display to user. */
          char *err_msg ;

          err_msg = g_strdup_printf("Feature details display failed: %s, %s",
                                    zMapRemoteCommandRC2Str(command_rc),
                                    reason) ;
          zMapLogCritical("%s", err_msg) ;
          zMapCritical("%s", err_msg) ;
          g_free(err_msg) ;

          /* Now clear up. */
          gtk_widget_destroy(GTK_WIDGET(show->window)) ;
        }
    }
  else
    {
      /* If we have an external program driving us then ask it for any extra information.
       * This will come as xml which we decode via the callbacks listed in the following structs.
       * If that fails then we may have extra stuff to add anyway. */
      show->xml_parsing_status = TRUE ;
      show->xml_curr_tag = ZMAPGUI_NOTEBOOK_INVALID ;
      show->xml_curr_chapter = NULL ;
      show->xml_curr_page = NULL ;

      parser = zMapXMLParserCreate(show, FALSE, FALSE) ;

      zMapXMLParserSetMarkupObjectTagHandlers(parser, starts_G, ends_G) ;

      if (!(zMapXMLParserParseBuffer(parser, reply, strlen(reply))))
        {


        }
      else
        {
          showFeature(show, show->xml_curr_notebook) ;
        }

      /* Free the parser!!! */
      zMapXMLParserDestroy(parser) ;
    }

  return ;
}


static void localShowFeature(ZMapWindowFeatureShow show)
{
  ZMapGuiNotebook extras_notebook = NULL ;

  /* Add any additional info. we can, not much at the moment. */
  switch(show->feature->mode)
    {
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      {
        /* For transcripts add exons and other local stuff. */
        extras_notebook = makeTranscriptExtras(show->zmapWindow, show->feature, show->editable) ;
        break ;
      }
    default:
      {
        break ;
      }
    }

  showFeature(show, extras_notebook) ;

  return ;
}



static void showFeature(ZMapWindowFeatureShow show, ZMapGuiNotebook extras_notebook)
{
  char *feature_name ;
  GtkWidget *notebook_widg ;

  feature_name = (char *)g_quark_to_string(show->feature->original_id) ;

  /* Make the notebook. */
  show->feature_book = createFeatureBook(show, feature_name, show->feature, show->item, extras_notebook) ;

  if (show->feature_book)
    {
      /* Now make the widgets to display the pages..... */
      show->notebook = zMapGUINotebookCreateWidget(show->feature_book) ;

      /* Attach a handler to be called when the noteback tab is changed. */
      notebook_widg = zMapGUINotebookGetCurrChapterWidg(show->notebook) ;
      show->notebook_switch_page_handler = g_signal_connect(G_OBJECT(notebook_widg), "switch-page",
                                                            G_CALLBACK(switchPageHandler), show) ;

      gtk_box_pack_start(GTK_BOX(show->vbox), show->notebook, TRUE, TRUE, 0) ;

      gtk_widget_show_all(show->window) ;

      zMapGUIRaiseToTop(show->window);
    }
  else
    {
      zMapWarning("%s", "Error creating feature dialog\n") ;
    }

  return ;
}


static ZMapFeature getFeature(FooCanvasItem *item)
{
  ZMapFeature feature = NULL ;

  feature = zmapWindowItemGetFeature(item);
  if (!zMapFeatureIsValid((ZMapFeatureAny)feature))
    return NULL ;

  return feature ;
}


static ZMapWindowFeatureShow featureShowCreate(ZMapWindow window, FooCanvasItem *item, const gboolean editable)
{
  ZMapWindowFeatureShow show = NULL ;

  show = g_new0(ZMapWindowFeatureShowStruct, 1) ;
  show->reusable = windowIsReusable() ;
  show->zmapWindow = window ;
  show->editable = editable ;

  return show ;
}


static void featureShowReset(ZMapWindowFeatureShow show, ZMapWindow window, char *title)
{
  show->reusable = windowIsReusable() ;
  show->zmapWindow = window ;

  show->curr_paragraph = NULL ;
  show->vbox_notebook = show->curr_notebook = show->curr_page_vbox = show->curr_paragraph_vbox = NULL ;
  show->curr_paragraph_rows = show->curr_paragraph_columns = 0 ;

  show->item = NULL ;
  show->feature = NULL ;

  zMapGUISetToplevelTitle(show->window,"Feature Show", title) ;

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
                                         ZMapFeature feature, FooCanvasItem *item,
                                         ZMapGuiNotebook extras_notebook)
{
  ZMapGuiNotebook feature_book = NULL ;
  ZMapGuiNotebookChapter dummy_chapter = NULL ;
  ZMapGuiNotebookPage page = NULL ;
  ZMapGuiNotebookSubsection subsection = NULL ;
  ZMapGuiNotebookParagraph paragraph = NULL ;
  ZMapGuiNotebookTagValue tag_value = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  char *chapter_title = NULL, *page_title = NULL, *description = NULL ;
  char *tmp = NULL ;
  char *notes = NULL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapGuiNotebook extras_notebook = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  feature_book = zMapGUINotebookCreateNotebook(name, show->editable, cleanUp, NULL) ;

  /* The feature fundamentals page. */
  switch(feature->mode)
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
    case ZMAPSTYLE_MODE_SEQUENCE:
      {
        if (zMapFeatureSequenceIsDNA(feature))
          {
            chapter_title = "DNA Sequence" ;
            page_title = "Details" ;
          }
        else
          {
            chapter_title = "Peptide Sequence" ;
            page_title = "Details" ;
          }

        break ;
      }


    default:
      {
        zMapWarnIfReached() ;
        break ;
      }
    }


  if (feature_book && chapter_title && page_title)
    {
      ZMapGuiNotebookCBStruct user_CBs = {cancelCB, NULL, applyCB, NULL, NULL, NULL, (show->editable ? saveCB : NULL), NULL} ;
      dummy_chapter = zMapGUINotebookCreateChapter(feature_book, chapter_title, &user_CBs) ;
      page = zMapGUINotebookCreatePage(dummy_chapter, page_title) ;
      subsection = zMapGUINotebookCreateSubsection(page, "Feature") ;


      /* General Feature Descriptions. */
      paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
                                                 ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE, NULL, NULL) ;

      /*! \todo If editable is true (i.e. we have a temp feature from the Annotation column), we 
       * want the default feature name and featureset to be those of the feature the user
       * originally copied into the Annotation column, rather than the temp feature
       * details. Defaults will need to be passed through somehow because we don't want to pollute
       * this code with Annotation column stuff. */
      tag_value = zMapGUINotebookCreateTagValue(paragraph, "Feature Name", NULL,
                                                ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                "string", g_strdup(g_quark_to_string(feature->original_id)),
                                                NULL) ;

      if (show->editable)
        {
          /* When editing a feature, add an entry to allow the user to set the feature_set.
           * By default it is the Annotation column but note that if the user saves back to 
           * the annotation column then a new feature is NOT created - rather the temp feature
           * in the annotation column is overwritten. This allows the user to edit things such
           * as the CDS or exon coords of the temp feature. */
          const char *featureset_name = ZMAP_FIXED_STYLE_SCRATCH_NAME ;

          tag_value = zMapGUINotebookCreateTagValue(paragraph, "Feature Set", "Please specify the Feature Set that you would like to save the feature to",
                                                    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                    "string", featureset_name, NULL) ;
        }
      else
        {
          /* When not editing a feature show the style instead (we possibly want to show this
           * when editing as well but we'll need to make it non-editable and would want it to
           * update when the user sets the featureset */
          style = *feature->style; /* zMapFindStyle(show->zmapWindow->display_styles, feature->style_id); */

          tag_value = zMapGUINotebookCreateTagValue(paragraph, "Feature Group [style_id]", NULL,
                                                    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                    "string", g_strdup(zMapStyleGetName(style)), NULL) ;

          if ((description = zMapStyleGetDescription(style)))
            {
              tag_value = zMapGUINotebookCreateTagValue(paragraph, "Style Description", NULL,
                                                        ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,   /* SCROLLED_TEXT,*/
                                                        "string", g_strdup(description), NULL) ;
            }
        }

      if ((notes = zmapWindowFeatureDescription(feature)))
        {
          tag_value = zMapGUINotebookCreateTagValue(paragraph, "Notes", NULL,
                                                    ZMAPGUI_NOTEBOOK_TAGVALUE_SCROLLED_TEXT,
                                                    "string", g_strdup(notes), NULL) ;
        }


      /* Feature specific stuff. */
      if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
        {
          char *query_length ;

          subsection = zMapGUINotebookCreateSubsection(page, "Align") ;


          paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
                                                     ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE, NULL, NULL) ;

          tag_value = zMapGUINotebookCreateTagValue(paragraph, "Align Type", NULL,
                                                    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                    "string", g_strdup(zMapFeatureHomol2Str(feature->feature.homol.type)),
                                                    NULL) ;

          if (feature->feature.homol.length)
            query_length = g_strdup_printf("%d", feature->feature.homol.length) ;
          else
            query_length = g_strdup("<NOT SET>") ;
          tag_value = zMapGUINotebookCreateTagValue(paragraph, "Query length", NULL,
                                                    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                    "string", query_length,
                                                    NULL) ;

          /* Get a list of all the matches for this sequence.... */
          getAllMatches(show->zmapWindow, feature, item, subsection) ;
        }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

      /* ORIGINAL FEATURE SHOW CODE....I'M LEAVING THIS HERE UNTIL HAVANA DECIDE EXACTLY WHAT THEY
       * WANT...THEY ASKED TO HAVE PROPERTIES AT THE BOTTOM SO I'VE DONE THAT BY MOVING THIS SECTION
       * TO BE LAST...SEE BELOW...BUT THEY NOW THINK THIS WON'T SUIT EVERYONE.... 22/10/2013 EG */

      else if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
        {
          paragraph = zMapGUINotebookCreateParagraph(subsection, "Properties",
                                                     ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE, NULL, NULL) ;

          if (feature->feature.transcript.flags.cds)
            tmp = g_strdup_printf("%d -> %d", feature->feature.transcript.cds_start, feature->feature.transcript.cds_end) ;
          else
            tmp = g_strdup_printf("%s", NOT_SET_TEXT) ;

          tag_value = zMapGUINotebookCreateTagValue(paragraph, "CDS", NULL,
                                                    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                    "string",
                                                    tmp,
                                                    NULL) ;

          /* NOTE
           * in Otterlace start not found is displayed as <not set> or 1 or 2 or 3
           * GFF specifies . or 0/1/2
           * so we present a 'human' number here not what's specified in GFF
           * See RT 271175 if this is wrong then that ticket needs to be revived and otterlace changed
           * or for havana to accept the different numbers
           * refer to other calls to zMapFeaturePhase2Str()
           */
          if (feature->feature.transcript.flags.start_not_found)
            tmp = g_strdup_printf("%s", zMapFeaturePhase2Str(feature->feature.transcript.start_not_found)) ;
          else
            tmp = g_strdup_printf("%s", NOT_SET_TEXT) ;

          tag_value = zMapGUINotebookCreateTagValue(paragraph, "Start Not Found", NULL,
                                                    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                    "string",
                                                    tmp,
                                                    NULL) ;

          if (feature->feature.transcript.flags.end_not_found)
            tmp = g_strdup_printf("%s", SET_TEXT) ;
          else
            tmp = g_strdup_printf("%s", NOT_SET_TEXT) ;

          tag_value = zMapGUINotebookCreateTagValue(paragraph, "End Not Found", NULL,
                                                    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                    "string",
                                                    tmp,
                                                    NULL) ;
        }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      /* If there is any extra information then do the merge. */
      if (extras_notebook)
        zMapGUINotebookMergeNotebooks(feature_book, extras_notebook) ;



      /* TRY DOING IT HERE FOR HAVANA REQUEST....SEE COMMENTS ABOVE... */
      if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
        {
          /* Report cds, start/end not found if they are set. If editable is true
           * then show them so the user can set them */
          if (feature->feature.transcript.flags.cds
              || feature->feature.transcript.flags.start_not_found || feature->feature.transcript.flags.end_not_found
              || show->editable)
            {
              subsection = zMapGUINotebookCreateSubsection(page, "Properties") ;


              paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
                                                         ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE, NULL, NULL) ;

              if (feature->feature.transcript.flags.cds)
                {
                  int display_start, display_end ;

                  zmapWindowCoordPairToDisplay(show->zmapWindow,
                                               feature->feature.transcript.cds_start,
                                               feature->feature.transcript.cds_end,
                                               &display_start, &display_end) ;

                  tmp = g_strdup_printf("%d, %d", display_start, display_end) ;
                }
              else
                {
                  tmp = g_strdup_printf("%s", NOT_SET_TEXT) ;
                }

              tag_value = zMapGUINotebookCreateTagValue(paragraph, "CDS (start, end)", NULL,
                                                        ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                        "string",
                                                        tmp,
                                                        NULL) ;

              /* NOTE
               * in Otterlace start not found is displayed as <not set> or 1 or 2 or 3
               * GFF specifies . or 0/1/2
               * so we present a 'human' number here not what's specified in GFF
               * See RT 271175 if this is wrong then that ticket needs to be revived and otterlace changed
               * or for havana to accept the different numbers
               * refer to other calls to zMapFeaturePhase2Str()
               */
              if (feature->feature.transcript.flags.start_not_found)
                tmp = g_strdup_printf("%s", zMapFeaturePhase2Str(feature->feature.transcript.start_not_found)) ;
              else
                tmp = g_strdup_printf("%s", NOT_SET_TEXT) ;

              tag_value = zMapGUINotebookCreateTagValue(paragraph, "Start Not Found", NULL,
                                                        ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                        "string",
                                                        tmp,
                                                        NULL) ;

              if (feature->feature.transcript.flags.end_not_found)
                tmp = g_strdup_printf("%s", SET_TEXT) ;
              else
                tmp = g_strdup_printf("%s", NOT_SET_TEXT) ;

              tag_value = zMapGUINotebookCreateTagValue(paragraph, "End Not Found", NULL,
                                                        ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                                        "string",
                                                        tmp,
                                                        NULL) ;
            }
        }
    }

  return feature_book ;
}


/* Handles the response to the feature-show dialog */
void featureShowDialogResponseCB(GtkDialog *dialog, gint response_id, gpointer data)
{
  switch (response_id)
  {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_APPLY:
      saveCB(NULL, data) ;
      break;
      
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_REJECT:
      cancelCB(NULL, data) ;
      break;
      
    default:
      break;
  };
}


/* Create the feature display window, this can get very long if our peer (e.g. otterlace)
 * returns a lot of information so we need scrolling. */
static void createEditWindow(ZMapWindowFeatureShow feature_show, char *title)
{
  GtkWidget *scrolled_window, *vbox ;

  /* Create the edit window. */
  if (feature_show->editable)
    {
      /* Note that we don't use the passed-in title which is the feature name as this is a temp
       * name and not relevant */
      feature_show->window = zMapGUIDialogNew("Create Feature", NULL, G_CALLBACK(featureShowDialogResponseCB), feature_show) ;
      gtk_dialog_add_button(GTK_DIALOG(feature_show->window), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL) ;
      gtk_dialog_add_button(GTK_DIALOG(feature_show->window), GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT) ;
      gtk_dialog_set_default_response(GTK_DIALOG(feature_show->window), GTK_RESPONSE_ACCEPT) ;
    }
  else
    {
      feature_show->window = zMapGUIDialogNew("Feature Details", title, G_CALLBACK(featureShowDialogResponseCB), feature_show) ;
      gtk_dialog_add_button(GTK_DIALOG(feature_show->window), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE) ;
      gtk_dialog_set_default_response(GTK_DIALOG(feature_show->window), GTK_RESPONSE_CLOSE) ;
    }

  vbox = GTK_DIALOG(feature_show->window)->vbox ;

  gtk_container_set_border_width(GTK_CONTAINER (feature_show->window), TOP_BORDER_WIDTH) ;

  g_object_set_data(G_OBJECT(feature_show->window), "zmap_feature_show", feature_show) ;
  g_signal_connect(G_OBJECT(feature_show->window), "destroy", G_CALLBACK(destroyCB), feature_show) ;

  /* Attach handler to be called when we are displayed, currently this attempts to size
   * the window correctly. */
  feature_show->notebook_mapevent_handler = g_signal_connect(G_OBJECT(feature_show->window), "map-event",
                                                             G_CALLBACK(mapeventCB), feature_show) ;

  /* Add ptr so our parent ZMapWindow knows about us and can destroy us when its deleted. */
  g_ptr_array_add(feature_show->zmapWindow->feature_show_windows, (gpointer)(feature_show->window)) ;


  /* Top level vbox containing the menu bar and below it in a scrolled window all
   * the feature details. */
  gtk_box_pack_start(GTK_BOX(vbox), makeMenuBar(feature_show), FALSE, FALSE, 0);


  /* Annotators asked for an overall scrolled window for feature details. */
  feature_show->scrolled_window = scrolled_window = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC) ;
  gtk_container_add(GTK_CONTAINER(vbox), scrolled_window) ;


  /* vbox contains all feature details, must be added with a viewport because vbox does not have
   * its own window. */
  feature_show->vbox = vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_box_set_spacing(GTK_BOX(vbox), 5) ;
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vbox) ;


  return ;
}



/* Called when we are first mapped to set initial size of window. */
static gboolean mapeventCB(GtkWidget *widget, GdkEvent  *event, gpointer   user_data)
{
  gboolean event_handled = FALSE ;                            /* make sure event is propagated. */
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)user_data ;
  int top_width, top_height, vbox_width, vbox_height ;
  int max_width, max_height ;
  GtkRequisition size_req ;
  float width_proportion = 0.8, height_proportion = 0.8 ;   /* Users don't want the window to
                                                               occupy too much space. */

  /* Disconnect the handler, we only do this the first time the window is shown. */
  g_signal_handler_disconnect(feature_show->window, feature_show->notebook_mapevent_handler) ;
  feature_show->notebook_mapevent_handler = 0 ;

  /* Get max possible window size and adjust for users preference. */
  zMapGUIGetMaxWindowSize(feature_show->window, &max_width, &max_height) ;
  max_width *= width_proportion ;
  max_height *= height_proportion ;


  top_width = top_height = vbox_width = vbox_height = 0 ;

  /* Get size of top level window. */
  gtk_window_get_size(GTK_WINDOW(feature_show->window), &top_width, &top_height) ;

  /* Get size of our vbox containing all the info. allowing for the external border
   * we set for the vbox. */
  gtk_widget_size_request(feature_show->vbox, &size_req) ;
  vbox_width = size_req.width + (2 * TOP_BORDER_WIDTH) ;
  vbox_height = size_req.height + (2 * TOP_BORDER_WIDTH) ;

  /* Unfortunately there seems to be some extra border, perhaps from the scrolled window
   * or viewport....and in fact the scrolled window frequently is shown....not sure why,
   * big fudge factor here...seems to require more on the mac than gnu desktop. */
  vbox_width += 60 ;
  vbox_height += 80 ;


  /* If vbox is a different size than top window then we should expand/contract top window to
   * be a more sensible size on the screen but not exceeding the max window size. */
  if (vbox_width != top_width || vbox_height != top_height)
    {
      if (vbox_width != top_width)
        top_width = vbox_width ;

      if (top_width > max_width)
        top_width = max_width ;

      if (vbox_height != top_height)
        top_height = vbox_height ;

      if (top_height > max_height)
        top_height = max_height ;

      gtk_window_resize(GTK_WINDOW(feature_show->window), top_width, top_height) ;
    }

  return event_handled ;
}


/* Called each time the user switches tabs, I've left this in because we may want to implement
 * resizing of the window each time they switch a tab.... */
static void switchPageHandler(GtkNotebook *notebook, gpointer new_page, guint new_page_index, gpointer user_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)user_data ;

  printf("in notebook switch page handler\n") ;


  printf("leaving notebook switch page handler\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


/* Called when feature details window is destroyed, either by user or by
 * ZMapWindow being destroyed. */
static void destroyCB(GtkWidget *widget, gpointer data)
{
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)data ;

  g_ptr_array_remove(feature_show->zmapWindow->feature_show_windows, (gpointer)feature_show->window);

  /* Now free the feature_book, may be NULL if there was an error during creation. */
  if (feature_show->feature_book)
    zMapGUINotebookDestroyNotebook(feature_show->feature_book) ;

  g_free(feature_show) ;

  return ;
}



/* make the menu from the global defined above ! */
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


static void applyCB(ZMapGuiNotebookAny any_section, void *user_data_unused)
{

  return ;
}

static void saveCB(ZMapGuiNotebookAny any_section, void *data)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)data ;

  if (show && show->feature_book && show->feature_book->chapters)
    {
      show->save_failed = FALSE ;

      g_list_foreach(show->feature_book->chapters, saveChapterCB, show);

      /* only destroy the dialog if the save succeeded */
      if (!show->save_failed)
        gtk_widget_destroy(GTK_WIDGET(show->window)) ;
    }

  return ;
}

static void cancelCB(ZMapGuiNotebookAny any_section, void *data)
{
  ZMapWindowFeatureShow feature_show = (ZMapWindowFeatureShow)data;

  gtk_widget_destroy(GTK_WIDGET(feature_show->window)) ;

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
      zMapWarning("%s", "Unexpected menu callback action\n");
      zMapWarnIfReached() ;
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
static ZMapWindowFeatureShow findReusableShow(GPtrArray *window_list, const gboolean editable)
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
          if (show->reusable && show->editable == editable)
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
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data);

  printWarning("response", "start") ;

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_INVALID ;

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
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data) ;

  printWarning("notebook", "start") ;

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

  return TRUE;
}

static gboolean xml_notebook_end_cb(gpointer user_data, ZMapXMLElement element,
                                    ZMapXMLParser parser)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data);

  printWarning("notebook", "end") ;

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_INVALID ;

  return TRUE;
}



static gboolean xml_chapter_start_cb(gpointer user_data, ZMapXMLElement element,
                                     ZMapXMLParser parser)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data) ;

  printWarning("chapter", "start") ;

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

  return TRUE;
}

static gboolean xml_chapter_end_cb(gpointer user_data, ZMapXMLElement element,
                                   ZMapXMLParser parser)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data) ;

  printWarning("chapter", "end") ;

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_BOOK ;

  return TRUE;
}




static gboolean xml_page_start_cb(gpointer user_data, ZMapXMLElement element,
                                  ZMapXMLParser parser)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data);
  ZMapXMLAttribute attr = NULL ;
  char *page_name = NULL ;

  printWarning("page", "start") ;

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

  return TRUE;
}

static gboolean xml_page_end_cb(gpointer user_data, ZMapXMLElement element,
                                ZMapXMLParser parser)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data);

  printWarning("page", "end") ;

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_CHAPTER ;

  return TRUE;
}


static gboolean xml_subsection_start_cb(gpointer user_data, ZMapXMLElement element,
                                        ZMapXMLParser parser)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data);
  ZMapXMLAttribute attr = NULL ;
  char *subsection_name = NULL ;

  printWarning("subsection", "start") ;

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

  return TRUE ;
}

static gboolean xml_subsection_end_cb(gpointer user_data, ZMapXMLElement element,
                                      ZMapXMLParser parser)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data);

  printWarning("subsection", "end") ;

  show->xml_curr_tag = ZMAPGUI_NOTEBOOK_PAGE ;

  return TRUE;
}




static gboolean xml_paragraph_start_cb(gpointer user_data, ZMapXMLElement element,
                                       ZMapXMLParser parser)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data);
  ZMapXMLAttribute attr = NULL ;

  printWarning("paragraph", "start") ;

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
                  char *curr_pos = NULL ;
                  gboolean found = TRUE ;
                  int col_ind = 0;

                  target = columns = zMapXMLUtilsUnescapeStrdup(zMapXMLAttributeValueToStr(attr) );

                  /* We need to parse out the titles..... */
                  do
                    {
                      char *new_col ;

                      if ((new_col = strtok_r(target, "'", &curr_pos)))
                        {
                          target = NULL ;                   /* required for repeated strtok_r calls. */

                          /* HACK TO CHECK THE THING IS NOT BLANK....remove when xml fixed... */
                          if (*new_col == ' ')
                            continue ;

                          headers = g_list_append(headers,
                                                  GINT_TO_POINTER(g_quark_from_string(new_col))) ;



                          if(show->get_evidence && !g_ascii_strcasecmp(new_col,"Accession.SV"))
                            {
                              show->evidence_column = col_ind;
                            }

                          col_ind++;

                          
                        }
                      else
                        {
                          found = FALSE ;
                        }

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

                  g_free(columns);
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
                      char *target,*columns ;
                      char *curr_pos = NULL ;
                      GList *column_data = NULL ;

                      target = columns = zMapXMLUtilsUnescapeStrdup(zMapXMLAttributeValueToStr(attr) );

                      do
                        {
                          char *new_col ;

                          if ((new_col = strtok_r(target, " ", &curr_pos)))
                            {
                              target = NULL ;

                              column_data = g_list_append(column_data,
                                                          GINT_TO_POINTER(g_quark_from_string(new_col))) ;
                            }
                          else
                            {
                              found = FALSE ;
                            }
                        } while (found) ;

                      types = column_data ;

                      g_free(columns);
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
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data);

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
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data);
  ZMapXMLAttribute attr = NULL ;

  printWarning("tagvalue", "start") ;

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
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)(user_data);
  char *content ;

  printWarning("tagvalue", "end") ;

  if ((content = zMapXMLElementStealContent(element)))
    {
      GList *type ;
      if (show->xml_curr_type != ZMAPGUI_NOTEBOOK_TAGVALUE_COMPOUND)
        {
          show->xml_curr_tagvalue = zMapGUINotebookCreateTagValue(show->xml_curr_paragraph,
                                                                  show->xml_curr_tagvalue_name, NULL, show->xml_curr_type,
                                                                  "string", content,
                                                                  NULL) ;

          show->evidence = g_list_prepend(show->evidence, GUINT_TO_POINTER(g_quark_from_string(content)));
        }
      else if((type = g_list_first(show->xml_curr_paragraph->compound_types)))
        {
          gboolean found = TRUE ;
          char * columns = zMapXMLUtilsUnescapeStrdup(content);
          char *target = columns ;
          GList *column_data = NULL ;
          int col_ind = 0;
          char *curr_pos = NULL ;

          /* Make a list of the names of the columns. */

          do
            {
              char *new_col ;

              if ((new_col = strtok_r(target, " ", &curr_pos)))
                {
                  target = NULL ;                           /* For repeat strtok_r calls. */

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
                }
              else
                {
                  found = FALSE ;
                }

            } while (found) ;

          show->xml_curr_tagvalue = zMapGUINotebookCreateTagValue(show->xml_curr_paragraph,
                                                                  show->xml_curr_tagvalue_name, NULL, show->xml_curr_type,
                                                                  "compound", column_data,
                                                                  NULL) ;
          g_free(columns);
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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapXMLTagHandler message = (ZMapXMLTagHandler)(tag_handler);
  ZMapXMLElement mess_element = NULL;

  if ((mess_element = zMapXMLElementGetChildByName(element, "message"))
      && !message->error_message && mess_element->contents->str)
    {
      message->error_message = g_strdup(mess_element->contents->str);
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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
  if (!result)
    return ;

  if ((list = zmapWindowFToIFindSameNameItems(window, window->context_to_item,
                                              zMapFeatureStrand2Str(set_strand), zMapFeatureFrame2Str(set_frame),
                                              feature)))
    {
      ZMapGuiNotebookParagraph paragraph ;
      GList *headers = NULL, *types = NULL ;
      AddParaStruct para_data ;

      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Strand: Sequence/Match"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Sequence Start"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Sequence End"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Match Start"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Match End"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Score"))) ;
      headers = g_list_append(headers, GINT_TO_POINTER(g_quark_from_string("Percent ID"))) ;

      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("string"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("int"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("int"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("int"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("int"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("float"))) ;
      types = g_list_append(types, GINT_TO_POINTER(g_quark_from_string("float"))) ;


      /* Would like to have the option to make this just a vertically scrolled window but
       * that results in a blank area....I'm not sure why....searching on the web
       * shows nothing, see comments for this function in zmapGUINotebook.c  */
      paragraph = zMapGUINotebookCreateParagraph(subsection, "Matches",
                                                 ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE, headers, types) ;

      para_data.window = window ;
      para_data.paragraph = paragraph ;
      g_list_foreach(list, addTagValue, &para_data) ;
    }

  return ;
}

/* GFunc() to create a tagvalue item for the supplied alignment feature. */
static void addTagValue(gpointer data, gpointer user_data)
{
  ID2Canvas id2c = (ID2Canvas) data;
  //  FooCanvasItem *item = (FooCanvasItem *) id2c->item ;
  /*! \todo #warning need to revisit this when alignments get done as composite/ column items, need function for item/feature bounds */

  AddPara para_data = (AddPara)user_data ;
  ZMapGuiNotebookParagraph paragraph = para_data->paragraph ;
  ZMapFeature feature ;
  GList *column_data = NULL ;
  ZMapGuiNotebookTagValue tagvalue ;
  int tmp = 0 ;
  //  char *clone_id,
  char *strand ;
  int display_start, display_end ;


  feature = (ZMapFeature)  id2c->feature_any;        // getFeature(item) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (feature->feature.homol.flags.has_clone_id)
    clone_id = g_strdup(g_quark_to_string(feature->feature.homol.clone_id)) ;
  else
    clone_id = g_strdup("<NOT SET>") ;
  column_data = g_list_append(column_data, clone_id) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Align to col. header: "Strand: Sequence/Match" */
  strand = g_strdup_printf("       %s / %s        ",
                           zMapFeatureStrand2Str(zmapWindowStrandToDisplay(para_data->window, feature->strand)),
                           zMapFeatureStrand2Str(feature->feature.homol.strand)) ;
  column_data = g_list_append(column_data, strand) ;

  zmapWindowCoordPairToDisplay(para_data->window, feature->x1, feature->x2,
                               &display_start, &display_end) ;

  column_data = g_list_append(column_data, GINT_TO_POINTER(display_start)) ;
  column_data = g_list_append(column_data, GINT_TO_POINTER(display_end)) ;
  column_data = g_list_append(column_data, GINT_TO_POINTER(feature->feature.homol.y1)) ;
  column_data = g_list_append(column_data, GINT_TO_POINTER(feature->feature.homol.y2)) ;

  /* Hack: we assume size(float) <= size(int) && size(float) == 4 and load the float into an int. */
  memcpy(&tmp, &(feature->score), 4) ;
  column_data = g_list_append(column_data, GINT_TO_POINTER(tmp)) ;

  memcpy(&tmp, &(feature->feature.homol.percent_id), 4) ;
  column_data = g_list_append(column_data, GINT_TO_POINTER(tmp)) ;


  tagvalue = zMapGUINotebookCreateTagValue(paragraph,
                                           NULL, NULL, ZMAPGUI_NOTEBOOK_TAGVALUE_COMPOUND,
                                           "compound", column_data,
                                           NULL) ;
  return ;
}



static ZMapGuiNotebook makeTranscriptExtras(ZMapWindow window, ZMapFeature feature, const gboolean editable)
{
  ZMapGuiNotebook extras_notebook = NULL ;
  ZMapGuiNotebookChapter dummy_chapter = NULL ;
  ZMapGuiNotebookPage page = NULL ;
  ZMapGuiNotebookSubsection subsection = NULL ;
  ZMapGuiNotebookParagraph paragraph = NULL ;
  GList *headers = NULL, *types = NULL ;
  int i = 0 ;

  extras_notebook = zMapGUINotebookCreateNotebook(NULL, editable, cleanUp, NULL) ;

  if (extras_notebook)
    {
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
          int display_start, display_end, index ;

          if (window->flags[ZMAPFLAG_REVCOMPED_FEATURES])
            index = feature->feature.transcript.exons->len - (i + 1) ;
          else
            index = i ;

          exon_span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, index);

          zmapWindowCoordPairToDisplay(window, exon_span->x1, exon_span->x2,
                                       &display_start, &display_end) ;

          column_data = g_list_append(column_data, GINT_TO_POINTER(display_start)) ;
          column_data = g_list_append(column_data, GINT_TO_POINTER(display_end)) ;

          char *tag_name = g_strdup_printf("Exon%d", i) ;

          tag_value = zMapGUINotebookCreateTagValue(paragraph,
                                                    tag_name, NULL, ZMAPGUI_NOTEBOOK_TAGVALUE_COMPOUND,
                                                    "compound", column_data,
                                                    NULL) ;

          g_free(tag_name) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* Looks like our page code probably leaks as we don't clear stuff up.... */
          g_list_free(column_data) ;
          column_data = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
        }
    }


  return extras_notebook ;
}





/* It is probably just about worth having this here as a unified place to handle requests but
 * that may need revisiting.... */
static void callXRemote(ZMapWindow window, ZMapFeatureAny feature_any,
                        char *action, FooCanvasItem *real_item,
                         ZMapRemoteAppProcessReplyFunc handler_func, gpointer handler_data)
{
  ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
  ZMapXMLUtilsEventStack xml_elements ;
  ZMapWindowSelectStruct select = {0} ;
  ZMapFeatureSetStruct feature_set = {0} ;
  ZMapFeatureSet multi_set ;
  ZMapFeature feature_copy ;
  int chr_bp ;

  /* We should only ever be called with a feature, not a set or anything else. */
  if (feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE)
    return ;


  /* OK...IN HERE IS THE PLACE FOR THE HACK FOR COORDS....NEED TO COPY FEATURE
   * AND INSERT NEW CHROMOSOME COORDS...IF WE CAN DO THIS FOR THIS THEN WE
   * CAN HANDLE VIEW FEATURE STUFF IN SAME WAY...... */
  feature_copy = (ZMapFeature)zMapFeatureAnyCopy(feature_any) ;
  feature_copy->parent = feature_any->parent ;            /* Copy does not do parents so we fill in. */


  /* REVCOMP COORD HACK......THIS HACK IS BECAUSE OUR COORD SYSTEM IS MUCKED UP FOR
   * REVCOMP'D FEATURES..... */
  /* Convert coords */
  if (window->flags[ZMAPFLAG_REVCOMPED_FEATURES])
    {
      /* remap coords to forward strand range and also swop
       * them as they get reversed in revcomping.... */
      chr_bp = feature_copy->x1 ;
      chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
      feature_copy->x1 = chr_bp ;


      chr_bp = feature_copy->x2 ;
      chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
      feature_copy->x2 = chr_bp ;

      zMapUtilsSwop(int, feature_copy->x1, feature_copy->x2) ;

      if (feature_copy->strand == ZMAPSTRAND_FORWARD)
        feature_copy->strand = ZMAPSTRAND_REVERSE ;
      else
        feature_copy->strand = ZMAPSTRAND_FORWARD ;


      if (ZMAPFEATURE_IS_TRANSCRIPT(feature_copy))
        {
          if (!zMapFeatureTranscriptChildForeach(feature_copy, ZMAPFEATURE_SUBPART_EXON,
                                                 revcompTransChildCoordsCB, window)
              || !zMapFeatureTranscriptChildForeach(feature_copy, ZMAPFEATURE_SUBPART_INTRON,
                                                    revcompTransChildCoordsCB, window))
            zMapLogCritical("RemoteControl error revcomping coords for transcript %s",
                            zMapFeatureName((ZMapFeatureAny)(feature_copy))) ;

          zMapFeatureTranscriptSortExons(feature_copy) ;
        }
    }

  /* Streuth...why doesn't this use a 'creator' function...... */
#ifdef FEATURES_NEED_MAGIC
  feature_set.magic       = feature_copy->magic ;
#endif
  feature_set.struct_type = ZMAPFEATURE_STRUCT_FEATURESET;
  feature_set.parent      = feature_copy->parent->parent;
  feature_set.unique_id   = feature_copy->parent->unique_id;
  feature_set.original_id = feature_copy->parent->original_id;

  feature_set.features = g_hash_table_new(NULL, NULL) ;
  g_hash_table_insert(feature_set.features, GINT_TO_POINTER(feature_copy->unique_id), feature_copy) ;

  multi_set = &feature_set ;


  /* I don't get this at all... */
  select.type = ZMAPWINDOW_SELECT_DOUBLE;

  /* Set up xml/xremote request. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  select.xml_handler.zmap_action = g_strdup(action);

  select.xml_handler.xml_events = zMapFeatureAnyAsXMLEvents((ZMapFeatureAny)(multi_set), ZMAPFEATURE_XML_XREMOTE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  xml_elements = zMapFeatureAnyAsXMLEvents(feature_copy) ;

  /* free feature_copy, remove reference to parent otherwise we are removed from there. */
  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      feature_copy->parent = NULL ;
      zMapFeatureDestroy(feature_copy) ;
    }


  if (feature_set.unique_id)
    {
      g_hash_table_destroy(feature_set.features) ;
      feature_set.features = NULL ;
    }


  /* HERE VIEW IS CALLED WHICH IS FINE.....BUT.....WE NOW NEED ANOTHER CALL WHERE
     WINDOW CALLS REMOTE TO MAKE THE REQUEST.....BECAUSE VIEW WILL NO LONGER BE
     DOING THIS...*/
  (*(window->caller_cbs->select))(window, window->app_data, &select) ;



  /* Send request to peer program. */
  (*(window_cbs_G->remote_request_func))(window_cbs_G->remote_request_func_data,
					 window,
					 action, xml_elements,
					 handler_func, handler_data,
                                         remoteReplyErrHandler, handler_data) ;

  return ;
}



/* Handles replies from remote program to commands sent from this layer. */
static void localProcessReplyFunc(gboolean reply_ok, char *reply_error,
                                  char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
                                  gpointer reply_handler_func_data)
{
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)reply_handler_func_data ;

  if (!reply_ok)
    {
      zMapCritical("Bad reply from peer: \"%s\"", reply_error) ;
    }
  else
    {
      replyShowFeature(show->zmapWindow, show,
                       command, command_rc, reason, reply) ;
    }

  return ;
}



/* zmapWindowFeatureGetEvidence() makes an xremote call to the zmap peer to get evidence
 * for a feature, once the peer replies the reply is sent here and we parse out the
 * evidence data and then pass that back to our callers callback routine. */
static void getEvidenceReplyFunc(gboolean reply_ok, char *reply_error,
                                 char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
                                 gpointer reply_handler_func_data)
{
  GetEvidenceData evidence_data = (GetEvidenceData)reply_handler_func_data ;

  if (!reply_ok)
    {
      zMapLogCritical("Bad reply from peer: \"%s\"", reply_error) ;
    }
  else
    {
      if (command_rc == REMOTE_COMMAND_RC_OK)
        {
          ZMapXMLParser parser ;
          gboolean parses_ok ;

          /* Create the parser and call set up our start/end handlers to parse the xml reply. */
          parser = zMapXMLParserCreate(evidence_data->show, FALSE, FALSE);

          zMapXMLParserSetMarkupObjectTagHandlers(parser, evidence_data->starts, evidence_data->ends) ;

          if ((parses_ok = zMapXMLParserParseBuffer(parser,
                                                    reply,
                                                    strlen(reply))) != TRUE)
            {
              zMapLogWarning("Parsing error : %s", zMapXMLParserLastErrorMsg(parser));

            }
          else
            {
              /* The parse worked so pass back the list of evidence to our caller. */
              (evidence_data->evidence_cb)(evidence_data->show->evidence, evidence_data->evidence_cb_data) ;
            }

          zMapXMLParserDestroy(parser);

          g_free(evidence_data->show) ;
          g_free(evidence_data) ;
        }
      else
        {
          char *err_msg ;

          err_msg = g_strdup_printf("Fetching of feature data from external client failed: %s",
                                    reason) ;

          zMapWarning("%s", err_msg) ;
          zMapLogWarning("%s", err_msg) ;
        }
    }

  return ;
}


/* HACK....function to remap coords to forward strand range and also swop
 * them as they get reversed in revcomping.... */
static void revcompTransChildCoordsCB(gpointer data, gpointer user_data)
{
  ZMapSpan child = (ZMapSpan)data ;
  ZMapWindow window = (ZMapWindow)user_data ;
  int chr_bp ;

  chr_bp = child->x1 ;
  chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
  child->x1 = chr_bp ;


  chr_bp = child->x2 ;
  chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
  child->x2 = chr_bp ;

  zMapUtilsSwop(int, child->x1, child->x2) ;

  return ;
}


static ChapterFeature readChapter(ZMapGuiNotebookChapter chapter)
{
  ChapterFeature result = g_new0(ChapterFeatureStruct, 1) ;

  ZMapGuiNotebookPage page ;
  char *string_value = NULL ;
  GList *column_data = NULL ;

  if ((page = zMapGUINotebookFindPage(chapter, "Details")))
    {
      if (zMapGUINotebookGetTagValue(page, "Feature Name", "string", &string_value))
        result->feature_name = g_strdup(string_value);

      if (zMapGUINotebookGetTagValue(page, "Feature Set", "string", &string_value))
        result->feature_set = g_strdup(string_value);

      if (zMapGUINotebookGetTagValue(page, "CDS (start, end)", "string", &string_value))
        result->CDS = g_strdup(string_value);

      if (zMapGUINotebookGetTagValue(page, "Start Not Found", "string", &string_value))
        result->start_not_found = g_strdup(string_value);

      if (zMapGUINotebookGetTagValue(page, "End Not Found", "string", &string_value))
        result->end_not_found = g_strdup(string_value);
    }

  if ((page = zMapGUINotebookFindPage(chapter, "Exons")))
    {
      int i = 0;
      gboolean done = FALSE;
      
      do
        {
          char *tag_name = g_strdup_printf("Exon%d", i) ;

          if (zMapGUINotebookGetTagValue(page, tag_name, "compound", &column_data))
            result->exons = g_list_append(result->exons, column_data) ;
          else
            done = TRUE ; /* no more tags */

          g_free(tag_name) ;
          ++i ;

        } while(!done) ;
    }


  return result ;
}


/* Read and save the given chapter */
static void saveChapterCB(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)data ;
  ZMapWindowFeatureShow show = (ZMapWindowFeatureShow)user_data ;

  ChapterFeature chapter_feature = readChapter(chapter) ;
  saveChapter(chapter, chapter_feature, show) ;
}


static ZMapFeatureSet getFeaturesetFromName(ZMapWindow window, char *name)
{
  GQuark column_id = zMapFeatureSetCreateID(name) ;
  ZMapFeatureSet feature_set = zmapFeatureContextGetFeaturesetFromId(window->feature_context, column_id) ;
  return feature_set ;
}


/* Create a feature from the details in chapter_feature */
/*! \todo Check that this is a transcript */
static void saveChapter(ZMapGuiNotebookChapter chapter, ChapterFeature chapter_feature, ZMapWindowFeatureShow show)
{
  zMapReturnIfFail(chapter_feature) ;

  GError *error = NULL ;
  gboolean ok = TRUE ;

  ZMapFeature feature = NULL ;
  ZMapWindow window = show->zmapWindow ;
  ZMapFeatureSet feature_set = getFeaturesetFromName(window, chapter_feature->feature_set) ;
  ZMapFeatureTypeStyle style = NULL ;
  gboolean revcomp = FALSE ;
  int offset = window->feature_context->parent_span.x1 - 1 ; /* need to convert user coords to chromosome coords */

  if (window->flags[ZMAPFLAG_REVCOMPED_FEATURES])
    {
      revcomp = TRUE ;
      offset = window->feature_context->parent_span.x2 - offset + 1 ; /* inverts coords when revcomp'd */
    }

  if (feature_set)
    {
      style = feature_set->style ;
    }
  else
    {
      ok = FALSE ;
      g_set_error(&error, g_quark_from_string("ZMap"), 99, "Please specify a Feature Set") ;
    }

  if (ok)
    {
      /* Create the feature with default values */
      feature = zMapFeatureCreateFromStandardData(chapter_feature->feature_name,
                                                  NULL,
                                                  NULL,
                                                  ZMAPSTYLE_MODE_TRANSCRIPT,
                                                  &style,
                                                  0, /* start */
                                                  0, /* end */
                                                  FALSE,
                                                  0.0,
                                                  ZMAPSTRAND_FORWARD);
    }

  if (ok)
    {
      zMapFeatureTranscriptInit(feature);
      //zMapFeatureAddTranscriptStartEnd(feature, chapter_feature->start_not_found, 0, chapter_feature->end_not_found);

      if (chapter_feature->CDS)
        {
          const int cds_start = atoi(chapter_feature->CDS) + offset ;
          char *cp = strchr(chapter_feature->CDS, ',') ;
      
          if (strcmp(chapter_feature->CDS, NOT_SET_TEXT) == 0)
            {
              /* Not set - ignore */
            }
          else if (cp && cp + 1)
            {
              const int cds_end = atoi(cp + 1) + offset ;
              zMapFeatureAddTranscriptCDS(feature, TRUE, cds_start, cds_end) ;
            }
          else
            {
              ok = FALSE ;
              g_set_error(&error, g_quark_from_string("ZMap"), 99, "Invalid format in CDS field. Should be: [start],[end]") ;
            }
        }
    }

  if (ok)
    {
      GList *item = chapter_feature->exons ;
      for ( ; item; item = item->next)
        {
          GList *compound = (GList*)(item->data) ;
          if (g_list_length(compound) == 2)
            {
              ZMapSpan exon = g_new0(ZMapSpanStruct, 1) ;
              const int start = GPOINTER_TO_INT(compound->data) ;
              const int end = GPOINTER_TO_INT(compound->next->data) ;
              
              if (revcomp && start < 0 && end < 0)
                {
                  /* Coords are shown as negative when revcomp'd - need to un-negate them */
                  exon->x1 = end + offset ;
                  exon->x2 = start + offset ;
                }
              else
                {
                  exon->x1 = start + offset ;
                  exon->x2 = end + offset ;
                }

              zMapFeatureAddTranscriptExonIntron(feature, exon, NULL) ;

              /* Update the feature ID because the coords may have changed */
              feature->unique_id = zMapFeatureCreateID(feature->mode,
                                                       (char *)g_quark_to_string(feature->original_id),
                                                       feature->strand,
                                                       feature->x1,
                                                       feature->x2,
                                                       0, 0);
            }
          else
            {
              ok = FALSE ;
              g_set_error(&error, g_quark_from_string("ZMap"), 99, "Invalid format for exon coords") ;
            }
        }
    }

  if (ok)
    {
      zMapFeatureTranscriptRecreateIntrons(feature) ;
    }

  if (ok)
    {
      ZMapWindowMergeNewFeatureStruct merge = {feature, feature_set, NULL} ;
      window->caller_cbs->merge_new_feature(window, window->app_data, &merge) ;

      /* The scratch feature has been saved. However, we have now created a new "real" feature 
       * which is unsaved. */
      window->flags[ZMAPFLAG_SCRATCH_NEEDS_SAVING] = FALSE ;
      window->flags[ZMAPFLAG_FEATURES_NEED_SAVING] = TRUE ;

      /*! \todo Check that feature was saved successfully before reporting back. Also close
       * the feature details dialog now we're finished. */
      //zMapMessage("%s", "Feature saved") ;
    }
  else
    {
      show->save_failed = TRUE ;

      if (error)
        {
          zMapWarning("%s", error->message) ;
          g_error_free(error) ;
        }
    }
}


static void remoteReplyErrHandler(ZMapRemoteControlRCType error_type, char *err_msg, void *user_data)
{
  /* Don't know what user_data is here...need to find out.... */





  return ;
}




/********************* end of file ********************************/
