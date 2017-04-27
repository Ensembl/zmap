#ifndef ZGUIWIDGETMENUZOOM_H
#define ZGUIWIDGETMENUZOOM_H

#include <cstddef>
#include <string>

#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGZoomActionType.h"

#include <QApplication>
#include <QString>
#include <QMenu>

QT_BEGIN_NAMESPACE
class QAction ;
QT_END_NAMESPACE

//
// Context menu for zoom actions; used within control panel.
//

namespace ZMap
{

namespace GUI
{

class ZGuiWidgetMenuZoom : public QMenu,
        public Util::InstanceCounter<ZGuiWidgetMenuZoom>,
        public Util::ClassName<ZGuiWidgetMenuZoom>
{

    Q_OBJECT

public:
    static ZGuiWidgetMenuZoom * newInstance(QWidget* parent = Q_NULLPTR ) ;

    ZGZoomActionType getZoomActionType() const {return m_action_type ; }
    ~ZGuiWidgetMenuZoom() ;

public slots:
    void slot_action_triggered() ;

private:
    static const QString m_menu_title,
        m_label_max,
        m_label_alldna,
        m_label_min ;
    static const int m_action_id ;

    ZGuiWidgetMenuZoom(QWidget* parent = Q_NULLPTR) ;
    ZGuiWidgetMenuZoom(const ZGuiWidgetMenuZoom& ) = delete ;
    ZGuiWidgetMenuZoom& operator=(const ZGuiWidgetMenuZoom& )  = delete ;

    QAction * m_action_title,
        *m_action_max,
        *m_action_alldna,
        *m_action_min ;

    ZGZoomActionType m_action_type ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIWIDGETMENUZOOM_H
