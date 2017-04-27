/*  File: ZViewNode.cpp
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

#include "ZViewNode.h"
#include "TestView02.h"
#include "TestGraphicsView02.h"
#include <QtWidgets>
#include <QSplitter>
#include <QEvent>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QGraphicsScene>
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZViewNode01>::m_name("ZViewNode01") ;
template <> const std::string Util::ClassName<ZViewNode02>::m_name("ZViewNode02") ;
template <> const std::string Util::ClassName<ZViewNode03>::m_name("ZViewNode03") ;
template <> const std::string Util::ClassName<ZViewNode04>::m_name("ZViewNode04") ;
template <> const std::string Util::ClassName<ZViewNode05>::m_name("ZViewNode05") ;

ZViewNode01::ZViewNode01(QWidget *parent)
    : QWidget(parent),
      Util::InstanceCounter<ZViewNode01>(),
      Util::ClassName<ZViewNode01>(),
      m_view(Q_NULLPTR),
      m_splitter(Q_NULLPTR)
{
}

ZViewNode01::ZViewNode01(TestView02 *view, QWidget *parent)
    : QWidget(parent),
      Util::InstanceCounter<ZViewNode01>(),
      Util::ClassName<ZViewNode01>(),
      m_layout(Q_NULLPTR),
      m_view(view),
      m_splitter(Q_NULLPTR)
{
    if (!view)
        throw std::runtime_error(std::string("ZViewNode::ZViewNode() ; view argument may not be NULL ")) ;
    m_layout = new QHBoxLayout(this) ;
    m_layout->addWidget(m_view) ;
    QObject::connect(m_view, SIGNAL(split(const QString &)), this, SLOT(split(const QString &))) ;
}

ZViewNode01::~ZViewNode01()
{
}

bool ZViewNode01::setView(TestView02 *view)
{
    bool result = true ;
    if (!m_view && !m_splitter && view && m_layout && m_layout->isEmpty() )
    {
        m_view = view ;
        m_layout->addWidget(m_view) ;
        result = true ;
    }
    return result ;
}

TestView02 *ZViewNode01::detatchView()
{
    TestView02 *view = m_view ;
    m_view = Q_NULLPTR ;
    view->setParent(Q_NULLPTR) ;
    return view ;
}



void ZViewNode01::split(const QString &data)
{
    qDebug() << "ZViewNode::split()" << this << m_view ;

    // (1) create splitter of the appropriate orientation
    //     and add the splitter as a child of the current
    //     node (or at least of the layout of this node)
    if (data == QString("v"))
    {
        m_splitter = new QSplitter(Qt::Vertical) ;
    }
    else if (data == QString("h"))
    {
        m_splitter = new QSplitter(Qt::Horizontal) ;
    }

    // (3) create new viewnodes one with the current view and one with a new one
    //     and add both of these to the splitter; we reparent the current view
    //     as part of this step
    if (m_splitter)
    {
        m_layout->removeWidget(m_view) ;
        QObject::disconnect(m_view, 0, 0, 0) ;

        m_layout->addWidget(m_splitter) ;

        ZViewNode01 *node1 = new ZViewNode01(m_view) ;
        node1->setParent(m_splitter) ;
        QObject::connect(m_view, SIGNAL(unsplit()), this, SLOT(unsplit())) ;

        TestView02 *new_view = new TestView02(QString("new view")) ;
        new_view->getGraphicsView()->setScene(m_view->getGraphicsView()->scene()) ;

        ZViewNode01 *node2 = new ZViewNode01(new_view) ;
        node2->setParent(m_splitter) ;
        QObject::connect(new_view, SIGNAL(unsplit()), this, SLOT(unsplit())) ;

        m_view = Q_NULLPTR ;
    }
}

void ZViewNode01::unsplit()
{
    if (m_splitter && !m_view )
    {
        int n = m_splitter->count() ;
        if (n == 2)
        {
            ZViewNode01 *node1 = dynamic_cast<ZViewNode01*>(m_splitter->widget(0)),
                    *node2 = dynamic_cast<ZViewNode01*>(m_splitter->widget(1)) ;
            qDebug() << "ZViewNode::unsplit()" << this << node1 << node2 ;
            m_layout->removeWidget(m_splitter) ;
            QObject::disconnect(node1->getView(), 0, 0, 0) ;
            QObject::disconnect(node2->getView(), 0, 0, 0) ;
            m_view = node1->detatchView() ;
            m_layout->addWidget(m_view) ;
            QObject::connect(m_view, SIGNAL(split(const QString &)), this, SLOT(split(const QString &))) ;
//            QObject *widget = parent() ;
//            ZViewNode *node = dynamic_cast<ZViewNode *>(widget) ;
//            if (node)
//            {
//                QObject::connect(m_view, SIGNAL(split(const QString &)), node, SLOT(unsplit())) ;
//            }
            delete m_splitter ;
            m_splitter = Q_NULLPTR ;
        }
    }
}





// constructor to create an unsplit view only
ZViewNode02::ZViewNode02(TestView02 *view, QWidget *parent)
    : QWidget(parent),
      Util::InstanceCounter<ZViewNode02>(),
      Util::ClassName<ZViewNode02>(),
      m_view(view),
      m_splitter(Q_NULLPTR)
{
    if (!m_view)
        throw std::runtime_error(std::string("ZViewNode02::ZViewNode02() ; view may not be NULL ")) ;

    QHBoxLayout *layout = new QHBoxLayout() ;
    setLayout(layout) ;

    m_splitter = new QSplitter() ;
    layout->addWidget(m_splitter) ;

    m_splitter->addWidget(m_view) ;

    // connect up the split signals
    QObject::connect(m_view, SIGNAL(split(QString)), this, SLOT(split(QString))) ;
    QObject::connect(m_view, SIGNAL(unsplit()), this, SLOT(unsplitFromView())) ;
}

ZViewNode02::~ZViewNode02()
{
}

TestView02* ZViewNode02::detatchView()
{
    TestView02 *view = m_view ;
    view->setParent(Q_NULLPTR) ;
    m_view = Q_NULLPTR ;
    return view ;
}

void ZViewNode02::split(const QString &data)
{
    // if our splitter has one child only and it is the view,
    // then we can split
    if (m_splitter && m_view && (m_splitter->count() == 1) && (m_splitter->widget(0) == m_view))
    {
        // remove the current view from the splitter
        m_view->setParent(Q_NULLPTR) ;
        QObject::disconnect(m_view, 0, 0, 0) ;

        // create two new nodes and add them to the splitter
        // their split signal is connected on construction
        TestView02 *new_view = new TestView02(QString("a view")) ;
        new_view->getGraphicsView()->setScene(m_view->getGraphicsView()->scene()) ;
        if (new_view)
        {
            ZViewNode02 *node1 = new ZViewNode02(m_view) ;
            ZViewNode02 *node2 = new ZViewNode02(new_view) ;
            QString dat = data.toLower() ;
            Qt::Orientation orientation = Qt::Horizontal ;
            if (dat == QString("h"))
                orientation = Qt::Horizontal ;
            else if (dat == QString("v"))
                orientation = Qt::Vertical ;
            m_splitter->setOrientation(orientation) ;
            if (node1 && node2)
            {
                m_splitter->addWidget(node1) ;
                m_splitter->addWidget(node2) ;
                m_view = Q_NULLPTR ;
                QObject::connect(node1, SIGNAL(unsplit()), this, SLOT(unsplitNodes())) ;
                QObject::connect(node2, SIGNAL(unsplit()), this, SLOT(unsplitNodes())) ;
            }
        }

    }
}

void ZViewNode02::unsplitNodes()
{
    if (m_splitter && !m_view && (m_splitter->count() == 2))
    {
        ZViewNode02 *node = dynamic_cast<ZViewNode02*>(QObject::sender()) ;
        if (node)
        {
            // get the view from the sender and...
            m_view = node->detatchView() ;

            if (m_view)
            {
                // now reconnect it appropriately
                QObject::disconnect(m_view, 0, 0, 0) ;
                QObject::connect(m_view, SIGNAL(split(QString)), this, SLOT(split(QString))) ;
                QObject::connect(m_view, SIGNAL(unsplit()), this, SLOT(unsplitFromView())) ;

                // remove the two nodes from the splitter
                QWidget *w1 = m_splitter->widget(0),
                        *w2 = m_splitter->widget(1) ;
                w1->setParent(Q_NULLPTR) ;
                QObject::disconnect(w1, 0, 0, 0) ;
                delete w1 ;
                w2->setParent(Q_NULLPTR) ;
                QObject::disconnect(w2, 0, 0, 0) ;
                delete w2 ;

                // add our view to this
                m_splitter->addWidget(m_view) ;
            }
        }
    }
}

void ZViewNode02::unsplitFromView()
{
    emit unsplit() ;
}








ZViewNode03::ZViewNode03(TestView02 *view, QWidget *parent)
    : QSplitter(parent),
      Util::InstanceCounter<ZViewNode03>(),
      Util::ClassName<ZViewNode03>(),
      m_view(view),
      m_node1(Q_NULLPTR),
      m_node2(Q_NULLPTR)
{
    if (!m_view)
        throw std::runtime_error(std::string("ZViewNode03::ZViewNode03() ; view may not be NULL ")) ;

    // add the view to this
    addWidget(m_view) ;

    // connect it up
    connectView() ;
}

ZViewNode03::~ZViewNode03()
{
}

TestView02* ZViewNode03::detatchView()
{
    TestView02 *view = m_view ;
    view->setParent(Q_NULLPTR) ;
    m_view = Q_NULLPTR ;
    return view ;
}


void ZViewNode03::split(const QString &data)
{
    // if we only have one child only and it is the view,
    // then we can split
    if (m_view && (count() == 1) && (widget(0) == m_view))
    {
        // remove the current view
        TestView02 *view = detatchView() ;

        // create two new nodes and add them to this
        // their split signal is connected on construction
        TestView02 *new_view = new TestView02(QString("a view")) ;
        new_view->getGraphicsView()->setScene(view->getGraphicsView()->scene()) ;
        if (new_view)
        {
            m_node1 = new ZViewNode03(view) ;
            m_node2 = new ZViewNode03(new_view) ;
            QString dat = data.toLower() ;
            Qt::Orientation orientation = Qt::Horizontal ;
            if (dat == QString("h"))
                orientation = Qt::Horizontal ;
            else if (dat == QString("v"))
                orientation = Qt::Vertical ;
            setOrientation(orientation) ;
            if (m_node1 && m_node2)
            {
                addWidget(m_node1) ;
                addWidget(m_node2) ;
                QObject::connect(m_node1, SIGNAL(unsplit()), this, SLOT(unsplitNodes())) ;
                QObject::connect(m_node2, SIGNAL(unsplit()), this, SLOT(unsplitNodes())) ;
            }
        }

    }
}

void ZViewNode03::unsplitFromView()
{
    emit unsplit() ;
}

void ZViewNode03::unsplitNodes()
{
    if (!m_view && (count() == 2) && (widget(0) == m_node1) && (widget(1) == m_node2))
    {
        ZViewNode03 *node = dynamic_cast<ZViewNode03*>(QObject::sender()) ;
        if (node)
        {
            // get the view from the sender and...
            m_view = node->detatchView() ;

            if (m_view)
            {
                // remove the two nodes from this
                m_node1->setParent(Q_NULLPTR) ;
                QObject::disconnect(m_node1, 0, 0, 0) ;
                delete m_node1 ;
                m_node1 = Q_NULLPTR ;
                m_node2->setParent(Q_NULLPTR) ;
                QObject::disconnect(m_node2, 0, 0, 0) ;
                delete m_node2 ;
                m_node2 = Q_NULLPTR ;

                // add our view to this
                addWidget(m_view) ;

                // ...reconnect it appropriately
                connectView() ;
            }
        }
    }
}

void ZViewNode03::connectView()
{
    QObject::disconnect(m_view, 0, 0, 0) ;
    QObject::connect(m_view, SIGNAL(split(QString)), this, SLOT(split(QString))) ;
    QObject::connect(m_view, SIGNAL(unsplit()), this, SLOT(unsplitFromView())) ;
}






ZViewNode04::ZViewNode04(TestView02 *view, QWidget *parent)
    : QSplitter(parent),
      Util::InstanceCounter<ZViewNode04>(),
      Util::ClassName<ZViewNode04>(),
      m_view(view)
{
    if (!m_view)
        throw std::runtime_error(std::string("ZViewNode04::ZViewNode04() ; view may not be NULL ")) ;

    // add the view to this
    addWidget(m_view) ;

    // connect it up
    connectView() ;
}

ZViewNode04::~ZViewNode04()
{
}

TestView02* ZViewNode04::detatchView()
{
    TestView02 *view = m_view ;
    view->setParent(Q_NULLPTR) ;
    m_view = Q_NULLPTR ;
    return view ;
}


void ZViewNode04::split(const QString &data)
{
    // if we only have one child only and it is the view,
    // then we can split
    if (m_view && (count() == 1) && (widget(0) == m_view))
    {
        // remove the current view
        TestView02 *view = detatchView() ;

        // create two new nodes and add them to this
        // their split signal is connected on construction
        TestView02 *new_view = new TestView02(QString("a view")) ;
        new_view->getGraphicsView()->setScene(view->getGraphicsView()->scene()) ;
        if (new_view)
        {
            ZViewNode04 *node1 = new ZViewNode04(view),
                    *node2 = new ZViewNode04(new_view) ;
            QString dat = data.toLower() ;
            Qt::Orientation orientation = Qt::Horizontal ;
            if (dat == QString("h"))
                orientation = Qt::Horizontal ;
            else if (dat == QString("v"))
                orientation = Qt::Vertical ;
            setOrientation(orientation) ;
            if (node1 && node2)
            {
                addWidget(node1) ;
                addWidget(node2) ;
                QObject::connect(node1, SIGNAL(unsplit()), this, SLOT(unsplitNodes())) ;
                QObject::connect(node2, SIGNAL(unsplit()), this, SLOT(unsplitNodes())) ;
            }
        }

    }
}


void ZViewNode04::unsplitFromView()
{
    emit unsplit() ;
}


void ZViewNode04::unsplitNodes()
{
    if (!m_view && (count() == 2))
    {
        ZViewNode04 *node = dynamic_cast<ZViewNode04*>(QObject::sender()) ;
        if (node)
        {
            // get the view from the sender and...
            m_view = node->detatchView() ;

            if (m_view)
            {
                ZViewNode04 *node1 = dynamic_cast<ZViewNode04*>(widget(0)),
                        *node2 = dynamic_cast<ZViewNode04*>(widget(1)) ;
                // remove the two nodes from this
                deleteNode(node1) ;
                deleteNode(node2) ;

                // add our view to this and connect it
                addWidget(m_view) ;
                connectView() ;
            }
        }
    }
}

void ZViewNode04::connectView()
{
    if (m_view)
    {
        QObject::disconnect(m_view, 0, 0, 0) ;
        QObject::connect(m_view, SIGNAL(split(QString)), this, SLOT(split(QString))) ;
        QObject::connect(m_view, SIGNAL(unsplit()), this, SLOT(unsplitFromView())) ;
    }
}

void ZViewNode04::deleteNode(ZViewNode04 *node)
{
    if (node)
    {
        node->setParent(Q_NULLPTR) ;
        QObject::disconnect(node, 0, 0, 0) ;
        delete node ;
    }
}



// public one to create top level node...
ZViewNode05::ZViewNode05(QWidget *parent)
    : QSplitter(parent),
      Util::InstanceCounter<ZViewNode05>(),
      Util::ClassName<ZViewNode05>(),
      m_graphics_scene(Q_NULLPTR)
{
    m_graphics_scene = new QGraphicsScene ;
    if (!m_graphics_scene)
        throw std::runtime_error(std::string("ZViewNode05::ZViewNode05() ; unable to create graphics scene for this node ")) ;
    m_graphics_scene->setParent(this) ;
    m_graphics_scene->setItemIndexMethod(QGraphicsScene::NoIndex) ;

    TestView02 *view = new TestView02(QString("Name here"), this) ;
    view->getGraphicsView()->setScene(m_graphics_scene) ;
    if (!view)
        throw std::runtime_error(std::string("ZViewNode05::ZViewNode05() ; unable to create view for this node ")) ;
    addWidget(view) ;
    connectView(view) ;
}

ZViewNode05::ZViewNode05(TestView02 *view)
    : QSplitter(Q_NULLPTR),
      Util::InstanceCounter<ZViewNode05>(),
      Util::ClassName<ZViewNode05>(),
      m_graphics_scene(Q_NULLPTR)
{
    if (!view)
        throw std::runtime_error(std::string("ZViewNode05::ZViewNode05() ; view may not be Q_NULLPTR ")) ;
    if (!view->getGraphicsView() || !view->getGraphicsView()->scene())
        throw std::runtime_error(std::string("ZViewNode05::ZViewNode05() ; view does not have a scene set ")) ;
    addWidget(view) ;
    connectView(view) ;
    m_graphics_scene = view->getGraphicsView()->scene() ;
}


ZViewNode05::ZViewNode05(QGraphicsScene* graphics_scene)
    : QSplitter(Q_NULLPTR),
      Util::InstanceCounter<ZViewNode05>(),
      Util::ClassName<ZViewNode05>(),
      m_graphics_scene(graphics_scene)
{
    TestView02 *view = new TestView02(QString("Name here"), this) ;
    if (!view)
        throw std::runtime_error(std::string("ZViewNode05::ZViewNode05() ; unable to create view for this node ")) ;
    view->getGraphicsView()->setScene(m_graphics_scene) ;

    addWidget(view) ;
    connectView(view) ;

}

ZViewNode05::~ZViewNode05()
{
}


TestView02* ZViewNode05::detatchView()
{
    TestView02 *view = Q_NULLPTR ;
    if ((count() == 1) && (view = dynamic_cast<TestView02*>(widget(0))))
    {
        view->setParent(Q_NULLPTR) ;
    }
    return view ;
}


void ZViewNode05::split(const QString &data)
{
    // if we only have one child only and it is the view,
    // then we can split
    TestView02 *view = Q_NULLPTR ;
    if ((view = detatchView()))
    {
        // create two new nodes and add them to this
        // their split signal is connected on construction
        ZViewNode05 *node1 = new ZViewNode05(view),
                *node2 = new ZViewNode05(m_graphics_scene) ;
        QString dat = data.toLower() ;
        Qt::Orientation orientation = Qt::Horizontal ;
        if (dat == QString("h"))
            orientation = Qt::Horizontal ;
        else if (dat == QString("v"))
            orientation = Qt::Vertical ;
        setOrientation(orientation) ;
        if (node1 && node2)
        {
            addWidget(node1) ;
            addWidget(node2) ;
            QObject::connect(node1, SIGNAL(unsplit()), this, SLOT(unsplitNodes())) ;
            QObject::connect(node2, SIGNAL(unsplit()), this, SLOT(unsplitNodes())) ;
        }
    }
}


void ZViewNode05::unsplitFromView()
{
    emit unsplit() ;
}


void ZViewNode05::unsplitNodes()
{
    if (count() == 2)
    {
        ZViewNode05 *node = dynamic_cast<ZViewNode05*>(QObject::sender()) ;
        if (node)
        {
            // get the view from the sender and...
            TestView02 *view = node->detatchView() ;

            if (view)
            {
                // remove the two nodes from this
                deleteNodes() ;

                // add our view to this and connect it
                addWidget(view) ;
                connectView(view) ;
            }
        }
    }
}


void ZViewNode05::connectView(TestView02 *view)
{
    if (view)
    {
        QObject::disconnect(view, 0, 0, 0) ;
        QObject::connect(view, SIGNAL(split(QString)), this, SLOT(split(QString))) ;
        QObject::connect(view, SIGNAL(unsplit()), this, SLOT(unsplitFromView())) ;
    }
}

void ZViewNode05::deleteNodes()
{
    if (count()==2)
    {
        ZViewNode05 *node1 = dynamic_cast<ZViewNode05*>(widget(0)),
                *node2 = dynamic_cast<ZViewNode05*>(widget(1)) ;
        if (node1)
            deleteNode(node1) ;
        if (node2)
            deleteNode(node2) ;
    }
}

void ZViewNode05::deleteNode(ZViewNode05 *node)
{
    if (node)
    {
        node->setParent(Q_NULLPTR) ;
        QObject::disconnect(node, 0, 0, 0) ;
        delete node ;
    }
}

} // namespace GUI

} // namespace ZMap

