/*  File: zmapControlStyles.cpp
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
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

#include <ZMap/zmap.hpp>

//#include <ZMap/zmapUtils.hpp>
//#include <ZMap/zmapUtilsGUI.hpp>
//#include <ZMap/zmapGLibUtils.hpp>
#include <zmapControl_P.hpp>

#include <vector>


#define STYLES_CHAPTER "Styles"
#define STYLES_PAGE "Styles"


class TreeNode
{
public:
  TreeNode* find(ZMapFeatureTypeStyle style) ;
  void add_child_style(ZMapFeatureTypeStyle style) ;
  void set_style(ZMapFeatureTypeStyle) ;

private:
  ZMapFeatureTypeStyle m_style = NULL ;
  std::vector<TreeNode*> m_children ;
};


typedef struct GetStylesDataStruct_
{
  TreeNode *result ;
  GHashTable *styles ;
} GetStylesDataStruct, *GetStylesData ;


static void cleanUpCB(ZMapGuiNotebookAny any_section, void *user_data) ;
static void applyCB(ZMapGuiNotebookAny any_section, void *user_data) ;
static void saveCB(ZMapGuiNotebookAny any_section, void *user_data) ;
static void cancelCB(ZMapGuiNotebookAny any_section, void *user_data_unused) ;
static void readChapter(ZMapGuiNotebookChapter chapter, ZMap zmap) ;
static void saveChapter(ZMapGuiNotebookChapter chapter, ZMap zmap) ;

static ZMapGuiNotebookChapter addStylesChapter(ZMapGuiNotebook note_book_parent, ZMap zmap, ZMapView view) ;


/* 
 *                  Globals
 */

static const char *help_title_G = "ZMap Styles" ;
static const char *help_text_G =
  "The ZMap Styles Window allows you to configure the appearance\n"
  " of the featuresets in ZMap.\n\n"
  "After you have made your changes you can click:\n\n"
  "\t\"Cancel\" to discard them and quit the resources dialog\n"
  "\t\"Ok\" to apply them and quit the resources dialog\n" ;


/* 
 *                   External interface routines
 */

void zmapControlShowStyles(ZMap zmap)
{
  ZMapGuiNotebook note_book ;
  char *notebook_title ;

  zMapReturnIfFailSafe(!(zmap->styles_note_book)) ;

  /* Construct the styles representation */
  notebook_title = g_strdup_printf("Edit Styles for zmap %s", zMapGetZMapID(zmap)) ;
  note_book = zMapGUINotebookCreateNotebook(notebook_title, TRUE, cleanUpCB, zmap) ;
  g_free(notebook_title) ;

  if (note_book)
    {
      ZMapView zmap_view = zMapViewGetView(zmap->focus_viewwindow);

      /* Display the styles. */
      zMapGUINotebookCreateDialog(note_book, help_title_G, help_text_G) ;
      
      addStylesChapter(note_book, zmap, zmap_view) ;

      zmap->styles_note_book = note_book ;
    }
  else
    {
      zMapWarning("%s", "Error creating preferences dialog\n") ;
    }
}



/* 
 *                      Internal routines
 */

static void cleanUpCB(ZMapGuiNotebookAny any_section, void *user_data)
{
  ZMapGuiNotebook note_book = (ZMapGuiNotebook)any_section ;
  ZMap zmap = (ZMap)user_data ;

  zMapGUINotebookDestroyNotebook(note_book) ;

  zmap->styles_note_book = NULL ;

  return ;
}


static void treeAddStyle(TreeNode *tree, ZMapFeatureTypeStyle style, GHashTable *styles)
{
  if (!tree->find(style))
    {
      ZMapFeatureTypeStyle parent = (ZMapFeatureTypeStyle)g_hash_table_lookup(styles, GINT_TO_POINTER(style->parent_id)) ;

      if (parent)
        {
          /* If the parent doesn't exist, recursively create it */
          treeAddStyle(tree, parent, styles) ;

          TreeNode *parent_node = tree->find(parent) ;

          /* Add the child to the parent node */
          if (parent_node)
            parent_node->add_child_style(style) ;
          else
            zMapWarning("%s", "Error adding style to tree") ;
        }
      else
        {
          /* This style has no parent, so add it to the root style in the tree */
          tree->add_child_style(style) ;
        }
      
    }
}


static void populateStyleHierarchy(gpointer key, gpointer value, gpointer user_data)
{
  //GQuark style_id = (GQuark)(GPOINTER_TO_INT(key)) ;
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)value ;
  GetStylesData data = (GetStylesData)user_data ;

  TreeNode *tree = data->result ;

  treeAddStyle(tree, style, data->styles) ;
}


static TreeNode* getStylesHierarchy(ZMap zmap)
{
  TreeNode *result = new TreeNode ;
  zMapReturnValIfFail(zmap, result) ;

  GHashTable *styles = zMapViewGetStyles(zmap->focus_viewwindow) ;

  if (styles)
    {
      GetStylesDataStruct data = {result, styles} ;

      g_hash_table_foreach(styles, populateStyleHierarchy, &data) ;
    }

  return result ;
}


/* Does the work to create a chapter in the styles notebook for listing current styles. */
static ZMapGuiNotebookChapter addStylesChapter(ZMapGuiNotebook note_book_parent, ZMap zmap, ZMapView view)
{
  ZMapGuiNotebookChapter chapter = NULL ;
  ZMapGuiNotebookCBStruct user_CBs = {cancelCB, NULL, applyCB, zmap, NULL, NULL, saveCB, zmap} ;
  ZMapGuiNotebookPage page ;
  ZMapGuiNotebookSubsection subsection ;
  ZMapGuiNotebookParagraph paragraph ;

  chapter = zMapGUINotebookCreateChapter(note_book_parent, STYLES_CHAPTER, &user_CBs) ;

  page = zMapGUINotebookCreatePage(chapter, STYLES_PAGE) ;

  subsection = zMapGUINotebookCreateSubsection(page, NULL) ;

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
                                             ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
                                             NULL, NULL) ;

  TreeNode *styles_hierarchy = getStylesHierarchy(zmap) ;


  return chapter ;
}


/* User applies the preferences. */
static void applyCB(ZMapGuiNotebookAny any_section, void *user_data)
{
  ZMap zmap = (ZMap)user_data;

  readChapter((ZMapGuiNotebookChapter)any_section, zmap) ;

  return ;
}

/* Save the preferences to the config file */
static void saveCB(ZMapGuiNotebookAny any_section, void *user_data)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)any_section ;
  ZMap zmap = (ZMap)user_data;

  readChapter(chapter, zmap);

  saveChapter(chapter, zmap) ;

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
  //ZMapView view = zMapViewGetView(zmap->focus_viewwindow);

  if ((page = zMapGUINotebookFindPage(chapter, STYLES_PAGE)))
    {

    }
  
  return ;
}


void saveChapter(ZMapGuiNotebookChapter chapter, ZMap zmap)
{

  return ;
}


/* 
 *                  TreeNode
 */

TreeNode* TreeNode::find(ZMapFeatureTypeStyle style)
{
  TreeNode *result = NULL ;

  if (m_style == style)
    {
      result = this ;
    }
  else
    {
      for (std::vector<TreeNode*>::iterator iter = m_children.begin(); iter != m_children.end(); ++iter)
        {
          TreeNode *node = *iter ;

          if (node->find(style))
            {
              result = node ;
              break ;
            }
        }
    }

  return result ;
}

void TreeNode::add_child_style(ZMapFeatureTypeStyle style)
{
  /* Add a new child tree node with this style to our list of children */
  TreeNode *node = new TreeNode ;
  node->set_style(style) ;

  m_children.push_back(node) ;
}

void TreeNode::set_style(ZMapFeatureTypeStyle style)
{
  m_style = style ;
}
