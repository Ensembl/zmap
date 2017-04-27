/*  File: ZGuiTextDisplayDialogue.h
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

#ifndef ZGUITEXTDISPLAYDIALOGUE_H
#define ZGUITEXTDISPLAYDIALOGUE_H

#include <QDialog>
#include <cstddef>
#include <string>

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"

//
// Dialogue for display of some text (e.g. the feature and/or featureset
// info displays). By default this should be used in a non-modal way
// from the ZGuiViewControl instance or from ZGuiMain.
//

QT_BEGIN_NAMESPACE
class QTextEdit ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

class ZGuiTextDisplayDialogue : public QDialog,
        public Util::InstanceCounter<ZGuiTextDisplayDialogue>,
        public Util::ClassName<ZGuiTextDisplayDialogue>
{
    Q_OBJECT

public:
    static ZGuiTextDisplayDialogue* newInstance(QWidget *parent=Q_NULLPTR) ;

    ~ZGuiTextDisplayDialogue() ;

    void setText(const QString& text) ;
    void setText(const std::string& text) ;
    QString getTextAsQString() const ;
    std::string getTextAsStdString() const ;

signals:
    void signal_received_close_event() ;

public slots:

protected:
    void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE ;

private:

    explicit ZGuiTextDisplayDialogue(QWidget* parent= Q_NULLPTR) ;
    ZGuiTextDisplayDialogue(const ZGuiTextDisplayDialogue& ) = delete ;
    ZGuiTextDisplayDialogue& operator=(const ZGuiTextDisplayDialogue&) = delete ;

    QWidget *createControls01() ;
    static const QString m_display_name,
        m_placeholder_text ;


    QTextEdit *m_text_edit ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUITEXTDISPLAYDIALOGUE_H
