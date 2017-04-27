#ifndef ZGMESSAGESETGUISESSIONCOLOR_H
#define ZGMESSAGESETGUISESSIONCOLOR_H

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

class ZGMessageSetGuiSessionColor : public ZGMessage,
        public Util::InstanceCounter<ZGMessageSetGuiSessionColor>,
        public Util::ClassName<ZGMessageSetGuiSessionColor>
{
public:

    ZGMessageSetGuiSessionColor();
    ZGMessageSetGuiSessionColor(ZInternalID message_id,
                                ZInternalID gui_id,
                                const ZGColor& color) ;
    ZGMessageSetGuiSessionColor(const ZGMessageSetGuiSessionColor& message) ;
    ZGMessageSetGuiSessionColor& operator=(const ZGMessageSetGuiSessionColor& message) ;
    ~ZGMessageSetGuiSessionColor() ;
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


#endif // ZGMESSAGESETGUISESSIONCOLOR_H
