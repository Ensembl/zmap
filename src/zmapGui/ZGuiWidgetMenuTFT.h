#ifndef ZGUIWIDGETMENUTFT_H
#define ZGUIWIDGETMENUTFT_H

#include <cstddef>
#include <string>

#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGTFTActionType.h"

#include <QApplication>
#include <QString>
#include <QMenu>

QT_BEGIN_NAMESPACE
class QAction ;
QT_END_NAMESPACE

//
// Context menu for three frame translation actions to be
// used within the control panel.
//

namespace ZMap
{

namespace GUI
{

class ZGuiWidgetMenuTFT : public QMenu,
        public Util::InstanceCounter<ZGuiWidgetMenuTFT>,
        public Util::ClassName<ZGuiWidgetMenuTFT>
{

    Q_OBJECT

public:

    static ZGuiWidgetMenuTFT * newInstance(QWidget* parent = Q_NULLPTR) ;

    ZGTFTActionType getTFTActionType() const {return m_action_type ; }
    ~ZGuiWidgetMenuTFT() ;

public slots:
    void slot_action_triggered() ;

private:

    static const QString m_menu_title,
        m_label_none,
        m_label_features,
        m_label_tft,
        m_label_featurestft ;
    static const int m_action_id ;

    ZGuiWidgetMenuTFT(QWidget* parent = Q_NULLPTR ) ;
    ZGuiWidgetMenuTFT(const ZGuiWidgetMenuTFT& ) = delete ;
    ZGuiWidgetMenuTFT& operator=(const ZGuiWidgetMenuTFT&) = delete ;

    QAction * m_action_title,
        *m_action_none,
        *m_action_features,
        *m_action_tft,
        *m_action_featurestft ;

    ZGTFTActionType m_action_type ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIWIDGETMENUTFT_H
