#ifndef ZGPACMESSAGESETSEPARATORCOLOR_H
#define ZGPACMESSAGESETSEPARATORCOLOR_H


#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGColor.h"
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

class ZGPacMessageSetSeparatorColor : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageSetSeparatorColor>,
        public Util::ClassName<ZGPacMessageSetSeparatorColor>
{
public:

    ZGPacMessageSetSeparatorColor();
    ZGPacMessageSetSeparatorColor(ZInternalID message_id,
                                  ZInternalID sequence_id,
                                  const ZGColor& color) ;
    ZGPacMessageSetSeparatorColor(const ZGPacMessageSetSeparatorColor& message) ;
    ZGPacMessageSetSeparatorColor& operator=(const ZGPacMessageSetSeparatorColor& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageSetSeparatorColor() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZGColor getColor() const {return m_color ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id ;
    ZGColor m_color ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGPACMESSAGESETSEPARATORCOLOR_H
