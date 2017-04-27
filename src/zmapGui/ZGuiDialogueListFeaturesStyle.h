/*  File: ZGuiDialogueListFeaturesStyle.h
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

#ifndef ZGUIDIALOGUELISTFEATURESSTYLE_H
#define ZGUIDIALOGUELISTFEATURESSTYLE_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include <QProxyStyle>
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

class ZGuiDialogueListFeaturesStyle : public QProxyStyle,
        public Util::InstanceCounter<ZGuiDialogueListFeaturesStyle>,
        public Util::ClassName<ZGuiDialogueListFeaturesStyle>
{
public:
    static ZGuiDialogueListFeaturesStyle* newInstance() ;

    ~ZGuiDialogueListFeaturesStyle() ;

    int styleHint(StyleHint hint,
                  const QStyleOption *option=Q_NULLPTR,
                  const QWidget *widget=Q_NULLPTR,
                  QStyleHintReturn *returnData=Q_NULLPTR) const override ;

private:

    ZGuiDialogueListFeaturesStyle();
    ZGuiDialogueListFeaturesStyle(const ZGuiDialogueListFeaturesStyle&) = delete ;
    ZGuiDialogueListFeaturesStyle& operator=(const ZGuiDialogueListFeaturesStyle& ) = delete ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIDIALOGUELISTFEATURESSTYLE_H
