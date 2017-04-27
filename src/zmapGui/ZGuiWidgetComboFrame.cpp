#include "ZGuiWidgetComboFrame.h"
#include <stdexcept>
#include <sstream>

Q_DECLARE_METATYPE(ZMap::GUI::ZGFrameType)

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetComboFrame>::m_name("ZGuiWidgetComboFrame") ;
const ZGuiWidgetComboFrameRegister ZGuiWidgetComboFrame::m_frame_register = {ZGFrameType::Star,
                                                                             ZGFrameType::Dot,
                                                                             ZGFrameType::Zero,
                                                                     ZGFrameType::One,
                                                                     ZGFrameType::Two} ;
const int ZGuiWidgetComboFrame::m_frame_id(qMetaTypeId<ZGFrameType>()) ;

ZGuiWidgetComboFrame::ZGuiWidgetComboFrame(QWidget* parent)
    : QComboBox(parent),
      Util::InstanceCounter<ZGuiWidgetComboFrame>(),
      Util::ClassName<ZGuiWidgetComboFrame>(),
      m_frame_map()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetComboFrame::ZGuiWidgetComboFrame() ; can not instantiate without QApplication existing ")) ;

    QVariant var ;
    ZGFrameType frame ;
    std::stringstream str ;
    int index = 0 ;
    for (auto it = m_frame_register.begin() ; it != m_frame_register.end() ; ++it, str.str(""), ++index)
    {
        frame = *it ;
        var.setValue(frame) ;
        str << frame ;
        insertItem(index, QString::fromStdString(str.str()), var) ;
        m_frame_map.insert(std::pair<ZGFrameType,int>(frame, index)) ;
    }
}

ZGuiWidgetComboFrame::~ZGuiWidgetComboFrame()
{
}

ZGFrameType ZGuiWidgetComboFrame::getFrame() const
{
    ZGFrameType frame = ZGFrameType::Invalid ;
    QVariant var = currentData() ;
    if (var.canConvert<ZGFrameType>() && var.convert(m_frame_id))
        frame = var.value<ZGFrameType>() ;
    return frame ;
}

bool ZGuiWidgetComboFrame::setFrame(ZGFrameType frame)
{
    bool result = false ;
    auto it = m_frame_map.find(frame) ;
    if (it != m_frame_map.end())
    {
        setCurrentIndex(it->second) ;
        result = true ;
    }
    return result ;
}

ZGuiWidgetComboFrame* ZGuiWidgetComboFrame::newInstance(QWidget *parent)
{
    ZGuiWidgetComboFrame * thing = Q_NULLPTR ;

    try
    {
        thing = new ZGuiWidgetComboFrame(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

} // namespace GUI

} // namespace ZMap

