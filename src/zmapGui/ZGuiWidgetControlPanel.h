#ifndef ZGUIWIDGETCONTROLPANEL_H
#define ZGUIWIDGETCONTROLPANEL_H

#include <cstddef>
#include <string>
#include <utility>
#include <QApplication>
#include <QWidget>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QHBoxLayout ;
class QFrame ;
class QPushButton ;
class QToolButton ;
class QSpinBox ;
QT_END_NAMESPACE

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGTFTActionType.h"
#include "ZGZoomActionType.h"

namespace ZMap
{

namespace GUI
{

class ZGuiWidgetFramedLabel ;
class ZGuiWidgetButtonMouseSensitive ;
class ZGuiWidgetButtonContextMenu ;
class ZGuiWidgetToolButtonContextMenu ;
class ZGuiGraphicsView ;

class ZGuiWidgetControlPanel : public QWidget,
        public Util::InstanceCounter<ZGuiWidgetControlPanel>,
        public Util::ClassName<ZGuiWidgetControlPanel>
{

    Q_OBJECT

public:
    static ZGuiWidgetControlPanel* newInstance(QWidget* parent = Q_NULLPTR) ;

    ~ZGuiWidgetControlPanel() ;

    void setCurrentView(ZGuiGraphicsView * view) ;
    ZGuiGraphicsView* getCurrentView() const {return m_current_view ; }

    void setFilterRange(const std::pair<int, int> & data) ;
    void setFilterValue(int value) ;
    void setFilterStep(int value) ;
    int getFilterValue() const ;

    void setStrandLabel(const QString& str) ;
    QString getStrandLabel() const ;

    void setCoordsLabel(const QString& str) ;
    QString getCoordsLabel() const ;

    void setStatusLabel(const QString& str) ;
    QString getStatusLabel() const ;
    void setStatusLabelTooltip(const QString & label) ;

    void disconnectCurrentView() ;
    void connectCurrentView() ;

signals:
    void signal_stop() ;
    void signal_reload() ;
    void signal_back() ;
    void signal_hsplit() ;
    void signal_vsplit() ;
    void signal_unsplit() ;
    void signal_unlock() ;
    void signal_revcomp() ;
    void signal_3frame(ZGTFTActionType type) ;
    void signal_dna() ;
    void signal_columns() ;
    void signal_zoomin() ;
    void signal_zoomout() ;
    void signal_zoomaction(ZGZoomActionType type) ;
    void signal_zoom_level(const qreal&) ;
    void signal_filter(int) ; // for filter value changed

public slots:
    void slot_zoom_changed(const qreal& value) ;
    //void slot_aa_changed(bool) ;

private slots:
    void slot_stop() ;
    void slot_reload() ;
    void slot_back() ;
    void slot_hsplit() ;
    void slot_vsplit() ;
    void slot_unsplit() ;
    void slot_unlock() ;
    void slot_revcomp() ;
    void slot_3frame() ;
    void slot_3frame_right() ;
    void slot_dna() ;
    void slot_columns() ;
    void slot_zoomin() ;
    void slot_zoomout() ;
    void slot_zoom_right() ;
    void slot_filter(int) ;
    void slot_print() ;
    void slot_aa(bool) ;

private:

    ZGuiWidgetControlPanel(QWidget *parent = Q_NULLPTR) ;
    ZGuiWidgetControlPanel(const ZGuiWidgetControlPanel&) = delete ;
    ZGuiWidgetControlPanel& operator=(const ZGuiWidgetControlPanel& ) = delete ;

    void setTooltips() ;
    void makeConnections() ;
    void setTooltipAA() ;

    static const std::pair<int, int> m_filter_range_default ;
    static const int m_filter_single_step_default ;
    static const QString m_label_strand_default,
        m_label_coords_default,
        m_label_status_default,
        m_tooltip_stop,
        m_tooltip_reload,
        m_tooltip_back,
        m_tooltip_hsplit,
        m_tooltip_vsplit,
        m_tooltip_unsplit,
        m_tooltip_unlock,
        m_tooltip_revcomp,
        m_tooltip_3frame,
        m_tooltip_dna,
        m_tooltip_columns,
        m_tooltip_zoomin,
        m_tooltip_zoomout,
        m_tooltip_filter_score,
        m_tooltip_strand,
        m_tooltip_coords,
        m_tooltip_status,
        m_tooltip_zoomlevel,
        m_tooltip_print,
        m_tooltip_aa ;

    QHBoxLayout *m_top_layout ;

    // convert these to tool button...
    QToolButton *m_button_stop,
        *m_button_reload,
        *m_button_back,
        *m_button_hsplit,
        *m_button_vsplit,
        *m_button_unsplit,
        *m_button_unlock,
        *m_button_revcomp,
        *m_button_dna,
        *m_button_columns,
        *m_button_print,
        *m_button_aa  ;
    ZGuiWidgetToolButtonContextMenu *m_button_3frame,
        *m_button_zoomin,
        *m_button_zoomout ;

    QSpinBox *m_filter_score ;

    ZGuiWidgetFramedLabel *m_label_zoom,
        *m_label_strand,
        *m_label_coords,
        *m_label_status ;

    ZGuiGraphicsView *m_current_view ;

};

} // namespace GUI

} // nameapce ZMap

#endif // ZGUIWIDGETCONTROLPANEL_H
