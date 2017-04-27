/*  File: ZGuiView.h
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

#ifndef ZGUIVIEW_H
#define ZGUIVIEW_H

#include "ZGViewOrientationType.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"

#include <QApplication>
#include <QFrame>
#include <QLayout>
#include <cstddef>
#include <string>

QT_BEGIN_NAMESPACE
class QLabel;
class QSlider;
class QToolButton;
QT_END_NAMESPACE


namespace ZMap
{

namespace GUI
{

class ZGuiGraphicsView ;

// This class contains a view on a graphics scene and a few controls.
class ZGuiView : public QFrame,
        public Util::InstanceCounter<ZGuiView>,
        public Util::ClassName<ZGuiView>
{
    Q_OBJECT

public:

    static ZGuiView *newInstance(const QString& data,
                                 QWidget* parent=Q_NULLPTR) ;

    ~ZGuiView() ;

    ZGuiGraphicsView * getGraphicsView() const {return m_graphics_view ; }

    void setCurrent(bool) ;
    bool isCurrent() const {return m_is_current ; }
    void setOrientation(ZGViewOrientationType type) ;

signals:
    void signal_split(const QString &data) ;
    void signal_unsplit() ;
    void signal_itemTakenFocus() ;
    //void signal_itemLostFocus() ;
    void signal_madeCurrent(ZGuiView*) ;

public slots:
    void slot_itemTakenFocus() ;
    //void slot_itemLostFocus() ;
//    void zoomOutH(int level = 1);
//    void zoomInH(int level = 1);

private slots:
    void computeMatrix();

protected:
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE ;
    void keyPressEvent(QKeyEvent *event)  Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE ;

private:

    ZGuiView(const QString & data, QWidget *parent = Q_NULLPTR) ;
    ZGuiView(const ZGuiView& getGraphicsView) = delete ;
    ZGuiView& operator=(const ZGuiView& getGraphicsView) = delete ;

    static const QString m_name_base ;

    void setFrameDrawing() ;
    void setFrameDrawingFocus() ;
    void setFrameDrawingNormal() ;
    void setBordersStyle() ;

    ZGuiGraphicsView *m_graphics_view ;

    ZGViewOrientationType m_view_orientation ;
    bool m_is_current ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGUIVIEW_H
