#ifndef ZGPACMESSAGEDELETESEPARATOR_H
#define ZGPACMESSAGEDELETESEPARATOR_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{


class ZGPacMessageDeleteSeparator : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageDeleteSeparator>,
        public Util::ClassName<ZGPacMessageDeleteSeparator>
{
public:

    ZGPacMessageDeleteSeparator();
    ZGPacMessageDeleteSeparator(ZInternalID message_id,
                                ZInternalID gui_id) ;
    ZGPacMessageDeleteSeparator(const ZGPacMessageDeleteSeparator& data) ;
    ZGPacMessageDeleteSeparator& operator=(const ZGPacMessageDeleteSeparator& data) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageDeleteSeparator() ;

    ZInternalID getGuiID() const {return m_gui_id ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_gui_id ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGPACMESSAGEDELETESEPARATOR_H
