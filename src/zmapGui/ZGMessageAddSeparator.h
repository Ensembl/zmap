#ifndef ZGMESSAGEADDSEPARATOR_H
#define ZGMESSAGEADDSEPARATOR_H

#include <cstddef>
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGMessage.h"


namespace ZMap
{

namespace GUI
{

class ZGMessageAddSeparator : public ZGMessage,
        public Util::InstanceCounter<ZGMessageAddSeparator>,
        public Util::ClassName<ZGMessageAddSeparator>
{
public:
    ZGMessageAddSeparator();
    ZGMessageAddSeparator(ZInternalID message_id,
                          ZInternalID gui_id) ;
    ZGMessageAddSeparator(const ZGMessageAddSeparator& data) ;
    ZGMessageAddSeparator& operator=(const ZGMessageAddSeparator& data) ;
    ZGMessage* clone() const override ;
    ~ZGMessageAddSeparator() ;

    ZInternalID getGuiID() const {return m_gui_id ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_gui_id ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGMESSAGEADDSEPARATOR_H
