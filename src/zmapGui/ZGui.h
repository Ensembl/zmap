#ifndef ZGUI_H
#define ZGUI_H
/*  File: ZGui.h
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

#include <cstddef>
#include <string>
#include <map>
#include <set>

#include <QApplication>
#include <QWidget>
#include <QString>

QT_BEGIN_NAMESPACE
class QLayout ;
class QCloseEvent ;
QT_END_NAMESPACE

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"

namespace ZMap
{

namespace GUI
{

class ZGuiTopControl ;
class ZGuiViewControl ;
class ZGuiPresentation ;
class ZGuiSequences ;

// this is the thing that actually appears on the screen
class ZGui : public QWidget,
        public Util::InstanceCounter<ZGui>,
        public Util::ClassName<ZGui>
{
    Q_OBJECT

public:
    static ZGui * newInstance(ZInternalID id, ZGuiPresentation* parent = Q_NULLPTR ) ;

    ZGui(ZInternalID id, ZGuiPresentation* parent = Q_NULLPTR ) ;
    ~ZGui() ;

    ZInternalID getID() const {return m_id ; }
    bool addSequence(ZInternalID id) ;
    bool addSequences(const std::set<ZInternalID>& dataset ) ;
    bool deleteSequence(ZInternalID id) ;
    bool orientation() const ;
    bool setOrientation(bool orient) ;

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE ;

private:
    static const QString m_display_name ;

    ZGui(const ZGui &gui) = delete ;
    ZGui& operator=(const ZGui& gui) = delete ;

    ZGuiPresentation *m_parent ;
    ZInternalID m_id ;
    QLayout *m_layout ;
    ZGuiTopControl *m_top_control ;
    ZGuiViewControl *m_view_control ;
    ZGuiSequences *m_sequences ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUI_H
