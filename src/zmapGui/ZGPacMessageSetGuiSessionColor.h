#ifndef ZGPACMESSAGESETGUISESSIONCOLOR_H
#define ZGPACMESSAGESETGUISESSIONCOLOR_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "ZGColor.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{


class ZGPacMessageSetGuiSessionColor : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageSetGuiSessionColor>,
        public Util::ClassName<ZGPacMessageSetGuiSessionColor>
{
public:

    ZGPacMessageSetGuiSessionColor();
    ZGPacMessageSetGuiSessionColor(ZInternalID message_id,
                                   ZInternalID gui_id,
                                   const ZGColor& color) ;
    ZGPacMessageSetGuiSessionColor(const ZGPacMessageSetGuiSessionColor& message) ;
    ZGPacMessageSetGuiSessionColor& operator=(const ZGPacMessageSetGuiSessionColor& message) ;
    ~ZGPacMessageSetGuiSessionColor() ;
    ZGPacMessage* clone() const override ;

    ZInternalID getGuiID() const {return m_gui_id ; }
    ZGColor getColor() const {return m_color ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_gui_id ;
    ZGColor m_color ;

};

} // namespace GUI
} // namespace ZMap

#endif // ZGPACMESSAGESETGUISESSIONCOLOR_H
