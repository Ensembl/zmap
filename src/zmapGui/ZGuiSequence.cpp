/*  File: ZGuiSequence.cpp
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

#include "ZGuiSequence.h"
#include "ZGuiScene.h"
#include "ZGuiGraphicsView.h"
#include "ZGuiViewNode.h"
#include "ZGuiViewControl.h"

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <QLabel>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiSequence>::m_name("ZGuiSequence") ;
const QString ZGuiSequence::m_name_base("ZGuiSequence_"),
    ZGuiSequence::m_style_sheet_current("QSplitter#%1 {border: 2px solid black;}"),
    ZGuiSequence::m_style_sheet_normal("QSplitter#%1 {}") ;

QString ZGuiSequence::getNameFromID(ZInternalID id)
{
    QString str ;
    return m_name_base + str.setNum(id) ;
}

ZGuiSequence::ZGuiSequence(ZInternalID id,
                           const QString& sequence_name,
                           ZGuiScene* m_sequence_scene,
                           QWidget* parent)
    : QSplitter(parent),
      Util::InstanceCounter<ZGuiSequence>(),
      Util::ClassName<ZGuiSequence>(),
      m_id(id),
      m_sequence_scene(m_sequence_scene),
      m_locus_scene(Q_NULLPTR),
      m_locus_view(Q_NULLPTR),
      m_sequence_view(Q_NULLPTR),
      m_current_view(Q_NULLPTR),
      m_sequence_name(sequence_name),
      m_is_current(false)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; can not create without instance of QApplication existing ")) ;
    if (m_id != m_sequence_scene->getID())
        throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; id passed in does not match that of the scene ")) ;
    if (!m_sequence_scene)
        throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; may not create with a scene that is NULL ")) ;
    setOrientation(Qt::Vertical) ;

    // top level label...
    QLabel *label = new QLabel(QString(sequence_name)) ;
    addWidget(label) ;

    // if we have a locus scene, then an extra view is created at the top; this is a
    // non-splittable view
    if (m_sequence_scene->hasLocusScene())
    {
        if (!(m_locus_view = ZGuiGraphicsView::newInstance(Q_NULLPTR)))
            throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; unable to create locus view ")) ;
        m_locus_scene = m_sequence_scene->getLocusScene() ;
        if (!m_locus_scene)
            throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; unable to access locus_scene ")) ;
        if (!connect(m_locus_view, SIGNAL(signal_requestMadeCurrent()), this, SLOT(slot_viewMadeCurrent())))
            throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; unable to connect locus_view madeCurrent() to appropriate slot")) ;
        if (!connect(m_locus_view, SIGNAL(signal_contextMenuEvent(QPointF)), this, SIGNAL(signal_contextMenuEvent(QPointF))))
            throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; unable to connect contextMenuEvent() to appropriate receiver")) ;
        addWidget(m_locus_view) ;
    }

    // this is the normal splittable sequence view
    if (!(m_sequence_view = ZGuiViewNode::newInstance(m_sequence_name, id, m_sequence_scene, Q_NULLPTR)))
        throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; unable to create sequence splittable view ")) ;
    addWidget(m_sequence_view) ;
    if (!connect(m_sequence_view, SIGNAL(signal_madeCurrent(ZGuiGraphicsView*)), this, SLOT(slot_viewMadeCurrent(ZGuiGraphicsView*))))
        throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; unable to connect sequence_view madeCurrent() to appropriate slot ")) ;

    // get the selectionChanged() signal
    //if (!connect(m_sequence_scene, SIGNAL(signal_contextMenuEvent(QPointF)), this, SIGNAL(signal_contextMenuEvent(QPointF))))
    //    throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; unable to connect sequence context menu event ")) ;
    if (!connect(m_sequence_view, SIGNAL(signal_contextMenuEvent(QPointF)), this, SIGNAL(signal_contextMenuEvent(QPointF))))
        throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence() ; unable to connect sequence_view context menu event")) ;
    if (!connect(m_sequence_scene, SIGNAL(selectionChanged()), this, SLOT(slot_sequenceSelectionChanged())))
        throw std::runtime_error(std::string("ZGuiSequence::ZGuiSequence()) ; unable to connect sequence selection changed ")) ;

    //setFrameDrawing() ;
    setObjectName(getNameFromID(m_id)) ;
    setBordersStyle() ;

    // on construction we do not have a current view, so the only one in
    // the sequence view should be set to be current
    m_current_view = m_sequence_view->getView() ;
}

ZGuiSequence::~ZGuiSequence()
{
}

void ZGuiSequence::setFrameDrawing()
{
    if (m_is_current)
    {
        setFrameDrawingFocus() ;
    }
    else
    {
        setFrameDrawingNormal() ;
    }
}

void ZGuiSequence::setFrameDrawingFocus()
{
    setFrameStyle(Box | Plain) ;
}

void ZGuiSequence::setFrameDrawingNormal()
{
    setFrameStyle(Box | Raised) ;
}

void ZGuiSequence::setCurrent(bool flag)
{
    m_is_current = flag ;
    //setFrameDrawing() ;

    setBordersStyle() ;

    // so now we are coming back down again innit
    QList<ZGuiGraphicsView*> list = findChildren<ZGuiGraphicsView*>(QString(),
                                                                    Qt::FindChildrenRecursively) ;
    if (m_is_current)
    {
        for (auto it = list.begin() ; it != list.end() ; ++it)
        {
            ZGuiGraphicsView * view = *it ;

            if (view)
            {
                view->setCurrent(view == m_current_view) ;
            }
        }
    }
    else
    {
        for (auto it = list.begin() ; it != list.end() ; ++it)
        {
            ZGuiGraphicsView * view = *it ;
            if (view)
            {
                view->setCurrent(false) ;
            }
        }
    }
}

void ZGuiSequence::setBordersStyle()
{
    if (m_is_current)
    {
        setStyleSheet(m_style_sheet_current.arg(objectName())) ;
    }
    else
    {
        setStyleSheet(m_style_sheet_normal.arg(objectName())) ;
    }
}

void ZGuiSequence::mousePressEvent(QMouseEvent* event)
{
    if (event)
    {
        emit signal_sequenceMadeCurrent(this) ;
    }
    QSplitter::mousePressEvent(event) ;
}

ZGuiSequence* ZGuiSequence::newInstance(ZInternalID id,
                                        const QString& sequence_name,
                                        ZGuiScene* scene,
                                        QWidget *parent)
{
    ZGuiSequence * sequence = Q_NULLPTR ;

    try
    {
        sequence = new ZGuiSequence(id, sequence_name, scene, parent) ;
    }
    catch (...)
    {
        sequence = Q_NULLPTR ;
    }

    return sequence ;
}


////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiSequence::slot_viewMadeCurrent(ZGuiGraphicsView* view)
{
#ifndef QT_NO_DEBUG
    qDebug() << "ZGuiSequence::slot_viewMadeCurrent() ; " << this << view ;
#endif
    if (view && m_current_view != view)
    {
        m_current_view = view ;
    }
    emit signal_sequenceMadeCurrent(this) ;
}

void ZGuiSequence::slot_viewMadeCurrent()
{
    ZGuiGraphicsView *view = dynamic_cast<ZGuiGraphicsView*>(QObject::sender()) ;
    slot_viewMadeCurrent(view) ;
}

//void ZGuiSequence::slot_sequenceMadeCurrent()
//{
//    emit signal_sequenceMadeCurrent(m_id) ;
//}

void ZGuiSequence::slot_contextMenuEvent(const QPointF& pos)
{
    emit signal_contextMenuEvent(pos) ;
}

void ZGuiSequence::slot_sequenceSelectionChanged()
{
    if (m_sequence_scene)
    {
        QList<QGraphicsItem*> list = m_sequence_scene->selectedItems() ;
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiSequence::slot_sequenceSelectionChanged() ; " << list.size() ;
        for (auto it = list.begin() ; it != list.end() ; ++it)
        {
            if (*it)
                qDebug() << "item is: " << *it ;
        }
#endif
    }
    emit signal_sequenceSelectionChanged() ;
}

} // namespace GUI

} // namespace ZMap

