/*  File: zmapUtilsGUI.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Set of general GUI functions including menus, file
 *              choosers, GTK notebooks and utility functions.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_GUI_H
#define ZMAP_UTILS_GUI_H

#include <gtk/gtk.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <ZMap/zmapUtilsMesg.h>
#include <ZMap/zmapFeature.h>


typedef enum {ZMAPGUI_USERDATA_INVALID, ZMAPGUI_USERDATA_BOOL, ZMAPGUI_USERDATA_TEXT} ZMapGUIMsgUserDataType ;


typedef struct
{
  ZMapGUIMsgUserDataType type ;
  gboolean hide_input ;					    /* hide text input, e.g. for passwords. */
  union
  {
    gboolean bool ;
    char *text ;
  } data ;
} ZMapGUIMsgUserDataStruct, *ZMapGUIMsgUserData ;



typedef enum {ZMAPGUI_PIXELS_PER_CM, ZMAPGUI_PIXELS_PER_INCH,
	      ZMAPGUI_PIXELS_PER_POINT} ZMapGUIPixelConvType ;

/*! A bit field I found I needed to make it easier to calc what had been clamped. */
typedef enum
  {
    ZMAPGUI_CLAMP_INIT  = 0,
    ZMAPGUI_CLAMP_NONE  = (1 << 0),
    ZMAPGUI_CLAMP_START = (1 << 1),
    ZMAPGUI_CLAMP_END   = (1 << 2)
  } ZMapGUIClampType;




/* ZMap custom cursors that can be returned by zMapGUIGetCursor(), they must all have the "zmap_" prefix. */
#define ZMAPGUI_CURSOR_PREFIX    "zmap_"
#define ZMAPGUI_CURSOR_CROSS     "zmap_cross"
#define ZMAPGUI_CURSOR_CROSSHAIR "zmap_crosshair"
#define ZMAPGUI_CURSOR_CIRCLE    "zmap_circle"
#define ZMAPGUI_CURSOR_NOENTRY   "zmap_noentry"



/* Struct to hold text attributes for zMapGUIShowTextFull(), the attributes are applied
 * to text in the range  start -> end  (zero-based coords). */
typedef struct ZMapGuiTextAttrStructName
{
  int start, end ;					    /* text indices. */

  /* Text colours. */
  GdkColor *foreground ;
  GdkColor *background ;
} ZMapGuiTextAttrStruct, *ZMapGuiTextAttr ;



/*! Callback function for menu items. The id indicates which menu_item was selected
 *  resulting in the call to this callback. */
typedef void (*ZMapGUIMenuItemCallbackFunc)(int menu_item_id, gpointer callback_data) ;
typedef void (*ZMapGUIRadioButtonCBFunc)(GtkWidget *button, gpointer data, gboolean button_active);


/*! Types of menu entry. */
typedef enum {ZMAPGUI_MENU_NONE, ZMAPGUI_MENU_BRANCH, ZMAPGUI_MENU_SEPARATOR,
	      ZMAPGUI_MENU_TOGGLE, ZMAPGUI_MENU_TOGGLEACTIVE,
	      ZMAPGUI_MENU_RADIO, ZMAPGUI_MENU_RADIOACTIVE,
	      ZMAPGUI_MENU_NORMAL, ZMAPGUI_MENU_HIDE } ZMapGUIMenuType ;


/*! Types of help page that can be displayed. */
typedef enum {ZMAPGUI_HELP_GENERAL, ZMAPGUI_HELP_ALIGNMENT_DISPLAY,
	      ZMAPGUI_HELP_RELEASE_NOTES, ZMAPGUI_HELP_KEYBOARD, ZMAPGUI_HELP_WHATS_NEW } ZMapHelpType ;


/*! Bit of explanation here....
 * I changed my mind and we now have an array of structs which contain all the information required.
 * This includes which window to split (original or new) and which orientation to split!
 */
typedef enum
{
  ZMAPSPLIT_NONE,
  ZMAPSPLIT_ORIGINAL,
  ZMAPSPLIT_LAST,
  ZMAPSPLIT_NEW,
  ZMAPSPLIT_BAD_PATTERN
} ZMapSplitWindow;


typedef struct
{
  ZMapSplitWindow subject;
  GtkOrientation orientation;
  /* ... This shouldn't get too specific if it stays here! ... */
} ZMapSplitPatternStruct, *ZMapSplitPattern;


/*!
 * Defines a menu item. */
typedef struct
{
  ZMapGUIMenuType type ;				    /* Title, separator etc. */
  char *name ;						    /*!< Title string of menu item. */
  int id ;						    /*!< Number uniquely identifying this
							      menu item within a menu. */
  ZMapGUIMenuItemCallbackFunc callback_func ;		    /*!< Function to call when this item
							      is selected.  */
  gpointer callback_data ;				    /*!< Data to pass to callback function. */

  /* accelerator is at the end, because I'm lazy... */
  char *accelerator;		                            /*!< an accelerator for the menu option... */
} ZMapGUIMenuItemStruct, *ZMapGUIMenuItem ;



typedef struct
{
  int        value;
  char      *name;
  GtkWidget *widget;
} ZMapGUIRadioButtonStruct, *ZMapGUIRadioButton ;


/* A wrongly named (makes it look too general) data struct to hold
 * info for align/block information plus the original data that may
 * have been set in the GUIMenuItem. Code currently does a swap
 *
 * tmp = item->callback_data;
 * sub->original = tmp;
 * item->callback_data = sub;
 *
 * Not really that much fun, but I couldn't think of an alternative...
 *
 */

typedef struct
{
  GQuark align_unique_id;
  GQuark block_unique_id;
  gpointer original_data;
}ZMapGUIMenuSubMenuDataStruct, *ZMapGUIMenuSubMenuData;



/* A couple of macros to correctly test keyboard modifiers for events that include them:
 *
 *       zMapGUITestModifiers() will return TRUE even if other modifiers are on (e.g. caps lock).
 *   zMapGUITestModifiersOnly() the second will return TRUE _only_ if no other modifiers are on.
 *
 * Use them like this:
 *
 * if (zMapGUITestModifiers(but_event, (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
 *   {
 *      doSomething() ;
 *   }
 *
 *  */
#define zMapGUITestModifiers(EVENT, MODS) \
  (((EVENT)->state & (MODS)) == (MODS))

#define zMapGUITestModifiersOnly(EVENT, MODS) \
  (((EVENT)->state | (MODS)) == ((EVENT)->state & (MODS)))





/*! Convenience routines for creating GTK notebooks and their pages and fields.
 *
 * The idea is that the caller creates a "tree" of pages and fields and then
 * this package creates/manages the notebook from this tree. The tree must have
 * all levels in it, e.g. you cannot skip say "chapter".
 *
 */

/*! Subparts of NoteBook */
typedef enum
  {
    ZMAPGUI_NOTEBOOK_INVALID,
    ZMAPGUI_NOTEBOOK_BOOK,
    ZMAPGUI_NOTEBOOK_CHAPTER,
    ZMAPGUI_NOTEBOOK_PAGE,
    ZMAPGUI_NOTEBOOK_SUBSECTION,
    ZMAPGUI_NOTEBOOK_PARAGRAPH,
    ZMAPGUI_NOTEBOOK_TAGVALUE
  } ZMapGuiNotebookType ;


/*! Types of paragraph. */
typedef enum
  {
    ZMAPGUI_NOTEBOOK_PARAGRAPH_INVALID,
    ZMAPGUI_NOTEBOOK_PARAGRAPH_SIMPLE,			    /* Simple vertical list of tag values. */
    ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,		    /* Aligned table of tag value pairs. */
    ZMAPGUI_NOTEBOOK_PARAGRAPH_HOMOGENOUS,		    /* All tag value pairs have same tag
							       which is only displayed once. */
    ZMAPGUI_NOTEBOOK_PARAGRAPH_COMPOUND_TABLE		    /* One or more values per tag in
							       multi-column list. */
  } ZMapGuiNotebookParagraphDisplayType ;


/*! Types of display for tag value pair. */
typedef enum
  {
    ZMAPGUI_NOTEBOOK_TAGVALUE_INVALID,

    ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,			    /* For boolean values, click box on/off. */
    ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,			    /* Simple tag value pair */
    ZMAPGUI_NOTEBOOK_TAGVALUE_COMPOUND,			    /* Multiple values per tag (only for
							       compound table paragraph). */
    ZMAPGUI_NOTEBOOK_TAGVALUE_SCROLLED_TEXT,		    /* frame with scrolled text area. */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* THIS HAS TO GO.....AND BECOME SOMETHING LIKE TREEVIEW.... */
    ZMAPGUI_NOTEBOOK_TAGVALUE_ITEM			    /* treeview of feature fundamentals */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  } ZMapGuiNotebookTagValueDisplayType ;


/*! Types of value for the tag value pair. */
typedef enum
  {
    ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_INVALID,
    ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_BOOL,
    ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_INT,
    ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_FLOAT,
    ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_STRING,
    ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_ITEM,
    ZMAPGUI_NOTEBOOK_TAGVALUE_TYPE_COMPOUND
  } ZMapGuiNotebookTagValueDataType ;


/* forward decs to avoid later declare problems. */
typedef struct _ZMapGuiNotebookAnyStruct *ZMapGuiNotebookAny ;
typedef struct _ZMapGuiNotebookStruct *ZMapGuiNotebook ;
typedef struct _ZMapGuiNotebookChapterStruct *ZMapGuiNotebookChapter ;
typedef struct _ZMapGuiNotebookPageStruct *ZMapGuiNotebookPage ;
typedef struct _ZMapGuiNotebookSubsectionStruct *ZMapGuiNotebookSubsection ;
typedef struct _ZMapGuiNotebookParagraphStruct *ZMapGuiNotebookParagraph ;
typedef struct _ZMapGuiNotebookTagValueStruct *ZMapGuiNotebookTagValue ;


/*! Prototype for "Cancel", "OK" and "Save" callbacks.  */
typedef void (*ZMapGUINotebookCallbackFunc)(ZMapGuiNotebookAny any_section, gpointer user_data) ;
/*! Prototype for "Edit" callbacks  */
typedef gboolean (*ZMapGUINotebookEditCallbackFunc)(ZMapGuiNotebookAny any_section,
						    const char *entry_text, gpointer user_data);

/*! Creating a notebook requires this struct with callback functions for when
 * user selects "Cancel" or "OK". */
typedef struct
{
  ZMapGUINotebookCallbackFunc     cancel_func ;
  gpointer                        cancel_data ;
  ZMapGUINotebookCallbackFunc     apply_func  ;
  gpointer                        apply_data  ;
  ZMapGUINotebookEditCallbackFunc edit_func   ;
  gpointer                        edit_data   ;
  ZMapGUINotebookCallbackFunc     save_func   ;
  gpointer                        save_data   ;
} ZMapGuiNotebookCBStruct, *ZMapGuiNotebookCB ;


/*! General note book struct, points to any of the notebook, chapter, paragraph or tagvalue structs.
 * All the other structs must start with these fields. */
typedef struct _ZMapGuiNotebookAnyStruct
{
  ZMapGuiNotebookType type ;
  GQuark name ;
  ZMapGuiNotebookAny parent ;
  unsigned int ignore_duplicates : 1;	/* RT_66037 */
  GList *children ;
} ZMapGuiNotebookAnyStruct ;


/*! Notebooks contain sets of chapters. */
typedef struct _ZMapGuiNotebookStruct
{
  ZMapGuiNotebookType type ;
  GQuark name ;
  ZMapGuiNotebookAny parent_unused ;			    /* Always NULL, no parent ever. */
  unsigned int ignore_duplicates : 1;	/* RT_66037 */
  GList *chapters ;

  gboolean editable ;
  ZMapGUINotebookCallbackFunc cleanup_cb ;
  void *user_cleanup_data ;
} ZMapGuiNotebookStruct ;


/*! Chapters contain sets of pages, chapters must be named. Callbacks are given per chapter
 * because these are most likely to correspond to natural divisions within the code. */
typedef struct _ZMapGuiNotebookChapterStruct
{
  ZMapGuiNotebookType type ;
  GQuark name ;
  ZMapGuiNotebook parent ;
  unsigned int ignore_duplicates : 1;	/* RT_66037 */
  GList *pages ;

  gboolean changed ;					    /* FALSE if no changes have been made. */

  /* user callbacks to be called on various actions. */
  ZMapGuiNotebookCBStruct user_CBs ;

} ZMapGuiNotebookChapterStruct ;


/*! Pages contain sets of Subsections, pages must be named. */
typedef struct _ZMapGuiNotebookPageStruct
{
  ZMapGuiNotebookType type ;
  GQuark name ;
  ZMapGuiNotebookChapter parent ;
  unsigned int ignore_duplicates : 1;	/* RT_66037 */
  GList *subsections ;

} ZMapGuiNotebookPageStruct ;


/*! Subsection contain sets of paragraphs, name is optional. */
typedef struct _ZMapGuiNotebookSubsectionStruct
{
  ZMapGuiNotebookType type ;
  GQuark name ;
  ZMapGuiNotebookPage parent ;
  unsigned int ignore_duplicates : 1;	/* RT_66037 */
  GList *paragraphs ;

} ZMapGuiNotebookSubsectionStruct ;


/*! Each paragraph contains sets of tagvalues. The paragraph can be named or anonymous. */
typedef struct _ZMapGuiNotebookParagraphStruct
{
  ZMapGuiNotebookType type ;
  GQuark name ;
  ZMapGuiNotebookSubsection parent ;
  unsigned int ignore_duplicates : 1;	/* RT_66037 */
  GList *tag_values ;

  ZMapGuiNotebookParagraphDisplayType display_type ;

  /* For compound paragraph we need column header titles and type of data in each column. */
  int num_cols ;
  GList *compound_titles ;
  GList *compound_types ;

} ZMapGuiNotebookParagraphStruct ;


/*! Tag Value, each piece of data is a tag/values tuple */
typedef struct _ZMapGuiNotebookTagValueStruct
{
  ZMapGuiNotebookType type ;
  GQuark tag ;
  ZMapGuiNotebookParagraph parent ;
  unsigned int ignore_duplicates : 1;	/* RT_66037 */
  GList *children_unused ;				    /* Always NULL, no children ever. */

  ZMapGuiNotebookTagValueDisplayType display_type ;

  ZMapGuiNotebookTagValueDataType data_type ;
  union
  {
    gboolean bool_value ;
    int int_value ;
    double float_value ;
    char *string_value ;
    FooCanvasItem *item_value ;
    GList *compound_values ;
  } data ;

  union
  {
    gboolean bool_value ;
    int int_value ;
    double float_value ;
    char *string_value ;
    FooCanvasItem *item_value ;
    GList *compound_values ;
  } original_data ;

  int num_values ;					    /* Only with compound_values. */

} ZMapGuiNotebookTagValueStruct ;


typedef void (*ZMapFileChooserContentAreaCB)(GtkWidget *vbox, gpointer user_data);





gint my_gtk_run_dialog_nonmodal(GtkWidget *toplevel) ;

void zMapGUIRaiseToTop(GtkWidget *widget);

GtkWidget *zMapGUIFindTopLevel(GtkWidget *widget) ;

void zMapGUIMakeMenu(char *menu_title, GList *menu_sets, GdkEventButton *button_event) ;
void zMapGUIPopulateMenu(ZMapGUIMenuItem menu,
			 int *start_index_inout,
			 ZMapGUIMenuItemCallbackFunc callback_func,
			 gpointer callback_data) ;
gboolean zMapGUIGetFixedWidthFont(GtkWidget *widget,
                                  GList *prefFamilies, gint points, PangoWeight weight,
				  PangoFont **font_out, PangoFontDescription **desc_out) ;
void zMapGUIGetFontWidth(PangoFont *font, int *width_out) ;
void zMapGUIGetPixelsPerUnit(ZMapGUIPixelConvType conv_type, GtkWidget *widget, double *x, double *y) ;
char *zMapGUIMakeTitleString(char *window_type, char *message) ;

GdkCursor *zMapGUIGetCursor(char *cursor_name) ;

GtkWidget *zMapGUIPopOutWidget(GtkWidget *popout, char *title);

void zMapGUIShowMsg(ZMapMsgType msg_type, char *msg) ;
void zMapGUIShowMsgFull(GtkWindow *parent, char *msg,
			ZMapMsgType msg_type, GtkJustification justify, int display_timeout, gboolean close_button) ;
gboolean zMapGUIMsgGetBool(GtkWindow *parent, ZMapMsgType msg_type, char *msg) ;
char *zMapGUIMsgGetText(GtkWindow *parent, ZMapMsgType msg_type, char *msg, gboolean hide_text) ;


void zMapGUIShowAbout(void) ;

void zMapGUIShowHelp(ZMapHelpType help_contents) ;
void zMapGUISetHelpURL(char *URL_base) ;

void zMapGUIShowText(char *title, char *text, gboolean edittable) ;
GtkWidget *zMapGUIShowTextFull(char *title, char *text, gboolean edittable, GList *text_attributes,
			       GtkTextBuffer **buffer_out);

char *zmapGUIFileChooser(GtkWidget *toplevel, char *title, char *directory, char *file_suffix) ;
char *zmapGUIFileChooserFull(GtkWidget *toplevel, char *title, char *directory, char *file_suffix,
			     ZMapFileChooserContentAreaCB content_func, gpointer content_data) ;

void zMapGUICreateRadioGroup(GtkWidget *gtkbox,
                             ZMapGUIRadioButton all_buttons,
                             int default_button, int *value_out,
                             ZMapGUIRadioButtonCBFunc clickedCB, gpointer clickedData);

ZMapGUIClampType zMapGUICoordsClampSpanWithLimits(double  top_limit, double  bot_limit,
                                                  double *top_inout, double *bot_inout);
ZMapGUIClampType zMapGUICoordsClampToLimits(double  top_limit, double  bot_limit,
                                            double *top_inout, double *bot_inout);

void zMapGUISetClipboard(GtkWidget *widget, char *contents);

void zMapGUIPanedSetMaxPositionHandler(GtkWidget *widget, GCallback callback, gpointer user_data);


/* Note book functions. */
ZMapGuiNotebook zMapGUINotebookCreateNotebook(char *notebook_name, gboolean editable,
					      ZMapGUINotebookCallbackFunc cleanup_cb, void *user_cleanup_data) ;
ZMapGuiNotebookChapter zMapGUINotebookCreateChapter(ZMapGuiNotebook note_book, char *chapter_name,
						    ZMapGuiNotebookCB user_callbacks) ;
ZMapGuiNotebookPage zMapGUINotebookCreatePage(ZMapGuiNotebookChapter chapter, char *page_name) ;
ZMapGuiNotebookSubsection zMapGUINotebookCreateSubsection(ZMapGuiNotebookPage page, char *subsection_name) ;
ZMapGuiNotebookParagraph zMapGUINotebookCreateParagraph(ZMapGuiNotebookSubsection subsection,
							char *paragraph_name,
							ZMapGuiNotebookParagraphDisplayType display_type,
							GList *headers, GList *types) ;
ZMapGuiNotebookTagValue zMapGUINotebookCreateTagValue(ZMapGuiNotebookParagraph paragraph,
						      char *tag_value_name,
						      ZMapGuiNotebookTagValueDisplayType display_type,
						      const char *arg_type, ...) ;
void zMapGUINotebookAddPage(ZMapGuiNotebookChapter chapter, ZMapGuiNotebookPage page) ;
void zMapGUINotebookAddChapter(ZMapGuiNotebook notebook, ZMapGuiNotebookChapter chapter) ;
ZMapGuiNotebookPage zMapGUINotebookFindPage(ZMapGuiNotebookChapter chapter, const char *paragraph_name) ;
gboolean zMapGUINotebookGetTagValue(ZMapGuiNotebookPage page, const char *tagvalue_name,
				    const char *arg_type, ...) ;
void zMapGUINotebookDestroyNotebook(ZMapGuiNotebook note_book) ;
void zMapGUINotebookDestroyAny(ZMapGuiNotebookAny note_any) ;
void zMapGUINotebookMergeNotebooks(ZMapGuiNotebook notebook, ZMapGuiNotebook notebook_new) ;
GtkWidget *zMapGUINotebookCreateDialog(ZMapGuiNotebook notebook_spec, char *help_title, char *help_text) ;
GtkWidget *zMapGUINotebookCreateWidget(ZMapGuiNotebook notebook_spec) ;





#endif /* ZMAP_UTILS_GUI_H */
