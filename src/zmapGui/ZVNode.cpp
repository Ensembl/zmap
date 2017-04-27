/*  File: ZVNode.cpp
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

#include "ZVNode.h"
#include <iostream>

namespace ZMap
{

namespace GUI
{

ZVNode::ZVNode(ZVNode *parent)
    : Util::InstanceCounter<ZVNode>(),
      m_parent(parent),
      m_node1(nullptr),
      m_node2(nullptr)
{

}

ZVNode::~ZVNode()
{
    if (m_node1)
        delete m_node1 ;
    if (m_node2)
        delete m_node2 ;
}

void ZVNode::print() const
{
    std::cout << "id: " << this
              << "; p: " << m_parent
              << "; n1: " << m_node1
              << "; n2: " << m_node2
              << std::endl ;

    if (m_node1)
        m_node1->print() ;
    if (m_node2)
        m_node2->print() ;
}

bool ZVNode::isSplit() const
{
    if (!m_node1 && !m_node2)
        return false ;
    else if (m_node1 && m_node2)
        return true ;
    return false ;
}

std::pair<ZVNode*, ZVNode*> ZVNode::split()
{
    if (!isSplit())
    {
        m_node1 = new ZVNode(this) ;
        m_node2 = new ZVNode(this) ;
    }
    return std::pair<ZVNode*,ZVNode*>(m_node1,m_node2) ;
}

void ZVNode::unsplit()
{
    if (!isSplit())
    {
        if (m_parent)
            m_parent->unsplit() ;
    }
    else
    {
        delete m_node1 ;
        m_node1 = nullptr ;
        delete m_node2 ;
        m_node2 = nullptr ;
    }
}


} // namespace GUI

} // namespace ZMap
