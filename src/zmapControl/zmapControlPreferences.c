/*  File: zmapControlPreferences.c
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
 * Description: Implements showing the preferences configuration
 *              window for a zmapControl instance.
 *
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: Nov 20 09:32 2008 (rds)
 * Created: Wed Oct 24 15:48:11 2007 (edgrif)
 * CVS info:   $Id: zmapControlPreferences.c,v 1.6 2010-06-14 15:40:12 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>







#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <zmapControl_P.h>



static void cleanUpCB(ZMapGuiNotebookAny any_section, void *user_data) ;



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




void zmapControlShowPreferences(ZMap zmap)
{
  ZMapGuiNotebook note_book ;
  char *notebook_title ;
  GtkWidget *notebook_dialog ;
  ZMapGuiNotebookChapter chapter ;

  /* Construct the preferences representation */
  notebook_title = g_strdup_printf("ZMap Preferences for: %s", zMapGetZMapID(zmap)) ;
  note_book = zMapGUINotebookCreateNotebook(notebook_title, TRUE, cleanUpCB, NULL) ;
  g_free(notebook_title) ;


  /* Now we should add preferences for the current zmapview and current zmapwindow.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMapViewRedraw(zmap->focus_viewwindow) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  chapter = zMapViewBlixemGetConfigChapter(note_book) ;

  if(0)
    {
      ZMapWindow window;
      
      window  = zMapViewGetWindow(zmap->focus_viewwindow);
      
      chapter = zMapWindowGetConfigChapter(window, note_book) ;
    }

#ifdef NO_EDITING_YET
  chapter = zMapViewSourcesGetConfigChapter(note_book) ;
#endif /* NO_EDITING_YET */


  /* Display the preferences. */
  notebook_dialog = zMapGUINotebookCreateDialog(note_book, help_title_G, help_text_G) ;


  return ;
}



/* 
 *                      Internal routines
 */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* If we add configuration for control here is some code.... */

static void addControlChapter(ZMapGuiNotebook note_book, char *chapter_name)
{
  ZMapGuiNotebookChapter chapter ;
  ZMapGuiNotebookCBStruct callbacks = {cancelCB, NULL, okCB, NULL} ;


  chapter = zMapGUINotebookCreateChapter(note_book, chapter_name, &callbacks) ;

  if (strcmp(chapter_name, "ZMap") == 0)
    {
      addControlPage(chapter, "Test Page 1") ;

      addControlPage(chapter, "Test Page 2") ;
    }
  else
    {
      addControlPage(chapter, "Test Page 1 for ZMap 2") ;

      addControlPage(chapter, "Test Page 2 for ZMap 2") ;
    }


  return ;
}



static void addControlPage(ZMapGuiNotebookChapter chapter, char *page_name)
{
  ZMapGuiNotebookPage page ;
  ZMapGuiNotebookParagraph paragraph ;
  ZMapGuiNotebookTagValue tag_value ;

  page = (ZMapGuiNotebookPage)zMapGUINotebookCreateSectionAny(ZMAPGUI_NOTEBOOK_PAGE, page_name) ;

  chapter->pages = g_list_append(chapter->pages, page) ;

  if (strcmp(page_name, "Test Page 1") == 0)
    {
      paragraph = (ZMapGuiNotebookParagraph)zMapGUINotebookCreateSectionAny(ZMAPGUI_NOTEBOOK_PARAGRAPH, NULL) ;
      paragraph->display_type = ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE ;
      page->paragraphs = g_list_append(page->paragraphs, paragraph) ;

      tag_value = (ZMapGuiNotebookTagValue)zMapGUINotebookCreateSectionAny(ZMAPGUI_NOTEBOOK_TAGVALUE, "Description") ;
      tag_value->display_type = ZMAPGUI_NOTEBOOK_TAGVALUE_SCROLLED_TEXT ;
      tag_value->text = g_strdup("A fabulous load of old tosh...................") ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;
    }
  else
    {
      paragraph = (ZMapGuiNotebookParagraph)zMapGUINotebookCreateSectionAny(ZMAPGUI_NOTEBOOK_PARAGRAPH, NULL) ;
      paragraph->display_type = ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE ;
      page->paragraphs = g_list_append(page->paragraphs, paragraph) ;

      tag_value = (ZMapGuiNotebookTagValue)zMapGUINotebookCreateSectionAny(ZMAPGUI_NOTEBOOK_TAGVALUE, "Align Type") ;
      tag_value->display_type = ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE ;
      tag_value->text = g_strdup("Rubbish") ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;

      tag_value = (ZMapGuiNotebookTagValue)zMapGUINotebookCreateSectionAny(ZMAPGUI_NOTEBOOK_TAGVALUE, "Query length") ;
      tag_value->display_type = ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE ;
      tag_value->text = g_strdup("9999999") ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;

      tag_value = (ZMapGuiNotebookTagValue)zMapGUINotebookCreateSectionAny(ZMAPGUI_NOTEBOOK_TAGVALUE, "Query phase") ;
      tag_value->display_type = ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE ;
      tag_value->text = g_strdup("Bad phase") ;
      paragraph->tag_values = g_list_append(paragraph->tag_values, tag_value) ;
    }


  return ;
}


static void okCB(ZMapGuiNotebookChapter chapter)
{
  printf("in ok for chapter: %s\n", g_quark_to_string(chapter->name)) ;

  return ;
}


static void cancelCB(ZMapGuiNotebookChapter chapter)
{
  printf("in cancel for chapter: %s\n", g_quark_to_string(chapter->name)) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static void cleanUpCB(ZMapGuiNotebookAny any_section, void *user_data)
{
  ZMapGuiNotebook note_book = (ZMapGuiNotebook)any_section ;

  zMapGUINotebookDestroyNotebook(note_book) ;

  note_book = NULL;

  return ;
}
