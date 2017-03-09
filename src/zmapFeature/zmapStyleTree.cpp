/*  File: zmapStyleTree.c
 *  Author: Gemma Guest (gb10@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Implements the zmapStyle GObject
 *
 * Exported functions: See ZMap/zmapStyleTree.hpp
 *
 *-------------------------------------------------------------------
 */

#include <vector>
#include <algorithm>
#include <string.h>

#include <zmapStyle_P.hpp>
#include <ZMap/zmapStyleTree.hpp>
#include <ZMap/zmapUtilsMesg.hpp>
#include <ZMap/zmapUtilsDebug.hpp>




/* Utility struct used to merge a styles hash table into a tree */
typedef struct MergeDataStruct_
{
  ZMapStyleTree *styles_tree ;
  GHashTable *styles_hash ;
  ZMapStyleMergeMode merge_mode ;
} MergeDataStruct, *MergeData ;



ZMapStyleTree::~ZMapStyleTree()
{
  for (std::vector<ZMapStyleTree*>::iterator iter = m_children.begin(); iter != m_children.end(); ++iter)
    {
      delete *iter ;
    }
}


/* Return true if this tree node represents the given style */
gboolean ZMapStyleTree::is_style(ZMapFeatureTypeStyle style) const
{
  gboolean result = FALSE ;

  if (m_style && style && m_style->unique_id == style->unique_id)
    result = TRUE ;

  return result ;
}


/* Return true if this tree node represents the given style id */
gboolean ZMapStyleTree::is_style(const GQuark style_id) const
{
  gboolean result = FALSE ;

  if (m_style && m_style->unique_id == style_id)
    result = TRUE ;

  return result ;
}

/* Find the tree node for the given style. Return the tree node or null if not found. */
const ZMapStyleTree* ZMapStyleTree::find(ZMapFeatureTypeStyle style) const
{
  const ZMapStyleTree *result = NULL ;

  if (is_style(style))
    {
      result = this ;
    }
  else
    {
      for (std::vector<ZMapStyleTree*>::const_iterator iter = m_children.begin(); !result && iter != m_children.end(); ++iter)
        {
          const ZMapStyleTree *child = *iter ;
          result = child->find(style) ;
        }
    }

  return result ;
}


/* Find the tree node for the given style. Return the tree node or null if not found. */
ZMapStyleTree* ZMapStyleTree::find(ZMapFeatureTypeStyle style) 
{
  ZMapStyleTree *result = NULL ;

  if (is_style(style))
    {
      result = this ;
    }
  else
    {
      for (std::vector<ZMapStyleTree*>::iterator iter = m_children.begin(); !result && iter != m_children.end(); ++iter)
        {
          ZMapStyleTree *child = *iter ;
          result = child->find(style) ;
        }
    }

  return result ;
}


/* Find the tree node for the given style id. Return the tree node or null if not found. */
ZMapStyleTree* ZMapStyleTree::find(const GQuark style_id)
{
  ZMapStyleTree *result = NULL ;

  if (is_style(style_id))
    {
      result = this ;
    }
  else
    {
      for (std::vector<ZMapStyleTree*>::iterator iter = m_children.begin(); !result && iter != m_children.end(); ++iter)
        {
          ZMapStyleTree *child = *iter ;
          result = child->find(style_id) ;
        }
    }

  return result ;
}


/* Find the style struct for the given style id. Return the struct pointer or null if not found. */
ZMapFeatureTypeStyle ZMapStyleTree::find_style(const GQuark style_id)
{
  ZMapFeatureTypeStyle result = NULL ;
  ZMapStyleTree *node = find(style_id) ;

  if (node)
    result = node->get_style() ;

  return result ;
}


/* Find the parent node of the given style. Return the tree node or null if not found. */
ZMapStyleTree* ZMapStyleTree::find_parent(ZMapFeatureTypeStyle style)
{
  ZMapStyleTree *result = NULL ;

  /* Check if any of our child nodes are the given style */
  for (std::vector<ZMapStyleTree*>::iterator iter = m_children.begin(); !result && iter != m_children.end(); ++iter)
    {
      ZMapStyleTree *child = *iter ;
      
      if (child->is_style(style))
        result = this ;
    }

  /* if not found, recurse */
  for (std::vector<ZMapStyleTree*>::iterator iter = m_children.begin(); !result && iter != m_children.end(); ++iter)
    {
      ZMapStyleTree *child = *iter ;
      result = child->find_parent(style) ;
    }
  

  return result ;
}



/* Add a new child tree node with this style to our list of children */
void ZMapStyleTree::add_child_style(ZMapFeatureTypeStyle style)
{
  ZMapStyleTree *node = new ZMapStyleTree(style) ;

  m_children.push_back(node) ;
}


ZMapFeatureTypeStyle ZMapStyleTree::get_style() const
{
  return m_style ;
}


void ZMapStyleTree::set_style(ZMapFeatureTypeStyle style)
{
  m_style = style ;
}


const std::vector<ZMapStyleTree*>& ZMapStyleTree::get_children() const
{
  return m_children ;
}


/* Add the given style into the style hierarchy tree in the appropriate place according to its
 * parent. Does nothing if the style is already in the tree. Assumes the style's parent is already
 * in the tree or that it should be added to the root node if not. */
void ZMapStyleTree::add_style(ZMapFeatureTypeStyle style)
{
  zMapReturnIfFail(style) ;

  if (!find(style))
    {
      if (style->parent_id)
        {
          /* Add the child to the parent node */
          ZMapStyleTree *parent_node = find(style->parent_id) ;

          if (parent_node)
            {
              parent_node->add_child_style(style) ;
            }
          else
            {
              zMapLogWarning("Adding style '%s' to parent '%s' failed; parent style not found",
                             g_quark_to_string(style->unique_id), g_quark_to_string(style->parent_id)) ;
            }
        }
      else
        {
          /* This style has no parent, so add it to the root style in the tree */
          add_child_style(style) ;
        }
      
    }
}


/* Check whether a style has the given style in its parent hierarchy */
static gboolean styleDependsOnParent(ZMapFeatureTypeStyle style, 
                                     const GQuark search_id,
                                     GHashTable *styles)
{
  gboolean result = FALSE ;

  if (style && search_id)
    {
      if (!style->parent_id)
        {
          /* No parent; not found */
          result = FALSE ;
        }
      else if (style->parent_id == search_id)
        {
          /* Found */
          result = TRUE ;
        }
      else if (style->parent_id == style->unique_id)
        {
          /* Don't recurse through a style with a direct cyclic dependancy on itself */
          result = FALSE ;
        }
      else
        {
          /* Recursively look through the parent hierarchy */
          ZMapFeatureTypeStyle parent_style = (ZMapFeatureTypeStyle)g_hash_table_lookup(styles, GINT_TO_POINTER(style->parent_id)) ;

          if (parent_style)
            result = styleDependsOnParent(parent_style, search_id, styles) ;
        }
    }

  return result ;
}


/* Does the work to add a new style. Assumes the style does not already exist.  */
void ZMapStyleTree::do_add_style(ZMapFeatureTypeStyle style, GHashTable *styles, ZMapStyleMergeMode merge_mode)
{
  zMapReturnIfFail(style && styles) ;

  /* Check if we need to add this to a parent style */
  if (!style->parent_id)
    {
      /* This style has no parent, so add it to the root style in the tree */
      add_child_style(style) ;
    }
  else if (styleDependsOnParent(style, style->unique_id, styles))
    {
      /* The child has itself in its parent hierarchy. We can't add styles with cyclic dependancies. */
      zMapLogWarning("Style '%s' cannot be added because it has a cyclic dependancy on parent '%s'.",
                     g_quark_to_string(style->unique_id),
                     g_quark_to_string(style->parent_id)) ;
    }
  else if (style->parent_id && style->parent_id != style->unique_id)
    {
      ZMapFeatureTypeStyle parent = (ZMapFeatureTypeStyle)g_hash_table_lookup(styles, GINT_TO_POINTER(style->parent_id)) ;

      /* If there is no parent with this id in the input hash table then create an empty
       * style with this id just as a placeholder */
      if (!parent)
        {
          parent = zMapStyleCreate(g_quark_to_string(style->parent_id), g_quark_to_string(style->parent_id)) ;
        }

      if (parent)
        {
          ZMapStyleTree *parent_node = find(parent) ;

          /* If the parent doesn't exist, recursively create it */
          if (!parent_node)
            {
              add_style(parent, styles, merge_mode) ;
              parent_node = find(parent) ;
            }

          /* Add the child to the parent node */
          if (parent_node)
            {
              parent_node->add_child_style(style) ;
            }
          else
            {
              zMapLogWarning("Error adding style '%s' to tree: no parent style '%s'", 
                             g_quark_to_string(style->original_id),
                             g_quark_to_string(style->parent_id)) ;
            }
        }
      else
        {
          zMapLogWarning("Could not create parent style '%s'; style '%s' will be omitted.",
                         g_quark_to_string(style->parent_id),
                         g_quark_to_string(style->original_id)) ;
        }
    }
}


/* Add/update the given style into the style hierarchy tree in the appropriate place according to its
 * parent. Recursively add/update the style's parent(s) if not already in the tree. If the style
 * is already in the tree it is ignored/updated/replaced depending on the merge mode.
 * 
 * This function is provided to support the old acedb way of loading styles where we may only
 * know the parent id up front and not have the whole parent style available. In this case the
 * ids are added into a hash table and we only merge them into the styles tree when we've parsed
 * them all. */
void ZMapStyleTree::add_style(ZMapFeatureTypeStyle style, 
                              GHashTable *styles,
                              ZMapStyleMergeMode merge_mode)
{
  zMapReturnIfFail(style && styles) ;
  ZMapStyleTree *found_node = find(style) ;

  if (!found_node)
    {
      do_add_style(style, styles, merge_mode) ;
    }
  else
    {
      ZMapFeatureTypeStyle curr_style = found_node->get_style() ;

      /* We only need to do a merge if we have two different versions of this style, i.e. if the
       * pointers are not the same */
      if (curr_style && curr_style != style)
        {
          switch (merge_mode)
            {
            case ZMAPSTYLE_MERGE_PRESERVE:
              {
                /* Leave the existing style untouched. */
                break ;
              }
            case ZMAPSTYLE_MERGE_REPLACE:
              {
                /* Remove the existing style and put the new one in its place. */
                found_node->set_style(style) ;
                zMapStyleDestroy(curr_style) ;
                break ;
              }
            case ZMAPSTYLE_MERGE_MERGE:
              {
                /* Merge the existing and new styles. */
                zMapStyleMerge(curr_style, style) ;
                break ;
              }
            default:
              {
                zMapWarnIfReached() ;
                break ;
              }
            }
        }
    }
}


/* Add the given child nodes to this node */
void ZMapStyleTree::add_children(const std::vector<ZMapStyleTree*> &children)
{
  for (std::vector<ZMapStyleTree*>::const_iterator iter = children.begin(); iter != children.end(); ++iter)
    {
      add_child(*iter) ;
    }
}


/* Add the given node as a child to this node */
void ZMapStyleTree::add_child(ZMapStyleTree *child)
{
  if (child)
    {
      m_children.push_back(child) ;
    }
}


/* Remove the given child from this node */
void ZMapStyleTree::remove_child(ZMapStyleTree *child)
{
  if (child)
    {
      std::vector<ZMapStyleTree*>::iterator iter = std::find(m_children.begin(), m_children.end(), child) ;

      if (iter != m_children.end())
        {
          m_children.erase(iter) ;
        }

    }
}


/* Remove the given style from the style hierarchy tree. If the style has any children then they
 * are moved up in the hierarchy to the style's parent. */
void ZMapStyleTree::remove_style(ZMapFeatureTypeStyle style)
{
  ZMapStyleTree *parent = find_parent(style) ;
  ZMapStyleTree *node = find(style) ;

  if (parent && node)
    {
      /* Add the node's children to the parent's child list */
      const std::vector<ZMapStyleTree*>& node_children = node->get_children() ;
      parent->add_children(node_children) ;

      /* Remove the node from the parent's child list */
      parent->remove_child(node) ;
    }
}


/* Predicate for sorting styles alphabetically */
inline bool styleLessThan(const ZMapStyleTree *node1, const ZMapStyleTree *node2)
{
  bool result = FALSE ;

  ZMapFeatureTypeStyle style1 = node1->get_style() ;
  ZMapFeatureTypeStyle style2 = node2->get_style() ;

  if (style1 && style1->original_id && style2 && style2->original_id)
    {
      result = (strcmp(g_quark_to_string(style1->original_id), 
                       g_quark_to_string(style2->original_id)) < 0);
    }
  
  return result ;
}


/* Sort all list of nodes alphabetically by style name */
void ZMapStyleTree::sort()
{
  /* Sort our own list of children */
  std::sort(m_children.begin(), m_children.end(), styleLessThan);
  
  /* Recurse */
  for (std::vector<ZMapStyleTree*>::iterator iter = m_children.begin(); iter != m_children.end(); ++iter)
    {
      ZMapStyleTree *child = *iter ;
      child->sort() ;
    }
}


/* Called for each style in a hash table. Add the style to the appropriate node 
 * in the tree */
static void mergeStyleCB(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)value ;
  MergeData merge_data = (MergeData)user_data ;

  merge_data->styles_tree->add_style(style, merge_data->styles_hash, merge_data->merge_mode) ;
}


void ZMapStyleTree::merge(GHashTable *styles_hash, ZMapStyleMergeMode merge_mode)
{
  if (styles_hash)
    {
      MergeDataStruct merge_data = {this, styles_hash, merge_mode} ;
      
      g_hash_table_foreach(styles_hash, mergeStyleCB, &merge_data) ;
    }
}


/* Call the given function on each style in the tree */
void ZMapStyleTree::foreach(ZMapStyleForeachFunc func, gpointer data)
{
  if (func)
    {
      // Call the function on this style, note we may be called before a style has been set on
      // this object.
      if (m_style)
        (*func)(m_style, data) ;

      /* Recurse */
      for (std::vector<ZMapStyleTree*>::iterator iter = m_children.begin(); iter != m_children.end(); ++iter)
        {
          ZMapStyleTree *child = *iter ;
          child->foreach(func, data) ;
        }
    }
}


/* Return the total number of styles in the entire tree hierarchy */
int ZMapStyleTree::count() const
{
  /* Count this node */
  int total = 1 ;

  /* Recurse through children */
  for (std::vector<ZMapStyleTree*>::const_iterator iter = m_children.begin(); iter != m_children.end(); ++iter)
    {
      ZMapStyleTree *child = *iter ;
      total += child->count() ;
    }

  return total ;
}


/* Return true if this node has child nodes */
gboolean ZMapStyleTree::has_children() const
{
  gboolean result = FALSE ;

  if (m_children.size() > 0)
    result = TRUE ;

  return result ;
}


/* Return true if the given style is in the tree and if it has child styles */
gboolean ZMapStyleTree::has_children(ZMapFeatureTypeStyle style) const
{
  gboolean result = FALSE ;

  if (style)
    {
      const ZMapStyleTree *node = find(style) ;

      if (node)
        result = node->has_children() ;
    }

  return result ;
}
