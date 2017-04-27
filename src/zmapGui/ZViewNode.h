/*  File: ZViewNode.h
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

#ifndef ZVIEWNODE_H
#define ZVIEWNODE_H

#include <cstddef>
#include <string>

#include "InstanceCounter.h"
#include "ClassName.h"
#include <QFrame>
#include <QSplitter>

QT_BEGIN_NAMESPACE
class QGraphicsScene ;
class QLayout ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

class TestView02 ;

class ZViewNode01 : public QWidget,
        public Util::InstanceCounter<ZViewNode01>,
        public Util::ClassName<ZViewNode01>
{
    Q_OBJECT

public:

    ZViewNode01(QWidget *parent = Q_NULLPTR) ;
    ZViewNode01(TestView02 *view, QWidget *parent = Q_NULLPTR);
    ~ZViewNode01() ;

    bool setView(TestView02 *view) ;
    TestView02 *getView() const {return m_view; }
    TestView02 *detatchView() ;

public slots:
    void split(const QString &) ;
    void unsplit() ;

private:

    QLayout *m_layout ;
    TestView02 *m_view ;
    QSplitter *m_splitter ;
};


//new version of the same thing
class ZViewNode02: public QWidget,
        public Util::InstanceCounter<ZViewNode02>,
        public Util::ClassName<ZViewNode02>
{
    Q_OBJECT

public:

    ZViewNode02(TestView02 *view, QWidget *parent=Q_NULLPTR) ;
    ~ZViewNode02() ;

    TestView02 *detatchView() ;

signals:
    void unsplit() ;

public slots:
    void split(const QString &data) ;
    void unsplitFromView() ;
    void unsplitNodes() ;

protected:

private:

    TestView02 *m_view ;
    QSplitter *m_splitter ;
} ;

// third version of the same thing; in this case the node isa splitter
class ZViewNode03: public QSplitter,
        public Util::InstanceCounter<ZViewNode03>,
        public Util::ClassName<ZViewNode03>
{
    Q_OBJECT

public:

    ZViewNode03(TestView02 *view, QWidget *parent=Q_NULLPTR) ;
    ~ZViewNode03() ;

    TestView02 *detatchView() ;

signals:
    void unsplit() ;

public slots:
    void split(const QString &data) ;
    void unsplitFromView() ;
    void unsplitNodes() ;

protected:

private:
    void connectView() ;

    TestView02 *m_view ;
    ZViewNode03 *m_node1, *m_node2 ;
};


// another version of the same thing with
// no need to store the extra nodes; we still need to pass the
// view around
class ZViewNode04: public QSplitter,
        public Util::InstanceCounter<ZViewNode04>,
        public Util::ClassName<ZViewNode04>
{
    Q_OBJECT

public:

    ZViewNode04(TestView02 *view, QWidget *parent=Q_NULLPTR) ;
    ~ZViewNode04() ;

    TestView02 *detatchView() ;

signals:
    void unsplit() ;

public slots:
    void split(const QString &data) ;
    void unsplitFromView() ;
    void unsplitNodes() ;

protected:

private:

    void connectView() ;
    void deleteNode(ZViewNode04 *node) ;

    TestView02 *m_view ;
};

// another version of the same thing, with no extra storage;
// wow, saved 8 bytes per instance!
class ZViewNode05: public QSplitter,
        public Util::InstanceCounter<ZViewNode05>,
        public Util::ClassName<ZViewNode05>
{
    Q_OBJECT

public:

    ZViewNode05(QWidget *parent=0) ;
    ~ZViewNode05() ;

    TestView02 *detatchView() ;
    QGraphicsScene * getGraphicsScene() const {return m_graphics_scene ; }

signals:
    void unsplit() ;

public slots:
    void split(const QString &data) ;
    void unsplitFromView() ;
    void unsplitNodes() ;

protected:

private:

    ZViewNode05(TestView02 * view) ;
    ZViewNode05(QGraphicsScene* graphics_scene)  ;

    void connectView(TestView02 *view) ;
    void deleteNodes() ;
    void deleteNode(ZViewNode05 *node) ;

    QGraphicsScene *m_graphics_scene ;
};


} // namespace GUI

} // namespace ZMap



#endif // ZVIEWNODE_H
