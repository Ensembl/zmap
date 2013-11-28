/*  File: zmapGUINotebook.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * Description: Implements general convenience routines for creating
 *              a GTK Notebook widget and the pages/fields within
 *              the Notebook.
 *
 * Exported functions: See ZMap/zmapUtilsGUI.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapGUITreeView.h>


/* Defines for strings for setting/getting our data from GtkWidgets. */
#define GUI_NOTEBOOK_SETDATA "zMapGuiNotebookData"
#define GUI_NOTEBOOK_STACK_SETDATA  "zMapGuiNotebookStackData"
#define GUI_NOTEBOOK_CURR_SETDATA   "zMapGuiNotebookCurrData"
#define GUI_NOTEBOOK_BUTTON_SETDATA "zMapGuiNotebookButtonData"

#define GUI_NOTEBOOK_DEFAULT_BORDER   0
#define GUI_NOTEBOOK_FRAME_BORDER     0
#define GUI_NOTEBOOK_CONTAINER_BORDER 0
#define GUI_NOTEBOOK_BOX_SPACING      0
#define GUI_NOTEBOOK_BOX_PADDING      1

#define GUI_NOTEBOOK_TABLE_XOPTIONS   (GTK_EXPAND | GTK_FILL)
#define GUI_NOTEBOOK_TABLE_YOPTIONS   (GTK_FILL)

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
  GFunc non_destroy_func ;

  ZMapGuiNotebook notebook_spec ;

  ZMapGuiNotebookChapter current_focus_chapter;

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

  ZMapGUITreeView zmap_tree_view;

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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static GtkWidget *addFeatureSection(FooCanvasItem *item, GtkWidget *parent, MakeNotebook make_notebook) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void saveChapterCB(gpointer data, guint cb_action, GtkWidget *widget);
static void saveAllChaptersCB(gpointer data, guint cb_action, GtkWidget *widget);
static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);
static void destroyCB(GtkWidget *widget, gpointer data);
static void changeNotebookCB(GtkWidget *widget, gpointer unused_data) ;
static void cancelCB(GtkWidget *widget, gpointer user_data) ;
static void invoke_cancel_callback(gpointer data, gpointer user_data) ;
static void okCB(GtkWidget *widget, gpointer user_data) ;
static void invoke_apply_callback(gpointer data, gpointer user_data) ;
static void invoke_save_callback(gpointer data, gpointer user_data) ;
static void invoke_save_all_callback(gpointer data, gpointer user_data) ;

static ZMapGuiNotebookTagValue findTagInPage(ZMapGuiNotebookPage page, const char *tagvalue_name) ;
static void eachSubsectionCB(gpointer data, gpointer user_data) ;
static void eachParagraphCB(gpointer data, gpointer user_data) ;
static void eachTagValueCB(gpointer data, gpointer user_data) ;
ZMapGuiNotebookAny getAnyParent(ZMapGuiNotebookAny any_child, ZMapGuiNotebookType parent_type) ;
static void callChapterCBs(MakeNotebook make_notebook);
static void callCBsAndDestroy(MakeNotebook make_notebook) ;
static void buttonToggledCB(GtkToggleButton *button, gpointer user_data) ;
static void entryActivateCB(GtkEntry *entry, gpointer  user_data) ;
static void changeCB(GtkEntry *entry, gpointer user_data) ;
static gboolean validateTagValue(ZMapGuiNotebookTagValue tag_value, char *text, gboolean update_original) ;
static void freeBookResources(gpointer data, gpointer user_data_unused) ;

static gboolean mergeAny(ZMapGuiNotebookAny note_any, ZMapGuiNotebookAny note_any_new) ;
static void mergeChildren(void *data, void *user_data) ;
static ZMapGuiNotebookAny findAnyChild(GList *children, ZMapGuiNotebookAny new_child) ;
static gint compareFuncCB(gconstpointer a, gconstpointer b) ;
static void flagNotebookIgnoreDuplicates(gpointer any, gpointer unused);

static void translate_string_types_to_gtypes(gpointer list_data, gpointer new_list_data);

static gboolean disallow_empty_strings(ZMapGuiNotebookAny notebook_any,
				       const char *entry_text, gpointer user_data);

static GtkWidget *makeNotebookWidget(MakeNotebook make_notebook) ;


static gboolean rowSelectCB(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path,
			    gboolean path_currently_selected, gpointer user_data) ;

static void propogateExpand(GtkWidget *box, GtkWidget *child, GtkWidget *topmost);
static GtkWidget *notebookNewFrameIn(const char *frame_name, GtkWidget *parent_container);




/* menu GLOBAL! */
static GtkItemFactoryEntry menu_items_G[] =
  {
    /* File */
    { "/_File",         NULL,                NULL,              0,    "<Branch>", NULL},
    { "/File/Save",     "<control>S",        saveChapterCB,     0,    NULL,       NULL},
    { "/File/Save All", "<control><shift>S", saveAllChaptersCB, 0,    NULL,       NULL},
    { "/File/Close",    "<control>W",        requestDestroyCB,  0,    NULL,       NULL},

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



/* The model for building a notebook for displaying/editting resources/data etc is:
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

  notebook = (ZMapGuiNotebook)createSectionAny(ZMAPGUI_NOTEBOOK_BOOK, notebook_name) ;

  if (notebook)
    {
      notebook->editable = editable ;
      notebook->cleanup_cb = cleanup_cb ;
      notebook->user_cleanup_data = user_cleanup_data ;
    }
  else
    {
      zMapWarning("%s", "Error creating notebook\n") ;
    }

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

  /* zMapAssert((!user_callbacks || (user_callbacks && user_callbacks->cancel_func && user_callbacks->apply_func))) ;*/
  if (!((!user_callbacks || (user_callbacks && user_callbacks->cancel_func && user_callbacks->apply_func))) )
    return chapter ;

  chapter = (ZMapGuiNotebookChapter)createSectionAny(ZMAPGUI_NOTEBOOK_CHAPTER, chapter_name) ;

  if (user_callbacks)
    {
      chapter->user_CBs.cancel_func = user_callbacks->cancel_func ;
      chapter->user_CBs.apply_func  = user_callbacks->apply_func ;
      chapter->user_CBs.save_func   = user_callbacks->save_func;
      chapter->user_CBs.edit_func   = user_callbacks->edit_func;

      if(chapter->user_CBs.edit_func == NULL)
	chapter->user_CBs.edit_func = disallow_empty_strings;

      chapter->user_CBs.cancel_data = user_callbacks->cancel_data ;
      chapter->user_CBs.apply_data  = user_callbacks->apply_data ;
      chapter->user_CBs.save_data   = user_callbacks->save_data;
      chapter->user_CBs.edit_data   = user_callbacks->edit_data;
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
 *                        types are int, float and string..
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

      g_list_foreach(types, translate_string_types_to_gtypes,
		     &(paragraph->compound_types));
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

      tag_value->original_data.bool_value =
	tag_value->data.bool_value = bool_arg ;
    }
  else if (strcmp(arg_type, "int") == 0)
    {
      tag_value->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_INT ;

      int_arg = va_arg(args, int) ;

      tag_value->original_data.int_value =
	tag_value->data.int_value = int_arg ;
    }
  else if (strcmp(arg_type, "float") == 0)
    {
      tag_value->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_FLOAT ;

      double_arg = va_arg(args, double) ;

      tag_value->original_data.float_value =
	tag_value->data.float_value = double_arg ;
    }
  else if (strcmp(arg_type, "string") == 0)
    {
      tag_value->data_type = ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_STRING ;

      string_arg = va_arg(args, char *) ;

      tag_value->data.string_value =
	tag_value->original_data.string_value = g_strdup(string_arg) ;
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
      zMapWarnIfReached() ;
    }

  va_end (args);


  if (paragraph)
    {
      tag_value->parent = paragraph ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;
    }

  return tag_value ;
}


/*! Add a chapter to a notebook.
 *
 * NOTE: this could be made into a general function that adds any to any.
 *
 *
 * @param   notebook     Parent notebook.
 * @param   page         Child chapter.
 * @return               void
 */
void zMapGUINotebookAddChapter(ZMapGuiNotebook notebook, ZMapGuiNotebookChapter chapter)
{
  /* zMapAssert(notebook && chapter) ; */
  if (!notebook || !chapter ) 
    return ; 

  notebook->chapters = g_list_append(notebook->chapters, chapter) ;

  return ;
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
  /* zMapAssert(chapter && page) ;*/
  if (!chapter || !page ) 
    return ; 

  chapter->pages = g_list_append(chapter->pages, page) ;

  return ;
}



/*! Merge two notebooks.
 *
 * The merge consists of:
 *
 * Any sub part of notebook_new that is _not_ in notebook is moved from
 * notebook_new to notebook. Individual sub parts are not merged into
 * each other.
 *
 * Note that the merge is destructive, notebook_new is destroyed.
 *
 *
 * @param   notebook      Target of merge.
 * @param   notebook_new  Subject of merge.
 * @return                void
 */
void zMapGUINotebookMergeNotebooks(ZMapGuiNotebook notebook, ZMapGuiNotebook notebook_new)
{
  /* zMapAssert(notebook); */
  if (!notebook || !notebook_new) 
    return ; 
  /* zMapAssert(notebook_new) ; */

/* MH17:
  don't ignore or else we won't get the description from otterlace
  erm... not true since i renamed the style description
  put this back in case something broke by changing it
 */
 flagNotebookIgnoreDuplicates(notebook, NULL);


  mergeAny((ZMapGuiNotebookAny)notebook, (ZMapGuiNotebookAny)notebook_new) ;

  return ;
}


/* THIS INTERFACE IS NOT GREAT AND NEEDS RENAMING SO IT'S MORE OBVIOUS
 * WHAT IS RETURNED.... */

/* Create the compound notebook widget from the notebook tree.
 * 
 * NOTE: returns a vobx containing a notebook widg, use
 * zMapGUINotebookGetNoteBookWidg() to get the actual notebook widget.
 * 
 *
 * @param notebook_spec  The notebook tree.
 * @return               top container widget of notebook
 */
GtkWidget *zMapGUINotebookCreateWidget(ZMapGuiNotebook notebook_spec)
{
  MakeNotebook make_notebook  ;
  GtkWidget *note_widg = NULL ;

  /* zMapAssert(notebook_spec) ; */
  if (!notebook_spec) 
    return note_widg ; 

  make_notebook = g_new0(MakeNotebookStruct, 1) ;

  make_notebook->notebook_spec = notebook_spec ;

  note_widg = makeNotebookWidget(make_notebook) ;

  return note_widg ;
}


/* Returns the stack notebook widget. */
GtkWidget *zMapGUINotebookGetNoteBookWidg(GtkWidget *compound_note_widget)
{
  GtkWidget *notebook_widg = NULL ;

  notebook_widg = g_object_get_data(G_OBJECT(compound_note_widget), GUI_NOTEBOOK_STACK_SETDATA) ;

  return notebook_widg ;
}


/* Returns the Chapter notebook widget. */
GtkWidget *zMapGUINotebookGetCurrChapterWidg(GtkWidget *compound_note_widget)
{
  GtkWidget *notebook_widg = NULL ;

  notebook_widg = g_object_get_data(G_OBJECT(compound_note_widget), GUI_NOTEBOOK_CURR_SETDATA) ;

  return notebook_widg ;
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

  /* zMapAssert(notebook_spec && help_title && *help_title && help_text && *help_text) ;*/
  if (!notebook_spec || !help_title || !*help_title || !help_text || !*help_text ) 
    return dialog ; 

  make_notebook = g_new0(MakeNotebookStruct, 1) ;

  make_notebook->notebook_spec = notebook_spec ;

  make_notebook->help_title = help_title ;
  make_notebook->help_text = help_text ;


  /*
   * Make dialog
   */
  make_notebook->toplevel = dialog = zMapGUIToplevelNew(NULL, (char *)g_quark_to_string(notebook_spec->name)) ;
  gtk_container_set_border_width(GTK_CONTAINER (dialog), 10);
  g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(destroyCB), make_notebook) ;

  /* Record our state on the dialog widget. */
  g_object_set_data(G_OBJECT(dialog), GUI_NOTEBOOK_SETDATA, make_notebook) ;

  make_notebook->vbox = vbox = gtk_vbox_new(FALSE, GUI_NOTEBOOK_BOX_SPACING) ;
  gtk_box_set_spacing(GTK_BOX(vbox), 5) ;
  gtk_container_add(GTK_CONTAINER(dialog), vbox) ;


  /*
   * Add a menu bar
   */
  gtk_box_pack_start(GTK_BOX(vbox), makeMenuBar(make_notebook), FALSE, FALSE, GUI_NOTEBOOK_BOX_PADDING) ;


  /*
   * Add panel with chapter chooser and stack of chapters
   */
  note_widg = makeNotebookWidget(make_notebook) ;
  gtk_box_pack_start(GTK_BOX(vbox), note_widg, FALSE, FALSE, GUI_NOTEBOOK_BOX_PADDING) ;


  /*
   * Make panel of  usual button stuff.
   */
  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(make_notebook->vbox), frame, FALSE, FALSE, GUI_NOTEBOOK_BOX_PADDING) ;

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

  /* zMapAssert(chapter && page_name && *page_name) ; */
  if (!chapter || !page_name || !*page_name) 
    return page ; 

  page_id = g_quark_from_string(page_name) ;

  if ((page_item = g_list_find_custom(chapter->pages, GINT_TO_POINTER(page_id), compareFuncCB)))
    {
      page = (ZMapGuiNotebookPage)(page_item->data) ;
    }

  return page ;
}

/*! Find a given chapter within a notebook
 *
 * @param notebook     Notebook to search
 * @param chapter_name Name of the Chapter to find.
 * @return             ZMapGuiNotebookChapter or NULL if no matching chapter found.
 */
ZMapGuiNotebookChapter zMapGUINotebookFindChapter(ZMapGuiNotebook notebook, const char *chapter_name)
{
  ZMapGuiNotebookChapter matching_chapter = NULL;
  GList *chapter_item;
  GQuark chapter_id;

  chapter_id = g_quark_from_string(chapter_name);

  if((chapter_item = g_list_find_custom(notebook->chapters, GUINT_TO_POINTER(chapter_id), compareFuncCB)))
    {
      matching_chapter = (ZMapGuiNotebookChapter)(chapter_item->data);
    }

  return matching_chapter ;
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
 * N.B. In the case of strings the returned value will be the actual pointer stored in the
 * tag value.  The caller is responsible for copying this and later freeing it. So be warned
 * the internal value might go away anytime!
 *
 */
gboolean zMapGUINotebookGetTagValue(ZMapGuiNotebookPage page, const char *tagvalue_name,
				    const char *arg_type, ...)
{
  gboolean result = FALSE ;
  ZMapGuiNotebookTagValue tagvalue ;

  /* zMapAssert(page && tagvalue_name && *tagvalue_name && arg_type && *arg_type) ; */ 
  if (!page || tagvalue_name || !*tagvalue_name || !arg_type || !*arg_type) 
    return result ; 

  if ((tagvalue = findTagInPage(page, tagvalue_name)))
    {
      result = TRUE ;
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
	  /* Return just the pointer. It's the caller's responsibility to copy it and later free it. */
	  *string_arg = tagvalue->data.string_value;
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
          result = FALSE ;
          zMapWarnIfReached() ;
	}

      va_end (args);
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
  /* Free everything... */
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
  ZMapGuiNotebookAny book_any = NULL ;
  int size = 0 ;

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
      zMapWarnIfReached() ;
      break ;
    }

  if (size)
    {
      book_any = g_malloc0(size) ;
      book_any->type = type ;

      if (name)
        book_any->name = g_quark_from_string(name) ;
    }
  return book_any ;
}


/* A GFunc() to free up the book resources... */
static void freeBookResources(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookAny book_any = (ZMapGuiNotebookAny)data ;
  ZMapGuiNotebookAny *book_any_ptr = (ZMapGuiNotebookAny *)user_data;

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
	    g_list_foreach(book_any->children, freeBookResources, user_data) ;
	    g_list_free(book_any->children) ;
	  }

	break ;
      }

    case ZMAPGUI_NOTEBOOK_TAGVALUE:
      {
	ZMapGuiNotebookTagValue tag_value = (ZMapGuiNotebookTagValue)book_any ;

	if (tag_value->data_type == ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_STRING &&
	    tag_value->original_data.string_value != NULL)
	  g_free(tag_value->original_data.string_value) ;

	break ;
      }

    default:
      zMapWarnIfReached() ;
      break ;
    }

  book_any->type = ZMAPGUI_NOTEBOOK_INVALID ;		    /* Mark block as invalid. */

  if(book_any_ptr != NULL && *book_any_ptr == book_any)
    {
      g_free(book_any);
      *book_any_ptr = book_any = NULL;
    }
  else
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
  top_widg = make_notebook->notebook_vbox = gtk_vbox_new(FALSE, GUI_NOTEBOOK_BOX_SPACING) ;

  /* Add chapter chooser */
  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(make_notebook->notebook_vbox), frame, FALSE, FALSE, GUI_NOTEBOOK_BOX_PADDING) ;

  vbox_buttons = gtk_vbox_new(FALSE, GUI_NOTEBOOK_BOX_SPACING) ;
  gtk_container_add(GTK_CONTAINER(frame), vbox_buttons) ;

  make_notebook->notebook_chooser = gtk_hbutton_box_new() ;
  gtk_box_pack_start(GTK_BOX(vbox_buttons), make_notebook->notebook_chooser, FALSE, FALSE, GUI_NOTEBOOK_BOX_PADDING) ;

  /* Make chapter stack holder */
  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(make_notebook->notebook_vbox), frame, TRUE, TRUE, GUI_NOTEBOOK_BOX_PADDING) ;

  make_notebook->notebook_stack = gtk_notebook_new() ;
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(make_notebook->notebook_stack), FALSE) ;
  gtk_container_add(GTK_CONTAINER(frame), make_notebook->notebook_stack) ;

  /* Record stack notebook widg on top vbox widg. */
  g_object_set_data(G_OBJECT(top_widg), GUI_NOTEBOOK_STACK_SETDATA, make_notebook->notebook_stack) ;

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

  /* Record current chapter notebook on top widget of notebook (currently a vbox). */
  g_object_set_data(G_OBJECT(make_notebook->notebook_vbox), GUI_NOTEBOOK_CURR_SETDATA, notebook_widget) ;

  button = gtk_radio_button_new_with_label_from_widget((make_notebook->prev_button
							? GTK_RADIO_BUTTON(make_notebook->prev_button)
							: NULL),
						       g_quark_to_string(chapter->name)) ;
  make_notebook->prev_button = button ;
  g_object_set_data(G_OBJECT(button), GUI_NOTEBOOK_BUTTON_SETDATA, notebook_widget) ;
  gtk_container_add(GTK_CONTAINER(make_notebook->notebook_chooser), button) ;
  g_signal_connect(G_OBJECT(button), "pressed", G_CALLBACK(changeNotebookCB), make_notebook) ;

  notebook_pos = gtk_notebook_append_page(GTK_NOTEBOOK(make_notebook->notebook_stack), notebook_widget, NULL) ;

  g_list_foreach(chapter->pages, makePageCB, make_notebook) ;

  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
    make_notebook->current_focus_chapter = chapter;

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

  make_notebook->curr_page_vbox = gtk_vbox_new(FALSE, GUI_NOTEBOOK_BOX_SPACING) ;

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

  subsection_frame = notebookNewFrameIn(g_quark_to_string(subsection->name),
					make_notebook->curr_page_vbox);

  make_notebook->curr_subsection_vbox = gtk_vbox_new(FALSE, GUI_NOTEBOOK_BOX_SPACING) ;
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

  paragraph_frame = notebookNewFrameIn(g_quark_to_string(paragraph->name),
				       make_notebook->curr_subsection_vbox);

  make_notebook->curr_paragraph_vbox = gtk_vbox_new(FALSE, GUI_NOTEBOOK_BOX_SPACING) ;
  gtk_container_add(GTK_CONTAINER(paragraph_frame), make_notebook->curr_paragraph_vbox) ;

  if (paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE
      || paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_HOMOGENOUS)
    {
      make_notebook->curr_paragraph_rows = 0 ;
      make_notebook->curr_paragraph_columns = 2 ;
      make_notebook->curr_paragraph_table = gtk_table_new(make_notebook->curr_paragraph_rows,
							  make_notebook->curr_paragraph_columns,
							  FALSE) ;
      gtk_table_set_homogeneous(GTK_TABLE(make_notebook->curr_paragraph_table),
				(paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_HOMOGENOUS));

      gtk_table_set_row_spacings(GTK_TABLE(make_notebook->curr_paragraph_table),
				 GUI_NOTEBOOK_DEFAULT_BORDER);

      gtk_container_add(GTK_CONTAINER(make_notebook->curr_paragraph_vbox), make_notebook->curr_paragraph_table) ;

    }
  else if (paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE
	   || paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_HORZ_TABLE
	   || paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_VERT_TABLE)
    {
      GtkWidget *scrolled_window ;
      GtkPolicyType horiz_policy, vert_policy ;

      scrolled_window = gtk_scrolled_window_new(NULL, NULL) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I'd like to do this but either HORZ or VERT result in a blank paragraph..I don't know why. */
      if (paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE)
	{
	  horiz_policy = vert_policy = GTK_POLICY_AUTOMATIC ;
	}
      else if (paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_HORZ_TABLE)
	{
	  horiz_policy = GTK_POLICY_AUTOMATIC ;
	  vert_policy = GTK_POLICY_NEVER ;
	}
      else
	{
	  horiz_policy = GTK_POLICY_NEVER ;
	  vert_policy = GTK_POLICY_AUTOMATIC ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      horiz_policy = vert_policy = GTK_POLICY_AUTOMATIC ;

      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				     horiz_policy, vert_policy) ;

      gtk_container_add(GTK_CONTAINER(make_notebook->curr_paragraph_vbox), scrolled_window) ;


      make_notebook->zmap_tree_view = zMapGUITreeViewCreate();

      g_object_set(G_OBJECT(make_notebook->zmap_tree_view),
		   "row-counter-column", TRUE,
		   "column_count",       g_list_length(paragraph->compound_titles),
		   "selection-mode",     GTK_SELECTION_SINGLE,
		   "selection-func",     rowSelectCB,
		   "selection-data",     NULL,
		   "column_names_q",     paragraph->compound_titles,
		   "column_types",       paragraph->compound_types,
		   "sortable",           TRUE,
		   "sort-column-index",  0,
		   "sort-order",         GTK_SORT_ASCENDING,
		   NULL);

      make_notebook->curr_paragraph_treeview = GTK_WIDGET(zMapGUITreeViewGetView(make_notebook->zmap_tree_view));

      gtk_container_add(GTK_CONTAINER(scrolled_window), make_notebook->curr_paragraph_treeview) ;

      make_notebook->curr_paragraph_model = zMapGUITreeViewGetModel(make_notebook->zmap_tree_view);

      propogateExpand(make_notebook->curr_subsection_vbox, paragraph_frame, make_notebook->curr_page_vbox);
    }

  g_list_foreach(paragraph->tag_values, makeTagValueCB, make_notebook) ;

 if (paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE)
   {
     zMapGUITreeViewAttach(make_notebook->zmap_tree_view);
     //setModelInView(GTK_TREE_VIEW(make_notebook->curr_paragraph_treeview), make_notebook->curr_paragraph_model) ;
   }

  return ;
}


static gboolean editing_finished_cb(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
  GtkEntry *entry = GTK_ENTRY(widget) ;
  ZMapGuiNotebookTagValue tag_value = (ZMapGuiNotebookTagValue)user_data ;
  const char *entry_text ; 

  g_signal_handlers_block_by_func (widget, editing_finished_cb, user_data) ;

  entry_text = gtk_entry_get_text(entry) ;

  if (!event->in)
    {
      ZMapGuiNotebookChapter chapter = NULL ;
      gboolean edit_allowed = TRUE ;
      char *tag_value_text = NULL ;

      chapter = (ZMapGuiNotebookChapter)getAnyParent((ZMapGuiNotebookAny)tag_value, ZMAPGUI_NOTEBOOK_CHAPTER) ;

      if(chapter->user_CBs.edit_func)
	edit_allowed = (chapter->user_CBs.edit_func)((ZMapGuiNotebookAny)tag_value, entry_text,
						     chapter->user_CBs.edit_data) ;

      switch(tag_value->data_type)
	{
	case ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_STRING:
	  if ((tag_value->original_data.string_value))
	    tag_value_text = g_strdup(tag_value->original_data.string_value);
	  else
	    tag_value_text = g_strdup("") ;
	  break;
	case ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_FLOAT:
	  tag_value_text = g_strdup_printf("%g", tag_value->original_data.float_value);
	  break;
	case ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_INT:
	  tag_value_text = g_strdup_printf("%d", tag_value->original_data.int_value);
	  break;
	case ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_BOOL:
	  tag_value_text = g_strdup((tag_value->original_data.bool_value ? "true" : "false"));
	  break;
	default:
          zMapWarnIfReached() ;
          break;
	}

      if (tag_value_text)
        {
          if (g_ascii_strcasecmp(entry_text, tag_value_text) != 0)
            {
              if (edit_allowed)
                {
                  /* update_original with entry_text */
                  validateTagValue(tag_value, (char *)entry_text, TRUE) ;
                }
              else
                {
                  /* revert to original */
                  gtk_entry_set_text(entry, tag_value_text) ;
                }
            }

          g_free(tag_value_text) ;
        }
    }

  g_signal_handlers_unblock_by_func (widget, editing_finished_cb, user_data) ;

  /* Gtk-WARNING **: GtkEntry - did not receive focus-out-event. If you
   * connect a handler to this signal, it must return
   * FALSE so the entry gets the event as well */
  return FALSE ;
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
	gtk_misc_set_alignment(GTK_MISC(tag), 0.0, 0.5);
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

	    if(text && *text)
	      gtk_entry_set_text(GTK_ENTRY(value), text) ;
#ifdef WARN_OF_EMPTY_TAGS
	    else
	      printf("tag '%s' has an empty value.\n", g_quark_to_string(tag_value->tag));
#endif /* WARN_OF_EMPTY_TAGS */

	    gtk_entry_set_editable(GTK_ENTRY(value), notebook->editable) ;

	    if(notebook->editable)
	      {
		/* It appears from RT # 85457 that somehow entry_set_editable(e, FALSE)
		 * doesn't ensure that activate/changed won't be called... window manager bug?
		 * Anyway I'm hoping the addition of the conditional will really ensure this. */
		g_signal_connect(G_OBJECT(value), "activate", G_CALLBACK(entryActivateCB), tag_value) ;

		g_signal_connect(G_OBJECT(value), "changed", G_CALLBACK(changeCB), tag_value) ;

		/* I'm unsure as to the user friendliness of this code... Could easily be turned off */
		g_signal_connect(G_OBJECT(value), "focus-out-event", G_CALLBACK(editing_finished_cb), tag_value) ;
	      }

	    g_free(text) ;
	  }

	if (make_notebook->curr_paragraph->display_type == ZMAPGUI_NOTEBOOK_PARAGRAPH_SIMPLE)
	  {
	    GtkWidget *hbox ;

	    container = hbox = gtk_hbox_new(FALSE, GUI_NOTEBOOK_BOX_SPACING) ;
#ifdef BOX_NOT_CONTAINER
	    gtk_container_add(GTK_CONTAINER(make_notebook->curr_paragraph_vbox), container) ;

	    gtk_container_add(GTK_CONTAINER(hbox), tag) ;

	    gtk_container_add(GTK_CONTAINER(hbox), value) ;
#endif
	    gtk_box_pack_start(GTK_BOX(make_notebook->curr_paragraph_vbox), hbox, FALSE, TRUE, GUI_NOTEBOOK_BOX_PADDING);

	    gtk_box_pack_start(GTK_BOX(hbox), tag,   TRUE, TRUE, GUI_NOTEBOOK_BOX_PADDING);
	    gtk_box_pack_start(GTK_BOX(hbox), value, TRUE, TRUE, GUI_NOTEBOOK_BOX_PADDING);
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
	      gtk_table_attach(GTK_TABLE(make_notebook->curr_paragraph_table),
			       tag,
			       0,
			       1,
			       make_notebook->curr_paragraph_rows - 1,
			       make_notebook->curr_paragraph_rows,
			       GUI_NOTEBOOK_TABLE_XOPTIONS,
			       GUI_NOTEBOOK_TABLE_YOPTIONS,
			       GUI_NOTEBOOK_BOX_PADDING,
			       GUI_NOTEBOOK_BOX_PADDING) ;

	    gtk_table_attach(GTK_TABLE(make_notebook->curr_paragraph_table),
			     value,
			     1,
			     2,
			     make_notebook->curr_paragraph_rows - 1,
			     make_notebook->curr_paragraph_rows,
			     GUI_NOTEBOOK_TABLE_XOPTIONS,
			     GUI_NOTEBOOK_TABLE_YOPTIONS,
			     GUI_NOTEBOOK_BOX_PADDING,
			     GUI_NOTEBOOK_BOX_PADDING) ;
	  }

	break ;
      }
    case ZMAPGUI_NOTEBOOK_TAGVALUE_SCROLLED_TEXT:
      {
	GtkWidget *frame, *scrolled_window, *view ;
	GtkTextBuffer *buffer ;

	frame = notebookNewFrameIn(g_quark_to_string(tag_value->tag),
				   make_notebook->curr_paragraph_vbox);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL) ;
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC) ;
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window) ;

	view = gtk_text_view_new() ;
	gtk_container_add(GTK_CONTAINER(scrolled_window), view) ;
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), notebook->editable) ;
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)) ;
	gtk_text_buffer_set_text(buffer, tag_value->data.string_value, -1) ;

	break ;
      }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

      /* REDUNDANT..... */

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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


    case ZMAPGUI_NOTEBOOK_TAGVALUE_COMPOUND:
      {
	zMapGUITreeViewAddTupleFromColumnData(make_notebook->zmap_tree_view,
					      tag_value->data.compound_values);
#ifdef OLD_VERSION
	addDataToModel(make_notebook->curr_paragraph->num_cols, make_notebook->curr_paragraph->compound_types,
		       GTK_TREE_MODEL(make_notebook->curr_paragraph_model),
		       &(make_notebook->curr_paragraph_iter),
		       tag_value->data.compound_values) ;
#endif
	break ;
      }

    default:
      {
        zMapWarnIfReached() ;
	break ;
      }
    }



  return ;
}


/* Raise the notebook attached to the button to the top of the stack of notebooks. */
static void changeNotebookCB(GtkWidget *widget, gpointer make_notebook_data)
{
  MakeNotebook make_notebook = (MakeNotebook)make_notebook_data;
  GtkWidget *notebook, *notebook_stack ;
  const char *chapter_name;
  gint notebook_pos, notebook_pages ;

  /* Get hold of the notebook, the notebook_stack and the position of the notebook within
   * the notebook_stack. */
  notebook = g_object_get_data(G_OBJECT(widget), GUI_NOTEBOOK_BUTTON_SETDATA) ;

  notebook_stack = g_object_get_data(G_OBJECT(notebook), GUI_NOTEBOOK_STACK_SETDATA) ;

  if ((notebook_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook_stack))) > 1)
    {
      notebook_pos = gtk_notebook_page_num(GTK_NOTEBOOK(notebook_stack), notebook) ;

      if((chapter_name = gtk_button_get_label(GTK_BUTTON(widget))))
	{
	  ZMapGuiNotebookChapter focus_chapter = NULL;

	  focus_chapter = zMapGUINotebookFindChapter(make_notebook->notebook_spec, chapter_name);

	  make_notebook->current_focus_chapter = focus_chapter;
	}

      /* Raise the notebook to the top of the notebooks */
      gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_stack), notebook_pos) ;

      /* Set this notebook to show the first page. */
      gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0) ;

      /* Set this notebook widget on the top notebook. */
      g_object_set_data(G_OBJECT(make_notebook->notebook_vbox), GUI_NOTEBOOK_CURR_SETDATA, notebook) ;
    }

  return ;
}



/* requestDestroyCB(), cancelCB() & okCB() all cause destroyCB() to be called via
 * the gtk widget destroy mechanism. */

static void saveChapterCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  MakeNotebook make_notebook = (MakeNotebook)data;

  make_notebook->non_destroy_func = invoke_save_callback;

  callChapterCBs(make_notebook);

  return ;
}

static void saveAllChaptersCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  MakeNotebook make_notebook = (MakeNotebook)data;

  make_notebook->non_destroy_func = invoke_save_all_callback;

  callChapterCBs(make_notebook);

  return ;
}

static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  MakeNotebook make_notebook = (MakeNotebook)data ;

  make_notebook->destroy_func = invoke_cancel_callback ;

  gtk_widget_destroy(make_notebook->toplevel) ;

  return ;
}

static void cancelCB(GtkWidget *widget, gpointer user_data)
{
  MakeNotebook make_notebook = (MakeNotebook)user_data ;

  make_notebook->destroy_func = invoke_cancel_callback;

  gtk_widget_destroy(make_notebook->toplevel) ;

  return ;
}

static void okCB(GtkWidget *widget, gpointer user_data)
{
  MakeNotebook make_notebook = (MakeNotebook)user_data ;

  make_notebook->destroy_func = invoke_apply_callback ;

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
    make_notebook->destroy_func = invoke_cancel_callback ;

  callCBsAndDestroy(make_notebook) ;

  return ;
}

static void warn_unset_func(gpointer data, gpointer user_data)
{
  zMapLogWarning("%s", "MakeNotebook->(non_)destroy_func was not set when calling callChapterCBs");
}

static void callChapterCBs(MakeNotebook make_notebook)
{
  if(!make_notebook->non_destroy_func)
    make_notebook->non_destroy_func = warn_unset_func;

  g_list_foreach(make_notebook->notebook_spec->chapters,
		 make_notebook->non_destroy_func, make_notebook->current_focus_chapter);

  make_notebook->non_destroy_func = NULL;

  return ;
}

static void callCBsAndDestroy(MakeNotebook make_notebook)
{
  if(!make_notebook->destroy_func)
    make_notebook->destroy_func = warn_unset_func;

  g_list_foreach(make_notebook->notebook_spec->chapters, make_notebook->destroy_func, NULL) ;

  make_notebook->destroy_func = NULL;

  /* Only destroy if created! */
  if(make_notebook->zmap_tree_view)
    zMapGUITreeViewDestroy(make_notebook->zmap_tree_view);

  if(make_notebook->notebook_spec->cleanup_cb)
    (make_notebook->notebook_spec->cleanup_cb)((ZMapGuiNotebookAny)(make_notebook->notebook_spec),
					       make_notebook->notebook_spec->user_cleanup_data) ;
  make_notebook->notebook_spec = NULL;
  g_free(make_notebook) ;

  return ;
}

static void invoke_save_callback(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)data;
  ZMapGuiNotebookAny notebook_any = (ZMapGuiNotebookAny)data;
  ZMapGuiNotebookChapter current_chapter = (ZMapGuiNotebookChapter)user_data;

  if(current_chapter == chapter)
    {
      if(chapter->user_CBs.save_func)
	(chapter->user_CBs.save_func)(notebook_any, chapter->user_CBs.save_data);

      chapter->changed = FALSE;
    }

  return ;
}

static void invoke_save_all_callback(gpointer data, gpointer user_data)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)data;
  ZMapGuiNotebookAny notebook_any = (ZMapGuiNotebookAny)data;

  if(chapter->user_CBs.save_func)
    (chapter->user_CBs.save_func)(notebook_any, chapter->user_CBs.save_data);

  return ;
}

static void invoke_cancel_callback(gpointer data, gpointer user_data_unused)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)data ;

  if (chapter->user_CBs.cancel_func)
    chapter->user_CBs.cancel_func((ZMapGuiNotebookAny)chapter, chapter->user_CBs.cancel_data) ;

  return ;
}


static void invoke_apply_callback(gpointer data, gpointer user_data_unused)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)data ;

  if (chapter->changed)
    {
      if (chapter->user_CBs.apply_func)
	chapter->user_CBs.apply_func((ZMapGuiNotebookAny)chapter, chapter->user_CBs.apply_data) ;

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

  if ((validateTagValue(tag_value, text, FALSE)))
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
#ifdef RDS_DEBUGGING
  printf("changedCB called\n");
#endif /* RDS_DEBUGGING */
  if ((validateTagValue(tag_value, text, FALSE)))
    {
      ZMapGuiNotebookChapter chapter ;

      /* If tag ok then record that something has been changed. */
      chapter = (ZMapGuiNotebookChapter)getAnyParent((ZMapGuiNotebookAny)tag_value, ZMAPGUI_NOTEBOOK_CHAPTER) ;
      chapter->changed = TRUE ;
    }

  return ;
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



/* Callback to paste text version of row contents into clipboard each time a row is selected by user. */
static gboolean rowSelectCB(GtkTreeSelection *selection, GtkTreeModel *tree_model, GtkTreePath *path,
			    gboolean path_currently_selected, gpointer unused)
{
  gboolean select_row = TRUE ;
  GtkTreeIter iter ;
  int n_columns;


  /* gtk will call this function for _every_ row change so we get called first
   * for the about to be selected row, then the about to be deselected row and
   * (for reasons I don't understand then the about to be selected row _again_.
   * This feels like a gtk bug to me, we don't bother to handle it as it simply
   * means we put stuff in the cut buffer twice.... */
  if (!path_currently_selected && gtk_tree_model_get_iter(tree_model, &iter, path))
    {
      GString *text ;
      int i ;
      GtkTreeView *tree_view ;
      GtkWidget *toplevel ;

      text = g_string_sized_new(500) ;

      n_columns = gtk_tree_model_get_n_columns(tree_model);

      /* gtk has no call to allow us to get several row values in one go by building an
       * argument list so we have to chunter through them one by painful one. */
      /* What would it have taken to do a gtk_tree_model_get_values() */
#ifdef GTK_TREE_MODEL_GET_VALUES_CODE
      GValue *vals  = g_new0(GValue, n_columns);
      gint *columns = g_new0(gint, n_columns);
      for(i=0; i<n_columns;i++){ columns[i]=i; }
      gtk_tree_model_get_values(tree_model,
				&iter, n_columns,
				columns,
				vals);
      g_free(vals);
      g_free(columns);
#endif

      for(i = 0; i < n_columns; i++)
	{
	  GType column_type;

	  column_type = gtk_tree_model_get_column_type(tree_model, i);

	  switch(column_type)
	    {
	    case G_TYPE_STRING:
	      {
		char *tmp;
		gtk_tree_model_get(tree_model, &iter, i, &tmp, -1);
		g_string_append_printf(text, "%s ", tmp);
	      }
	      break;
	    case G_TYPE_INT:
	      {
		int tmp;
		gtk_tree_model_get(tree_model, &iter, i, &tmp, -1);
		g_string_append_printf(text, "%d ", tmp);
	      }
	      break;
	    case G_TYPE_FLOAT:
	      {
		float tmp;
		gtk_tree_model_get(tree_model, &iter, i, &tmp, -1);
		g_string_append_printf(text, "%f ", tmp);
	      }
	      break;
	    case G_TYPE_BOOLEAN:
	      {
		gboolean tmp;
		gtk_tree_model_get(tree_model, &iter, i, &tmp, -1);
		g_string_append_printf(text, "%s ", (tmp ? "true" : "false"));
	      }
	      break;
	    default:
	      g_string_append_printf(text, "column error idx (%d)", i);
	      break;
	    }
	}

      /* Now paste and then destroy the gstring... */
      tree_view = gtk_tree_selection_get_tree_view(selection) ;
      toplevel  = zMapGUIFindTopLevel(GTK_WIDGET(tree_view)) ;
      zMapGUISetClipboard(toplevel, text->str) ;

      g_string_free(text, TRUE) ;
    }


  return select_row ;
}


static gboolean validateTagValue(ZMapGuiNotebookTagValue tag_value, char *text, gboolean update_original)
{
  gboolean status = FALSE ;


  switch (tag_value->data_type)
    {
    case ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_BOOL:
      {
	gboolean tmp = FALSE ;

	if ((status = zMapStr2Bool(text, &tmp)))
	  {
	    if(!update_original)
	      tag_value->data.bool_value = tmp ;
	    else
	      tag_value->original_data.bool_value = tag_value->data.bool_value = tmp;
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
	    if(!update_original)
	      tag_value->data.int_value = tmp ;
	    else
	      tag_value->original_data.int_value = tag_value->data.int_value = tmp ;
	    status = TRUE ;
	  }
	else if(text && text[0] == '\0')
	  tag_value->data.int_value = tmp;
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
	    if(!update_original)
	      tag_value->data.float_value = tmp ;
	    else
	      tag_value->original_data.float_value = tag_value->data.float_value = tmp;
	    status = TRUE ;
	  }
	else if(text && text[0] == '\0')
	  tag_value->data.float_value = tmp;
	else
	  {
	    zMapWarning("Invalid float number: %s", text) ;
	  }

	break ;
      }
    case ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_STRING:
      {
	/* There's no point checking here as the text _is_ the pointer from the gtkentry! */
	/* This means it's automatically updated anyway... */
	if (text)
	  {
	    tag_value->data.string_value = (text && *text ? text : NULL);

	    if(update_original && *text)
	      {
		if(tag_value->original_data.string_value)
		  g_free(tag_value->original_data.string_value);
		tag_value->original_data.string_value = g_strdup(text);
	      }

	    if(*text)
	      status = TRUE ;
	  }
	else
	  {
	    tag_value->data.string_value = NULL;
	    /* This is very bad... */
	    zMapWarning("Invalid string: %s", (text ? "No string" : "NULL string pointer")) ;
	  }

	break ;
      }
    default:
      {
        zMapWarning("%s", "Invalid tag data type") ;
        zMapWarnIfReached() ;
	break ;
      }
    }

  return status ;
}


/* Merges one notebook_any with another, the two _must_ be of the same notebook type.
 * The merge is of the _children_ of these notebook_any objects, NOT the objects
 * themselves. The merge is done destructively: if there are children they are moved to note_any
 * and any levels in note_any_new without children are removed. */
static gboolean mergeAny(ZMapGuiNotebookAny note_any, ZMapGuiNotebookAny note_any_new)
{
  gboolean result = FALSE ;				    /* Usused....????? */

  /* zMapAssert(note_any->type == note_any_new->type) ;*/
  if (!note_any || !note_any_new || note_any->type != note_any_new->type ) 
    return result ; 

  /* Now merge the children of note_any_new into the children of note_any */
  if (note_any_new->children)
    g_list_foreach(note_any_new->children, mergeChildren, note_any) ;

  /* Now clear up note_any_new by removing it from its parent (if it has one) and then destroying
   * it. NOTE that the order this all happens in is important, stuff in note_any_new is destroyed
   * on the way up. */
  if (note_any_new->parent)
    {
      note_any_new->parent->children = g_list_remove(note_any_new->parent->children, note_any_new) ;
    }

  zMapGUINotebookDestroyAny(note_any_new) ;

  return result ;
}


/* Merge a child notebookany into the other children of the given parent. */
static void mergeChildren(void *data, void *user_data)
{
  ZMapGuiNotebookAny new_child = (ZMapGuiNotebookAny)data ;
  ZMapGuiNotebookAny parent = (ZMapGuiNotebookAny)user_data ;
  ZMapGuiNotebookAny child ;

  if (!(new_child->name))
    {
      /* child has no name so merge it with the first child in the list (this allows
       * merging of anonymous items. */

      child = parent->children->data ;
      mergeAny(child, new_child) ;
    }
  else if ((child = findAnyChild(parent->children, new_child)))
    {
      /* child has same name as a child already in the list so merge them together. */

      mergeAny(child, new_child) ;
    }
  else
    {
      /* child has a name and is not in the list of children so just add it. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* DEBUG.... */
      char *new_child_name = g_quark_to_string(new_child->name) ;
      char *parent_name = g_quark_to_string(parent->name) ;

      printf("About to add %s to children of %s\n",
	     g_quark_to_string(new_child->name),
	     g_quark_to_string(parent->name)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      parent->children = g_list_append(parent->children, new_child) ;
      new_child->parent->children = g_list_remove(new_child->parent->children, new_child) ;
    }

  return ;
}


/* Looks for a child in children with the same name as new_child, returns NULL
 * if not found, child with same name if found. */
static ZMapGuiNotebookAny findAnyChild(GList *children, ZMapGuiNotebookAny new_child)
{
  ZMapGuiNotebookAny child = NULL ;
  GList *list_item ;
  GQuark child_id ;

  child_id = new_child->name ;

  if ((list_item = g_list_find_custom(children, GINT_TO_POINTER(child_id), compareFuncCB)))
    {
      child = (ZMapGuiNotebookAny)(list_item->data) ;
    }

  return child ;
}




/* A GCompareFunc() to find a notebookany with a given name. */
static gint compareFuncCB(gconstpointer a, gconstpointer b)
{
  gint result = -1 ;
  ZMapGuiNotebookAny any = (ZMapGuiNotebookAny)a ;
  GQuark tag_id = GPOINTER_TO_INT(b) ;

  if ((any->name == tag_id) &&
      ((any->type != ZMAPGUI_NOTEBOOK_TAGVALUE) ||
       (any->type == ZMAPGUI_NOTEBOOK_TAGVALUE && any->ignore_duplicates)))
    result = 0 ;

  return result ;
}

static void flagNotebookIgnoreDuplicates(gpointer list_data, gpointer unused)
{
  ZMapGuiNotebookAny any = (ZMapGuiNotebookAny)list_data;

  any->ignore_duplicates = 1;

  g_list_foreach(any->children, flagNotebookIgnoreDuplicates, NULL);

  return ;
}


static void propogateExpand(GtkWidget *box, GtkWidget *child, GtkWidget *topmost)
{
  gboolean expand, fill;
  guint padding;
  GtkPackType pack_type;

  do
    {
      if(GTK_IS_BOX(box))
	{
	  gtk_box_query_child_packing(GTK_BOX(box), child, &expand, &fill, &padding, &pack_type);

	  expand = TRUE;
	  gtk_box_set_child_packing(GTK_BOX(box), child, expand, fill, padding, pack_type);

	}
      if(box == topmost)
	break;
    }
  while((child = box) && (box = gtk_widget_get_parent(child)));

  return ;
}

static GtkWidget *notebookNewFrameIn(const char *frame_name, GtkWidget *parent_container)
{
  GtkWidget *frame;
  gboolean with_frame = TRUE;

  frame = gtk_frame_new(frame_name);

  if(!with_frame)
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);

  /* BOX is subclass of container... check first. */
  if(GTK_IS_BOX(parent_container))
    {
      gtk_box_pack_start(GTK_BOX(parent_container), frame, FALSE, TRUE, GUI_NOTEBOOK_BOX_PADDING);
    }
  else if(GTK_IS_CONTAINER(parent_container))
    {
      gtk_container_set_border_width(GTK_CONTAINER(parent_container),
				     GUI_NOTEBOOK_CONTAINER_BORDER);

      gtk_container_add(GTK_CONTAINER(parent_container), frame);
    }

  return frame;
}

static void translate_string_types_to_gtypes(gpointer list_data, gpointer new_list_data)
{
  GList **new_list = (GList **)new_list_data;
  GType type;
  gboolean known_type = TRUE;

  if (g_quark_from_string("bool") == GPOINTER_TO_INT(list_data))
    type = G_TYPE_BOOLEAN ;
  if (g_quark_from_string("int") == GPOINTER_TO_INT(list_data))
    type = G_TYPE_INT ;
  else if (g_quark_from_string("float") == GPOINTER_TO_INT(list_data))
    type = G_TYPE_FLOAT ;
  else if (g_quark_from_string("string") == GPOINTER_TO_INT(list_data))
    type = G_TYPE_STRING ;
  else
    known_type = FALSE;

  if(known_type)
    *new_list = g_list_append(*new_list, GINT_TO_POINTER(type));

  return ;
}


static gboolean disallow_empty_strings(ZMapGuiNotebookAny notebook_any, const char *entry_text, gpointer user_data)
{
  gboolean allowed = TRUE;

  if(!entry_text || (entry_text && !*entry_text))
    allowed = FALSE;

  return allowed;
}

