#ifndef ZGPACMESSAGESETGUIMENUBARCOLOR_H
#define ZGPACMESSAGESETGUIMENUBARCOLOR_H

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

class ZGPacMessageSetGuiMenuBarColor : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageSetGuiMenuBarColor>,
        public Util::ClassName<ZGPacMessageSetGuiMenuBarColor>
{
public:

    ZGPacMessageSetGuiMenuBarColor();
    ZGPacMessageSetGuiMenuBarColor(ZInternalID message_id,
                                   ZInternalID gui_id ,
                                   const ZGColor& color) ;
    ZGPacMessageSetGuiMenuBarColor(const ZGPacMessageSetGuiMenuBarColor& message) ;
    ZGPacMessageSetGuiMenuBarColor& operator=(const ZGPacMessageSetGuiMenuBarColor& message) ;
    ~ZGPacMessageSetGuiMenuBarColor() ;
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

#endif // ZGPACMESSAGESETGUIMENUBARCOLOR_H
