/*  File: ZGuiViewNode.h
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

#ifndef ZGUIVIEWNODE_H
#define ZGUIVIEWNODE_H

#include <cstddef>
#include <string>
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <QApplication>
#include <QSplitter>

//
// This is the splittable node; represents a sequence in a splittable
// view
//

namespace ZMap
{

namespace GUI
{

class ZGuiGraphicsView ;
class ZGuiScene ;

class ZGuiViewNode : public QSplitter,
        public Util::InstanceCounter<ZGuiViewNode>,
        public Util::ClassName<ZGuiViewNode>
{

    Q_OBJECT

public:
    static ZGuiViewNode* newInstance(const QString& sequence_name,
                                     ZInternalID id,
                                     ZGuiScene *sequence,
                                     QWidget* parent) ;


    ~ZGuiViewNode() ;

    ZGuiGraphicsView *detatchView() ;
    ZGuiGraphicsView *getView() ;
    ZInternalID getID() const {return m_id ; }
    void setCurrent(ZGuiGraphicsView* view, bool current) ;
    ZGuiScene* getScene() const {return m_sequence; }

signals:
    void signal_unsplit() ;
    void signal_madeCurrent(ZGuiGraphicsView *) ;
    void signal_contextMenuEvent(const QPointF &pos) ;

public slots:
    void slot_itemTakenFocus() ;
    void slot_contextMenuEvent(const QPointF& pos) ;
    void slot_split(const QString &data) ;
    void slot_unsplit() ;
    void slot_viewMadeCurrent(ZGuiGraphicsView*) ;
    void slot_viewMadeCurrent() ;

protected:

private:
    static const QString m_orientation_h,
        m_orientation_v ;

    ZGuiViewNode(const QString &sequence_name,
                 ZInternalID id,
                 ZGuiScene *sequence,
                 QWidget *parent);

    ZGuiViewNode(const QString &sequence_name,
                 ZInternalID id,
                 ZGuiGraphicsView * view) ;
    ZGuiViewNode(const QString &sequence_name,
                 ZInternalID id,
                 ZGuiScene  *sequence) ;

    ZGuiViewNode(const ZGuiViewNode& view) = delete ;
    ZGuiViewNode& operator=(const ZGuiViewNode& view) = delete ;

    void connectView(ZGuiGraphicsView *view) ;
    void connectNode(ZGuiViewNode *node) ;
    void deleteNodes() ;
    void deleteNode(ZGuiViewNode *node) ;

    ZInternalID m_id ;
    QString m_sequence_name ;
    ZGuiScene *m_sequence ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIVIEWNODE_H
