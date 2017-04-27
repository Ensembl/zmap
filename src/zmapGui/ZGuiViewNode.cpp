/*  File: ZGuiViewNode.cpp
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

#include "ZGuiViewNode.h"
#include "ZGuiGraphicsView.h"
#include "ZGuiScene.h"
#include "ZDebug.h"
#include "ZGuiWidgetTest.h"
#include <QtWidgets>
#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiViewNode>::m_name("ZGuiViewNode") ;
const QString ZGuiViewNode::m_orientation_h("h"),
    ZGuiViewNode::m_orientation_v("v") ;

// this constructor creates the top level node for the sequence
ZGuiViewNode::ZGuiViewNode(const QString &sequence_name,
                           ZInternalID id,
                           ZGuiScene *sequence,
                           QWidget *parent)
    : QSplitter(parent),
      Util::InstanceCounter<ZGuiViewNode>(),
      Util::ClassName<ZGuiViewNode>(),
      m_id(id),
      m_sequence_name(sequence_name),
      m_sequence(sequence)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; may not create without instance of QApplication existing ")) ;
    if (!m_id)
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; id passed in was not valid ")) ;

    if (!m_sequence)
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; sequence passed in is not valid ")) ;
    connect(m_sequence, SIGNAL(signal_contextMenuEvent(QPointF)), this, SLOT(slot_contextMenuEvent(QPointF))) ;

    setOrientation(Qt::Vertical) ;

    ZGuiGraphicsView *view = Q_NULLPTR ;
    if (!(view = ZGuiGraphicsView::newInstance()))
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; unable to create view ")) ;
    view->setScene(m_sequence) ;
    addWidget(view) ;
    connectView(view) ;
}

ZGuiViewNode::ZGuiViewNode(const QString &sequence_name,
                           ZInternalID id,
                           ZGuiGraphicsView *view)
    : QSplitter(Q_NULLPTR),
      Util::InstanceCounter<ZGuiViewNode>(),
      Util::ClassName<ZGuiViewNode>(),
      m_id(id),
      m_sequence_name(sequence_name),
      m_sequence(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; may not create without instance of QApplication existing")) ;
    if (!m_id)
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; id passed in may not be 0 ")) ;
    if (!view)
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; view may not be NULL ")) ;
    if (!view->getGraphicsScene())
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; view does not have a scene ")) ;
    addWidget(view) ;
    connectView(view) ;
    m_sequence = view->getGraphicsScene() ;
}

ZGuiViewNode::ZGuiViewNode(const QString &sequence_name,
                           ZInternalID id,
                           ZGuiScene *scene)
    : QSplitter(Q_NULLPTR),
      Util::InstanceCounter<ZGuiViewNode>(),
      Util::ClassName<ZGuiViewNode>(),
      m_id(id),
      m_sequence_name(sequence_name),
      m_sequence(scene)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; may not create without instance of QApplication existing ")) ;
    if (!m_id)
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; id was not valid ")) ;
    ZGuiGraphicsView *view = Q_NULLPTR ;
    if (!(view = ZGuiGraphicsView::newInstance()))
        throw std::runtime_error(std::string("ZGuiViewNode::ZGuiViewNode() ; unable to create view for this node ")) ;
    view->setScene(m_sequence) ;
    addWidget(view) ;
    connectView(view) ;
}

ZGuiViewNode::~ZGuiViewNode()
{
}

ZGuiViewNode* ZGuiViewNode::newInstance(const QString& sequence_name,
                                 ZInternalID id,
                                        ZGuiScene *scene,
                                 QWidget* parent)
{
    ZGuiViewNode *node = Q_NULLPTR ;

    try
    {
        node = new ZGuiViewNode(sequence_name, id, scene, parent) ;
    }
    catch (...)
    {
        node = Q_NULLPTR ;
    }

    return node ;
}

void ZGuiViewNode::setCurrent(ZGuiGraphicsView *view, bool current)
{

    if (count() == 1)
    {
        if (view == widget(0))
        {
            view->setCurrent(current) ;
            view->setFocus() ;
        }
    }
    else if (count() == 2)
    {
        ZGuiViewNode *node = Q_NULLPTR ;
        if ((node = dynamic_cast<ZGuiViewNode*>(widget(0))))
        {
            node->setCurrent(view, current) ;
        }
        if ((node = dynamic_cast<ZGuiViewNode*>(widget(1))))
        {
            node->setCurrent(view, current) ;
        }
    }
}

// this finds the view child and detatches it from the current object
ZGuiGraphicsView * ZGuiViewNode::detatchView()
{
    ZGuiGraphicsView * view = Q_NULLPTR ;

    // an alternative approach to finding children of a particular type is
    // to do something like
    //QList<ZGuiGraphicsView *> list = findChildren<ZGuiGraphicsView*>(QString(), Qt::FindDirectChildrenOnly) ;

    if ((count() == 1) && (view = dynamic_cast<ZGuiGraphicsView*>(widget(0))))
    {
        view->setParent(Q_NULLPTR) ;
    }
    return view ;
}

// this returns a pointer to child view, but only if this node is a
// view node rather than a split node
ZGuiGraphicsView* ZGuiViewNode::getView()
{
    ZGuiGraphicsView * view = Q_NULLPTR ;

    if (count() == 1)
    {
        view = dynamic_cast<ZGuiGraphicsView*>(widget(0)) ;
    }

    return view ;
}

void ZGuiViewNode::slot_split(const QString &data)
{
    // if we have one child only and it is the view,
    // then we can split
    ZGuiGraphicsView *view = Q_NULLPTR ;
    if ((view = detatchView()))
    {

        // create two new nodes and add them to this
        // their split signal is connected on construction
        ZGuiViewNode *node1 = new ZGuiViewNode(m_sequence_name, m_id, view),
                *node2 = new ZGuiViewNode(m_sequence_name, m_id, m_sequence) ;
        Qt::Orientation orientation(Qt::Horizontal) ;
        if (data.toLower() == m_orientation_v)
            orientation = Qt::Vertical ;
        setOrientation(orientation) ;
        if (node1 && node2)
        {
            addWidget(node1) ;
            addWidget(node2) ;
            connectNode(node1) ;
            connectNode(node2) ;
            //emit view->signal_itemTakenFocus();
            view->setFocus() ;
        }
    }
}


void ZGuiViewNode::slot_unsplit()
{
    QObject* sender = QObject::sender() ;
    if (count() == 1 && (dynamic_cast<ZGuiGraphicsView*>(sender)))
    {
        emit signal_unsplit() ;
    }
    else if (count() == 2)
    {
        ZGuiViewNode *node = dynamic_cast<ZGuiViewNode*>(sender) ;
        if (node)
        {
            // get the view from the sender and...
            ZGuiGraphicsView *view = node->detatchView() ;

            if (view)
            {
                // remove the two nodes from this
                deleteNodes() ;

                // add our view to this and connect it
                addWidget(view) ;
                connectView(view) ;
                //emit view->signal_itemTakenFocus();
                view->setFocus() ;
            }
        }
    }
}

void ZGuiViewNode::connectView(ZGuiGraphicsView *view)
{
    if (view)
    {
        disconnect(view, 0, 0, 0) ;
        connect(view, SIGNAL(signal_split(QString)), this, SLOT(slot_split(QString))) ;
        connect(view, SIGNAL(signal_unsplit()), this, SLOT(slot_unsplit())) ;
        connect(view, SIGNAL(signal_itemTakenFocus()), this, SLOT(slot_itemTakenFocus())) ;
        connect(view, SIGNAL(signal_requestMadeCurrent()), this, SLOT(slot_viewMadeCurrent())) ;
        connect(view, SIGNAL(signal_contextMenuEvent(QPointF)), this, SIGNAL(signal_contextMenuEvent(QPointF))) ;
    }
}

void ZGuiViewNode::connectNode(ZGuiViewNode* node)
{
    if (node)
    {
        disconnect(node, 0, 0, 0) ;
        connect(node, SIGNAL(signal_unsplit()), this, SLOT(slot_unsplit())) ;
        //connect(node, SIGNAL(signal_sequenceMadeCurrent()), this, SLOT(slot_itemTakenFocus())) ;
        connect(node, SIGNAL(signal_madeCurrent(ZGuiGraphicsView*)), this, SLOT(slot_viewMadeCurrent(ZGuiGraphicsView*))) ;
    }
}

void ZGuiViewNode::deleteNodes()
{
    if (count() == 2)
    {
        ZGuiViewNode *node1 = dynamic_cast<ZGuiViewNode*>(widget(0)),
            *node2 = dynamic_cast<ZGuiViewNode*>(widget(1)) ;
        if (node1)
            deleteNode(node1) ;
        if (node2)
            deleteNode(node2) ;
    }
}

void ZGuiViewNode::deleteNode(ZGuiViewNode *node)
{
    if (node)
    {
        node->setParent(Q_NULLPTR) ;
        // disconnect(node, 0, 0, 0) ; // should happen on deletion anyway
        delete node ;
    }
}

void ZGuiViewNode::slot_itemTakenFocus()
{
    ZGuiGraphicsView *view = dynamic_cast<ZGuiGraphicsView*> (QObject::sender()) ;
//#ifndef QT_NO_DEBUG
//    QObject* sender = QObject::sender() ;
//    qDebug() << "ZGuiViewNode::slot_itemTakenFocus() " << this << view ;
//#endif
    if (view)
    {
        emit signal_madeCurrent(view) ;
    }
}

void ZGuiViewNode::slot_contextMenuEvent(const QPointF& pos)
{
#ifndef QT_NO_DEBUG
    QObject* sender = QObject::sender() ;
    ZGuiScene *scene = dynamic_cast<ZGuiScene*> (sender) ;
    qDebug() << "ZGuiViewNode::slot_contextMenuEvent() " << this << scene
             << scene->getID() << pos ;
#endif
    //emit signal_contextMenuEvent(pos) ;
}

void ZGuiViewNode::slot_viewMadeCurrent()
{
    ZGuiGraphicsView * view = dynamic_cast<ZGuiGraphicsView*>(QObject::sender()) ;
    if (view)
    {
        slot_viewMadeCurrent(view) ;
    }
}

void ZGuiViewNode::slot_viewMadeCurrent(ZGuiGraphicsView* view)
{
//#ifndef QT_NO_DEBUG
//    qDebug() << "ZGuiViewNode::slot_viewMadeCurrent() " << view ;
//#endif
    emit signal_madeCurrent(view) ;
}

} // namespace GUI

} // namespace ZMap

