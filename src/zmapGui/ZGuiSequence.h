/*  File: ZGuiSequence.h
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


#ifndef ZGUISEQUENCE_H
#define ZGUISEQUENCE_H

#include <cstddef>
#include <string>
#include "ZInternalID.h"
#include "Utilities.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <QApplication>
#include <QSplitter>
#include <QString>
#include <string>


namespace ZMap
{

namespace GUI
{

class ZGuiViewNode ;
class ZGuiGraphicsView ;
class ZGuiScene ;

//
// This is a container for a locus view and
// a splittable view node.
//

class ZGuiSequence : public QSplitter,
        public Util::InstanceCounter<ZGuiSequence>,
        public Util::ClassName<ZGuiSequence>
{

    Q_OBJECT

public:

    static QString getNameFromID(ZInternalID id) ;
    static ZGuiSequence * newInstance(const ZInternalID id,
                                      const QString &sequence_name,
                                      ZGuiScene* scene,
                                      QWidget* parent = Q_NULLPTR ) ;


    ~ZGuiSequence() ;
    ZInternalID getID () const {return m_id ; }

    void setCurrent(bool) ;
    bool isCurrent() const {return m_is_current ; }
    ZGuiGraphicsView * getCurrentView() const {return m_current_view ; }

signals:
    void signal_sequenceMadeCurrent(ZGuiSequence *) ;
    void signal_contextMenuEvent(const QPointF& pos) ;
    void signal_sequenceSelectionChanged() ;

public slots:
    void slot_contextMenuEvent(const QPointF& pos) ;
    void slot_viewMadeCurrent() ;
    void slot_viewMadeCurrent(ZGuiGraphicsView*) ;
    void slot_sequenceSelectionChanged() ;

protected:
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE ;

private:

     ZGuiSequence(ZInternalID id,
                  const QString& sequence_name,
                  ZGuiScene* scene,
                  QWidget *parent = 0) ;
     ZGuiSequence(const ZGuiSequence& ) = delete ;
     ZGuiSequence& operator=(const ZGuiSequence& ) = delete ;

     static const QString m_name_base,
        m_style_sheet_current,
        m_style_sheet_normal ;

     void setFrameDrawing() ;
     void setFrameDrawingFocus() ;
     void setFrameDrawingNormal() ;
     void setBordersStyle() ;

     ZInternalID m_id ;
     ZGuiScene *m_sequence_scene,
        *m_locus_scene ;
     ZGuiGraphicsView *m_locus_view ;
     ZGuiViewNode *m_sequence_view ;
     ZGuiGraphicsView *m_current_view ;
     QString m_sequence_name ;
     bool m_is_current ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUISEQUENCE_H
