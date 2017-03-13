/*  File: zmapStyleTree.h
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
 * Description: This tree contains a hierarchy of styles. The hierarchy is
 *              constructed from the parent releationships specified in
 *              the styles config file / default styles. Styles and parent
 *              relationships can also be modified by the user within zmap.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_STYLE_TREE_H
#define ZMAP_STYLE_TREE_H

#include <vector>
#include "ZMap/zmapStyle.hpp"


typedef void (*ZMapStyleForeachFunc)(ZMapFeatureTypeStyle style, gpointer data) ;



class ZMapStyleTree
{
public:

  ZMapStyleTree() : m_style(NULL) {} ;
  ZMapStyleTree(ZMapFeatureTypeStyle style) : m_style(style) {} ;
  ~ZMapStyleTree() ;

  gboolean has_children(ZMapFeatureTypeStyle style) const ;

  ZMapFeatureTypeStyle get_style() const ;
  const std::vector<ZMapStyleTree*>& get_children() const ;

  int count() const ;

  void foreach(ZMapStyleForeachFunc func, gpointer data) ;

  const ZMapStyleTree* find(ZMapFeatureTypeStyle style) const ;
  ZMapStyleTree* find(ZMapFeatureTypeStyle style) ;
  ZMapStyleTree* find(const GQuark style_id) ;
  ZMapFeatureTypeStyle find_style(const GQuark style_id) ;

  void add_style(ZMapFeatureTypeStyle style) ;
  void add_style(ZMapFeatureTypeStyle style, GHashTable *styles, ZMapStyleMergeMode merge_mode = ZMAPSTYLE_MERGE_PRESERVE) ;
  void remove_style(ZMapFeatureTypeStyle style) ;
  void merge(GHashTable *styles_hash, ZMapStyleMergeMode merge_mode) ;
  void sort() ;

private:

  ZMapFeatureTypeStyle m_style ;
  std::vector<ZMapStyleTree*> m_children ;

  gboolean is_style(ZMapFeatureTypeStyle style) const ;
  gboolean is_style(const GQuark style_id) const ;

  gboolean has_children() const ;

  void set_style(ZMapFeatureTypeStyle style) ;
  ZMapStyleTree* find_parent(ZMapFeatureTypeStyle style) ;

  void add_children(const std::vector<ZMapStyleTree*> &children) ;
  void add_child(ZMapStyleTree *child) ;
  void remove_child(ZMapStyleTree *child) ;

  void add_child_style(ZMapFeatureTypeStyle style) ;

  void do_add_style(ZMapFeatureTypeStyle style, GHashTable *styles, ZMapStyleMergeMode merge_mode) ;
};



#endif /* ZMAP_STYLE_TREE_H */
