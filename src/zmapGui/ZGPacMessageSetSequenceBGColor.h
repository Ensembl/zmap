#ifndef ZGPACMESSAGESETSEQUENCEBGCOLOR_H
#define ZGPACMESSAGESETSEQUENCEBGCOLOR_H

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

class ZGPacMessageSetSequenceBGColor : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageSetSequenceBGColor>,
        public Util::ClassName<ZGPacMessageSetSequenceBGColor>
{
public:

    ZGPacMessageSetSequenceBGColor();
    ZGPacMessageSetSequenceBGColor(ZInternalID message_id,
                                   ZInternalID sequence_id,
                                   const ZGColor& color) ;
    ZGPacMessageSetSequenceBGColor(const ZGPacMessageSetSequenceBGColor& message) ;
    ZGPacMessageSetSequenceBGColor& operator=(const ZGPacMessageSetSequenceBGColor& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageSetSequenceBGColor() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZGColor getColor() const {return m_color ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id ;
    ZGColor m_color ;

};

} // namespace GUI

} // namespace ZMap



#endif // ZGPACMESSAGESETSEQUENCEBGCOLOR_H
