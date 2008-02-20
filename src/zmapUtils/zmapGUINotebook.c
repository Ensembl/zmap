/*  File: zmapGUINotebook.c
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
 * Description: Implements general convenience routines for creating
 *              a GTK Notebook widget and the pages/fields within
 *              the Notebook.
 *
 * Exported functions: See ZMap/zmapUtilsGUI.h
 * HISTORY:
 * Last edited: Feb 20 09:21 2008 (edgrif)
 * Created: Wed Oct 24 10:08:38 2007 (edgrif)
 * CVS info:   $Id: zmapGUINotebook.c,v 1.4 2008-02-20 14:53:24 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>


/* Defines for strings for setting/getting our data from GtkWidgets. */
#define GUI_NOTEBOOK_SETDATA "zMapGuiNotebookData"
#define GUI_NOTEBOOK_STACK_SETDATA "zMapGuiNotebookStackData"
#define GUI_NOTEBOOK_BUTTON_SETDATA "zMapGuiNotebookButtonData"


/* Holds the state for the config dialog. */
typedef struct
{
  GtkWidget *toplevel ;
  GtkWidget *vbox ;
  GtkWidget *notebook_vbox ;
  GtkWidget *notebook_chooser ;
  GtkWidget *prev_button ;
  GtkWidget *notebook_stack ;
  GtkWidget *notebook ;

  char *help_title ;
  char *help_text ;

  GFunc destroy_func ;

  ZMapGuiNotebook notebook_spec ;

  /* State while parsing a notebook. */
  ZMapGuiNotebookParagraph curr_paragraph ;

  GtkWidget *curr_notebook ;
  GtkWidget *curr_page_vbox ;
  GtkWidget *curr_subsection_vbox ;
  GtkWidget *curr_paragraph_vbox ;
  GtkWidget *curr_paragraph_table ;
  guint curr_paragraph_rows, curr_paragraph_columns ;
  GtkWidget *curr_paragraph_treeview ;
  GtkTreeModel *curr_paragraph_model ;
  GtkTreeIter curr_paragraph_iter ;

} MakeNotebookStruct, *MakeNotebook ;


typedef struct
{
  GQuark tag_id ;
  ZMapGuiNotebookTagValue tag ;
} TagFindStruct, *TagFind ;



/* Used from a size event handler to try and get scrolled window to be a reasonable size
 * compared to its children. */
typedef struct
{
  GtkWidget *scrolled_window ;
  GtkWidget *tree_view ;
  int init_width, init_height ;
  int curr_width, curr_height ;
} TreeViewSizeCBDataStruct, *TreeViewSizeCBData ;


static ZMapGuiNotebookAny createSectionAny(ZMapGuiNotebookType type, char *name) ;
static GtkWidget *makeMenuBar(MakeNotebook make_notebook) ;
static void makeChapterCB(gpointer data, gpointer user_data) ;
static void makePageCB(gpointer data, gpointer user_data) ;
static void makeSubsectionCB(gpointer data, gpointer user_data) ;
static void makeParagraphCB(gpointer data, gpointer user_data) ;
static void makeTagValueCB(gpointer data, gpointer user_data) ;
static GtkWidget *addFeatureSection(FooCanvasItem *item, GtkWidget *parent, MakeNotebook make_notebook) ;

static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);
static void destroyCB(GtkWidget *widget, gpointer data);
static void changeNotebookCB(GtkWidget *widget, gpointer unused_data) ;
static void cancelCB(GtkWidget *widget, gpointer user_data) ;
static void callUserCancelCB(gpointer data, gpointer user_data) ;
static void okCB(GtkWidget *widget, gpointer user_data) ;
static void callUserOkCB(gpointer data, gpointer user_data) ;

static gint compareFuncCB(gconstpointer a, gconstpointer b) ;
static ZMapGuiNotebookTagValue findTagInPage(ZMapGuiNotebookPage page, const char *tagvalue_name) ;
static void eachSubsectionCB(gpointer data, gpointer user_data) ;
static void eachParagraphCB(gpointer data, gpointer user_data) ;
static void eachTagValueCB(gpointer data, gpointer user_data) ;
ZMapGuiNotebookAny getAnyParent(ZMapGuiNotebookAny any_child, ZMapGuiNotebookType parent_type) ;
static void callCBsAndDestroy(MakeNotebook make_notebook) ;
static void buttonToggledCB(GtkToggleButton *button, gpointer user_data) ;
static void entryActivateCB(GtkEntry *entry, gpointer  user_data) ;
static void changeCB(GtkEntry *entry, gpointer user_data) ;
static gboolean validateTagValue(ZMapGuiNotebookTagValue tag_value, char *text) ;



static void freeBookResources(gpointer data, gpointer user_data_unused) ;

static GtkWidget *makeNotebookWidget(MakeNotebook make_notebook) ;

static GtkWidget *createView(GList *column_titles, GList *column_types) ;
static GtkTreeModel *createModel(int num_cols, GList *column_types) ;
static void addDataToModel(int num_cols, GList *column_types,
			   GtkTreeModel *model, GtkTreeIter *iter, GList *value_list) ;
static void setModelInView(GtkTreeView *tree_view, GtkTreeModel *model) ;


static GtkTreeModel *makeTreeModel(FooCanvasItem *item) ;
static GtkCellRenderer *getColRenderer(void) ;
static void sizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;
static void ScrsizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;
static void sizeRequestCB(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data) ;
static void ScrsizeRequestCB(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data) ;



/* menu GLOBAL! */
static GtkItemFactoryEntry menu_items_G[] =
  {
    /* File */
    { "/_File",              NULL,         NULL,       0,          "<Branch>", NULL},
    { "/File/Close",         "<control>W", requestDestroyCB, 0,    NULL,       NULL},

    /* Help */
    { "/_Help",         NULL, NULL,       0,  "<LastBranch>", NULL},
    { "/Help/Overview", NULL, helpMenuCB, 0,  NULL,           NULL},
  } ;





/* Hacked from zmapwindow....this must go, don't want to do this like...... */
/* Controls type of window list created. */
typedef enum
  {
    ZMAPWINDOWLIST_FEATURE_TREE,			    /* Show features as clickable "trees". */
    ZMAPWINDOWLIST_FEATURE_LIST,			    /* Show features as single lines. */
    ZMAPWINDOWLIST_DNA_LIST,				    /* Show dna matches as single lines. */
  } ZMapWindowListType ;









/*! @addtogroup zmapguiutils
 * @{
 *  */


/*!
 * The model for building a notebook for displaying/editting resources/data etc is: 
 * 
 * 1) Use zMapGUINotebookCreateXXX() functions to build up a tree representing your data.
 * 
 * 2) Pass the tree to zMapGUINotebookCreateDialog() and it will create the dialog or
 *    pass it to zMapGUINotebookCreateWidget() to create just the notebook widget for
 *    embedding in other windows.
 * 
 * 3) Once the user has made their changes, each of your chapter callbacks will be
 *    called so you can extract the changed data from your original tree using the
 *    zMapGUINotebookFindPage() and zMapGUINotebookGetTagValue() calls.
 * 
 *    The model here is that each "chapter" will specify resources for a major component
 *    and hence its resources should be processed by a single callback.
 * 
 * 4) Finally your cleanup routine will be called so that you can free the tree
 *    (we could make this automatic if no cleanup function is supplied ?).
 * 
 * NOTE that the code currently assumes that tags are unique within pages, this could
 * be changed but would mean providing a tagvalue search function that searched within
 * paragraphs/subsections.
 * 
 * You should also note that if you don't supply a parent to chapter/page etc calls
 * then its your responsibility to hook the tree up correctly.
 * 
 * 
 *  */


/*! Create a notebook, this is the top level of the data structure from which the GUI 
 * notebook compound widget dialog will be built from.
 * 
 * @param notebook_name      String displayed in title of notebook dialog.
 * @param editable           TRUE means user can edit values, FALSE means they can't.
 * @param cleanup_cb         The callback function you want called when the dialog is destroyed.
 * @param user_cleanup_data  Data you want passed to your cleanup callback function.
 * @return                   ZMapGuiNotebook
 */
ZMapGuiNotebook zMapGUINotebookCreateNotebook(char *notebook_name, gboolean editable,
					      ZMapGUINotebookCallbackFunc cleanup_cb, void *user_cleanup_data)
{
  ZMapGuiNotebook notebook = NULL ;

  zMapAssert(notebook_name && *notebook_name && cleanup_cb) ;

  notebook = (ZMapGuiNotebook)createSectionAny(ZMAPGUI_NOTEBOOK_BOOK, notebook_name) ;

  notebook->editable = editable ;
  notebook->cleanup_cb = cleanup_cb ;
  notebook->user_cleanup_data = user_cleanup_data ;

  return notebook ;
}


/*! Create a chapter within a notebook.
 * 
 * @param note_book       If non-NULL, the chapter is added to this note_book.
 * @param chapter_name    String displayed in button to select this subnotebook.
 * @param user_callbacks  Cancel and OK callbacks called when user clicks relevant button.
 * @return                ZMapGuiNotebookChapter
 */
ZMapGuiNotebookChapter zMapGUINotebookCreateChapter(ZMapGuiNotebook note_book,
						    char *chapter_name, ZMapGuiNotebookCB user_callbacks)
{
  ZMapGuiNotebookChapter chapter = NULL ;

  zMapAssert(note_book && chapter_name && *chapter_name
	     && (!user_callbacks || (user_callbacks && user_callbacks->cancel_cb && user_callbacks->ok_cb))) ;

  chapter = (ZMapGuiNotebookChapter)createSectionAny(ZMAPGUI_NOTEBOOK_CHAPTER, chapter_name) ;

  if (user_callbacks)
    {
      chapter->user_CBs.cancel_cb = user_callbacks->cancel_cb ;
      chapter->user_CBs.ok_cb = user_callbacks->ok_cb ;
    }

  if (note_book)
    {
      chapter->parent = note_book ;
      note_book->chapters = g_list_append(note_book->chapters, chapter) ;
    }

  return chapter ;
}


/*! Create a page within a chapter.
 * 
 * @param chapter    If non-NULL, the page is added to this chapter.
 * @param page_name  String displayed at top of notebook page.
 * @return           ZMapGuiNotebookPage
 */
ZMapGuiNotebookPage zMapGUINotebookCreatePage(ZMapGuiNotebookChapter chapter, char *page_name)
{
  ZMapGuiNotebookPage page = NULL ;

  page = (ZMapGuiNotebookPage)createSectionAny(ZMAPGUI_NOTEBOOK_PAGE, page_name) ;

  if (chapter)
    {
      page->parent = chapter ;
      chapter->pages = g_list_append(chapter->pages, page) ;
    }

  return page ;
}


/*! Create a subsection within a page.
 * 
 * @param chapter          If non-NULL, the subsection is added to the subsection.
 * @param subsection_name  String displayed at top of notebook subsection.
 * @return                 ZMapGuiNotebookSubsection
 */
ZMapGuiNotebookSubsection zMapGUINotebookCreateSubsection(ZMapGuiNotebookPage page, char *subsection_name)
{
  ZMapGuiNotebookSubsection subsection = NULL ;

  subsection = (ZMapGuiNotebookSubsection)createSectionAny(ZMAPGUI_NOTEBOOK_SUBSECTION, subsection_name) ;

  if (page)
    {
      subsection->parent = page ;
      page->subsections = g_list_append(page->subsections, subsection) ;
    }

  return subsection ;
}


/*! Create a paragraph within a subsection.
 * 
 * @param subsection      If non-NULL, the subsection is added to this chapter.
 * @param paragraph_name  If non-NULL, the string displayed at the top of the frame of this paragraph.
 * @param display_type    The type of paragraph (e.g. simple list, table etc.).
 * @param headers         If non-NULL is a list of GQuark giving column header names for a
 *                        compound paragraph. (n.b. list should be passed in and _not_ freed by caller)
 * @param types           If non-NULL is a list of GQuark giving types for each column, supported
 *                        types are int, float and string.. (n.b. list should be passed in and
 *                        _not_ freed by caller)
 *
 * @return                ZMapGuiNotebookParagraph
 */
ZMapGuiNotebookParagraph zMapGUINotebookCreateParagraph(ZMapGuiNotebookSubsection subsection,
							char *paragraph_name,
							ZMapGuiNotebookParagraphDisplayType display_type,
							GList *headers, GList *types)
{
  ZMapGuiNotebookParagraph paragraph = NULL ;

  paragraph = (ZMapGuiNotebookParagraph)createSectionAny(ZMAPGUI_NOTEBOOK_PARAGRAPH, paragraph_name) ;
  paragraph->display_type = display_type ;
  if (headers)
    {
      paragraph->num_cols = g_list_length(headers) ;
      paragraph->compound_titles = headers ;
      paragraph->compound_types = types ;

    }

  if (subsection)
    {
      paragraph->parent = subsection ;
      subsection->paragraphs = g_list_append(subsection->paragraphs, paragraph) ;
    }

  return paragraph ;
}



/*! Create a tagvalue within a paragraph.
 * 
 * @param paragraph            If non-NULL, the tagvalue is added to this paragraph.
 * @param tag_value_name  The "tag" string, displayed on the left of the tag value fields.
 * @param display_type    The type of tag-value (e.g. checkbox, simple, scrolled)
 * @param arg_type        The type of the data to be used for the current value of the value field.
 * @return                ZMapGuiNotebookTagValue
 *
 *
 * This function uses the va_arg mechanism to allow caller to say something like:
 * 
 * zMapGUINotebookCreateTagValue(paragraph, "some name", ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
 *                               "string", string_ptr) ;
 * 
 * Valid arg_types are currently "bool", "int", "float", "string" or "compound". Note that no checking of args
 * can be done, the caller just has to get this right.
 * 
 *  */
ZMapGuiNotebookTagValue zMapGUINotebookCreateTagValue(ZMapGuiNotebookParagraph paragraph,
						      char *tag_value_name,
						      ZMapGuiNotebookTagValueDisplayType display_type,
						      const gchar *arg_type, ...)
{
  ZMapGuiNotebookTagValue tag_value = NULL ;
  va_list args ;
  gboolean bool_arg ;
  int int_arg ;
  double double_arg ;
  char *string_arg ;
  FooCanvasItem *item_arg ;
  GList *compound_arg ;

  tag_value = (ZMapGuiNotebookTagValue)createSectionAny(ZMAPGUI_NOTEBOOK_TAGVALUE, tag_value_name) ;
  tag_value->display_type = display_type ;


  /* Convert given type.... */
  va_start(args, arg_type) ;

  if (strcmp(arg_type, "bool") == 0)
    {
      tag_value->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_BOOL ;

      bool_arg = va_arg(args, gboolean) ;

      tag_value->data.bool_value = bool_arg ;
    }
  else if (strcmp(arg_type, "int") == 0)
    {
      tag_value->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_INT ;

      int_arg = va_arg(args, int) ;

      tag_value->data.int_value = int_arg ;
    }
  else if (strcmp(arg_type, "float") == 0)
    {
      tag_value->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_FLOAT ;

      double_arg = va_arg(args, double) ;

      tag_value->data.float_value = double_arg ;
    }
  else if (strcmp(arg_type, "string") == 0)
    {
      tag_value->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_STRING ;

      string_arg = va_arg(args, char *) ;

      tag_value->data.string_value = g_strdup(string_arg) ;
    }
  else if (strcmp(arg_type, "item") == 0)
    {
      tag_value->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_ITEM ;

      item_arg = va_arg(args, FooCanvasItem *) ;

      tag_value->data.item_value = item_arg ;
    }
  else if (strcmp(arg_type, "compound") == 0)
    {
      tag_value->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_COMPOUND ;

      compound_arg = va_arg(args, GList *) ;

      tag_value->data.compound_values = compound_arg ;
    }
  else
    {
      zMapAssertNotReached() ;
    }

  va_end (args);


  if (paragraph)
    {
      tag_value->parent = paragraph ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;
    }

  return tag_value ;
}


/*! Add a page to a chapter.
 * 
 * NOTE: this could be made into a general function that adds any to any.
 * 
 * 
 * @param   chapter      Parent chapter.
 * @param   page         Child page
 * @return               void
 */
void zMapGUINotebookAddPage(ZMapGuiNotebookChapter chapter, ZMapGuiNotebookPage page)
{
  zMapAssert(chapter && page) ;

  chapter->pages = g_list_append(chapter->pages, page) ;

  return ;
}



/*! Create the compound notebook widget from the notebook tree.
 * 
 * @param notebook_spec  The notebook tree.
 * @return               top container widget of notebook
 */
GtkWidget *zMapGUINotebookCreateWidget(ZMapGuiNotebook notebook_spec)
{
  MakeNotebook make_notebook  ;
  GtkWidget *note_widg ;

  zMapAssert(notebook_spec) ;

  make_notebook = g_new0(MakeNotebookStruct, 1) ;

  make_notebook->notebook_spec = notebook_spec ;

  note_widg = makeNotebookWidget(make_notebook) ;

  return note_widg ;
}


/*! Create the compound notebook dialog from the notebook tree.
 * 
 * @param notebook_spec  The notebook tree.
 * @return               dialog widget
 */
GtkWidget *zMapGUINotebookCreateDialog(ZMapGuiNotebook notebook_spec, char *help_title, char *help_text)
{
  GtkWidget *dialog = NULL ;
  GtkWidget *vbox, *note_widg, *hbuttons, *frame, *button ;
  MakeNotebook make_notebook  ;

  zMapAssert(notebook_spec && help_title && *help_title && help_text && *help_text) ;

  make_notebook = g_new0(MakeNotebookStruct, 1) ;

  make_notebook->notebook_spec = notebook_spec ;

  make_notebook->help_title = help_title ;
  make_notebook->help_text = help_text ;


  /*
   * Make dialog
   */  
  make_notebook->toplevel = dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  g_object_set_data(G_OBJECT(dialog), GUI_NOTEBOOK_SETDATA, make_notebook) ;
  gtk_window_set_title(GTK_WINDOW(dialog), g_quark_to_string(notebook_spec->name)) ;
  gtk_container_set_border_width(GTK_CONTAINER (dialog), 10);
  g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(destroyCB), make_notebook) ;

  make_notebook->vbox = vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_box_set_spacing(GTK_BOX(vbox), 5) ;
  gtk_container_add(GTK_CONTAINER(dialog), vbox) ;


  /*
   * Add a menu bar
   */
  gtk_box_pack_start(GTK_BOX(vbox), makeMenuBar(make_notebook), FALSE, FALSE, 0) ;


  /*
   * Add panel with chapter chooser and stack of chapters
   */
  note_widg = makeNotebookWidget(make_notebook) ;
  gtk_box_pack_start(GTK_BOX(vbox), note_widg, FALSE, FALSE, 0) ;


  /*
   * Make panel of  usual button stuff.
   */
  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(make_notebook->vbox), frame, FALSE, FALSE, 0) ;

  hbuttons = gtk_hbutton_box_new() ;
  gtk_container_add(GTK_CONTAINER(frame), hbuttons) ;

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL) ;
  gtk_container_add(GTK_CONTAINER(hbuttons), button) ;
  g_signal_connect(G_OBJECT(button), "pressed", G_CALLBACK(cancelCB), make_notebook) ;

  button = gtk_button_new_from_stock(GTK_STOCK_OK) ;
  gtk_container_add(GTK_CONTAINER(hbuttons), button) ;
  g_signal_connect(G_OBJECT(button), "pressed", G_CALLBACK(okCB), make_notebook) ;

  gtk_widget_show_all(dialog) ;

  return dialog ;
}


/*! Find a given page within a chapter.
 * 
 * @param chapter    Chapter to be searched.
 * @param page_name  Name of page to be searched for.
 * @return           ZMapGuiNotebookPage or NULL if not in chapter.
 */
ZMapGuiNotebookPage zMapGUINotebookFindPage(ZMapGuiNotebookChapter chapter, const char *page_name)
{
  ZMapGuiNotebookPage page = NULL ;
  GList *page_item ;
  GQuark page_id ;

  zMapAssert(chapter && page_name && *page_name) ;

  page_id = g_quark_from_string(page_name) ;

  if ((page_item = g_list_find_custom(chapter->pages, GINT_TO_POINTER(page_id), compareFuncCB)))
    {
      page = (ZMapGuiNotebookPage)(page_item->data) ;
    }

  return page ;
}


/*! Find a tagvalue within a page and return its value.
 * 
 * @param page    Page to be searched.
 * @param tagvalue_name  Tagvalue to find.
 * @return           TRUE if tagvalue found and value returned, FALSE otherwise.
 * 
 * 
 * This function is the corollary of zMapGUINotebookCreateTagValue() and also uses
 * the va_arg mechanism to allow caller to say something like:
 * 
 * zMapGUINotebookGetTagValue(page, "tag name",
 *                            "int", &int_variable) ;
 * 
 * Valid arg_types are currently "bool", "int", "float" or "string". Note that no checking of args
 * can be done, the caller just has to get this right.
 * 
 * NOTE that the field following the arg_type _MUST_ be the address of a variable into
 * which the value of the tag will be returned.
 * 
 */
gboolean zMapGUINotebookGetTagValue(ZMapGuiNotebookPage page, const char *tagvalue_name,
				    const char *arg_type, ...)
{
  gboolean result = FALSE ;
  ZMapGuiNotebookTagValue tagvalue ;

  zMapAssert(page && tagvalue_name && *tagvalue_name && arg_type && *arg_type) ;

  if ((tagvalue = findTagInPage(page, tagvalue_name)))
    {
      va_list args ;

      /* Convert given type.... */
      va_start(args, arg_type) ;

      if (strcmp(arg_type, "bool") == 0)
	{
	  gboolean *bool_arg ;

	  tagvalue->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_BOOL ;

	  bool_arg = va_arg(args, gboolean *) ;

	  *bool_arg = tagvalue->data.bool_value ;
	}
      else   if (strcmp(arg_type, "int") == 0)
	{
	  int *int_arg ;

	  tagvalue->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_INT ;

	  int_arg = va_arg(args, int *) ;

	  *int_arg = tagvalue->data.int_value ;
	}
      else if (strcmp(arg_type, "float") == 0)
	{
	  double *double_arg ;

	  tagvalue->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_FLOAT ;

	  double_arg = va_arg(args, double *) ;

	  *double_arg = tagvalue->data.float_value ;
	}
      else if (strcmp(arg_type, "string") == 0)
	{
	  char **string_arg ;

	  tagvalue->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_STRING ;

	  string_arg = va_arg(args, char **) ;

	  *string_arg = tagvalue->data.string_value ;
	}
      else if (strcmp(arg_type, "item") == 0)
	{
	  FooCanvasItem **item_arg ;

	  tagvalue->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_ITEM ;

	  item_arg = va_arg(args, FooCanvasItem **) ;

	  *item_arg = tagvalue->data.item_value ;
	}
      else
	{
	  zMapAssertNotReached() ;
	}

      va_end (args);

      result = TRUE ;
    }

  return result ;
}


/*! Destroy a notebook or subpart of a notebook freeing all resources.
 * 
 * @param note_any   Any level within a notebook, and its children to be destroyed.
 * @return           <nothing>
 */
void zMapGUINotebookDestroyAny(ZMapGuiNotebookAny note_any)
{
  freeBookResources(note_any, NULL) ;

  return ;
}


/*! Destroy a notebook tree freeing all resources.
 * 
 * @param note_book  Notebook to be destroyed.
 * @return           <nothing>
 */
void zMapGUINotebookDestroyNotebook(ZMapGuiNotebook note_book)
{
  freeBookResources((ZMapGuiNotebookAny)note_book, NULL) ;

  return ;
}






/*! @} end of zmapguiutils docs. */





/* 
 *                        Internal routines.
 */


/* Create the bare bones, canonical notebook structs. */
static ZMapGuiNotebookAny createSectionAny(ZMapGuiNotebookType type, char *name)
{
  ZMapGuiNotebookAny book_any ;
  int size ;

  switch(type)
    {
    case ZMAPGUI_NOTEBOOK_BOOK:
      size = sizeof(ZMapGuiNotebookStruct) ;
      break ;
    case ZMAPGUI_NOTEBOOK_CHAPTER:
      size = sizeof(ZMapGuiNotebookChapterStruct) ;
      break ;
    case ZMAPGUI_NOTEBOOK_PAGE:
      size = sizeof(ZMapGuiNotebookPageStruct) ;
      break ;
    case ZMAPGUI_NOTEBOOK_SUBSECTION:
      size = sizeof(ZMapGuiNotebookParagraphStruct) ;
      break ;
    case ZMAPGUI_NOTEBOOK_PARAGRAPH:
      size = sizeof(ZMapGuiNotebookParagraphStruct) ;
      break ;
    case ZMAPGUI_NOTEBOOK_TAGVALUE:
      size = sizeof(ZMapGuiNotebookTagValueStruct) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  book_any = g_malloc0(size) ;
  book_any->type = type ;

  if (name)
    book_any->name = g_quark_from_string(name) ;

  return book_any ;
}


/* A GFunc() to free up the book resources... */
static void freeBookResources(gpointer data, gpointer user_data_unused)
{
  ZMapGuiNotebookAny book_any = (ZMapGuiNotebookAny)data ;


  switch(book_any->type)
    {
    case ZMAPGUI_NOTEBOOK_BOOK:
    case ZMAPGUI_NOTEBOOK_CHAPTER:
    case ZMAPGUI_NOTEBOOK_PAGE:
    case ZMAPGUI_NOTEBOOK_SUBSECTION:
    case ZMAPGUI_NOTEBOOK_PARAGRAPH:
      {
	if (book_any->children)
	  {
	    g_list_foreach(book_any->children, freeBookResources, NULL) ;
	    g_list_free(book_any->children) ;
	  }

	break ;
      }

    case ZMAPGUI_NOTEBOOK_TAGVALUE:
      {
	ZMapGuiNotebookTagValue tag_value = (ZMapGuiNotebookTagValue)book_any ;

	zMapAssert(!(book_any->children)) ;

	if (tag_value->data_type == ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_STRING)
	  g_free(tag_value->data.string_value) ;

	break ;
      }

    default:
      zMapAssertNotReached() ;
      break ;
    }

  book_any->type = ZMAPGUI_NOTEBOOK_INVALID ;		    /* Mark block as invalid. */

  g_free(book_any) ;

  return ;
}



/* Make the compound notebook widget from the notebook tree.
 */
static GtkWidget *makeNotebookWidget(MakeNotebook make_notebook)
{
  GtkWidget *top_widg = NULL ;
  GtkWidget *vbox_buttons, *frame ;


  /*
   * Add panel with chapter chooser and stack of chapters
   */
  top_widg = make_notebook->notebook_vbox = gtk_vbox_new(FALSE, 0) ;

  /* Add chapter chooser */
  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(make_notebook->notebook_vbox), frame, FALSE, FALSE, 0) ;

  vbox_buttons = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), vbox_buttons) ;

  make_notebook->notebook_chooser = gtk_hbutton_box_new() ;
  gtk_box_pack_start(GTK_BOX(vbox_buttons), make_notebook->notebook_chooser, FALSE, FALSE, 0) ;

  /* Make chapter stack holder */
  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(make_notebook->notebook_vbox), frame, TRUE, TRUE, 0) ;

  make_notebook->notebook_stack = gtk_notebook_new() ;
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(make_notebook->notebook_stack), FALSE) ;
  gtk_container_add(GTK_CONTAINER(frame), make_notebook->notebook_stack) ;
  
  g_list_foreach(make_notebook->notebook_spec->chapters, makeChapterCB, make_notebook) ;


  return top_widg ;
}


static GtkWidget *makeMenuBar(MakeNotebook make_notebook)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkWidget *menubar ;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel_group = gtk_accel_group_new();

  item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group) ;

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items_G, (gpointer)make_notebook) ;

  gtk_window_add_accel_group(GTK_WINDOW(make_notebook->toplevel), accel_group) ;

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  return menubar ;
}




static void makeChapterCB(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)data ;
  MakeNotebook make_notebook = (MakeNotebook)user_data ;
  GtkWidget *notebook_widget, *button ;
  gint notebook_pos ;


  /* Add notebooks, each notebook's widget is supplied to its corresponding button
   * so it can be raised to the top of the notebook stack when the button is clicked. */
  make_notebook->curr_notebook = notebook_widget = gtk_notebook_new() ;
  g_object_set_data(G_OBJECT(notebook_widget), GUI_NOTEBOOK_STACK_SETDATA, make_notebook->notebook_stack) ;
  
  button = gtk_radio_button_new_with_label_from_widget((make_notebook->prev_button
							? GTK_RADIO_BUTTON(make_notebook->prev_button)
							: NULL),
						       g_quark_to_string(chapter->name)) ;
  make_notebook->prev_button = button ;
  g_object_set_data(G_OBJECT(button), GUI_NOTEBOOK_BUTTON_SETDATA, notebook_widget) ;
  gtk_container_add(GTK_CONTAINER(make_notebook->notebook_chooser), button) ;
  g_signal_connect(G_OBJECT(button), "pressed", G_CALLBACK(changeNotebookCB), NULL) ;

  notebook_pos = gtk_notebook_append_page(GTK_NOTEBOOK(make_notebook->notebook_stack), notebook_widget, NULL) ;

  g_list_foreach(chapter->pages, makePageCB, make_notebook) ;

  return ;
}



/* A GFunc() to build notebook pages. */
static void makePageCB(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookPage page = (ZMapGuiNotebookPage)data ;
  MakeNotebook make_notebook = (MakeNotebook)user_data ;
  GtkWidget *notebook_label ;
  int notebook_index ;

  /* Put code here for a page... */
  notebook_label = gtk_label_new(g_quark_to_string(page->name)) ;

  make_notebook->curr_page_vbox = gtk_vbox_new(FALSE, 0) ;

  g_list_foreach(page->subsections, makeSubsectionCB, make_notebook) ;

  notebook_index = gtk_notebook_append_page(GTK_NOTEBOOK(make_notebook->curr_notebook),
					    make_notebook->curr_page_vbox, notebook_label) ;

  return ;
}


/* A GFunc() to build notebook subsections within pages. */
static void makeSubsectionCB(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookSubsection subsection = (ZMapGuiNotebookSubsection)data ;
  MakeNotebook make_notebook = (MakeNotebook)user_data ;
  GtkWidget *subsection_frame ;

  subsection_frame = gtk_frame_new(g_quark_to_string(subsection->name)) ;
  gtk_container_set_border_width(GTK_CONTAINER(subsection_frame), 5) ;
  gtk_container_add(GTK_CONTAINER(make_notebook->curr_page_vbox), subsection_frame) ;

  make_notebook->curr_subsection_vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(subsection_frame), make_notebook->curr_subsection_vbox) ;

  g_list_foreach(subsection->paragraphs, makeParagraphCB, make_notebook) ;

  return ;
}


/* A GFunc() to build paragraphs within subsections. */
static void makeParagraphCB(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookParagraph paragraph = (ZMapGuiNotebookParagraph)data ;
  MakeNotebook make_notebook = (MakeNotebook)user_data ;
  GtkWidget *paragraph_frame ;

  make_notebook->curr_paragraph = paragraph ;

  paragraph_frame = gtk_frame_new(g_quark_to_string(paragraph->name)) ;
  gtk_container_set_border_width(GTK_CONTAINER(paragraph_frame), 5) ;
  gtk_container_add(GTK_CONTAINER(make_notebook->curr_subsection_vbox), paragraph_frame) ;

  make_notebook->curr_paragraph_vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(paragraph_frame), make_notebook->curr_paragraph_vbox) ;

  if (paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE
      || paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_HOMOGENOUS)
    {
      make_notebook->curr_paragraph_rows = 0 ;
      make_notebook->curr_paragraph_columns = 2 ;
      make_notebook->curr_paragraph_table = gtk_table_new(make_notebook->curr_paragraph_rows,
							  make_notebook->curr_paragraph_columns,
							  FALSE) ;
      gtk_container_add(GTK_CONTAINER(make_notebook->curr_paragraph_vbox), make_notebook->curr_paragraph_table) ;
    }
  else if (paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE)
    {
      GtkWidget *scrolled_window ;

      scrolled_window = gtk_scrolled_window_new(NULL, NULL) ;
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC) ;
      gtk_container_add(GTK_CONTAINER(make_notebook->curr_paragraph_vbox), scrolled_window) ;

      /* Compound is represented by a treeview widget. */
      make_notebook->curr_paragraph_treeview = createView(paragraph->compound_titles, paragraph->compound_types) ;
      gtk_container_add(GTK_CONTAINER(scrolled_window), make_notebook->curr_paragraph_treeview) ;

      make_notebook->curr_paragraph_model = createModel(paragraph->num_cols, paragraph->compound_types) ;
    }

  g_list_foreach(paragraph->tag_values, makeTagValueCB, make_notebook) ;

 if (paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE)
   {
     setModelInView(GTK_TREE_VIEW(make_notebook->curr_paragraph_treeview), make_notebook->curr_paragraph_model) ;
   }

  return ;
}


/* A GFunc() to build notebook pages. */
static void makeTagValueCB(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookTagValue tag_value = (ZMapGuiNotebookTagValue)data ;
  MakeNotebook make_notebook = (MakeNotebook)user_data ;
  ZMapGuiNotebook notebook = make_notebook->notebook_spec ;
  GtkWidget *container = NULL ;

  switch(tag_value->display_type)
    {
    case ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX:
    case ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE:
      {
	GtkWidget *tag, *value ;

	tag = gtk_label_new(g_quark_to_string(tag_value->tag)) ;
	gtk_label_set_justify(GTK_LABEL(tag), GTK_JUSTIFY_RIGHT) ;

	if (tag_value->display_type == ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX)
	  {
	    value = gtk_check_button_new() ;

	    if (tag_value->data.bool_value == TRUE)
	      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(value), TRUE) ;

	    g_signal_connect(G_OBJECT(value), "toggled", G_CALLBACK(buttonToggledCB), tag_value) ;
	  }
	else
	  {
	    char *text ;

	    if (tag_value->data_type == ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_INT)
	      text = g_strdup_printf("%d", tag_value->data.int_value) ;
	    else if (tag_value->data_type == ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_FLOAT)
	      text = g_strdup_printf("%f", tag_value->data.float_value) ;
	    else
	      text = g_strdup(tag_value->data.string_value) ;

	    value = gtk_entry_new() ;
	    gtk_entry_set_text(GTK_ENTRY(value), text) ;
	    gtk_entry_set_editable(GTK_ENTRY(value), notebook->editable) ;
	    g_signal_connect(G_OBJECT(value), "activate", G_CALLBACK(entryActivateCB), tag_value) ;
	    g_signal_connect(G_OBJECT(value), "changed", G_CALLBACK(changeCB), tag_value) ;

	    g_free(text) ;
	  }

	if (make_notebook->curr_paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_SIMPLE)
	  {
	    GtkWidget *hbox ;

	    container = hbox = gtk_hbox_new(FALSE, 0) ;
	    gtk_container_add(GTK_CONTAINER(make_notebook->curr_paragraph_vbox), container) ;

	    gtk_container_add(GTK_CONTAINER(hbox), tag) ;

	    gtk_container_add(GTK_CONTAINER(hbox), value) ;
	  }
	else
	  {
	    make_notebook->curr_paragraph_rows++ ;
	    gtk_table_resize(GTK_TABLE(make_notebook->curr_paragraph_table),
			     make_notebook->curr_paragraph_rows,
			     make_notebook->curr_paragraph_columns) ;

	    if (make_notebook->curr_paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE
		|| (make_notebook->curr_paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_HOMOGENOUS
		    && make_notebook->curr_paragraph_rows == 1))
	      gtk_table_attach_defaults(GTK_TABLE(make_notebook->curr_paragraph_table),
					tag,
					0,
					1,
					make_notebook->curr_paragraph_rows - 1,
					make_notebook->curr_paragraph_rows) ;

	    gtk_table_attach_defaults(GTK_TABLE(make_notebook->curr_paragraph_table),
				      value,
				      1,
				      2,
				      make_notebook->curr_paragraph_rows - 1,
				      make_notebook->curr_paragraph_rows) ;
	  }

	break ;
      }
    case ZMAPGUI_NOTEBOOK_TAGVALUE_SCROLLED_TEXT:
      {
	GtkWidget *frame, *scrolled_window, *view ;
	GtkTextBuffer *buffer ;

	frame = gtk_frame_new(g_quark_to_string(tag_value->tag)) ;
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5) ;
	gtk_container_add(GTK_CONTAINER(make_notebook->curr_paragraph_vbox), frame) ;

	scrolled_window = gtk_scrolled_window_new(NULL, NULL) ;
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC) ;
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window) ;

	view = gtk_text_view_new() ;
	gtk_container_add(GTK_CONTAINER(scrolled_window), view) ;
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), notebook->editable) ;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)) ;
	gtk_text_buffer_set_text(buffer, tag_value->data.string_value, -1) ;

	break ;
      }

    case ZMAPGUI_NOTEBOOK_TAGVALUE_ITEM:
      {
	GtkWidget *scrolled_window, *treeView ;
	TreeViewSizeCBData size_data ;

	size_data = g_new0(TreeViewSizeCBDataStruct, 1) ;

	container = size_data->scrolled_window = scrolled_window = gtk_scrolled_window_new(NULL, NULL) ;
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC) ;
	gtk_container_add(GTK_CONTAINER(make_notebook->curr_paragraph_vbox), container) ;

	g_signal_connect(GTK_OBJECT(scrolled_window), "size-allocate",
			 GTK_SIGNAL_FUNC(ScrsizeAllocateCB), size_data) ;
	g_signal_connect(GTK_OBJECT(scrolled_window), "size-request",
			 GTK_SIGNAL_FUNC(ScrsizeRequestCB), size_data) ;

	size_data->tree_view = treeView = addFeatureSection(tag_value->data.item_value,
							    scrolled_window, make_notebook) ;

	gtk_container_add(GTK_CONTAINER(scrolled_window), treeView) ;
  
	g_signal_connect(GTK_OBJECT(treeView), "size-allocate",
			 GTK_SIGNAL_FUNC(sizeAllocateCB), size_data) ;
	g_signal_connect(GTK_OBJECT(treeView), "size-request",
			 GTK_SIGNAL_FUNC(sizeRequestCB), size_data) ;

	break ;
      }

    case ZMAPGUI_NOTEBOOK_TAGVALUE_COMPOUND:
      {
	addDataToModel(make_notebook->curr_paragraph->num_cols, make_notebook->curr_paragraph->compound_types,
		       GTK_TREE_MODEL(make_notebook->curr_paragraph_model),
		       &(make_notebook->curr_paragraph_iter),
		       tag_value->data.compound_values) ;

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


/* Raise the notebook attached to the button to the top of the stack of notebooks. */
static void changeNotebookCB(GtkWidget *widget, gpointer unused_data)
{
  GtkWidget *notebook, *notebook_stack ;
  gint notebook_pos ;

  /* Get hold of the notebook, the notebook_stack and the position of the notebook within
   * the notebook_stack. */
  notebook = g_object_get_data(G_OBJECT(widget), GUI_NOTEBOOK_BUTTON_SETDATA) ;

  notebook_stack = g_object_get_data(G_OBJECT(notebook), GUI_NOTEBOOK_STACK_SETDATA) ;

  notebook_pos = gtk_notebook_page_num(GTK_NOTEBOOK(notebook_stack), notebook) ;

  /* Raise the notebook to the top of the notebooks */
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_stack), notebook_pos) ;

  /* Set this notebook to show the first page. */
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0) ;

  return ;
}



/* requestDestroyCB(), cancelCB() & okCB() all cause destroyCB() to be called via
 * the gtk widget destroy mechanism. */

static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  MakeNotebook make_notebook = (MakeNotebook)data ;

  make_notebook->destroy_func = callUserCancelCB ;

  gtk_widget_destroy(make_notebook->toplevel) ;

  return ;
}

static void cancelCB(GtkWidget *widget, gpointer user_data)
{
  MakeNotebook make_notebook = (MakeNotebook)user_data ;

  make_notebook->destroy_func = callUserCancelCB ;

  gtk_widget_destroy(make_notebook->toplevel) ;

  return ;
}

static void okCB(GtkWidget *widget, gpointer user_data)
{
  MakeNotebook make_notebook = (MakeNotebook)user_data ;

  make_notebook->destroy_func = callUserOkCB ;

  gtk_widget_destroy(make_notebook->toplevel) ;

  return ;
}


/* This function gets called whenever the toplevel widget is destroyed. */
static void destroyCB(GtkWidget *widget, gpointer data)
{
  MakeNotebook make_notebook = (MakeNotebook)data ;

  /* If user clicks one of the window manager buttons/menu items to kill the window then
   * destroy func will not have been set. */
  if (!(make_notebook->destroy_func))
    make_notebook->destroy_func = callUserCancelCB ;

  callCBsAndDestroy(make_notebook) ;

  return ;
}



static void callCBsAndDestroy(MakeNotebook make_notebook)
{
  g_list_foreach(make_notebook->notebook_spec->chapters, make_notebook->destroy_func, NULL) ;

  make_notebook->notebook_spec->cleanup_cb((ZMapGuiNotebookAny)(make_notebook->notebook_spec),
					   make_notebook->notebook_spec->user_cleanup_data) ;

  g_free(make_notebook) ;

  return ;
}
 
static void callUserCancelCB(gpointer data, gpointer user_data_unused)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)data ;

  if (chapter->user_CBs.cancel_cb)
    chapter->user_CBs.cancel_cb((ZMapGuiNotebookAny)chapter, NULL) ;

  return ;
}


static void callUserOkCB(gpointer data, gpointer user_data_unused)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)data ;

  if (chapter->changed)
    {
      if (chapter->user_CBs.ok_cb)
	chapter->user_CBs.ok_cb((ZMapGuiNotebookAny)chapter, NULL) ;

      chapter->changed = FALSE ;
    }

  return ;
}



static void helpMenuCB(gpointer data, guint callback_action, GtkWidget *w)
{
  MakeNotebook make_notebook = (MakeNotebook)data ;

  zMapGUIShowText(make_notebook->help_title, make_notebook->help_text, FALSE) ;

  return ;
}


static void buttonToggledCB(GtkToggleButton *button, gpointer user_data)
{
  ZMapGuiNotebookTagValue tag_value = (ZMapGuiNotebookTagValue)user_data ;
  ZMapGuiNotebookChapter chapter ;

  tag_value->data.bool_value = gtk_toggle_button_get_active(button) ;

  /* Signal that there is actually something to change. */
  chapter = (ZMapGuiNotebookChapter)getAnyParent((ZMapGuiNotebookAny)tag_value, ZMAPGUI_NOTEBOOK_CHAPTER) ;
  chapter->changed = TRUE ;

  return ;
}


static void entryActivateCB(GtkEntry *entry, gpointer user_data)
{
  ZMapGuiNotebookTagValue tag_value = (ZMapGuiNotebookTagValue)user_data ;
  char *text ;

  text = (char *)gtk_entry_get_text(entry) ;

  if ((validateTagValue(tag_value, text)))
    {
      ZMapGuiNotebookChapter chapter ;

      /* If tag ok then record that something has been changed. */
      chapter = (ZMapGuiNotebookChapter)getAnyParent((ZMapGuiNotebookAny)tag_value, ZMAPGUI_NOTEBOOK_CHAPTER) ;
      chapter->changed = TRUE ;
    }

  return ;
}



static void changeCB(GtkEntry *entry, gpointer user_data)
{
  ZMapGuiNotebookTagValue tag_value = (ZMapGuiNotebookTagValue)user_data ;
  char *text ;

  text = (char *)gtk_entry_get_text(entry) ;

  if ((validateTagValue(tag_value, text)))
    {
      ZMapGuiNotebookChapter chapter ;

      /* If tag ok then record that something has been changed. */
      chapter = (ZMapGuiNotebookChapter)getAnyParent((ZMapGuiNotebookAny)tag_value, ZMAPGUI_NOTEBOOK_CHAPTER) ;
      chapter->changed = TRUE ;
    }

  return ;
}



/* A GCompareFunc() to find a notebookany with a given name. */
static gint compareFuncCB(gconstpointer a, gconstpointer b)
{
  gint result = -1 ;
  ZMapGuiNotebookAny any = (ZMapGuiNotebookAny)a ;
  GQuark tag_id = GPOINTER_TO_INT(b) ;

  if (any->name == tag_id)
    result = 0 ;

  return result ;
}


static ZMapGuiNotebookTagValue findTagInPage(ZMapGuiNotebookPage page, const char *tagvalue_name)
{
  ZMapGuiNotebookTagValue tagvalue = NULL ;
  TagFindStruct find_tag = {0} ;
  GQuark tag_id ;

  tag_id = g_quark_from_string(tagvalue_name) ;

  find_tag.tag_id = tag_id ;

  g_list_foreach(page->subsections, eachSubsectionCB, &find_tag) ;

  if (find_tag.tag)
    tagvalue = find_tag.tag ;

  return tagvalue ;
}


/* A GFunc() to go through subsections. */
static void eachSubsectionCB(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookParagraph paragraph = (ZMapGuiNotebookParagraph)data ;

  g_list_foreach(paragraph->tag_values, eachParagraphCB, user_data) ;

  return ;
}


/* A GFunc() to go through paragraphs. */
static void eachParagraphCB(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookParagraph paragraph = (ZMapGuiNotebookParagraph)data ;

  g_list_foreach(paragraph->tag_values, eachTagValueCB, user_data) ;

  return ;
}


/* A GFunc() to find a tag within paragraph. */
static void eachTagValueCB(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookTagValue tag = (ZMapGuiNotebookTagValue)data ;
  TagFind find_tag = (TagFind)user_data ;

  if (tag->tag == find_tag->tag_id)
    find_tag->tag = tag ;

  return ;
}


ZMapGuiNotebookAny getAnyParent(ZMapGuiNotebookAny any_child, ZMapGuiNotebookType parent_type)
{
  ZMapGuiNotebookAny parent = NULL ;
  ZMapGuiNotebookAny curr = any_child ;

  if (any_child->type > parent_type)
    {
      while (curr && curr->type > parent_type)
	{
	  curr = curr->parent ;
	}

      if (curr)
	parent = curr ;
    }

  return parent ;
}


GtkWidget *addFeatureSection(FooCanvasItem *item, GtkWidget *parent, MakeNotebook make_notebook)
{
  GtkWidget *feature_widget = NULL ;
  GtkTreeModel *treeModel = NULL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowFeatureListCallbacksStruct windowCallbacks = {NULL} ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  treeModel = makeTreeModel(item) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  windowCallbacks.selectionFuncCB = selectionFunc;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* all this window list stuff needs to be independent of zmapwindow....hacked for now... */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* NOTE I'M NOT REALLY SURE WHAT USER_DATA SHOULD GO IN HERE, make_notebook MAY NOT HAVE
   * THE RIGHT STUFF. */
  feature_widget = zmapWindowFeatureListCreateView(ZMAPWINDOWLIST_FEATURE_TREE, treeModel, 
						   getColRenderer(),
						   NULL,
						   make_notebook) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return feature_widget ;
}



static GtkTreeModel *makeTreeModel(FooCanvasItem *item)
{
  GtkTreeModel *tree_model = NULL ;
  GList *itemList         = NULL;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  tree_model = zmapWindowFeatureListCreateStore(ZMAPWINDOWLIST_FEATURE_TREE);

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  itemList  = g_list_append(itemList, item);

  zmapWindowFeatureListPopulateStoreList(tree_model, ZMAPWINDOWLIST_FEATURE_TREE, itemList, NULL) ;

  return tree_model ;
}


static GtkCellRenderer *getColRenderer(void)
{
  GtkCellRenderer *renderer = NULL;
  GdkColor background;

  gdk_color_parse("WhiteSmoke", &background) ;

  /* make renderer */
  renderer = gtk_cell_renderer_text_new() ;

  g_object_set (G_OBJECT (renderer),
                "foreground", "red",
                "background-gdk", &background,
                "editable", FALSE,
                NULL) ;

  return renderer ;
}


/* These were all for trying to get the stupid scrolled window and the tree view widgets to work
 * together properly, the treeview widget always comes up in a scrolled window that is too narrow and too short. */

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
  if (GTK_WIDGET_REALIZED(widget))
    {
      printf("ScrWin: sizeRequestCB:  height: %d, width: %d\n", 
	     requisition->height, requisition->width); 
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}



/* Make a treeview widget dynamically allocating columns according to the number
 * supplied by the caller. */
static GtkWidget *createView(GList *column_titles, GList *column_types)
{
  GtkWidget *view = NULL ;
  GtkCellRenderer *renderer ;
  GList *column ;
  int index ;

  view = gtk_tree_view_new() ;

  /* From GTK 2.12 it will be possible to build a list of indices/values and just do one gtk call. */
  index = 0 ;
  column = column_titles ;
  do
    {
      char *col_title ;

      col_title = (char *)g_quark_to_string(GPOINTER_TO_INT(column->data)) ;

      renderer = gtk_cell_renderer_text_new() ;

      gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
						  -1,      
						  col_title,  
						  renderer,
						  "text", index,
						  NULL) ;
      index++ ;
    } while ((column = g_list_next(column))) ;

  return view ;
}


/* Create the tree view model with different types for columns. */
static GtkTreeModel *createModel(int num_cols, GList *column_types)
{
  GtkListStore  *store;
  int i ;
  GType *types, *tmp ;
  GList *entry ;

  types = g_new0(GType, num_cols) ;
  entry = g_list_first(column_types) ;

  for (i = 0, tmp = types ; i < num_cols ; i++, tmp++)
    {
      if (g_quark_from_string("int") == GPOINTER_TO_INT(entry->data))
	*tmp = G_TYPE_INT ;
      else if (g_quark_from_string("float") == GPOINTER_TO_INT(entry->data))
	*tmp = G_TYPE_FLOAT ;
      else if (g_quark_from_string("string") == GPOINTER_TO_INT(entry->data))
	*tmp = G_TYPE_STRING ;
      else
	zMapAssertNotReached() ;

      entry = g_list_next(entry) ;
    }

  store = gtk_list_store_newv(num_cols, types) ;

  /* FREE THIS HERE ?..... */
  g_free(types) ;
  
  return GTK_TREE_MODEL(store) ;
}


/* Append a row and fill in row data. (See the online GObject docs for an explanation of all this
 * GValue stuff....) */
static void addDataToModel(int num_cols, GList *column_types,
			   GtkTreeModel *model, GtkTreeIter *iter, GList *value_list)
{
  GtkListStore *store = GTK_LIST_STORE(model) ;
  GList *tmp_list ;
  GValue value = {0, } ;
  int index ;
  GList *entry ;
  
  gtk_list_store_append(store, iter) ;

  index = 0 ;
  tmp_list = value_list ;
  entry = g_list_first(column_types) ;
  do
    {
      if (g_quark_from_string("bool") == GPOINTER_TO_INT(entry->data))
	{
	  gboolean bool_value = GPOINTER_TO_INT(tmp_list->data) ;

	  g_value_init(&value, G_TYPE_BOOLEAN) ;

	  g_value_set_boolean(&value, bool_value) ;
	}
      if (g_quark_from_string("int") == GPOINTER_TO_INT(entry->data))
	{
	  int int_value = GPOINTER_TO_INT(tmp_list->data) ;

	  g_value_init(&value, G_TYPE_INT) ;

	  g_value_set_int(&value, int_value) ;
	}
      else if (g_quark_from_string("float") == GPOINTER_TO_INT(entry->data))
	{
	  int tmp = GPOINTER_TO_INT(tmp_list->data) ;
	  float float_value ;

	  memcpy(&float_value, &tmp, 4) ;		    /* Let's hope floats are 4 bytes... */

	  g_value_init(&value, G_TYPE_FLOAT) ;

	  g_value_set_float(&value, float_value) ;
	}
      else if (g_quark_from_string("string") == GPOINTER_TO_INT(entry->data))
	{
	  char *str_value = (char *)(tmp_list->data) ;

	  g_value_init(&value, G_TYPE_STRING) ;

	  g_value_set_string(&value, str_value) ;
	}
      else
	{
	  zMapAssertNotReached() ;
	}

      gtk_list_store_set_value(store, iter, index, &value) ;

      g_value_unset(&value) ;
      entry = g_list_next(entry) ;
      index++ ;
    } while ((tmp_list = g_list_next(tmp_list))) ;

  return ;
}


/* Set the model in the view so the data will be drawn. */
static void setModelInView(GtkTreeView *tree_view, GtkTreeModel *model)
{

  gtk_tree_view_set_model(tree_view, model) ;

  /* Model gets reffed by view and by our creation of it, unreffing here means that model will
   * be destroyed when view is destroyed. */
  g_object_unref(model) ;

  return ;
}

static gboolean validateTagValue(ZMapGuiNotebookTagValue tag_value, char *text)
{
  gboolean status = FALSE ;


  /* NEED A BOOLEAN CONVERTOR AND A VAGUE STRING CHECKER.... */
  switch (tag_value->data_type)
    {
    case ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_BOOL:
      {
	gboolean tmp = FALSE ;

	if ((status = zMapStr2Bool(text, &tmp)))
	  {
	    tag_value->data.bool_value = tmp ;
	    status = TRUE ;
	  }
	else
	  {
	    zMapWarning("Invalid boolean value: %s", text) ;
	  }

	break ;
      }
    case ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_INT:
      {
	int tmp = 0 ;

	if ((status = zMapStr2Int(text, &tmp)))
	  {
	    tag_value->data.int_value = tmp ;
	    status = TRUE ;
	  }
	else
	  {
	    zMapWarning("Invalid integer number: %s", text) ;
	  }

	break ;
      }
    case ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_FLOAT:
      {
	double tmp = 0.0 ;

	if ((status = zMapStr2Double(text, &tmp)))
	  {
	    tag_value->data.float_value = tmp ;
	    status = TRUE ;
	  }
	else
	  {
	    zMapWarning("Invalid float number: %s", text) ;
	  }

	break ;
      }
    case ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_STRING:
      {
	/* Hardly worth checking but better than nothing ? */
	if (text && *text)
	  {
	    tag_value->data.string_value = text ;
	    status = TRUE ;
	  }
	else
	  {
	    zMapWarning("Invalid string: %s", (text ? "No string" : "NULL string pointer")) ;
	  }

	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
	break ;
      }
    }

  return status ;
}
