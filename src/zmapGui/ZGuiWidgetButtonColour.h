/*  File: ZGuiWidgetButtonColour.h
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

#ifndef ZGUIWIDGETBUTTONCOLOUR_H
#define ZGUIWIDGETBUTTONCOLOUR_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <QApplication>
#include <QPushButton>
#include <QColor>

#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{


class ZGuiWidgetButtonColour : public QPushButton,
        public Util::InstanceCounter<ZGuiWidgetButtonColour>,
        public Util::ClassName<ZGuiWidgetButtonColour>
{

    Q_OBJECT

public:
    static ZGuiWidgetButtonColour * newInstance(const QColor & col,
                                           QWidget *parent= Q_NULLPTR) ;

    ~ZGuiWidgetButtonColour() ;

    void setColour(const QColor &col) ;
    QColor getColour() const {return m_colour ; }

private slots:
    void slot_clicked() ;
    void slot_set_button_colour(const QColor&) ;

private:

    ZGuiWidgetButtonColour(const QColor& col,
                                    QWidget* parent = Q_NULLPTR ) ;
    ZGuiWidgetButtonColour(const ZGuiWidgetButtonColour&) = delete ;
    ZGuiWidgetButtonColour& operator=(const ZGuiWidgetButtonColour&) = delete ;

    static const QString m_style_string ;

    QColor m_colour ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIWIDGETBUTTONCOLOUR_H
