#ifndef ZGMESSAGEDELETESEPARATOR_H
#define ZGMESSAGEDELETESEPARATOR_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGMessage.h"
#include <cstddef>

namespace ZMap
{

namespace GUI
{

class ZGMessageDeleteSeparator : public ZGMessage,
        public Util::InstanceCounter<ZGMessageDeleteSeparator>,
        public Util::ClassName<ZGMessageDeleteSeparator>
{
public:
    ZGMessageDeleteSeparator();
    ZGMessageDeleteSeparator(ZInternalID message_id,
                             ZInternalID gui_id ) ;
    ZGMessageDeleteSeparator(const ZGMessageDeleteSeparator& data) ;
    ZGMessageDeleteSeparator& operator=(const ZGMessageDeleteSeparator& data) ;
    ZGMessage* clone() const override ;
    ~ZGMessageDeleteSeparator() ;

    ZInternalID getGuiID() const {return m_gui_id ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_gui_id ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGMESSAGEDELETESEPARATOR_H
