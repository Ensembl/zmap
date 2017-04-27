#ifndef ZGUIWIDGETCOMBOFRAME_H
#define ZGUIWIDGETCOMBOFRAME_H

#include <QApplication>
#include <QComboBox>
#include <QString>
#include <cstddef>
#include <string>
#include <vector>
#include <map>

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include "ZGFrameType.h"

namespace ZMap
{

namespace GUI
{

typedef std::vector<ZGFrameType> ZGuiWidgetComboFrameRegister ;
typedef std::map<ZGFrameType,int,ZGFrameCompare> ZGuiWidgetComboFrameMap ;

class ZGuiWidgetComboFrame : public QComboBox,
        public Util::InstanceCounter<ZGuiWidgetComboFrame>,
        public Util::ClassName<ZGuiWidgetComboFrame>
{

    Q_OBJECT

public:
    static ZGuiWidgetComboFrame* newInstance(QWidget* parent = Q_NULLPTR ) ;
    static const int m_frame_id ;

    ~ZGuiWidgetComboFrame() ;

    bool setFrame(ZGFrameType frame) ;
    ZGFrameType getFrame() const ;

private:
    static const ZGuiWidgetComboFrameRegister m_frame_register ;

    ZGuiWidgetComboFrame(QWidget* parent = Q_NULLPTR );
    ZGuiWidgetComboFrame(const ZGuiWidgetComboFrame& ) = delete ;
    ZGuiWidgetComboFrame& operator=(const ZGuiWidgetComboFrame&) = delete ;

    ZGuiWidgetComboFrameMap m_frame_map ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIWIDGETCOMBOFRAME_H
