/*  File: zmapControlPreferences.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 * Description: Implements showing the preferences configuration
 *              window for a zmapControl instance.
 *
 * Exported functions: See zmapControl_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <zmapControl_P.h>


#define CONTROL_CHAPTER "Display"
#define CONTROL_PAGE "Window"
#define CONTROL_SHRINKABLE "Shrinkable Window"
#define CONTROL_FILTERED "Highlight Filtered Columns"
#define CONTROL_ANNOTATION "Enable Annotation"



static void cleanUpCB(ZMapGuiNotebookAny any_section, void *user_data) ;
static ZMapGuiNotebookChapter addControlPrefsChapter(ZMapGuiNotebook note_book_parent, ZMap zmap, ZMapView view) ;
static void applyCB(ZMapGuiNotebookAny any_section, void *user_data) ;
static void cancelCB(ZMapGuiNotebookAny any_section, void *user_data_unused) ;
static void readChapter(ZMapGuiNotebookChapter chapter, ZMap zmap) ;



/* 
 *                  Globals
 */

static char *help_title_G = "ZMap Configuration" ;
static char *help_text_G =
  "The ZMap Configuration Window allows you to configure the appearance\n"
  " and operation of certains parts of ZMap.\n\n"
  "The Configuration Window has the following sections:\n\n"
  "\tThe menubar with general operations such as showing this help.\n"
  "\tThe section chooser where you can click on the section that you want to configure\n"
  "\tThe section resources notebook which displays the elements you can configure for\n"
  "\ta particular section. As you select different sections the resources notebook changes\n"
  "\t to allow you to configure that section.\n\n"
  "After you have made your changes you can click:\n\n"
  "\t\"Cancel\" to discard them and quit the resources dialog\n"
  "\t\"Ok\" to apply them and quit the resources dialog\n" ;

/* WHAT IS THIS FOR ????? 
 * (sm23) I reomved this as it is not used.  
 */
/* static ZMapGuiNotebook note_book_G = NULL ; */



/* 
 *                   External interface routines
 */

void zmapControlShowPreferences(ZMap zmap)
{
  ZMapGuiNotebook note_book ;
  char *notebook_title ;
  /* ZMapGuiNotebookChapter chapter ; */

  zMapReturnIfFailSafe(!(zmap->preferences_note_book)) ;


  /* Construct the preferences representation */
  notebook_title = g_strdup_printf("Preferences for zmap %s", zMapGetZMapID(zmap)) ;
  note_book = zMapGUINotebookCreateNotebook(notebook_title, TRUE, cleanUpCB, zmap) ;
  g_free(notebook_title) ;


  /* Now add preferences for the current zmapview and current zmapwindow.... */
  if (note_book)
    {
      ZMapView zmap_view = zMapViewGetView(zmap->focus_viewwindow);

      /* Add prefs for control. */
      addControlPrefsChapter(note_book, zmap, zmap_view) ;
      
      /* Add blixmem view prefs. */
      zMapViewBlixemGetConfigChapter(zmap_view, note_book) ;

      /* Display the preferences. */
      zMapGUINotebookCreateDialog(note_book, help_title_G, help_text_G) ;

      zmap->preferences_note_book = note_book ;
    }
  else
    {
      zMapWarning("%s", "Error creating preferences dialog\n") ;
    }

  return ;
}




/* 
 *                      Internal routines
 */

static void cleanUpCB(ZMapGuiNotebookAny any_section, void *user_data)
{
  ZMapGuiNotebook note_book = (ZMapGuiNotebook)any_section ;
  ZMap zmap = (ZMap)user_data ;

  zMapGUINotebookDestroyNotebook(note_book) ;

  zmap->preferences_note_book = NULL ;

  return ;
}


/* Does the work to create a chapter in the preferences notebook for general zmap settings. */
static ZMapGuiNotebookChapter addControlPrefsChapter(ZMapGuiNotebook note_book_parent, ZMap zmap, ZMapView view)
{
  ZMapGuiNotebookChapter chapter = NULL ;
  ZMapGuiNotebookCBStruct user_CBs = {cancelCB, NULL, applyCB, zmap, NULL, NULL, NULL, NULL} ;
  ZMapGuiNotebookPage page ;
  ZMapGuiNotebookSubsection subsection ;
  ZMapGuiNotebookParagraph paragraph ;

  chapter = zMapGUINotebookCreateChapter(note_book_parent, CONTROL_CHAPTER, &user_CBs) ;


  page = zMapGUINotebookCreatePage(chapter, CONTROL_PAGE) ;

  subsection = zMapGUINotebookCreateSubsection(page, NULL) ;

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
					     ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
					     NULL, NULL) ;

  zMapGUINotebookCreateTagValue(paragraph, CONTROL_SHRINKABLE, "Make the ZMap window shrinkable beyond the default minimum size",
                                ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                "bool", zmap->shrinkable) ;

  zMapGUINotebookCreateTagValue(paragraph, CONTROL_FILTERED, "Highlight the background of columns that have features that are not visible due to filtering",
                                ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                "bool", zMapViewGetFlag(view, ZMAPFLAG_HIGHLIGHT_FILTERED_COLUMNS)) ;

  zMapGUINotebookCreateTagValue(paragraph, CONTROL_ANNOTATION, "Enable feature editing within ZMap via the Annotation column. This enables the Annotation sub-menu when you right-click a feature.",
                                ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                "bool", zMapViewGetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION)) ;

  return chapter ;
}


/* User applies the preferences. */
static void applyCB(ZMapGuiNotebookAny any_section, void *user_data)
{
  ZMap zmap = (ZMap)user_data;

  readChapter((ZMapGuiNotebookChapter)any_section, zmap) ;

  return ;
}

/* User cancels preferences. */
static void cancelCB(ZMapGuiNotebookAny any_section, void *user_data_unused)
{
  return ;
}



/* Set the preferences. */
static void readChapter(ZMapGuiNotebookChapter chapter, ZMap zmap)
{
  ZMapGuiNotebookPage page ;
  gboolean bool_value = FALSE ;
  ZMapView view = zMapViewGetView(zmap->focus_viewwindow);

  if ((page = zMapGUINotebookFindPage(chapter, CONTROL_PAGE)))
    {
      if (zMapGUINotebookGetTagValue(page, CONTROL_SHRINKABLE, "bool", &bool_value))
	{
	  if (zmap->shrinkable != bool_value)
	    {
              zmap->shrinkable = bool_value ;

              gtk_window_set_policy(GTK_WINDOW(zmap->toplevel), zmap->shrinkable, TRUE, FALSE ) ;
	    }
	}

      if (zMapGUINotebookGetTagValue(page, CONTROL_FILTERED, "bool", &bool_value))
        {
          if (zMapViewGetFlag(view, ZMAPFLAG_HIGHLIGHT_FILTERED_COLUMNS) != bool_value)
            zMapViewSetFlag(view, ZMAPFLAG_HIGHLIGHT_FILTERED_COLUMNS, bool_value) ;
        }

      if (zMapGUINotebookGetTagValue(page, CONTROL_ANNOTATION, "bool", &bool_value))
        {
          if (zMapViewGetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION) != bool_value)
            zMapViewSetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION, bool_value) ;
        }

    }
  
  return ;
}


