#ifndef ZGMESSAGESETGUIMENUBARCOLOR_H
#define ZGMESSAGESETGUIMENUBARCOLOR_H

#include "ZGMessage.h"
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


class ZGMessageSetGuiMenuBarColor : public ZGMessage,
        public Util::InstanceCounter<ZGMessageSetGuiMenuBarColor>,
        public Util::ClassName<ZGMessageSetGuiMenuBarColor>
{
public:


    ZGMessageSetGuiMenuBarColor();
    ZGMessageSetGuiMenuBarColor(ZInternalID message_id,
                                ZInternalID gui_id,
                                const ZGColor & color) ;
    ZGMessageSetGuiMenuBarColor(const ZGMessageSetGuiMenuBarColor& message) ;
    ZGMessageSetGuiMenuBarColor& operator=(const ZGMessageSetGuiMenuBarColor& message) ;
    ~ZGMessageSetGuiMenuBarColor() ;
    ZGMessage* clone() const override ;

    ZInternalID getGuiID() const {return m_gui_id ; }
    ZGColor getColor() const {return m_color ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_gui_id ;
    ZGColor m_color ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGESETGUIMENUBARCOLOR_H
