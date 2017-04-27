/*  File: ZGuiSequences.cpp
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

#include "ZGuiSequences.h"
#include "ZGuiViewNode.h"
#include "ZGuiViewControl.h"
#include "ZGuiSequenceContextMenu.h"
#include "ZGuiGraphicsView.h"
#include "ZGuiSequence.h"
#include "ZGuiView.h"
#include "ZGuiScene.h"
#include <sstream>
#include <stdexcept>
#include <memory>
#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiSequences>::m_name("ZGuiSequences") ;

ZGuiSequences::ZGuiSequences(ZGuiViewControl* view_control, QWidget* parent)
    : QSplitter(parent),
      Util::InstanceCounter<ZGuiSequences>(),
      Util::ClassName<ZGuiSequences>(),
      m_view_control(view_control),
      m_current_sequence(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiSequences::ZGuiSequences() ; can not create without instance of QApplication existing ")) ;
    if (!m_view_control)
        throw std::runtime_error(std::string("ZGuiSequences::ZGuiSequences() ; m_view_control may not be set to nullptr ")) ;
}

ZGuiSequences::~ZGuiSequences()
{
}

ZGuiSequences* ZGuiSequences::newInstance(ZGuiViewControl *view_control,
                                      QWidget* parent)
{
    ZGuiSequences* sequences = Q_NULLPTR ;

    try
    {
        sequences = new ZGuiSequences(view_control, parent) ;
    }
    catch (...)
    {
        sequences = Q_NULLPTR ;
    }

    return sequences ;
}

// number of sequences present in the object
size_t ZGuiSequences::size() const
{
    QList<ZGuiSequence*> list = findChildren<ZGuiSequence*>(QString(),
                                                            Qt::FindDirectChildrenOnly) ;
    return list.size() ;
}

// create a sequence with the given ID and add to the container
bool ZGuiSequences::addSequence(ZInternalID id, ZGuiScene * sequence )
{
    bool result = false ;
    if (!id || isPresent(id) || !sequence || id != sequence->getID() )
        return result ;
    std::stringstream str ;
    str << "Sequence " << id ;
    QString sequence_name(QString::fromStdString(str.str()));

    ZGuiSequence *the_sequence = ZGuiSequence::newInstance(id, sequence_name, sequence) ;
    if (the_sequence)
    {
        addWidget(the_sequence) ;
        connect(the_sequence, SIGNAL(signal_sequenceMadeCurrent(ZGuiSequence*)), this, SLOT(slot_sequenceMadeCurrent(ZGuiSequence*))) ;
        connect(the_sequence, SIGNAL(signal_contextMenuEvent(QPointF)), this, SIGNAL(signal_contextMenuEvent(QPointF))) ;
        connect(the_sequence, SIGNAL(signal_sequenceSelectionChanged()), this, SIGNAL(signal_sequenceSelectionChanged())) ;
        if (size() == 1)
        {
            slot_sequenceMadeCurrent(the_sequence) ;
        }
        result = true ;
    }

    return result ;
}

bool ZGuiSequences::deleteSequence(ZInternalID id)
{
    bool result = false ;
    ZGuiSequence *sequence = Q_NULLPTR ;
    if (!id || !(sequence = find(id)))
        return result ;
    if (sequence)
    {
        sequence->setParent(Q_NULLPTR) ;
        delete sequence ;
    }
    QList<ZGuiSequence*> list = findChildren<ZGuiSequence*>(QString(),
                                                            Qt::FindDirectChildrenOnly) ;
    if (list.size())
    {
        ZGuiSequence* the_sequence = list.at(0) ;
        if (the_sequence)
        {
            the_sequence->setCurrent(true) ;
            //emit signal_sequenceMadeCurrent(the_sequence->getID()) ;
        }
    }
    else
    {
        // we have no sequence, so...
        emit signal_sequenceMadeCurrent(nullptr) ;
    }
    return result ;
}

// check whether a sequence with the given id is present in the container
bool ZGuiSequences::isPresent(ZInternalID id) const
{
    return static_cast<bool>(find(id)) ;
}

// find the ids of all sequences we store
std::set<ZInternalID> ZGuiSequences::getSequenceIDs() const
{
    std::set<ZInternalID> data ;
    QList<ZGuiSequence*> sequences = findChildren<ZGuiSequence*>(QString(),
                                                                 Qt::FindDirectChildrenOnly) ;
    for (auto it = sequences.begin() ; it != sequences.end() ; ++it)
    {
        if (*it)
        {
            data.insert((*it)->getID()) ;
        }
    }
    return data ;
}

// set the orders of the widgets in the splitter; ignores orientation
bool ZGuiSequences::setSequenceOrder(const std::vector<ZInternalID> & data)
{
    bool result = true ;
    // first check that the incoming list has the same size and contains the same
    // ids as the current object...
    std::set<ZInternalID> current = getSequenceIDs() ;
    if (data.size() != current.size())
        return result ;
    for (auto it = data.begin() ; it != data.end() ; ++it)
    {
        if (!(result = current.find(*it) != current.end()))
            break ;
    }
    // if this is still true, they were all found so we can proceed and
    // reorder...
    if (result)
    {
        int index = 0 ;
        for (auto it = data.begin() ; it != data.end() ; ++it, ++index)
        {
            ZGuiSequence* sequence = find(*it) ;
            if (sequence)
            {
                insertWidget(index, sequence) ;
            }
        }
    }

    return result ;
}

bool ZGuiSequences::setCurrent(ZInternalID id)
{
    bool result = false ;
    QList<ZGuiSequence*> list = findChildren<ZGuiSequence*>(QString(),
                                                            Qt::FindDirectChildrenOnly) ;
    for (auto it = list.begin() ; it != list.end() ; ++it)
    {
        ZGuiSequence *sequence = *it ;
        if (sequence)
        {
            if (sequence->getID() == id)
            {
                m_current_sequence = sequence ;
                sequence->setCurrent(true) ;
                result = true ;
            }
            else
            {
                sequence->setCurrent(false) ;
            }
        }
    }
    return result ;
}

// find the node with given ID and return pointer to it
ZGuiSequence * ZGuiSequences::find(ZInternalID id) const
{
    ZGuiSequence *sequence = Q_NULLPTR ;
    sequence = findChild<ZGuiSequence*>(ZGuiSequence::getNameFromID(id),
                                        Qt::FindDirectChildrenOnly) ;
    return sequence ;
}

// actions to be performed when a sequence is made the current sequence; currently
// simply inform the control object
void ZGuiSequences::slot_sequenceMadeCurrent(ZGuiSequence* sequence)
{
    if (m_view_control && sequence)
    {
        setCurrent(sequence->getID()) ;
        m_view_control->setCurrentSequence(sequence) ;
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiSequences::slot_sequenceMadeCurrent() " << sequence << sequence->getCurrentView() ;
#endif
    }
}

// context menu event received for a sequence; doesn't matter which window this is in
// as long as we can identify the culprit; this can be done from QObject internals
void ZGuiSequences::slot_contextMenuEvent(const QPointF& pos)
{
    //QObject* sender = QObject::sender() ;

    //ZGuiSequence *sequence = dynamic_cast<ZGuiSequence*>(sender) ;
    //if (m_view_control && sequence)
    //{
       // m_view_control->setCurrentSequence(sequence) ;
#ifndef QT_NO_DEBUG
        qDebug() << "ZGuiSequences::slot_contextMenuEvent() " << this;
#endif
    //}

    //std::unique_ptr<ZGuiSequenceContextMenu> menu(ZGuiSequenceContextMenu::newInstance(this)) ;
//#ifndef QT_NO_DEBUG
    //QAction* action = menu->exec(QCursor::pos()) ;
    //qDebug() << "ZGuiSequences::slot_contextMenuEvent(): " << action << (action ? action->text() : QString("no action selected")) ;
//#endif

    emit signal_contextMenuEvent(pos) ;
}

} // namespace GUI

} // namespace ZMap
