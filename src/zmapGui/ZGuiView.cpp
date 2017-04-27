/*  File: ZGuiView.cpp
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

#include "ZGuiView.h"
#include "ZGuiGraphicsView.h"
#include "ZGuiWidgetCreator.h"
#include <stdexcept>
#include <sstream>
#include <QtWidgets>
#include <QPainter>

#ifndef QT_NO_PRINTER
#include <QPrinter>
#include <QPrintDialog>
#endif
#ifndef QT_NO_OPENGL
#include <QtOpenGL>
#endif


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiView>::m_name ("ZGuiView") ;
const QString ZGuiView::m_name_base("ZGuiView_") ;

ZGuiView::ZGuiView(const QString &name, QWidget *parent)
    : QFrame(parent),
      Util::InstanceCounter<ZGuiView>(),
      Util::ClassName<ZGuiView>(),
      m_graphics_view(Q_NULLPTR),
      m_view_orientation(ZGViewOrientationType::Right),
      m_is_current(false)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiView::ZGuiView() ; may not create without instance of QApplication running ")) ;

    //setFrameDrawingNormal() ;

    if (!(m_graphics_view = ZGuiGraphicsView::newInstance()))
        throw std::runtime_error(std::string("ZGuiView::ZGuiView() ; could not create instance of ZGuiGraphicsView ")) ;
    connect(m_graphics_view, SIGNAL(signal_requestMadeCurrent()), this, SLOT(slot_itemTakenFocus())) ;

    // orientation button layout
    QHBoxLayout *orientationLayout = Q_NULLPTR ;
    if (!(orientationLayout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiView::ZGuiView() ; unable to create orientation layout ")) ;

    orientationLayout->addWidget(m_graphics_view ) ;
    setLayout(orientationLayout) ;

//    QGridLayout *topLayout = Q_NULLPTR ;
//    if (!(topLayout = ZGuiWidgetCreator::createGridLayout()))
//        throw std::runtime_error(std::string("ZGuiView::ZGuiView() ; unable to create grid layout ")) ;
//    topLayout->setMargin(0) ;
//    topLayout->addWidget(m_graphics_view, 1, 0);
//    setLayout(topLayout);


    computeMatrix();

    //setFrameDrawing() ;

    //setStyleSheet("border: 2px dark grey;") ;

    //setBordersStyle() ;
}

ZGuiView::~ZGuiView()
{
}

ZGuiView *ZGuiView::newInstance(const QString& data, QWidget* parent)
{
    ZGuiView *view = Q_NULLPTR ;

    try
    {
        view = new ZGuiView(data, parent) ;
    }
    catch (...)
    {
        view = Q_NULLPTR ;
    }

    if (view)
    {
        std::stringstream str ;
        str << view ;
        view->setObjectName(m_name_base + QString::fromStdString(str.str())) ;
    }

    return view ;
}

//void ZGuiView::resetView()
//{
//    m_view_orientation = ZGViewOrientationType::Right ;
//    m_slider_v->setValue(250);
//    m_slider_h->setValue(250);
//    computeMatrix();
//    m_graphics_view->ensureVisible(QRectF(0, 0, 0, 0));
//}

void ZGuiView::resizeEvent(QResizeEvent * event)
{
    computeMatrix() ;
    QFrame::resizeEvent(event) ;
}



//void ZGuiView::setResetButtonEnabled()
//{
//    m_button_reset->setEnabled(true);
//}

void ZGuiView::computeMatrix()
{
    qreal scaleV = 1.0 ; // qPow(qreal(2), (m_slider_v->value() - 250) / qreal(50)) ;
    qreal scaleH = 1.0 ; // qPow(qreal(2), (m_slider_h->value() - 250) / qreal(50)) ;

    QTransform transform = QTransform::fromScale(scaleH, scaleV) ;

    m_graphics_view->setTransform(transform, false) ;
    m_graphics_view->setViewOrientation(m_view_orientation) ;
}

//void ZGuiView::togglePointerMode()
//{
//    m_graphics_view->setDragMode(m_button_select_mode->isChecked()
//                              ? QGraphicsView::RubberBandDrag
//                              : QGraphicsView::ScrollHandDrag);
//    m_graphics_view->setInteractive(m_button_select_mode->isChecked());
//}

//void ZGuiView::toggleOpenGL()
//{
//#ifndef QT_NO_OPENGL
//    m_graphics_view->setViewport(m_button_opengl_mode->isChecked() ? new QGLWidget(QGLFormat(QGL::SampleBuffers)) : new QWidget);
//#endif
//}

//void ZGuiView::toggleAntialiasing()
//{
//    m_graphics_view->setRenderHint(QPainter::Antialiasing, m_button_antialias_mode->isChecked());
//}

//void ZGuiView::print()
//{
//#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
//    QPrinter printer;
//    QPrintDialog dialog(&printer, this);
//    if (dialog.exec() == QDialog::Accepted) {
//        QPainter painter(&printer);
//        m_graphics_view->render(&painter);
//    }
//#endif
//}

//void ZGuiView::zoomInV(int level)
//{
//    m_slider_v->setValue(m_slider_v->value() + level);
//}

//void ZGuiView::zoomOutV(int level)
//{
//    m_slider_v->setValue(m_slider_v->value() - level);
//}

//void ZGuiView::zoomInH(int level)
//{
//    m_slider_h->setValue(m_slider_h->value() + level);
//}

//void ZGuiView::zoomOutH(int level)
//{
//    m_slider_h->setValue(m_slider_h->value() - level);
//}

void ZGuiView::slot_itemTakenFocus()
{
    //setFrameDrawingFocus() ;
    //emit signal_itemTakenFocus() ;
    emit signal_madeCurrent(this) ;
}

//void ZGuiView::slot_itemLostFocus()
//{
//    setFrameDrawingNormal() ;
//    emit signal_itemLostFocus() ;
//}

void ZGuiView::setOrientation(ZGViewOrientationType type)
{
    if (type != ZGViewOrientationType::Invalid)
    {
        m_view_orientation = type ;
        computeMatrix() ;
    }
}

void ZGuiView::mousePressEvent(QMouseEvent *event)
{
    if (event)
    {
        emit signal_madeCurrent(this) ;
    }
}

void ZGuiView::keyPressEvent(QKeyEvent *event)
{
    if (event)
    {
        QString text = event->text().toLower() ;
        if ( (text == QString("h")) || (text == QString("v")) )
        {
            emit signal_split(text) ;
        }
        else if (text == QString("u") )
        {
            emit signal_unsplit() ;
        }
    }
}

void ZGuiView::setCurrent(bool flag)
{
    m_is_current = flag ;

//    if (m_is_current)
//    {
//        setStyleSheet("background-color:black;");
//    }
//    else
//    {
//        setStyleSheet("background-color:white;") ;
//    }
//    QString other("black") ;
//    if (m_is_current)
//    {
//        setStyleSheet(QString("background-color:%1;").arg(other)) ;
//    }
//    else
//    {
//        setStyleSheet("") ;
//    }
//    if (m_is_current)
//    {
//        //setStyleSheet(QString("background-color:%1;").arg(other)) ;
//        setStyleSheet("border: 2px solid black;") ;
//    }
//    else
//    {
//        //setStyleSheet("") ;
//        setStyleSheet("border: 2px;") ;
//    }

    //setBordersStyle() ;

    //
    // both of these, annoyingly are causing a bug/crash
    // on splitting/unsplitting a large number of views...
    // will basically have to experiment to see how to get
    // around it...
    //
    //


    //setFrameDrawing() ;

    if (m_graphics_view)
    {
        m_graphics_view->setCurrent(flag) ;
    }
}

void ZGuiView::setBordersStyle()
{
//    if (m_is_current)
//    {
//        //setStyleSheet(QString("background-color:%1;").arg(other)) ;
//        //QString str = QString("QFrame {border: 2px solid black;}") ;
//        //setStyleSheet(str) ;
//    }
//    else
//    {
//        //setStyleSheet("") ;
//        //QString str = QString("QFrame {}")  ;
//        //setStyleSheet(str) ;
//    }
}


} // namespace GUI

} // namespace ZMap

