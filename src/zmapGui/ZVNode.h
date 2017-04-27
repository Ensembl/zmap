/*  File: ZVNode.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2016: Genome Research Ltd.
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
 * This file is part of the ZMap genome database user interface
 * and was written by
 *
 * Steve Miller (sm23@sanger.ac.uk)
 *
 * Description:
 *-------------------------------------------------------------------
 */

#ifndef ZVNODE_H
#define ZVNODE_H

#include "InstanceCounter.h"
#include <string>
#include <utility>

namespace ZMap
{

namespace GUI
{


// this is an explicit nodesplitting/unsplitting model
class ZVNode: public Util::InstanceCounter<ZVNode>
{
public:

    ZVNode(ZVNode *parent) ;
    ~ZVNode() ;

    void print() const ;

    ZVNode *getParent() const {return m_parent ; }
    ZVNode *getNode1() const {return m_node1 ;  }
    ZVNode *getNode2() const {return m_node2 ; }

    std::pair<ZVNode*,ZVNode*> split() ;
    void unsplit() ;

    bool isSplit() const ;

private:
    ZVNode *m_parent, *m_node1, *m_node2 ;
};


} // namespace GUI

} // namespace ZMap


#endif // ZVNODE_H
