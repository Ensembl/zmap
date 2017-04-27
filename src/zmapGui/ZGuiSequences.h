/*  File: ZGuiSequences.h
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

#ifndef ZGUISEQUENCES_H
#define ZGUISEQUENCES_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <QApplication>
#include <QSplitter>
#include <cstddef>
#include <string>
#include <set>

namespace ZMap
{

namespace GUI
{

class ZGuiSequence ;
class ZGuiViewControl ;
class ZGuiScene ;

//
// Effectively simply a container ZGuiSequence
//
class ZGuiSequences : public QSplitter,
        public Util::InstanceCounter<ZGuiSequences>,
        public Util::ClassName<ZGuiSequences>
{

    Q_OBJECT

public:
    static ZGuiSequences* newInstance(ZGuiViewControl *view_control,
                                      QWidget* parent = Q_NULLPTR) ;

    ~ZGuiSequences() ;

    size_t size() const ;
    bool addSequence(ZInternalID id, ZGuiScene * sequence) ;
    bool deleteSequence(ZInternalID id) ;
    bool isPresent(ZInternalID id) const ;
    std::set<ZInternalID> getSequenceIDs() const ;
    bool setSequenceOrder(const std::vector<ZInternalID> & data) ;
    bool setCurrent(ZInternalID id) ;
    ZGuiSequence* getCurrent() const {return m_current_sequence ; }

signals:

    void signal_contextMenuEvent(const QPointF & pos) ;
    void signal_sequenceMadeCurrent(ZGuiSequence*) ;
    void signal_sequenceSelectionChanged() ;

public slots:

    void slot_sequenceMadeCurrent(ZGuiSequence*) ;
    void slot_contextMenuEvent(const QPointF& pos) ;

private:

    ZGuiSequences(ZGuiViewControl* view_control, QWidget *parent = Q_NULLPTR) ;
    ZGuiSequences(const ZGuiSequences &) = delete ;
    ZGuiSequences& operator=(const ZGuiSequences&) = delete ;

    ZGuiSequence *find(ZInternalID id) const ;
    ZGuiViewControl *m_view_control ;
    ZGuiSequence *m_current_sequence ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUISEQUENCES_H
