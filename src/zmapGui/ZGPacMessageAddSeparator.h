#ifndef ZGPACMESSAGEADDSEPARATOR_H
#define ZGPACMESSAGEADDSEPARATOR_H

#include <string>
#include <cstddef>
#include "ZGPacMessage.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZInternalID.h"

namespace ZMap
{

namespace GUI
{

class ZGPacMessageAddSeparator : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageAddSeparator>,
        public Util::ClassName<ZGPacMessageAddSeparator>
{
public:

    ZGPacMessageAddSeparator();
    ZGPacMessageAddSeparator(ZInternalID message_id,
                             ZInternalID gui_id) ;
    ZGPacMessageAddSeparator(const ZGPacMessageAddSeparator& data) ;
    ZGPacMessageAddSeparator& operator=(const ZGPacMessageAddSeparator& data) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageAddSeparator() ;

    ZInternalID getGuiID() const {return m_gui_id; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_gui_id ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGPACMESSAGEADDSEPARATOR_H
