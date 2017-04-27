#include "ZGuiWidgetControlPanel.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiWidgetFramedLabel.h"
#include "ZGuiWidgetButtonMouseSensitive.h"
#include "ZGuiWidgetButtonContextMenu.h"
#include "ZGuiWidgetToolButtonContextMenu.h"
#include "ZGuiWidgetMenuTFT.h"
#include "ZGuiWidgetMenuZoom.h"
#include "ZGuiGraphicsView.h"
#include <stdexcept>
#include <memory>
#include <new>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QFrame>
#include <QSpinBox>
#include <QLabel>
#ifndef QT_NO_DEBUG
#include <sstream>
#include <QDebug>
#endif
#ifndef QT_NO_PRINTER
#include <QPrinter>
#include <QPrintDialog>
#endif

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetControlPanel>::m_name ("ZGuiWidgetControlPanel" ) ;
const std::pair<int,int> ZGuiWidgetControlPanel::m_filter_range_default(0, 100) ;
const int ZGuiWidgetControlPanel::m_filter_single_step_default(5) ;
const QString ZGuiWidgetControlPanel::m_label_strand_default("+"),
    ZGuiWidgetControlPanel::m_label_coords_default("0 0"),
    ZGuiWidgetControlPanel::m_label_status_default("ZMap status..."),
    ZGuiWidgetControlPanel::m_tooltip_stop("Stop loading of data, reset ZMap"),
    ZGuiWidgetControlPanel::m_tooltip_reload("Reload data after a reset"),
    ZGuiWidgetControlPanel::m_tooltip_back("Step back to clear effect of previous action"),
    ZGuiWidgetControlPanel::m_tooltip_hsplit("Split view horizontally"),
    ZGuiWidgetControlPanel::m_tooltip_vsplit("Split view vertically"),
    ZGuiWidgetControlPanel::m_tooltip_unsplit("Unsplit selected view"),
    ZGuiWidgetControlPanel::m_tooltip_unlock("Unlock zoom/scroll from sibling view"),
    ZGuiWidgetControlPanel::m_tooltip_revcomp("Reverse complement sequence view"),
    ZGuiWidgetControlPanel::m_tooltip_3frame("Toggle display of reading frame columns (right-click for more options)"),
    ZGuiWidgetControlPanel::m_tooltip_dna("Toggle display of DNA"),
    ZGuiWidgetControlPanel::m_tooltip_columns("Column configuration"),
    ZGuiWidgetControlPanel::m_tooltip_zoomin("Zoom in 2x (right click for more options)"),
    ZGuiWidgetControlPanel::m_tooltip_zoomout("Zoom out 2x (right click for more options)"),
    ZGuiWidgetControlPanel::m_tooltip_filter_score("Filter selected column by score"),
    ZGuiWidgetControlPanel::m_tooltip_strand("\"+\" forward, \"-\" reverse complement"),
    ZGuiWidgetControlPanel::m_tooltip_coords("start/end coordinates of displayed sequence "),
    ZGuiWidgetControlPanel::m_tooltip_status("Status of selected view"),
    ZGuiWidgetControlPanel::m_tooltip_print("Print the current view "),
    ZGuiWidgetControlPanel::m_tooltip_zoomlevel("Zoom level of current view (bp per pixel)"),
    ZGuiWidgetControlPanel::m_tooltip_aa("Anti-aliased drawing for current view ");

ZGuiWidgetControlPanel* ZGuiWidgetControlPanel::newInstance(QWidget* parent)
{
    ZGuiWidgetControlPanel* thing = Q_NULLPTR ;
    try
    {
        thing = new (std::nothrow) ZGuiWidgetControlPanel(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

ZGuiWidgetControlPanel::ZGuiWidgetControlPanel(QWidget *parent)
    : QWidget(parent),
      Util::InstanceCounter<ZGuiWidgetControlPanel>(),
      Util::ClassName<ZGuiWidgetControlPanel>(),
      m_button_stop(Q_NULLPTR),
      m_button_reload(Q_NULLPTR),
      m_button_back(Q_NULLPTR),
      m_button_hsplit(Q_NULLPTR),
      m_button_vsplit(Q_NULLPTR),
      m_button_unsplit(Q_NULLPTR),
      m_button_revcomp(Q_NULLPTR),
      m_button_dna(Q_NULLPTR),
      m_button_columns(Q_NULLPTR),
      m_button_print(Q_NULLPTR),
      m_button_aa(Q_NULLPTR),
      m_button_3frame(Q_NULLPTR),
      m_button_zoomin(Q_NULLPTR),
      m_button_zoomout(Q_NULLPTR),
      m_filter_score(Q_NULLPTR),
      m_label_zoom(Q_NULLPTR),
      m_label_strand(Q_NULLPTR),
      m_label_coords(Q_NULLPTR),
      m_label_status(Q_NULLPTR),
      m_current_view(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create instance ")) ;

    if (!(m_top_layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create top layout ")) ;
    setLayout(m_top_layout) ;
    m_top_layout->setMargin(0) ;

    //
    // First group of buttons
    //
    if (!(m_button_stop = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_stop ")) ;
    m_button_stop->setText(QString("Stop")) ;
    m_button_stop->setEnabled(false) ;
    m_top_layout->addWidget(m_button_stop) ;

    if (!(m_button_reload = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_reload")) ;
    m_button_reload->setEnabled(false) ;
    m_button_reload->setText(QString("Reload")) ;
    m_top_layout->addWidget(m_button_reload) ;

    if (!(m_button_back = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_back")) ;
    m_button_back->setText(QString("Back")) ;
    m_top_layout->addWidget(m_button_back) ;

    QFrame *frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::VLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    //
    // Second group of buttons
    //
    if (!(m_button_hsplit = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_hsplit")) ;
    m_button_hsplit->setText(QString("H-Split")) ;
    m_top_layout->addWidget(m_button_hsplit) ;

    if (!(m_button_vsplit = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_vsplit")) ;
    m_button_vsplit->setText(QString("V-Split")) ;
    m_top_layout->addWidget(m_button_vsplit) ;

    if (!(m_button_unsplit = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_unsplit")) ;
    m_button_unsplit->setText(QString("Unsplit")) ;
    m_top_layout->addWidget(m_button_unsplit) ;
    //m_button_unsplit->setEnabled(false) ;

    if (!(m_button_unlock = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_unlock ")) ;
    m_button_unlock->setText(QString("Unlock")) ;
    //m_button_unlock->setEnabled(false) ;

    frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::VLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    //
    // Third group of buttons
    //
    if (!(m_button_revcomp = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_revcomp")) ;
    m_button_revcomp->setText(QString("Revcomp")) ;
    m_top_layout->addWidget(m_button_revcomp) ;

    if (!(m_button_3frame = ZGuiWidgetToolButtonContextMenu::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_3frame")) ;
    m_button_3frame->setText(QString("3 Frame")) ;
    m_top_layout->addWidget(m_button_3frame) ;

    if (!(m_button_dna = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_dna")) ;
    m_button_dna->setText(QString("DNA")) ;
    m_top_layout->addWidget(m_button_dna) ;

    if (!(m_button_columns = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_columns")) ;
    m_button_columns->setText(QString("Columns")) ;
    m_top_layout->addWidget(m_button_columns) ;

    frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::VLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    //
    // Wow, another group of buttons... and other stuff
    //
    if (!(m_button_zoomin = ZGuiWidgetToolButtonContextMenu::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_zoomin ")) ;
    m_button_zoomin->setIcon(QPixmap(":/zoomin.png"));
    m_top_layout->addWidget(m_button_zoomin) ;

    if (!(m_button_zoomout = ZGuiWidgetToolButtonContextMenu::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_zoomout ")) ;
    m_button_zoomout->setIcon(QPixmap(":/zoomout.png")) ;
    m_top_layout->addWidget(m_button_zoomout) ;

    if (!(m_label_zoom = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create zoom label")) ;
    m_top_layout->addWidget(m_label_zoom) ;
    m_label_zoom->setText(QString("0.0")) ;

    if (!(m_button_print = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create button_print ")) ;
    m_button_print->setIcon(QPixmap(":/fileprint.png")) ;
    m_button_print->setToolTip(m_tooltip_print) ;
#ifdef QT_NO_PRINTER
    m_button_print->setEnabled(false) ;
#endif
    m_top_layout->addWidget(m_button_print) ;

    if (!(m_button_aa = ZGuiWidgetCreator::createToolButton()))
        throw std::runtime_error(std::string("")) ;
    m_button_aa->setText(QString("A")) ;
    m_button_aa->setToolTip(m_tooltip_aa) ;
    m_button_aa->setCheckable(true) ;
    m_button_aa->setChecked(false) ;
    m_top_layout->addWidget(m_button_aa) ;

    frame = Q_NULLPTR ;
    if (!(frame = ZGuiWidgetCreator::createFrame()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create frame ")) ;
    frame->setFrameStyle(QFrame::VLine | QFrame::Plain) ;
    m_top_layout->addWidget(frame) ;

    // praise to our lady of widgets, another one
    QLabel* label = Q_NULLPTR ;
    if (!(label = ZGuiWidgetCreator::createLabel(QString("Filter:"))))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel(); unable to create filter_score label")) ;
    m_top_layout->addWidget(label) ;
    if (!(m_filter_score = ZGuiWidgetCreator::createSpinBox()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create filter_score ")) ;
    m_top_layout->addWidget(m_filter_score) ;
    m_filter_score->setRange(m_filter_range_default.first, m_filter_range_default.second) ;
    m_filter_score->setSingleStep(m_filter_single_step_default) ;

    m_top_layout->addStretch(100) ;

    if (!(m_label_strand = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create label_strand ")) ;
    m_top_layout->addWidget(m_label_strand) ;
    m_label_strand->setText(m_label_strand_default) ;

    if (!(m_label_coords = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create label_coords ")) ;
    m_top_layout->addWidget(m_label_coords) ;
    m_label_coords->setText(m_label_coords_default) ;

    if (!(m_label_status = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetControlPanel::ZGuiWidgetControlPanel() ; unable to create label_status ")) ;
    m_top_layout->addWidget(m_label_status) ;
    m_label_status->setText(m_label_status_default) ;

    // set default tooltips on start up
    setTooltips() ;

    // make connections
    makeConnections() ;
}

ZGuiWidgetControlPanel::~ZGuiWidgetControlPanel()
{
}

void ZGuiWidgetControlPanel::setFilterRange(const std::pair<int, int> &data)
{
    if (m_filter_score && data.first < data.second)
    {
        m_filter_score->setRange(data.first, data.second) ;
    }
}

void ZGuiWidgetControlPanel::setFilterStep(int value)
{
     if (m_filter_score && value > 0 )
     {
         m_filter_score->setSingleStep(value) ;
     }
}

void ZGuiWidgetControlPanel::setFilterValue(int value)
{
    if (m_filter_score)
    {
        m_filter_score->setValue(value) ;
    }
}

int ZGuiWidgetControlPanel::getFilterValue() const
{
    int result = 0 ;
    if (m_filter_score)
    {
        result = m_filter_score->value() ;
    }
    return result ;
}

void ZGuiWidgetControlPanel::setStrandLabel(const QString &str)
{
    if (m_label_strand)
    {
        m_label_strand->setText(str) ;
    }
}

QString ZGuiWidgetControlPanel::getStrandLabel() const
{
    QString str ;
    if (m_label_strand)
    {
        str = m_label_strand->text() ;
    }
    return str ;
}

void ZGuiWidgetControlPanel::setCoordsLabel(const QString &str)
{
    if (m_label_coords)
    {
        m_label_coords->setText(str) ;
    }
}

QString ZGuiWidgetControlPanel::getCoordsLabel() const
{
    QString str ;
    if (m_label_coords)
    {
        str = m_label_coords->text() ;
    }
    return str ;
}

void ZGuiWidgetControlPanel::setStatusLabel(const QString &str)
{
    if (m_label_status)
    {
        m_label_status->setText(str) ;
    }
}

QString ZGuiWidgetControlPanel::getStatusLabel() const
{
    QString str ;
    if (m_label_status)
    {
        str = m_label_status->text() ;
    }
    return str ;
}

void ZGuiWidgetControlPanel::setStatusLabelTooltip(const QString &str)
{
    if (m_label_status)
    {
        m_label_status->setToolTip(str) ;
    }
}

//
// set default tooltips for all contents...
//
void ZGuiWidgetControlPanel::setTooltips()
{
    if (m_button_stop)
        m_button_stop->setToolTip(m_tooltip_stop) ;
    if (m_button_reload)
        m_button_reload->setToolTip(m_tooltip_reload) ;
    if (m_button_back)
        m_button_back->setToolTip(m_tooltip_back) ;
    if (m_button_hsplit)
        m_button_hsplit->setToolTip(m_tooltip_hsplit) ;
    if (m_button_vsplit)
        m_button_vsplit->setToolTip(m_tooltip_vsplit) ;
    if (m_button_unsplit)
        m_button_unsplit->setToolTip(m_tooltip_unsplit) ;
    if (m_button_unlock)
        m_button_unlock->setToolTip(m_tooltip_unlock) ;
    if (m_button_revcomp)
        m_button_revcomp->setToolTip(m_tooltip_revcomp) ;
    if (m_button_3frame)
        m_button_3frame->setToolTip(m_tooltip_3frame) ;
    if (m_button_dna)
        m_button_dna->setToolTip(m_tooltip_dna) ;
    if (m_button_columns)
        m_button_columns->setToolTip(m_tooltip_columns ) ;
    if (m_button_zoomin)
        m_button_zoomin->setToolTip(m_tooltip_zoomin) ;
    if (m_button_zoomout)
        m_button_zoomout->setToolTip(m_tooltip_zoomout);
    if (m_filter_score)
        m_filter_score->setToolTip(m_tooltip_filter_score) ;
    if (m_label_strand)
        m_label_strand->setToolTip(m_tooltip_strand) ;
    if (m_label_coords)
        m_label_coords->setToolTip(m_tooltip_coords) ;
    if (m_label_status)
        m_label_status->setToolTip(m_tooltip_status );
    if (m_label_zoom)
        m_label_zoom->setToolTip(m_tooltip_zoomlevel) ;
    if (m_button_print)
        m_button_print->setToolTip(m_tooltip_print) ;
}

void ZGuiWidgetControlPanel::makeConnections()
{
    if (m_button_stop)
        connect(m_button_stop, SIGNAL(clicked(bool)), this, SLOT(slot_stop())) ;

    if (m_button_reload)
        connect(m_button_reload, SIGNAL(clicked(bool)), this, SLOT(slot_reload())) ;

    if (m_button_back)
        connect(m_button_back, SIGNAL(clicked(bool)), this, SLOT(slot_back())) ;

    if (m_button_hsplit)
        connect(m_button_hsplit, SIGNAL(clicked(bool)), this, SLOT(slot_hsplit())) ;

    if (m_button_vsplit)
        connect( m_button_vsplit, SIGNAL(clicked(bool)), this, SLOT(slot_vsplit())) ;

    if (m_button_unsplit)
        connect(m_button_unsplit , SIGNAL(clicked(bool)), this, SLOT(slot_unsplit())) ;

    if (m_button_unlock)
        connect(m_button_unlock , SIGNAL(clicked(bool)), this, SLOT(slot_unlock())) ;

    if (m_button_revcomp)
        connect(m_button_revcomp , SIGNAL(clicked(bool)), this, SLOT(slot_revcomp())) ;

    if (m_button_3frame)
    {
        connect(m_button_3frame, SIGNAL(clicked(bool)), this, SLOT(slot_3frame())) ;
        connect(m_button_3frame, SIGNAL(signal_contextMenuEvent()), this, SLOT(slot_3frame_right())) ;
    }

    if (m_button_dna)
        connect(m_button_dna , SIGNAL(clicked(bool)), this, SLOT(slot_dna())) ;

    if (m_button_columns)
        connect(m_button_columns , SIGNAL(clicked(bool)), this, SLOT(slot_columns())) ;

    if (m_button_zoomin)
    {
        connect(m_button_zoomin , SIGNAL(clicked(bool)), this, SLOT(slot_zoomin())) ;
        connect(m_button_zoomin, SIGNAL(signal_contextMenuEvent()), this, SLOT(slot_zoom_right())) ;
    }

    if (m_button_zoomout)
    {
        connect(m_button_zoomout, SIGNAL(clicked(bool)), this, SLOT(slot_zoomout())) ;
        connect(m_button_zoomout, SIGNAL(signal_contextMenuEvent()), this, SLOT(slot_zoom_right())) ;
    }

    if (m_button_print)
        connect(m_button_print, SIGNAL(clicked(bool)), this, SLOT(slot_print())) ;

    if (m_button_aa)
        connect(m_button_aa, SIGNAL(clicked(bool)), this, SLOT(slot_aa(bool))) ;

    if (m_filter_score)
        connect(m_filter_score , SIGNAL(valueChanged(int)), this, SLOT(slot_filter(int))) ;

}

void ZGuiWidgetControlPanel::setCurrentView(ZGuiGraphicsView* view)
{
    if (view != m_current_view)
    {
        m_current_view = view ;
    }
}

void ZGuiWidgetControlPanel::disconnectCurrentView()
{
    if (m_current_view)
    {
        disconnect(this, 0, m_current_view, 0) ;
    }
}

void ZGuiWidgetControlPanel::connectCurrentView()
{
    if (m_current_view)
    {
        connect(this, SIGNAL(signal_hsplit()), m_current_view, SLOT(slot_hsplit())) ;
        connect(this, SIGNAL(signal_vsplit()), m_current_view, SLOT(slot_vsplit())) ;
        connect(this, SIGNAL(signal_unsplit()), m_current_view, SLOT(slot_unsplit())) ;
        connect(this, SIGNAL(signal_zoomin()), m_current_view, SLOT(slot_zoomin_x2())) ;
        connect(this, SIGNAL(signal_zoomout()), m_current_view, SLOT(slot_zoomout_x2())) ;
        connect(this, SIGNAL(signal_zoom_level(qreal)), m_current_view, SLOT(slot_zoom_level(qreal))) ;
        connect(m_current_view, SIGNAL(signal_zoom_changed(qreal)), this, SLOT(slot_zoom_changed(qreal))) ;
        slot_zoom_changed(m_current_view->getZoom()) ;
        bool aa = m_current_view->getAntialiasing() ;
        m_button_aa->setChecked(aa) ;
        setTooltipAA() ;
    }
    else
    {
        // here we do things that reset the controls/labels when
        // no view is set
        m_label_zoom->setText(QString("0.0")) ;
        m_button_aa->setChecked(false) ;
        setTooltipAA() ;
    }
}


////////////////////////////////////////////////////////////////////////////////
/// Slots
////////////////////////////////////////////////////////////////////////////////

void ZGuiWidgetControlPanel::slot_stop()
{
    emit signal_stop() ;
}

void ZGuiWidgetControlPanel::slot_reload()
{
    emit signal_reload() ;
}

void ZGuiWidgetControlPanel::slot_back()
{
    emit signal_back() ;
}

void ZGuiWidgetControlPanel::slot_hsplit()
{
    emit signal_hsplit() ;
}

void ZGuiWidgetControlPanel::slot_vsplit()
{
    emit signal_vsplit() ;
}

void ZGuiWidgetControlPanel::slot_unsplit()
{
    emit signal_unsplit() ;
}

void ZGuiWidgetControlPanel::slot_unlock()
{
    emit signal_unlock() ;
}

void ZGuiWidgetControlPanel::slot_revcomp()
{
    emit signal_revcomp() ;
}

void ZGuiWidgetControlPanel::slot_3frame()
{
    emit signal_3frame(ZGTFTActionType::Features) ;
}

void ZGuiWidgetControlPanel::slot_dna()
{
    emit signal_dna() ;
}

void ZGuiWidgetControlPanel::slot_columns()
{
    emit signal_columns() ;
}

void ZGuiWidgetControlPanel::slot_zoomin()
{
    emit signal_zoomin() ;
}

void ZGuiWidgetControlPanel::slot_zoomout()
{
    emit signal_zoomout() ;
}

void ZGuiWidgetControlPanel::slot_filter(int value)
{
    emit signal_filter(value) ;
}

//
// This is the context menu for the TFT options
//
void ZGuiWidgetControlPanel::slot_3frame_right()
{
    std::unique_ptr<ZGuiWidgetMenuTFT> menu(ZGuiWidgetMenuTFT::newInstance(this)) ;
    QAction* action = menu->exec(QCursor::pos()) ;
    if (action)
    {
        ZGTFTActionType type = menu->getTFTActionType() ;
        emit signal_3frame(type) ;
#ifndef QT_NO_DEBUG
        std::stringstream str ;
        str << type ;
        qDebug() << "ZGuiWidgetControlPanel::slot_3frame_right() ; "
                 << action << QString::fromStdString(str.str()) ;
#endif
    }
}

//
// zoomin context menu
//
void ZGuiWidgetControlPanel::slot_zoom_right()
{
    std::unique_ptr<ZGuiWidgetMenuZoom> menu(ZGuiWidgetMenuZoom::newInstance(this)) ;
    QAction * action = menu->exec(QCursor::pos()) ;
    if (action)
    {
        ZGZoomActionType type = menu->getZoomActionType() ;
        emit signal_zoomaction(type) ;
#ifndef QT_NO_DEBUG
        std::stringstream str ;
        str << type ;
        qDebug() << "ZGuiWidgetControlPanel::slot_zoom_right() ; "
                 << action << QString::fromStdString(str.str()) ;
#endif
    }
}

void ZGuiWidgetControlPanel::slot_zoom_changed(const qreal &value)
{
    QString str ;
    if (m_label_zoom)
    {
        m_label_zoom->setText(str.setNum(value)) ;
    }
}

// print the current view
void ZGuiWidgetControlPanel::slot_print()
{
    if (!m_current_view)
        return ;
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrinter printer ;
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted)
     {
        QPainter painter(&printer);
        m_current_view->render(&painter);
    }
#endif
}

void ZGuiWidgetControlPanel::slot_aa(bool value)
{
    if (m_current_view)
    {
        m_current_view->setAntialiasing(value) ;
        setTooltipAA() ;
    }
}

void ZGuiWidgetControlPanel::setTooltipAA()
{
    bool aa = false ;
    if (m_current_view)
    {
        aa = m_current_view->getAntialiasing() ;
    }
    QString str(aa ? " (on)" : " (off)") ;
    m_button_aa->setToolTip(m_tooltip_aa + str) ;
}

} // namespace GUI

} // namespace ZMap

