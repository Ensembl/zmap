/*  File: zmapStyleTree.c
 *  Author: Gemma Guest (gb10@sanger.ac.uk)
 *  Copyright (c) 2015: Genome Research Ltd.
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
 * Description: Implements the zmapStyle GObject
 *
 * Exported functions: See ZMap/zmapStyle.h
 *
 *-------------------------------------------------------------------
 */

//#include <ZMap/zmap.hpp>
//
//#include <string.h>
//
//#include <ZMap/zmapUtils.hpp>
//#include <ZMap/zmapGLibUtils.hpp>
//#include <ZMap/zmapConfigIni.hpp>   // for zMapConfigLegacyStyles() only can remove when system has moved on
#include <zmapStyle_P.hpp>
#include <ZMap/zmapStyleTree.hpp>
#include <ZMap/zmapUtilsMesg.hpp>


ZMapStyleTree::ZMapStyleTree()
{
  m_style = NULL ;
}


ZMapStyleTree::ZMapStyleTree(ZMapFeatureTypeStyle style)
{
  m_style = style ;
}


/* Find the given style in the tree. Return the tree node or null if not found. */
ZMapStyleTree* ZMapStyleTree::find(ZMapFeatureTypeStyle style)
{
  ZMapStyleTree *result = NULL ;

  if (m_style == style)
    {
      result = this ;
    }
  else
    {
      for (std::vector<ZMapStyleTree*>::iterator iter = m_children.begin(); !result && iter != m_children.end(); ++iter)
        {
          ZMapStyleTree *node = *iter ;
          result = node->find(style) ;
        }
    }

  return result ;
}


/* Add a new child tree node with this style to our list of children */
void ZMapStyleTree::add_child_style(ZMapFeatureTypeStyle style)
{
  ZMapStyleTree *node = new ZMapStyleTree(style) ;

  m_children.push_back(node) ;
}


ZMapFeatureTypeStyle ZMapStyleTree::get_style()
{
  return m_style ;
}


std::vector<ZMapStyleTree*> ZMapStyleTree::get_children()
{
  return m_children ;
}


/* Add the given style into the style hierarchy tree in the appropriate place according to its
 * parent. Recursively add the style's parent(s) if not already in the tree. Does nothing if the
 * style is already in the tree. */
void ZMapStyleTree::add_style(ZMapFeatureTypeStyle style, GHashTable *styles)
{
  if (!this->find(style))
    {
      ZMapFeatureTypeStyle parent = (ZMapFeatureTypeStyle)g_hash_table_lookup(styles, GINT_TO_POINTER(style->parent_id)) ;

      if (parent)
        {
          /* If the parent doesn't exist, recursively create it */
          this->add_style(parent, styles) ;

          ZMapStyleTree *parent_node = this->find(parent) ;

          /* Add the child to the parent node */
          if (parent_node)
            parent_node->add_child_style(style) ;
          else
            zMapWarning("%s", "Error adding style to tree") ;
        }
      else
        {
          /* This style has no parent, so add it to the root style in the tree */
          this->add_child_style(style) ;
        }
      
    }
}
